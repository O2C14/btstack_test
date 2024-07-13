#include "bflb_mtimer.h"
#define BTSTACK_FILE__ "btstack_port.c"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*******************************************
 *       transport implementation
 ******************************************/

// #define USE_SRAM_FLASH_BANK_EMU

#include "btstack.h"

#include "btstack_config.h"
#include "btstack_event.h"
#include "btstack_memory.h"
#include "btstack_run_loop.h"
#include "btstack_run_loop_freertos.h"
#include "btstack_tlv_flash_bank.h"

#include "hci.h"
#include "hci_dump_embedded_stdout.h"
#include "hci_dump.h"
#include "hal_time_ms.h"
#include "btstack_debug.h"
#include "btstack_stdin.h"

#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"

#include <sys/errno.h>
#include "hci_onchip.h"
#include "bluetooth.h"
#include <shell.h>
#include "bl616_glb.h"
#include "btble_lib_api.h"
#include "easyflash.h"
#include "btstack_tlv.h"
#include "ble/le_device_db_tlv.h"
#include "classic/btstack_link_key_db_tlv.h"
static void (*transport_packet_handler)(uint8_t packet_type, uint8_t *packet, uint16_t size);

struct rx_msg_struct {
  uint8_t pkt_type;
  uint16_t src_id;
  uint8_t *param;
  uint8_t param_len;
} __packed;
static QueueHandle_t msg_queue;

/**
* CONFIG_BT_RX_BUF_COUNT: number of buffer for incoming ACL packages or HCI
* events,range 2 to 255
*/

#define CONFIG_BT_RX_BUF_COUNT     4
#define DATA_MSG_CNT               5

#define CONFIG_ACL_RX_BUF_LEN      1024
#define CONFIG_EVT_RX_BUF_LEN      (255 + 2 + 3)

#define CONFIG_BT_HCI_RESERVE      1
#define CONFIG_BT_RX_BUF_RSV_COUNT (1)
#if (CONFIG_BT_RX_BUF_RSV_COUNT >= CONFIG_BT_RX_BUF_COUNT)
#error "CONFIG_BT_RX_BUF_RSV_COUNT config error"
#endif

#if defined(BFLB_BLE_NOTIFY_ADV_DISCARDED)
extern void ble_controller_notify_adv_discarded(uint8_t *adv_bd_addr, uint8_t adv_type);
#endif

static ATTR_PSRAM_SECTION __ALIGNED(4) uint8_t acl_rx_pool[CONFIG_BT_HCI_RESERVE + CONFIG_BT_RX_BUF_COUNT][CONFIG_ACL_RX_BUF_LEN];
static btstack_memory_pool_t acl_rx_pool_handle;
static ATTR_PSRAM_SECTION __ALIGNED(4) uint8_t evt_rx_pool[CONFIG_BT_HCI_RESERVE + CONFIG_BT_RX_BUF_COUNT][CONFIG_EVT_RX_BUF_LEN];
static btstack_memory_pool_t evt_rx_pool_handle;
#define BT_HCI_EVT_CC_PARAM_OFFSET       0x05
#define BT_HCI_CCEVT_HDR_PARLEN          0x03
#define BT_HCI_CSEVT_LEN                 0x06
#define BT_HCI_CSVT_PARLEN               0x04
#define BT_HCI_EVT_LE_PARAM_OFFSET       0x02

#define BT_HCI_EVT_CMD_COMPLETE          0x0e
#define BT_HCI_EVT_CMD_STATUS            0x0f

#define BT_HCI_EVT_LE_META_EVENT         0x3e
#define BT_HCI_EVT_LE_ADVERTISING_REPORT 0x02
#define BT_HCI_EVT_NUM_COMPLETED_PACKETS 0x13

static uint8_t hci_acl_can_send_now;
static void transport_notify_packet_send(void)
{
  // notify upper stack that it might be possible to send again
  uint8_t event[] = { HCI_EVENT_TRANSPORT_PACKET_SENT, 0 };
  transport_packet_handler(HCI_EVENT_PACKET, &event[0], sizeof(event));
}
static void transport_notify_ready(void)
{
  // notify upper stack that it transport is ready
  uint8_t event[] = { HCI_EVENT_TRANSPORT_READY, 0 };
  transport_packet_handler(HCI_EVENT_PACKET, &event[0], sizeof(event));
}
static void transport_send_hardware_error(uint8_t error_code)
{
  uint8_t event[] = { HCI_EVENT_HARDWARE_ERROR, 1, error_code };
  transport_packet_handler(HCI_EVENT_PACKET, &event[0], sizeof(event));
}

