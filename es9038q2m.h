#ifndef __es9038q2m_H__
#define __es9038q2m_H__
#include "stdint.h"
#include "stdio.h"
union __attribute__((packed)) Registers_bits
{
    uint8_t datas[55];
    struct __attribute__((packed)) 
    {
        uint8_t System_Registers;//0
        uint8_t Input_selection;//1
        uint8_t Automute_Configuration;//2
        uint8_t SPDIF_Configuration;//3
        uint8_t Automute_Time;//4
        uint8_t Automute_Level;//5
        uint8_t De_emphasis;//6
        uint8_t Filter_Bandwidth;//7
        uint8_t GPIO1_2_Configuration;//8
        const uint8_t Register_9;
        uint8_t Master_Mode;//10
        uint8_t SPDIF_Select;//11
        uint8_t ASRC_DPLL_Bandwidth;//12
        uint8_t THD_Bypass;//13
        uint8_t Soft_Start_Configuration;//14
        uint8_t Volume_Control[2];//15 16
        uint32_t Master_Trim;// 17 18 19 20
        uint8_t GPIO_Input_Selection;//21
        uint16_t THD_Compensation_C2;//22 23
        uint16_t THD_Compensation_C3;//24 25
        const uint8_t Register_26;//
        uint8_t General_Configuration;//27
        const uint8_t Register_28;//28
        uint8_t GPIO_Configuration;//29
        uint16_t Charge_Pump_Cloc;//30 31
        const uint8_t Register_32;
        uint8_t Interrupt_Mask;//33
        uint32_t Programmable_NCO;//34 35 36 37
        const uint8_t Register_38;
        uint8_t General_Configuration_2;//39
        uint8_t Programmable_FIR_RAM_Address;//40
        uint8_t Programmable_FIR_RAM_Data[3];//41 42 43
        uint8_t Programmable_FIR_Configuration;//44
        uint8_t Low_Power_and_Auto_Calibration;//45
        uint8_t ADC_Configuration;//46
        uint16_t adc_ftr_scale[3];//47 52
    };
    
};
union __attribute__((packed)) only_read_Registers_bits{
    uint8_t datas[39];
    struct __attribute__((packed)) {
        uint8_t Chip_ID_and_Status;
        uint8_t GPIO_Readback;
        uint32_t DPLL_Number;
        uint8_t SPDIF_Channel_Status[24];
        const uint8_t Register_94;
        const uint8_t Register_95;
        uint8_t Input_Selection_and_Automute_Status;
        const uint8_t Register_97_99[3];
        uint8_t ADC_Readback[3];
    };
};
union __attribute__((packed)) All_Registers_bits{
    uint8_t datas[103];
    struct __attribute__((packed)) {
        union Registers_bits canwrite;
        const uint8_t nine_bytes[9];
        union only_read_Registers_bits only_read;
    };
};


void es9038q2m_I2C_Init(void);
void es9038q2m_Reg_Dump(void);
union All_Registers_bits *get_reg(void);
void es9038q2m_Reg_over_write(void);
void set18dbgain(uint8_t status);
// 下面是必要的接口
void es9038q2m_init(void);
void es9038q2m_set_volume(int volume);
void es9038q2m_set_data_width(uint8_t bit_width);

#endif /* __es9038q2m_H__ */
