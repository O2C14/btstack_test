#include "bflb_mtimer.h"
#include "board.h"
#include "bflb_gpio.h"
#define DBG_TAG "MAIN"
#include "log.h"
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "rfparam_adapter.h"
#include "task.h"
#include "bl616_glb.h"
#include "bflb_mtd.h"



static int btblecontroller_em_config(void)
{
  extern uint8_t __LD_CONFIG_EM_SEL;
  volatile uint32_t em_size;

  em_size = (uint32_t)&__LD_CONFIG_EM_SEL;

  if (em_size == 0) {
    GLB_Set_EM_Sel(GLB_WRAM160KB_EM0KB);
  } else if (em_size == 32 * 1024) {
    GLB_Set_EM_Sel(GLB_WRAM128KB_EM32KB);
  } else if (em_size == 64 * 1024) {
    GLB_Set_EM_Sel(GLB_WRAM96KB_EM64KB);
  } else {
    GLB_Set_EM_Sel(GLB_WRAM96KB_EM64KB);
  }

  return 0;
}

TaskHandle_t hbtstack_task;
/**
  * @brief  The application entry point.
  * @retval int
  */
static struct bflb_device_s *gpio = NULL;
static struct bflb_device_s *uart0;
#define PIN_JTAG_TDO GPIO_PIN_14
#define PIN_JTAG_TDI GPIO_PIN_19


void port_thread(void *args);
int main(void)
{
  /* Reset of all peripherals, initializes the Systick. */

  board_init();
  gpio = bflb_device_get_by_name("gpio");
  uart0 = bflb_device_get_by_name("uart0");
  shell_init_with_task(uart0);

  /*

  bflb_gpio_init(gpio, PIN_JTAG_TDO,
                 GPIO_FUNC_JTAG | GPIO_ALTERNATE | GPIO_FLOAT | GPIO_SMT_EN |
                     GPIO_DRV_1);
  bflb_gpio_init(gpio, PIN_JTAG_TDI,
                 GPIO_FUNC_JTAG | GPIO_ALTERNATE | GPIO_FLOAT | GPIO_SMT_EN |
                     GPIO_DRV_1);
  */

  /* Set ble controller EM Size */
  btblecontroller_em_config();

  /* Init rf */
  if (0 != rfparam_init(0, NULL, 0)) {
    printf("PHY RF init failed!\r\n");
    return 0;
  }

  /* For bt status save */
  bflb_mtd_init();
  easyflash_init();


  //xTaskCreate(port_thread, "btstack_thread", 2048, NULL, configMAX_PRIORITIES - 4, &hbtstack_task);
  xTaskCreate(port_thread, "btstack_thread", 2048, NULL, 1, &hbtstack_task);

  vTaskStartScheduler();
  //port_thread(NULL);
  /* We should never get here as control is now taken by the scheduler */
  while (1);
}