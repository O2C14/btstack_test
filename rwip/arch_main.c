#include <stdint.h>
#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>
#include <bflb_irq.h>
#include <bl616_glb.h>
#include <bl616_common.h>
#include "rw_data_api.h"
/**
 ****************************************************************************************
 * @brief Turn ON a led.
 ****************************************************************************************
 */
void led_set(uint8_t led_number)
{
}

/**
 ****************************************************************************************
 * @brief Turn OFF a led.
 ****************************************************************************************
 */
void led_reset(uint8_t led_number)
{
}

/**
 ****************************************************************************************
 * @brief Re-boot FW.
 *
 * This function is used to re-boot the FW when error has been detected, it is the end of
 * the current FW execution.
 * After waiting transfers on UART to be finished, and storing the information that
 * FW has re-booted by itself in a non-loaded area, the FW restart by branching at FW
 * entry point.
 *
 * Note: when calling this function, the code after it will not be executed.
 *
 * @param[in] error      Error detected by FW
 ****************************************************************************************
 */
void platform_reset(uint32_t error)
{
    printf("platform_reset error:%d\n", error);
    configASSERT(0);
}

/**
 * @brief Request an address to address memory copy using a DMA block (non blocking).
 *
 * If this function is called while DMA channel is in use, function is blocked till DMA channel is released
 *
 * @param[in] channel       DMA channel used (@see enum dma_channels)
 * @param[in] p_dst_addr    Destination address of the memory copy
 * @param[in] p_src_addr    Source address of the memory copy
 * @param[in] size          Size of data to copy in octets.
 */
void dma_copy(uint8_t channel, void *p_dst_addr, const void *p_src_addr, uint16_t size)
{
    memcpy(p_dst_addr, p_src_addr, size);
}

/**
 ****************************************************************************************
 * @brief Initialization of the BLE Data Path driver
 *
 * @param[in] init_type  Type of initialization (@see enum rwip_init_type)
 ****************************************************************************************
 */
void plf_data_path_init(uint8_t init_type)
{
}

/**
 ****************************************************************************************
 * @brief Retrieve the data path interface according to the direction
 *
 * @param[in]  type      Type of data path interface (@see enum iso_dp_type)
 * @param[in]  direction Data Path direction (@see enum iso_rx_tx_select)
 *
 * @return Pointer to the interface of the data path driver, NULL if no driver found
 ****************************************************************************************
 */
const struct data_path_itf *plf_data_path_itf_get(uint8_t type, uint8_t direction)
{
    return NULL; // for iso channal ?
}
extern const struct rwip_eif_api btstack_port_api;
const struct rwip_eif_api *rwip_eif_get(uint8_t idx)
{
    const struct rwip_eif_api *ret = NULL;
    switch (idx) {
        case 0: {
            ret = &btstack_port_api;
        } break;
        default: {
            configASSERT(0);
        } break;
    }
    return ret;
}

void rwip_init(uint32_t error);
void rwip_isr(void);
void rwble_isr(void);
void rwbt_isr(void);
uint32_t ke_event_get_all(void);
void rwip_schedule(void);
void arch_main_loop(void)
{
    while (1) {
        rwip_schedule();
        vTaskDelay(1); // refer original lib
    }
}

static TaskHandle_t rw_main_task_hdl;
// Don't rename this, the ld script needs it
void btble_controller_init(int task_priority)
{
    xTaskCreate(arch_main_loop, "rwip_controller", 1024 * 2, NULL, task_priority, &rw_main_task_hdl);

    rwip_init(0);

    bflb_irq_clear_pending(DM_IRQn);
    bflb_irq_attach(DM_IRQn, rwip_isr, NULL);
    bflb_irq_enable(DM_IRQn);

    bflb_irq_clear_pending(BLE_IRQn);
    bflb_irq_attach(BLE_IRQn, rwble_isr, NULL);
    bflb_irq_enable(BLE_IRQn);

    bflb_irq_clear_pending(BT_IRQn);
    bflb_irq_attach(BT_IRQn, rwbt_isr, NULL);
    bflb_irq_enable(BT_IRQn);
}

void assert_err(const char *condition, const char *file, int line)
{
    printf("condition %s\r\n", condition);
    printf("file [%s]\r\n", file);
    printf("line [%d]\r\n", line);
    vAssertCalled();
}
// for rf lib
void btble_assert_err(const char *condition, const char *file, int line)
{
    assert_err(condition, file, line);
}
void assert_param(int param0, int param1, const char *file, int line)
{
    printf("param %d %d\r\n", param0, param1);
    printf("file [%s]\r\n", file);
    printf("line [%d]\r\n", line);
}

void assert_warn(int param0, int param1, const char *file, int line)
{
    printf("param %d %d\r\n", param0, param1);
    printf("file [%s]\r\n", file);
    printf("line [%d]\r\n", line);
}
