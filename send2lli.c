#include <stdint.h>
#include "bflb_dma.h"
#include "bflb_i2s.h"
extern struct bflb_device_s * dma0_ch0;

uint8_t lli_mem_pool[3][4096];
static struct bflb_dma_channel_lli_pool_s tx_llipool[4];
static struct bflb_dma_channel_lli_transfer_s tx_transfers[1];
void lli_write(){
  if (get_i2s_status()) {
    tx_llipool[0].nextlli = 
    return;
  }
  tx_transfers[0].src_addr = (uint32_t)pcm_buffer;
  tx_transfers[0].dst_addr = (uint32_t)DMA_ADDR_I2S_TDR;
  tx_transfers[0].nbytes = (uint32_t)CURRENT_USED_BYTES; // 实际用的
  uint32_t num = bflb_dma_channel_lli_reload(dma0_ch0,
   tx_llipool, 
   (sizeof(tx_llipool) / sizeof(tx_llipool[0])),
   tx_transfers, 
   1);
}