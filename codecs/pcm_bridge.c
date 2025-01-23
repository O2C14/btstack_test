#include "pcm_bridge.h"
#include "bflb_dma.h"
#include "bflb_i2s.h"
#include "bflb_mtimer.h"
#include "bl616_glb.h"
#include "es9038q2m.h"
#include "csi_math.h"
#include <math.h>
#include <shell.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
// #include "1k_sin.h"

static struct bflb_device_s *i2s0;
static struct bflb_device_s *gpio;
struct bflb_device_s *dma0_ch0;
static struct bflb_dma_channel_lli_pool_s tx_llipool[4];
static struct bflb_dma_channel_lli_transfer_s tx_transfers[1];
// 2 channel, 16ms
ATTR_NOCACHE_RAM_SECTION __ALIGNED(32) uint8_t pcm_buffer[MAX_PCM_BUFFER_SIZE];
__ALIGNED(32) uint8_t tmp_pcm_buffer[TEMP_PCM_BUFFER_MAX_SIZE];
#define I2S_LRCK_PIN GPIO_PIN_13
#define I2S_DO_PIN   GPIO_PIN_15
#define I2S_BCLK_PIN GPIO_PIN_20
#define I2C0_SCL     GPIO_PIN_26
#define I2C0_SDA     GPIO_PIN_29
static struct bflb_i2s_config_s i2s_cfg = {
  .bclk_freq_hz =
      48000 * 16 * 2, /* bclk = Sampling_rate * frame_width * channel_num */
  .role = I2S_ROLE_MASTER,
  .format_mode = I2S_MODE_LEFT_JUSTIFIED,
  .channel_mode = I2S_CHANNEL_MODE_NUM_2,
  .frame_width = I2S_SLOT_WIDTH_16,
  .data_width = I2S_SLOT_WIDTH_16,
  .fs_offset_cycle = 0,
  .tx_fifo_threshold = 0,
  .rx_fifo_threshold = 0
};
static struct bflb_dma_channel_config_s tx_config = {
  .direction = DMA_MEMORY_TO_PERIPH,
  .src_req = DMA_REQUEST_NONE,
  .dst_req = DMA_REQUEST_I2S_TX,
  .src_addr_inc = DMA_ADDR_INCREMENT_ENABLE,
  .dst_addr_inc = DMA_ADDR_INCREMENT_DISABLE,
  .src_burst_count = DMA_BURST_INCR1,
  .dst_burst_count = DMA_BURST_INCR1,
  .src_width = DMA_DATA_WIDTH_16BIT,
  .dst_width = DMA_DATA_WIDTH_16BIT, // should equal to i2s frame width
};
#if 0
static ATTR_NOCACHE_RAM_SECTION __ALIGNED(4) uint8_t tx_buffer[38400] = { 0 };
typedef double SCALAR;
int sin_signle_generator(const SCALAR target_freq, const SCALAR sample_rate, uint8_t bits)
{
  uint32_t i = 0;
  const SCALAR _16bitmax = (SCALAR)(0x7FFF);
  const SCALAR _24bitmax = (SCALAR)(0x7FFFFF);
  const SCALAR _32bitmax = (SCALAR)(0x7FFFFFFF);
  const SCALAR aPI = 3.141592653589793238462643383279502884193993;
  SCALAR lenth = 0.;
  for (i = 0; (i < (uint32_t)sample_rate) && (lenth < 0.05); i++) { // 0.050000000000000003
    lenth = (1. / target_freq) * i;
  }
  // printf("%lf, \n",sin(1114.2));
  for (i = 0; i < (uint32_t)(lenth * sample_rate); i++) {
    // printf("%lf, ",sin((target_freq/sample_rate) *2.*aPI* i));
    SCALAR point = sin((target_freq / sample_rate) * 2. * aPI * i);
    switch (bits) {
      case 16:
        point *= _16bitmax;
        ((int16_t *)tx_buffer)[i * 2 + 0] = (int16_t)(point);
        ((int16_t *)tx_buffer)[i * 2 + 1] = (int16_t)(point);
        break;
      case 24:
        point *= _24bitmax;
        int32_t tmp = (int32_t)(point);
        ((uint8_t *)tx_buffer)[i * 6 + 0] = (uint8_t *)(&tmp)[0];
        ((uint8_t *)tx_buffer)[i * 6 + 1] = (uint8_t *)(&tmp)[1];
        ((uint8_t *)tx_buffer)[i * 6 + 2] = (uint8_t *)(&tmp)[2];
        ((uint8_t *)tx_buffer)[i * 6 + 3] = (uint8_t *)(&tmp)[0];
        ((uint8_t *)tx_buffer)[i * 6 + 4] = (uint8_t *)(&tmp)[1];
        ((uint8_t *)tx_buffer)[i * 6 + 5] = (uint8_t *)(&tmp)[2];
        break;
      case 32:
        point *= _32bitmax;
        ((int32_t *)tx_buffer)[i * 2 + 0] = (int32_t)(point);
        ((int32_t *)tx_buffer)[i * 2 + 1] = (int32_t)(point);
        break;
      default:
        break;
    }
  }
  return i * (bits >> 3) * 2;
}
#endif
static void dma0_transfer_done(void *arg);
static void i2s_dma_init()
{
  printf("i2s init\r\n");
  i2s0 = bflb_device_get_by_name("i2s0");
  /* i2s init */
  bflb_i2s_init(i2s0, &i2s_cfg);
  /* enable dma */
  bflb_i2s_link_txdma(i2s0, true);
  printf("dma init\r\n");
  dma0_ch0 = bflb_device_get_by_name("dma0_ch0");
  bflb_dma_channel_init(dma0_ch0, &tx_config);
  /*register i2s callback*/
  bflb_dma_channel_irq_attach(dma0_ch0, dma0_transfer_done, NULL);
  /*
  tx_transfers[0].dst_addr = (uint32_t)DMA_ADDR_I2S_TDR;
  tx_transfers[0].src_addr = (uint32_t)tx_buffer;
  tx_transfers[0].nbytes = sin_generator(1000., (SCALAR)48000, 32);
  */
  printf("dma lli init\r\n");
  uint32_t num = bflb_dma_channel_lli_reload(
      dma0_ch0, 
      tx_llipool, (sizeof(tx_llipool) / sizeof(tx_llipool[0])),
      tx_transfers, (sizeof(tx_transfers) / sizeof(tx_transfers[0])));
  bflb_dma_channel_lli_link_head(dma0_ch0, tx_llipool, num);
  // 这是连续循环模式
  printf("tx dma lli num: %d \r\n", num);
  //bflb_dma_channel_start(dma0_ch0);
}
static void dac_gpio_init(void)
{
  gpio = bflb_device_get_by_name("gpio");
  const uint32_t i2s_cfgset =
      GPIO_FUNC_I2S | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_1;
  // Only GPIO_DRV_1 can work,don't change

  /* I2S_RCLK LRCLK FS LRCK*/
  bflb_gpio_init(gpio, I2S_LRCK_PIN, i2s_cfgset);
  /* I2S_DO */
  bflb_gpio_init(gpio, I2S_DO_PIN, i2s_cfgset);
  /* I2S_BCLK */
  bflb_gpio_init(gpio, I2S_BCLK_PIN, i2s_cfgset);

  const uint32_t i2c_cfgset =
      GPIO_FUNC_I2C0 | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_3;

  /* I2C0_SCL */
  bflb_gpio_init(gpio, I2C0_SCL, i2c_cfgset);
  /* I2C0_SDA */
  bflb_gpio_init(gpio, I2C0_SDA, i2c_cfgset);
}

