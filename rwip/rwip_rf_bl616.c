#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "rwip_rf.h"

void btble_rf_init(struct bflb_rwip_rf_api *api);
static struct bflb_rwip_rf_api bflb_api;
static bool rf_txpwr_dec(uint8_t link_id)
{
    if (bflb_api.txpwr_dec) {
        bflb_api.txpwr_dec(link_id, 0);
    }
}
static bool rf_txpwr_inc(uint8_t link_id)
{
    if (bflb_api.txpwr_inc) {
        bflb_api.txpwr_inc(link_id, 0);
    }
}

/**
*****************************************************************************************
* @brief Initialization of RF.
*
* This function initializes the RF and fills the structure containing the function
* pointers and parameters required by the RW BT stack.
*
* @param[out]  api  Pointer to the BT RF API structure
*
*****************************************************************************************
*/
void rwip_bt_rf_init(struct rwip_rf_api *api)
{
    btble_rf_init(&bflb_api);
    api->reset = bflb_api.reset;
    api->force_agc_enable = bflb_api.force_agc_enable;
    api->txpwr_dec = rf_txpwr_dec;               // diff
    api->txpwr_inc = rf_txpwr_inc;               // diff
    api->txpwr_max_set = bflb_api.txpwr_max_set; // ble
    api->txpwr_dbm_get = bflb_api.txpwr_dbm_get;
    api->txpwr_cs_get = bflb_api.txpwr_cs_get_bt; // bt
    api->rssi_convert = bflb_api.rssi_convert_bt; // bt
    api->reg_rd = bflb_api.reg_rd;
    api->reg_wr = bflb_api.reg_wr;
    api->sleep = bflb_api.sleep;
    api->txpwr_min = bflb_api.txpwr_min_bt; // bt
    api->txpwr_max = bflb_api.txpwr_max_bt; // multi
    api->rssi_high_thr = bflb_api.rssi_high_thr;
    api->rssi_low_thr = bflb_api.rssi_low_thr;
    api->rssi_interf_thr = bflb_api.rssi_interf_thr;
    api->wakeup_delay = bflb_api.wakeup_delay;
}