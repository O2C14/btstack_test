#ifndef RWIP_RF_H
#define RWIP_RF_H
#include <stdint.h>
#include <stdbool.h>
/// API functions of the RF driver that are used by the BLE or BT software
struct rwip_rf_api {
    /// Function called upon HCI reset command reception
    void (*reset)(void);
    /// Function called to enable/disable force AGC mechanism (true: en / false : dis)
    void (*force_agc_enable)(bool);
    /// Function called when TX power has to be decreased for a specific link id
    bool (*txpwr_dec)(uint8_t);
    /// Function called when TX power has to be increased for a specific link id
    bool (*txpwr_inc)(uint8_t);
    /// Function called when TX power has to be set to max for a specific link id
    void (*txpwr_max_set)(uint8_t);
    /// Function called to convert a TX power CS power field into the corresponding value in dBm
    uint8_t (*txpwr_dbm_get)(uint8_t, uint8_t);
    /// Function called to convert a power in dBm into a control structure tx power field
    uint8_t (*txpwr_cs_get)(int8_t, bool);
    /// Function called to convert the RSSI read from the control structure into a real RSSI
    int8_t (*rssi_convert)(uint8_t);
    /// Function used to read a RF register
    uint32_t (*reg_rd)(uint32_t);
    /// Function used to write a RF register
    void (*reg_wr)(uint32_t, uint32_t);
    /// Function called to put the RF in deep sleep mode
    void (*sleep)(void);
    /// Index of minimum TX power
    uint8_t txpwr_min;
    /// Index of maximum TX power
    uint8_t txpwr_max;
    /// RSSI high threshold ('real' signed value in dBm)
    int8_t rssi_high_thr;
    /// RSSI low threshold ('real' signed value in dBm)
    int8_t rssi_low_thr;
    /// interferer threshold ('real' signed value in dBm)
    int8_t rssi_interf_thr;
    /// RF wakeup delay (in slots)
    uint8_t wakeup_delay;
};

struct bflb_rwip_rf_api // sizeof=0x50
{
    /// Function called upon HCI reset command reception
    void (*reset)(void);
    /// Function called to enable/disable force AGC mechanism (true: en / false : dis)
    void (*force_agc_enable)(bool);
    /// Function called when TX power has to be decreased for a specific link id
    bool (*txpwr_dec)(uint8_t, bool is_edr);
    /// Function called when TX power has to be increased for a specific link id
    bool (*txpwr_inc)(uint8_t, bool);

    int8_t (*txpwr_bt_curr)(void);

    int8_t (*txpwr_bt_edr_curr)(void);

    void (*ble_txpwr_max_set)(int8_t);
    
    int8_t (*ble_txpwr_max_get)(void);
    /// Function called when TX power has to be set to max for a specific link id
    void (*txpwr_max_set)(uint8_t, bool);
    /// Function called to convert a TX power CS power field into the corresponding value in dBm
    int8_t (*txpwr_dbm_get)(int8_t, uint8_t);
    /// Function called to convert a power in dBm into a control structure tx power field
    int8_t (*txpwr_cs_get)(int8_t, uint8_t);

    int8_t (*txpwr_cs_get_bt)(int8_t, uint8_t);
    /// Function called to convert the RSSI read from the control structure into a real RSSI
    int8_t (*rssi_convert)(uint8_t);

    int8_t (*rssi_convert_bt)(uint8_t);
    /// Function used to read a RF register
    uint32_t (*reg_rd)(uint32_t);
    void (*reg_wr)(uint32_t, uint32_t);
    /// Function called to put the RF in deep sleep mode
    void (*sleep)(void);
    /// Index of maximum TX power
    int8_t txpwr_min;
    /// RSSI high threshold ('real' signed value in dBm)
    int8_t txpwr_max;

    int8_t txpwr_ble_target;

    uint8_t txpwr_min_bt;

    uint8_t txpwr_max_bt;

    uint8_t txpwr_max_bt_edr;

    uint8_t txpwr_bt_target;

    uint8_t txpwr_bt_edr_target;

    int8_t rssi_high_thr;
    /// RSSI low threshold ('real' signed value in dBm)
    int8_t rssi_low_thr;
    /// interferer threshold ('real' signed value in dBm)
    int8_t rssi_interf_thr;
    /// RF wakeup delay (in slots)
    uint8_t wakeup_delay;
};

#endif