static void seti2sclock(uint32_t sample_rate, uint32_t data_width)
{
  uint8_t div = 0;
  uint32_t base_freq = 48000;

  if ((sample_rate % 44100) == 0) {
    GLB_Config_AUDIO_PLL_To_451P58M(); // 45.21472
    base_freq = 44100;

  } else if ((sample_rate % 48000) == 0) {
    GLB_Config_AUDIO_PLL_To_491P52M(); //
    base_freq = 48000;
  } else {
    printf("sample_rate error:%d\r\n", sample_rate);
  }
  switch (data_width) {
    case 8:
    case 16:
    case 32:
      break;
    default:
      printf("data_width error:%d use 16bit width\r\n", data_width);
      data_width = 16;
      break;
  }
  // div = (8 / (((sample_rate) / base_freq) * ((data_width) / 16)));
  div = ((8 * base_freq * 16) / (sample_rate * data_width));
  GLB_PER_Clock_UnGate(GLB_AHB_CLOCK_AUDIO);
  GLB_Set_I2S_CLK(ENABLE, div - 1, GLB_I2S_DI_SEL_I2S_DI_INPUT,
                  GLB_I2S_DO_SEL_I2S_DO_OUTPT);
  GLB_Set_Chip_Clock_Out3_Sel(GLB_CHIP_CLK_OUT_3_I2S_REF_CLK);
}

