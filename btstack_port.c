#define BTSTACK_FILE__ "btstack_port.c"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <shell.h>
#include <bl616_glb.h>
#include <bflb_mtimer.h>

#include <easyflash.h>

#include <FreeRTOSConfig.h>
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "btstack_config.h"
#include <btstack.h>
#include <btstack_tlv.h>
#include <btstack_event.h>
#include <btstack_memory.h>
#include <btstack_run_loop.h>
#include <btstack_run_loop_freertos.h>
#include <btstack_tlv_flash_bank.h>
#include <btstack_debug.h>
#include <btstack_stdin.h>

#include <hci.h>
#include <bluetooth.h>
#include <hci_dump_embedded_stdout.h>
#include <hci_dump.h>
#include <hal_time_ms.h>
#include <ble/le_device_db_tlv.h>
#include <classic/btstack_link_key_db_tlv.h>

#include "rw_data_api.h"

static void (*transport_packet_handler)(uint8_t packet_type, uint8_t *packet, uint16_t size);

struct rx_msg_struct {
    uint8_t pkt_type;
    uint8_t *bufptr;
    uint32_t size;
};
static QueueHandle_t msg_queue;

/**
* CONFIG_BT_RX_BUF_COUNT: number of buffer for incoming ACL packages or HCI
* events,range 2 to 255
*/

#define CONFIG_BT_RX_BUF_COUNT     4
#define DATA_MSG_CNT               5

#define CONFIG_ACL_RX_BUF_LEN      1024 // 1021 DH5_3
#define CONFIG_EVT_RX_BUF_LEN      (255 + 2 + 3)

#define CONFIG_BT_HCI_RESERVE      1
#define CONFIG_BT_RX_BUF_RSV_COUNT (1)
#if (CONFIG_BT_RX_BUF_RSV_COUNT >= CONFIG_BT_RX_BUF_COUNT)
#error "CONFIG_BT_RX_BUF_RSV_COUNT config error"
#endif

static __ALIGNED(4) uint8_t acl_sco_iso_rx_pool[CONFIG_BT_HCI_RESERVE + CONFIG_BT_RX_BUF_COUNT][CONFIG_ACL_RX_BUF_LEN];
static __ALIGNED(4) uint8_t evt_rx_pool[CONFIG_BT_HCI_RESERVE + CONFIG_BT_RX_BUF_COUNT][CONFIG_EVT_RX_BUF_LEN];
static __ALIGNED(4) uint8_t tx_ring_buffer[1024 * 4];

static btstack_memory_pool_t acl_sco_iso_rx_pool_handle;
static btstack_memory_pool_t evt_rx_pool_handle;
static btstack_ring_buffer_t tx_ring_buffer_handle;

static uint8_t hci_can_send_now;

static void transport_notify_packet_send(void)
{
    // notify upper stack that it might be possible to send again
    uint8_t event[] = { HCI_EVENT_TRANSPORT_PACKET_SENT, 0 };
    transport_packet_handler(HCI_EVENT_PACKET, &event[0], sizeof(event));
    return;
}
static void transport_notify_ready(void)
{
    // notify upper stack that it transport is ready
    uint8_t event[] = { HCI_EVENT_TRANSPORT_READY, 0 };
    transport_packet_handler(HCI_EVENT_PACKET, &event[0], sizeof(event));
    return;
}
static void transport_send_hardware_error(uint8_t error_code)
{
    uint8_t event[] = { HCI_EVENT_HARDWARE_ERROR, 1, error_code };
    transport_packet_handler(HCI_EVENT_PACKET, &event[0], sizeof(event));
    return;
}

struct rwip_data {
    uint8_t *bufptr;
    uint32_t size;
    rwip_eif_callback callback;
    void *dummy;
    bool controller_underrun;// or standby
} hci_send_to_controller;
// The read progress of rwip is always slightly faster than that of btstack
static void data_from_host(uint8_t *bufptr, uint32_t size, rwip_eif_callback callback, void *dummy)
{
    log_info("%s read %d", __func__, size);

    uint32_t read_bytes = 0;
    btstack_ring_buffer_read(&tx_ring_buffer_handle, bufptr, size, &read_bytes);
    if (read_bytes != size) {
        hci_send_to_controller.bufptr = bufptr + read_bytes;
        hci_send_to_controller.size = size - read_bytes;
        hci_send_to_controller.callback = callback;
        hci_send_to_controller.dummy = dummy;
        hci_send_to_controller.controller_underrun = 1;
        return;
    }

    hci_send_to_controller.controller_underrun = 0;
    hci_send_to_controller.bufptr = 0;
    hci_send_to_controller.size = 0;
    hci_send_to_controller.callback = 0;
    hci_send_to_controller.dummy = 0;
    callback(dummy, 0);
    return;
}