static void bl_packet_to_host(uint8_t pkt_type, uint16_t src_id, uint8_t *param, uint8_t param_len, const uint8_t *buf)
{
  uint16_t tlt_len;
  bool prio = true;
  uint8_t nb_h2c_cmd_pkts = 0x01;
  uint8_t *buf_data = buf;
  uint8_t to_hci_data_type = 0;
  //bt_buf_set_rx_adv(buf, false);

  switch (pkt_type) {
    case BT_HCI_CMD_CMP_EVT: {
      tlt_len = BT_HCI_EVT_CC_PARAM_OFFSET + param_len;
      *buf_data++ = BT_HCI_EVT_CMD_COMPLETE;
      *buf_data++ = BT_HCI_CCEVT_HDR_PARLEN + param_len;
      *buf_data++ = nb_h2c_cmd_pkts;
      *buf_data++ = ((uint8_t *)&src_id)[0];
      *buf_data++ = ((uint8_t *)&src_id)[1];

      memcpy(buf_data, param, param_len);
      to_hci_data_type = HCI_EVENT_PACKET;
      break;
    }
    case BT_HCI_CMD_STAT_EVT: {
      tlt_len = BT_HCI_CSEVT_LEN;
      *buf_data++ = BT_HCI_EVT_CMD_STATUS;
      *buf_data++ = BT_HCI_CSVT_PARLEN;
      *buf_data++ = *(uint8_t *)param; //STATUS
      *buf_data++ = nb_h2c_cmd_pkts;
      *buf_data++ = ((uint8_t *)&src_id)[0];
      *buf_data++ = ((uint8_t *)&src_id)[1];
      to_hci_data_type = HCI_EVENT_PACKET;

      break;
    }
    case BT_HCI_LE_EVT: {
      prio = false;
      if (param[0] == BT_HCI_EVT_LE_ADVERTISING_REPORT) {
        //bt_buf_set_rx_adv(buf, true);
      }
      tlt_len = BT_HCI_EVT_LE_PARAM_OFFSET + param_len;
      *buf_data++ = BT_HCI_EVT_LE_META_EVENT;
      *buf_data++ = param_len;
      memcpy(buf_data, param, param_len);
      to_hci_data_type = HCI_EVENT_PACKET;
      break;
    }
    case BT_HCI_EVT: {
      if (src_id != BT_HCI_EVT_NUM_COMPLETED_PACKETS) {
        prio = false;
      }
      tlt_len = BT_HCI_EVT_LE_PARAM_OFFSET + param_len;
      *buf_data++ = src_id;
      *buf_data++ = param_len;
      memcpy(buf_data, param, param_len);
      to_hci_data_type = HCI_EVENT_PACKET;
      break;
    }
    case BT_HCI_ACL_DATA: {
      prio = false;
      tlt_len = bt_onchiphci_hanlde_rx_acl(param, buf_data);
      if (tlt_len > CONFIG_ACL_RX_BUF_LEN) {
        printf("acl pkg is too big\r\n");
      }
      to_hci_data_type = HCI_ACL_DATA_PACKET;
      break;
    }
    default: {
      return;
    }
  }
  transport_packet_handler(to_hci_data_type, buf, tlt_len);
  return;
}