// extern void es9038q2m_init(void);

uint32_t current_sample_rate = 44100;
uint32_t current_data_width = 16;
double size_per_ms = 0.;
uint8_t pcm_inited = 0;
int32_t CURRENT_USED_BYTES = 0;
void pcm_open(uint32_t sample_rate, uint32_t data_width, uint32_t sound_channel_num)
{
  if ((pcm_inited == 0) || (current_sample_rate != sample_rate) ||
      (current_data_width != data_width)) {
    if (data_width == 24) {
      return;
    }
    if ((!sample_rate) || (!data_width) ||
        ((sample_rate % 44100 != 0) && (sample_rate % 48000 != 0))) {
      sample_rate = 44100;
      data_width = 16;
    }
    pcm_inited = 1;
    dac_gpio_init();
    es9038q2m_init();
    es9038q2m_set_data_width(data_width);
    // set i2s clock
    seti2sclock(sample_rate, data_width);

    // set i2s and dma config
    i2s_cfg.bclk_freq_hz = sample_rate * data_width * sound_channel_num;
    printf("samplerate:%d data_width:%d channel:%d\r\n", sample_rate, data_width, sound_channel_num);
    i2s_cfg.tx_fifo_threshold = 16 - 1;
    i2s_cfg.rx_fifo_threshold = 16 - 1;
    const bool Used_DMA_BURST = true; //打开这个可能会导致突然无声
    if (data_width == 16) {
      i2s_cfg.frame_width = I2S_SLOT_WIDTH_16;
      i2s_cfg.data_width = I2S_SLOT_WIDTH_16;
      if (Used_DMA_BURST) {
        i2s_cfg.tx_fifo_threshold = 8 - 1;
        i2s_cfg.rx_fifo_threshold = 8 - 1;
        tx_config.src_burst_count = DMA_BURST_INCR8;
        tx_config.dst_burst_count = DMA_BURST_INCR8;
        tx_config.src_width = DMA_DATA_WIDTH_16BIT;
        tx_config.dst_width = DMA_DATA_WIDTH_16BIT;
      }
    } else if (data_width == 32) {
      i2s_cfg.frame_width = I2S_SLOT_WIDTH_32;
      i2s_cfg.data_width = I2S_SLOT_WIDTH_32;
      if (Used_DMA_BURST) {
        i2s_cfg.tx_fifo_threshold = 4 - 1;
        i2s_cfg.rx_fifo_threshold = 4 - 1;
        tx_config.src_burst_count = DMA_BURST_INCR4;
        tx_config.dst_burst_count = DMA_BURST_INCR4;
        tx_config.src_width = DMA_DATA_WIDTH_32BIT;
        tx_config.dst_width = DMA_DATA_WIDTH_32BIT;
      }
    } else {
      printf("data_width error\n");
    }
    size_per_ms = (((double)sample_rate) / 1000.) * (((double)data_width) / 8.) * 2.;
    //CURRENT_USED_BYTES = (uint32_t)(192. * 64.); //64ms
    if (!(sample_rate % 44100)) {
      CURRENT_USED_BYTES = (int32_t)(64. * ((48000. * (double)(sample_rate / 44100)) / 1000.) * (((double)data_width) / 8.) * 2.);
    } else {
      CURRENT_USED_BYTES = (int32_t)(64. * size_per_ms);
    }
    tx_transfers[0].src_addr = (uint32_t)pcm_buffer;
    tx_transfers[0].dst_addr = (uint32_t)DMA_ADDR_I2S_TDR;
    tx_transfers[0].nbytes = (uint32_t)CURRENT_USED_BYTES; // 实际用的
    if (CURRENT_USED_BYTES > sizeof(pcm_buffer)) {
      printf("i2s buffer size %d bigger than %d\n", CURRENT_USED_BYTES, sizeof(pcm_buffer));
      CURRENT_USED_BYTES = sizeof(pcm_buffer);
    } else {
      printf("i2s buffer size %d\n", CURRENT_USED_BYTES);
    }
    i2s_dma_init();

    bflb_i2s_feature_control(i2s0, I2S_CMD_DATA_ENABLE, I2S_CMD_DATA_ENABLE_TX);

    current_sample_rate = sample_rate;
    current_data_width = data_width;
  } else {
    i2s_stop();
  }
}