static int transport_send_to_controller(uint8_t packet_type, uint8_t *packet, int size)
{
    log_info("%s write %d", __func__, size + sizeof(packet_type));
    btstack_ring_buffer_write(&tx_ring_buffer_handle, &packet_type, sizeof(packet_type));
    uint32_t bytes_available = btstack_ring_buffer_write(&tx_ring_buffer_handle, packet, size);
    if (bytes_available >= sizeof(tx_ring_buffer)) {
        hci_can_send_now = 0;
    } else {
        hci_can_send_now = 1;
        transport_notify_packet_send();
    }

    if (hci_send_to_controller.controller_underrun == 1) {
        data_from_host(
            hci_send_to_controller.bufptr,
            hci_send_to_controller.size,
            hci_send_to_controller.callback,
            hci_send_to_controller.dummy);
    }

    return 0;
}
static void data_to_host(uint8_t *bufptr, uint32_t size, rwip_eif_callback callback, void *dummy)
{
    log_info("%s %d", __func__, size);
    struct rx_msg_struct rx_msg = {
        .pkt_type = bufptr[0],
        .bufptr = 0,
        .size = size - 1,
    };
    switch (rx_msg.pkt_type) {
        case HCI_EVENT_PACKET: {
            taskENTER_CRITICAL();
            rx_msg.bufptr = btstack_memory_pool_get(evt_rx_pool_handle);
            taskEXIT_CRITICAL();
            break;
        }
        case HCI_SCO_DATA_PACKET:
        case HCI_ISO_DATA_PACKET:
        case HCI_ACL_DATA_PACKET: {
            if (rx_msg.pkt_type == HCI_ISO_DATA_PACKET)
            {
                // printf("iso\n");
            }
            taskENTER_CRITICAL();
            rx_msg.bufptr = btstack_memory_pool_get(acl_sco_iso_rx_pool_handle);
            taskEXIT_CRITICAL();
            break;
        }
        default: {
            return;
        }
    }
    memcpy(rx_msg.bufptr, bufptr + 1, rx_msg.size);
    static BaseType_t yield = pdFALSE;
    xQueueSendFromISR(msg_queue, &rx_msg, &yield);
    btstack_run_loop_poll_data_sources_from_irq();
    portYIELD_FROM_ISR(yield);

    callback(dummy, 0);
    return;
}

void flow_on()
{
    hci_can_send_now = 1;
    // transport_notify_packet_send();// Cannot be called here
}
void flow_off()
{
    hci_can_send_now = 0;
    // trigger_shutdown();
}
const struct rwip_eif_api btstack_port_api = {
    .read = data_from_host,
    .write = data_to_host,
    .flow_on = flow_on,
    .flow_off = flow_off,
};

uint32_t hal_time_ms(void)
{
    return (uint32_t)bflb_mtimer_get_time_ms();
}

// data source for integration with BTstack Runloop
static btstack_data_source_t transport_data_source;

static void transport_deliver_hci_packets(void)
{
    struct rx_msg_struct msg;

    while (xQueueReceive(msg_queue, &msg, 0) == pdTRUE) {
        if (msg.bufptr) {
            if (msg.pkt_type != HCI_EVENT_PACKET) {
                transport_packet_handler(msg.pkt_type, msg.bufptr, msg.size);
                taskENTER_CRITICAL();
                btstack_memory_pool_free(&acl_sco_iso_rx_pool_handle, msg.bufptr);
                taskEXIT_CRITICAL();

            } else {
                transport_packet_handler(msg.pkt_type, msg.bufptr, msg.size);
                taskENTER_CRITICAL();
                btstack_memory_pool_free(&evt_rx_pool_handle, msg.bufptr);
                taskEXIT_CRITICAL();
            }
            msg.bufptr = NULL;
        }
    }
}
static void transport_process(btstack_data_source_t *ds, btstack_data_source_callback_type_t callback_type)
{
    switch (callback_type) {
        case DATA_SOURCE_CALLBACK_POLL:
            transport_notify_ready();
            transport_deliver_hci_packets();
            break;
        default:
            break;
    }
}

