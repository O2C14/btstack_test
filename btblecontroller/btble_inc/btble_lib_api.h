#ifndef BLE_LIB_API_H_
#define BLE_LIB_API_H_

#include <stdbool.h>
#include <stdint.h> 


#if defined(CONFIG_BLE_MFG)
#define MAX_SWITCHING_PATTERN_LEN  (0x4B)
struct hci_le_rx_test_v2_cmd
{
    uint8_t     rx_channel;
    uint8_t     phy;
    uint8_t     mod_idx;
};

struct hci_le_tx_test_v4_cmd
{
    uint8_t     tx_channel;
    uint8_t     test_data_len;
    uint8_t     pkt_payl;
    uint8_t     phy;
    uint8_t     cte_len;
    uint8_t     cte_type;
    uint8_t     switching_pattern_len;
    uint8_t     antenna_id[MAX_SWITCHING_PATTERN_LEN];
    int8_t     tx_pwr_lvl;
};
#endif

#if defined(CONFIG_BT_MFG)
struct hci_vs_rx_test_cmd
{
    uint8_t     rx_channel;
    uint8_t     pkt_type;
};
struct hci_vs_tx_test_cmd
{
    uint8_t     tx_channel;
    uint16_t     test_data_len;
    uint8_t     pkt_payl;
    uint8_t     pkt_type;
    int8_t     tx_pwr_lvl;
};
#endif

void btble_controller_set_task_stack_size(uint16_t stack_size);
void btble_controller_init(uint8_t task_priority);
void ble_controller_deinit(void);
#if defined(CFG_NUTTX)
void btblecontroller_main(void *arg);
uint8_t btblecontroller_change_scan_itl_win(uint16_t interval, uint16_t window);
#endif

extern int32_t btble_controller_sleep(int32_t max_sleep_cycles);
extern void btble_controller_sleep_restore();
#if defined(CFG_BT_RESET)
void ble_controller_reset(void);
#endif

// return sleep duration, in unit of 1/32768s
// if 0, means not allow sleep
// if -1, means allow sleep, but there is no end of sleep interrupt (ble core deep sleep is not enabled)
bool ble_controller_sleep_is_ongoing(void);

char *btble_controller_get_lib_ver(void);

void btble_controller_remaining_mem(uint8_t** addr, int* size);
void btble_controller_set_cs2(uint8_t enable);    // cs2 is enabled by default
#if defined(BL702L)
void btble_controller_sleep_init(void);
typedef int (*btble_before_sleep_cb_t)(void);
typedef void (*btble_after_sleep_cb_t)(void);
typedef void (*btble_sleep_aborted_cb_t)(void);
int8_t btble_controller_get_tx_pwr(void);
void btble_set_before_sleep_callback(btble_before_sleep_cb_t cb);
void btble_set_after_sleep_callback(btble_after_sleep_cb_t cb);
#if !defined(CONFIG_BLE_MFG)
/*
  If ble sleep preparation is aborted before sleep, this callback will be trigerred. Please be noticed, 
  this callback is triggerd after before_sleep_callback.
  e.g. Application disables something before sleep, application needs to enable these when sleep is aborted.
*/
void btble_set_sleep_aborted_callback(btble_sleep_aborted_cb_t cb);
#endif
#endif

#if defined (CONFIG_BLE_MFG) || defined (CONFIG_BT_MFG) 
int bt_mfg_cli_register(void);
int reset_cmd_handler(void);
#if defined (CONFIG_BLE_MFG)
int hci_le_tx_test_v2_cmd_handler(struct hci_le_tx_test_v2_cmd const *param, uint16_t opcode, bool from_hci);
int hci_le_tx_test_v4_cmd_handler(struct hci_le_tx_test_v4_cmd const *param, uint16_t opcode,bool from_hci);
int hci_le_rx_test_v2_cmd_handler(struct hci_le_rx_test_v2_cmd const *param, uint16_t opcode, bool from_hci);
int hci_le_test_end_cmd_handler(void const *param, uint16_t opcode, bool from_hci);
bool ble_check_test_ongoing(void);
#endif
#if defined (CONFIG_BT_MFG)
int hci_vs_rx_test_cmd_handler(struct hci_vs_rx_test_cmd const *param, uint16_t opcode, bool from_hci);
int hci_vs_tx_test_cmd_handler(struct hci_vs_tx_test_cmd const *param, uint16_t opcode, bool from_hci);
int hci_vs_test_end_cmd_handler(void const *param, uint16_t opcode, bool from_hci);
#endif
#endif
#endif