static uint64_t last_tick = 0;
static bool i2s_log = 0;
static uint64_t use_tick()
{
  uint64_t now = bflb_mtimer_get_time_us();
  uint64_t tmp = 0;
  if (last_tick) {
    tmp = now - last_tick;
  }
  last_tick = now;
  return tmp;
}
int64_t writen_count = 0;
static void dma0_transfer_done(void *arg)
{
  writen_count -= (int64_t)CURRENT_USED_BYTES;
  if(writen_count < 0){
    i2s_stop();//重新同步
    printf("Underrun\n");//欠载
    //或者不停止,让 pcm_data_index 指向缓冲区头部
  }
  if (i2s_log) {
    printf("timeout(ms) %d\n",(int32_t)(((double)(-writen_count))/size_per_ms));
  }
}

int32_t pcm_data_index = 0;
int32_t reset_index = 0;
void set_start_loc(int32_t index)
{
  reset_index = index;
  if (!get_i2s_status()) {
    /*
    LDAC解决播放卡顿的猜想与解决方案:
    ACL 流刚建立起来的时候 pcm流的写入速度会稍慢于i2s发送的速度
    为了防止i2s的发送进度越过写入进度(套圈)可以将pcm流的起始位置放在i2s缓冲区的末尾
    */

    for (size_t i = 0; i < reset_index; i += 4) {
      *((uint32_t *)(&pcm_buffer[i])) = 0;
    }

    pcm_data_index = reset_index;
    last_tick = 0;
    writen_count = reset_index;
  }
}
void check_and_start()
{
  //if (((1<<17)&(*(volatile uint32_t *)(uintptr_t)(dma0_ch0->reg_base + (0x10))))==0) 
  {
    i2s_start();
  }
}
void check_buffer_edge(uint32_t size)
{
  writen_count += size;
  pcm_data_index += size;
  //越界检查
  if (pcm_data_index >= CURRENT_USED_BYTES) {
    if (pcm_data_index > CURRENT_USED_BYTES) {
      memcpy(&pcm_buffer[0], &pcm_buffer[CURRENT_USED_BYTES], pcm_data_index - CURRENT_USED_BYTES);
    }
    pcm_data_index = 0;
  }
  check_and_start();
}
inline void *get_pcm_tail()
{
  return pcm_buffer + pcm_data_index;
}