/**
 * init transport
 * @param transport_config
 */
static void transport_init(const void *transport_config)
{
    log_info("transport_init");

    btstack_ring_buffer_init(&tx_ring_buffer_handle, tx_ring_buffer, sizeof(tx_ring_buffer));

    btstack_memory_pool_create(&acl_sco_iso_rx_pool_handle,
                               acl_sco_iso_rx_pool,
                               CONFIG_BT_HCI_RESERVE + CONFIG_BT_RX_BUF_COUNT,
                               CONFIG_ACL_RX_BUF_LEN);
    btstack_memory_pool_create(&evt_rx_pool_handle,
                               evt_rx_pool,
                               CONFIG_BT_HCI_RESERVE + CONFIG_BT_RX_BUF_COUNT,
                               CONFIG_EVT_RX_BUF_LEN);
    msg_queue = xQueueCreate(DATA_MSG_CNT, sizeof(struct rx_msg_struct));

    // set up polling data_source
    btstack_run_loop_set_data_source_handler(&transport_data_source, &transport_process);
    btstack_run_loop_enable_data_source_callbacks(&transport_data_source, DATA_SOURCE_CALLBACK_POLL);
    btstack_run_loop_add_data_source(&transport_data_source);

    btble_controller_init(configMAX_PRIORITIES - 1);
}

/**
 * open transport connection
 */
static int transport_open(void)
{
    log_info("transport_open");
    return 0;
}

/**
 * close transport connection
 */
static int transport_close(void)
{
    log_info("transport_close");

    struct rx_msg_struct msg;

    while (1) {
        if (xQueueReceive(msg_queue, &msg, 0)) {
        } else {
            break;
        }
    }
    vQueueDelete(msg_queue);
    msg_queue = NULL;

    return 0;
}

/**
 * register packet handler for HCI packets: ACL and Events
 */
static void transport_register_packet_handler(void (*handler)(uint8_t packet_type, uint8_t *packet, uint16_t size))
{
    log_info("transport_register_packet_handler");
    transport_packet_handler = handler;
}

/**
 * support async transport layers, e.g. IRQ driven without buffers
 */
static int transport_can_send_packet_now(uint8_t packet_type)
{
    return hci_can_send_now;
}

static void trigger_shutdown(void)
{
    printf("CTRL-C - SIGINT received, shutting down..\n");
    log_info("sigint_handler: shutting down");
    hci_power_control(HCI_POWER_OFF);
}

static const hci_transport_t transport = {
    "BouffaloBT",
    &transport_init,
    &transport_open,
    &transport_close,
    &transport_register_packet_handler,
    &transport_can_send_packet_now,
    &transport_send_to_controller,
    NULL, // set baud rate
    NULL, // reset link
    NULL, // set SCO config
};

static const hci_transport_t *transport_get_instance(void)
{
    return &transport;
}

static btstack_packet_callback_registration_t hci_event_callback_registration;