static void bl_onchiphci_rx_packet_handler(uint8_t pkt_type, uint16_t src_id, uint8_t *param, uint8_t param_len)
{
  struct net_buf *buf = NULL;
  struct rx_msg_struct rx_msg = {
    .pkt_type = pkt_type,
    .src_id = src_id,
    .param_len = param_len,
  };
  if (param_len) {
    switch (pkt_type) {
      case BT_HCI_CMD_CMP_EVT:
      case BT_HCI_CMD_STAT_EVT:
      case BT_HCI_LE_EVT:
      case BT_HCI_EVT: {
        taskENTER_CRITICAL();
        rx_msg.param = btstack_memory_pool_get(evt_rx_pool_handle);
        taskEXIT_CRITICAL();
        break;
      }
      case BT_HCI_ACL_DATA: {
        taskENTER_CRITICAL();
        rx_msg.param = btstack_memory_pool_get(acl_rx_pool_handle);
        taskEXIT_CRITICAL();
        break;
      }
      default: {
        return;
      }
    }
  }
  memcpy(rx_msg.param, param, param_len);
  static BaseType_t yield = pdFALSE;
  xQueueSendFromISR(msg_queue, &rx_msg, &yield);
  btstack_run_loop_poll_data_sources_from_irq();
  portYIELD_FROM_ISR(yield);
}

uint32_t hal_time_ms(void)
{
  return (uint32_t)bflb_mtimer_get_time_ms();
}

// data source for integration with BTstack Runloop
static btstack_data_source_t transport_data_source;