int pcm_write(const uint8_t *buf, uint32_t size)
{
  set_start_loc(4096);
  memcpy(&pcm_buffer[pcm_data_index], buf, size);
  check_buffer_edge(size);
  return 1;
}

static void lli_stop();
int i2s_stop(void)
{
  if (get_i2s_status()) {

    lli_stop();
  }
  return 1;
}


bool i2s_start(void)
{
  if (get_i2s_status()) {
    return false;
  }
  //bflb_i2s_link_txdma(i2s0, true);
  uint32_t num = bflb_dma_channel_lli_reload(
      dma0_ch0, tx_llipool, (sizeof(tx_llipool) / sizeof(tx_llipool[0])),
      tx_transfers, 1);
  bflb_dma_channel_lli_link_head(dma0_ch0, tx_llipool, num);

  bflb_dma_channel_start(dma0_ch0);
  printf("i2s_start\r\n");
  return true;
}
bool get_i2s_status(void)
{
  //getreg32(channel_base + DMA_CxCONFIG_OFFSET);
  return (*(volatile uint32_t *)(uintptr_t)(dma0_ch0->reg_base + (0x10))) & 1;
  //return !(*((uint32_t *)0x2000AB04)&1);

}
static void set_i2s_status(bool enable)
{
  if (enable) {
    (*(volatile uint32_t *)(uintptr_t)(dma0_ch0->reg_base + (0x10))) |= 1;
  } else {
    (*(volatile uint32_t *)(uintptr_t)(dma0_ch0->reg_base + (0x10))) &= ~1;
  }
}
//断开dma传输循环,让i2s自动停止,确保下一次启动dma时dma的传输指针指向缓冲区开头。
static void lli_stop()
{
  printf("lli_stop\n");
  (*(volatile uint32_t *)(uintptr_t)(dma0_ch0->reg_base + (0x08))) = 0;
}