static void local_version_information_handler(uint8_t *packet)
{
    printf("Local version information:\n");
    uint16_t hci_version = packet[6];
    uint16_t hci_revision = little_endian_read_16(packet, 7);
    uint16_t lmp_version = packet[9];
    uint16_t manufacturer = little_endian_read_16(packet, 10);
    uint16_t lmp_subversion = little_endian_read_16(packet, 12);
    printf("- HCI Version    %#04x\n", hci_version);
    printf("- HCI Revision   %#04x\n", hci_revision);
    printf("- LMP Version    %#04x\n", lmp_version);
    printf("- LMP Subversion %#04x\n", lmp_subversion);
    printf("- Manufacturer   %#04x\n", manufacturer);
}
static const btstack_tlv_t btstack_tlv_impl;
static bd_addr_t local_addr = { 0 };
static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    const uint8_t *params;
    if (packet_type != HCI_EVENT_PACKET)
        return;
    switch (hci_event_packet_get_type(packet)) {
        case BTSTACK_EVENT_STATE:
            switch (btstack_event_state_get_state(packet)) {
                case HCI_STATE_WORKING:
                    printf("BTstack up and running on %s.\n", bd_addr_to_str(local_addr));
                    // setup global tlv
                    btstack_tlv_set_instance(&btstack_tlv_impl, NULL);

                    hci_set_link_key_db(btstack_link_key_db_tlv_get_instance(&btstack_tlv_impl, NULL));
                    // setup LE Device DB using TLV
                    le_device_db_tlv_configure(&btstack_tlv_impl, NULL);
                    break;
                case HCI_STATE_OFF:
                    printf("Good bye, see you.\n");
                    break;
                default:
                    break;
            }
            break;
        case HCI_EVENT_COMMAND_COMPLETE:
            switch (hci_event_command_complete_get_command_opcode(packet)) {
                case HCI_OPCODE_HCI_READ_LOCAL_VERSION_INFORMATION:
                    local_version_information_handler(packet);
                    break;
                case HCI_OPCODE_HCI_READ_BD_ADDR:
                    params = hci_event_command_complete_get_return_parameters(packet);
                    if (params[0] != 0)
                        break;
                    if (size < 12)
                        break;
                    reverse_48(&params[1], local_addr);
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}

static void (*btstack_stdin_handler)(char c) = NULL;

void btstack_stdin_setup(void (*stdin_handler)(char c))
{
    if (!btstack_stdin_handler) {
        btstack_stdin_handler = stdin_handler;
    }
}
void btstack_stdin_reset(void)
{
}
static void settings_erase();
void btstack_cmd(int args, char **argv)
{
    if (args < 2) {
        return;
    }

    if (strlen(argv[1]) > 1) {
        if (strcmp(argv[1], "erase") == 0) {
            settings_erase();
            return;
        }
    }

    if (!btstack_stdin_handler) {
        return;
    }

    btstack_stdin_handler(argv[1][0]);
}
SHELL_CMD_EXPORT_ALIAS(btstack_cmd, btstack, btstack);

bool ef_ready_flag = false;
static int bt_check_if_ef_ready()
{
    int err = 0;

    if (!ef_ready_flag) {
        err = easyflash_init();
        if (!err) {
            ef_ready_flag = true;
        } else {
            printf("easyflash init fail :(\n");
        }
    }

    return err;
}
static int bt_settings_set_bin(void *context, uint32_t tag, const uint8_t *data, uint32_t data_size)
{
    int err;

    err = bt_check_if_ef_ready();
    if (err)
        return err;
    char key[9] = { 0 };
    sprintf(key, "%x", tag);
    key[8] = 0;
    err = ef_set_env_blob(key, data, data_size);

    return err;
}

static int bt_settings_get_bin(void *context, uint32_t tag, uint8_t *buffer, uint32_t buffer_size)
{
    int err;
    size_t rlen;

    err = bt_check_if_ef_ready();
    if (err)
        return err;
    char key[9] = { 0 };
    sprintf(key, "%x", tag);
    key[8] = 0;
    rlen = ef_get_env_blob(key, buffer, buffer_size, NULL);

    return rlen;
}

static void settings_delete(void *context, uint32_t tag)
{
    char key[9] = { 0 };
    sprintf(key, "%x", tag);
    key[8] = 0;
    ef_del_env(key);
    return;
}
static void settings_erase()
{
    //like bflb_mtd_erase_all
    if (ef_port_erase(0, 32768) == 0) {
        printf("erase success\n");
    }
}
static const btstack_tlv_t btstack_tlv_impl = {
    .get_tag = &bt_settings_get_bin,
    .store_tag = &bt_settings_set_bin,
    .delete_tag = &settings_delete,
};
extern int btstack_main(int argc, const char *argv[]);
void port_thread(void *args)
{
    bt_check_if_ef_ready();
#ifdef CONFIG_BTSTACK_LOG
    hci_dump_init(hci_dump_embedded_stdout_get_instance());
#endif
    /// GET STARTED with BTstack ///
    btstack_memory_init();
    btstack_run_loop_init(btstack_run_loop_freertos_get_instance());

    // init HCI
    hci_init(transport_get_instance(), NULL);

    // inform about BTstack state
    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    btstack_main(0, NULL);

    //gap_set_security_level(LEVEL_2);

    log_info("btstack executing run loop...");
    btstack_run_loop_execute();
}
