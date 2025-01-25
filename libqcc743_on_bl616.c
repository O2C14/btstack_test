#include "bflb_efuse.h"
#include "bflb_irq.h"
void qcc74x_irq_clear_pending(int irq){
    bflb_irq_clear_pending(irq);
}
int qcc74x_irq_attach(int irq, irq_callback isr, void *arg){
    return bflb_irq_attach(irq, isr, arg);
}
void qcc74x_irq_enable(int irq){
    bflb_irq_enable(irq);
}
void qcc74x_irq_disable(int irq){
    bflb_irq_disable(irq);
}
void qcc74x_efuse_get_device_info(bflb_efuse_device_info_type *device_info){
    bflb_efuse_get_device_info(device_info);
}