void i2s_shell(int argc, char **argv)
{
  if (argc >= 2) {
    if (strcmp(argv[1], "start") == 0) {
      uint32_t num = bflb_dma_channel_lli_reload(
          dma0_ch0, tx_llipool, (sizeof(tx_llipool) / sizeof(tx_llipool[0])),
          tx_transfers, 1);
      bflb_dma_channel_lli_link_head(dma0_ch0, tx_llipool, num);

      bflb_dma_channel_start(dma0_ch0);
    } else if (strcmp(argv[1], "stop") == 0) {
      i2s_stop();
    } else if (strcmp(argv[1], "log") == 0) {
      i2s_log = 1;
    } else if (strcmp(argv[1], "unlog") == 0) {
      i2s_log = 0;
    } else if (strcmp(argv[1], "reset") == 0) {
      pcm_inited = 0;
      pcm_open(current_sample_rate, current_data_width, 2);
    }else if (strcmp(argv[1], "18db0") == 0) {
      set18dbgain(0);
    }else if (strcmp(argv[1], "18db1") == 0) {
      set18dbgain(3);
    }else if (strcmp(argv[1], "status") == 0) {

      printf("0x2000AB00 %x\n",*((int*)(0x2000AB00)));
      printf("0x2000AB04 %x\n",*((int*)(0x2000AB04)));
      printf("0x2000AB10 %x\n",*((int*)(0x2000AB10)));
      printf("0x2000AB80 %x\n",*((int*)(0x2000AB80)));
      printf("0x2000AB84 %x\n",*((int*)(0x2000AB84)));
      printf("0x2000AB88 %x\n",*((int*)(0x2000AB88)));
      printf("0x2000AB8C %x\n",*((int*)(0x2000AB8C)));
      printf("0x2000ABFC %x\n\n",*((int*)(0x2000ABFC)));

      printf("0x2000c000 %x\n",*((int*)(0x2000c000)));
      printf("0x2000c004 %x\n",*((int*)(0x2000c004)));
      printf("0x2000c008 %x\n",*((int*)(0x2000c008)));
      printf("0x2000c00c %x\n",*((int*)(0x2000c00c)));
      printf("0x2000c010 %x\n",*((int*)(0x2000c010)));
      printf("0x2000c014 %x\n",*((int*)(0x2000c014)));
      printf("0x2000c018 %x\n",*((int*)(0x2000c018)));
      printf("0x2000c01c %x\n",*((int*)(0x2000c01c)));
      printf("0x2000c020 %x\n",*((int*)(0x2000c020)));
      printf("0x2000c024 %x\n",*((int*)(0x2000c024)));
      printf("0x2000c028 %x\n",*((int*)(0x2000c028)));
      printf("0x2000c02c %x\n",*((int*)(0x2000c02c)));
      printf("0x2000c030 %x\n",*((int*)(0x2000c030)));
      printf("0x2000c034 %x\n",*((int*)(0x2000c034)));
      printf("0x2000c100 %x\n",*((int*)(0x2000c100)));
      printf("0x2000c104 %x\n",*((int*)(0x2000c104)));
      printf("0x2000c108 %x\n",*((int*)(0x2000c108)));
      printf("0x2000c10c %x\n",*((int*)(0x2000c10c)));
      printf("0x2000c110 %x\n",*((int*)(0x2000c110)));

    }else if (strcmp(argv[1], "clear") == 0) {

      *((int*)(0x2000c000))=0;
      *((int*)(0x2000c004))=0;
      *((int*)(0x2000c008))=0;
      *((int*)(0x2000c00c))=0;
      *((int*)(0x2000c010))=0;
      *((int*)(0x2000c014))=0;
      *((int*)(0x2000c018))=0;
      *((int*)(0x2000c01c))=0;
      *((int*)(0x2000c020))=0;
      *((int*)(0x2000c024))=0;
      *((int*)(0x2000c028))=0;
      *((int*)(0x2000c02c))=0;
      *((int*)(0x2000c030))=0;
      *((int*)(0x2000c034))=0;
      *((int*)(0x2000c100))=0;
      *((int*)(0x2000c104))=0;
      *((int*)(0x2000c108))=0;
      *((int*)(0x2000c10c))=0;
      *((int*)(0x2000c110))=0;
    }
  }
}
/*
0x2000c000 0
0x2000c004 0
0x2000c008 0
0x2000c00c 0
0x2000c010 0
0x2000c014 0
0x2000c018 0
0x2000c01c 1
0x2000c020 0
0x2000c024 0
0x2000c028 0
0x2000c02c 0
0x2000c030 1
0x2000c034 0
0x2000c100 22fc10c0
0x2000c104 2000ab88
0x2000c108 62fd19f0
0x2000c10c 4252968 11:0 剩余数,可能会等于字节数 31 显示当前lli能否引起中断
0x2000c110 4024c41


0x2000c10c 84252438
0x2000c110 3f24c41
*/

/*
0x2000c000 0
0x2000c004 0
0x2000c008 0
0x2000c00c 0
0x2000c010 0
0x2000c014 0
0x2000c018 0
0x2000c01c 1
0x2000c020 20000
0x2000c024 0
0x2000c028 0
0x2000c02c 0
0x2000c030 1
0x2000c034 0
0x2000c100 22fc07a0
0x2000c104 2000ab88
0x2000c108 62fd19f0
0x2000c10c 4252e18
0x2000c110 28624c41


0x2000c000 0
0x2000c004 0
0x2000c008 0
0x2000c00c 0
0x2000c010 0
0x2000c014 0
0x2000c018 0
0x2000c01c 1
0x2000c020 20000
0x2000c024 0
0x2000c028 0
0x2000c02c 0
0x2000c030 1
0x2000c034 0
0x2000c100 22fc2930
0x2000c104 2000ab88
0x2000c108 62fd19e0
0x2000c10c 84252570
0x2000c110 2a324c41
*/
SHELL_CMD_EXPORT_ALIAS(i2s_shell, i2s, i2s);