static void transport_deliver_hci_packets(void)
{
  void *tmp_buf = NULL;
  struct rx_msg_struct msg;

  while (xQueueReceive(msg_queue, &msg, 0) == pdTRUE) {
    if (msg.param) {
      if (msg.pkt_type == BT_HCI_ACL_DATA) {
        taskENTER_CRITICAL();
        tmp_buf = btstack_memory_pool_get(&acl_rx_pool_handle);
        taskEXIT_CRITICAL();
        bl_packet_to_host(msg.pkt_type, msg.src_id, msg.param, msg.param_len, tmp_buf);
        taskENTER_CRITICAL();
        btstack_memory_pool_free(&acl_rx_pool_handle, tmp_buf);
        btstack_memory_pool_free(&acl_rx_pool_handle, msg.param);
        taskEXIT_CRITICAL();
        tmp_buf = NULL;
      } else {
        taskENTER_CRITICAL();
        tmp_buf = btstack_memory_pool_get(&evt_rx_pool_handle);
        taskEXIT_CRITICAL();
        bl_packet_to_host(msg.pkt_type, msg.src_id, msg.param, msg.param_len, tmp_buf);
        taskENTER_CRITICAL();
        btstack_memory_pool_free(&evt_rx_pool_handle, tmp_buf);
        btstack_memory_pool_free(&evt_rx_pool_handle, msg.param);
        taskEXIT_CRITICAL();
      }
      msg.param = NULL;
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
  btble_controller_init(configMAX_PRIORITIES - 1);

  btstack_memory_pool_create(&acl_rx_pool_handle,
                             acl_rx_pool,
                             CONFIG_BT_HCI_RESERVE + CONFIG_BT_RX_BUF_COUNT,
                             CONFIG_ACL_RX_BUF_LEN);
  btstack_memory_pool_create(&evt_rx_pool_handle,
                             evt_rx_pool,
                             CONFIG_BT_HCI_RESERVE + CONFIG_BT_RX_BUF_COUNT,
                             CONFIG_EVT_RX_BUF_LEN);
  msg_queue = xQueueCreate(DATA_MSG_CNT, sizeof(struct rx_msg_struct));

  bt_onchiphci_interface_init(&bl_onchiphci_rx_packet_handler);


  hci_acl_can_send_now = 1;
  // set up polling data_source
  btstack_run_loop_set_data_source_handler(&transport_data_source, &transport_process);
  btstack_run_loop_enable_data_source_callbacks(&transport_data_source, DATA_SOURCE_CALLBACK_POLL);
  btstack_run_loop_add_data_source(&transport_data_source);
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
      if (msg.param) {
        vPortFree(msg.param);
      }
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
  switch (packet_type) {
    case HCI_COMMAND_DATA_PACKET:
      return 1;

    case HCI_ACL_DATA_PACKET:
      return hci_acl_can_send_now;
  }
  return 1;
}
struct bt_hci_cmd_hdr {
  uint16_t opcode;
  uint8_t param_len;
} __packed;
struct bt_hci_acl_hdr {
  uint16_t handle;
  uint16_t len;
} __packed;
#define bt_acl_handle(h)    ((h) & 0x0fff)
#define bt_acl_flags(h)     ((h) >> 12)

static int transport_send_packet(uint8_t packet_type, uint8_t *packet, int size)
{
  uint8_t pkt_type;
  uint16_t dest_id = 0x00;
  hci_pkt_struct pkt;
  if (size > CONFIG_ACL_RX_BUF_LEN) {
    printf("acl pkg is too big\r\n");
  }
  switch (packet_type) {
    case HCI_COMMAND_DATA_PACKET:
      pkt_type = BT_HCI_CMD;
      if (size < sizeof(struct bt_hci_cmd_hdr)) {
        break;
      }
      struct bt_hci_cmd_hdr *chdr = packet;
      if (size < chdr->param_len) {
        break;
      }
      switch (chdr->opcode) {
        //ble refer to hci_cmd_desc_tab_le, for the ones of which dest_ll is BLE_CTRL
        case HCI_OPCODE_HCI_LE_CONNECTION_UPDATE:
        case HCI_OPCODE_HCI_LE_READ_CHANNEL_MAP:
        case HCI_OPCODE_HCI_LE_READ_REMOTE_USED_FEATURES:
        case HCI_OPCODE_HCI_LE_START_ENCRYPTION:
        case HCI_OPCODE_HCI_LE_LONG_TERM_KEY_REQUEST_REPLY:
        case HCI_OPCODE_HCI_LE_LONG_TERM_KEY_NEGATIVE_REPLY:
        case HCI_OPCODE_HCI_LE_REMOTE_CONNECTION_PARAMETER_REQUEST_REPLY:
        case HCI_OPCODE_HCI_LE_REMOTE_CONNECTION_PARAMETER_REQUEST_NEGATIVE_REPLY:
        case HCI_OPCODE_HCI_LE_SET_DATA_LENGTH:
        case HCI_OPCODE_HCI_LE_READ_PHY:
        case HCI_OPCODE_HCI_LE_SET_PHY:
        //bredr identify link id, according to dest_id
        case HCI_OPCODE_HCI_READ_REMOTE_SUPPORTED_FEATURES_COMMAND:
        case HCI_OPCODE_HCI_READ_REMOTE_EXTENDED_FEATURES_COMMAND:
        case HCI_OPCODE_HCI_READ_ENCRYPTION_KEY_SIZE: {
          //dest_id is connectin handle
          dest_id = *(uint8_t *)(packet + sizeof(struct bt_hci_cmd_hdr));
        }
        default:
          break;
      }

      pkt.p.hci_cmd.opcode = chdr->opcode;
      pkt.p.hci_cmd.param_len = chdr->param_len;
      pkt.p.hci_cmd.params = packet + sizeof(struct bt_hci_cmd_hdr);
      break;
    case HCI_ACL_DATA_PACKET:
      pkt_type = BT_HCI_ACL_DATA;

      if (size < sizeof(struct bt_hci_acl_hdr)) {
        break;
      }
      struct bt_hci_acl_hdr *acl = packet;
      //connhandle +l2cap field
      uint16_t connhdl_l2cf, tlt_len;
      tlt_len = acl->len;
      connhdl_l2cf = acl->handle;
      if (size - sizeof(struct bt_hci_acl_hdr) < tlt_len) {
        break;
      }
      //get connection_handle
      dest_id = bt_acl_handle(connhdl_l2cf);
      pkt.p.acl_data.conhdl = dest_id;
      pkt.p.acl_data.pb_bc_flag = bt_acl_flags(connhdl_l2cf);
      pkt.p.acl_data.len = tlt_len;
      pkt.p.acl_data.buffer = packet + sizeof(struct bt_hci_acl_hdr);
      hci_acl_can_send_now = 0;
      break;
    default:
      transport_send_hardware_error(0x01); // invalid HCI packet
      return 0;
  }
  bt_onchiphci_send(pkt_type, dest_id, &pkt);
  btstack_run_loop_poll_data_sources_from_irq();
  hci_acl_can_send_now = 1;
  transport_notify_packet_send();
  return 0;
}

static const hci_transport_t transport = {
  "BouffaloBT",
  &transport_init,
  &transport_open,
  &transport_close,
  &transport_register_packet_handler,
  &transport_can_send_packet_now,
  &transport_send_packet,
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

  hci_dump_init(hci_dump_embedded_stdout_get_instance());

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
