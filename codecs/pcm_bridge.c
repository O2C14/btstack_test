#include "pcm_bridge.h"
#include "bflb_dma.h"
#include "bflb_i2s.h"
#include "bflb_mtimer.h"
#include "bl616_glb.h"
#include "es9038q2m.h"
#include <shell.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "gpio_config.h"
#include "bflb_clock.h"
#include "glb_reg.h"
// #include "1k_sin.h"

static struct bflb_device_s *i2s0;
static struct bflb_device_s *gpio;
struct bflb_device_s *dma0_ch0;
static struct bflb_dma_channel_lli_pool_s tx_llipool[40];
static struct bflb_dma_channel_lli_transfer_s tx_transfers[20];
// 2 channel, 16ms
ATTR_NOCACHE_RAM_SECTION uint8_t pcm_buffer[MAX_PCM_BUFFER_SIZE];
uint8_t tmp_pcm_buffer[TEMP_PCM_BUFFER_MAX_SIZE];

static struct bflb_i2s_config_s i2s0_config = {
    .bclk_freq_hz =
        48000 * 16 * 2, /* bclk = Sampling_rate * frame_width * channel_num */
    .role = I2S_ROLE_MASTER,
    .format_mode = I2S_MODE_LEFT_JUSTIFIED,
    .channel_mode = I2S_CHANNEL_MODE_NUM_2,
    .frame_width = I2S_SLOT_WIDTH_16,
    .data_width = I2S_SLOT_WIDTH_16,
    .fs_offset_cycle = 1,
    .tx_fifo_threshold = 0,
    .rx_fifo_threshold = 0
};
static struct bflb_dma_channel_config_s dma0_ch0_config = {
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

static void dma0_transfer_done(void *arg);
static void i2s_dma_init()
{
    printf("i2s init\r\n");
    i2s0 = bflb_device_get_by_name("i2s0");
    /* i2s init */
    bflb_i2s_init(i2s0, &i2s0_config);
    /* enable dma */
    bflb_i2s_link_txdma(i2s0, true);
    printf("dma init\r\n");
    dma0_ch0 = bflb_device_get_by_name("dma0_ch0");
    bflb_dma_channel_init(dma0_ch0, &dma0_ch0_config);
    /*register i2s callback*/
    bflb_dma_channel_irq_attach(dma0_ch0, dma0_transfer_done, NULL);
    /*
    tx_transfers[0].dst_addr = (uint32_t)DMA_ADDR_I2S_TDR;
    tx_transfers[0].src_addr = (uint32_t)tx_buffer;
    tx_transfers[0].nbytes = sin_generator(1000., (SCALAR)48000, 32);
  
    printf("dma lli init\r\n");
    uint32_t num = bflb_dma_channel_lli_reload(
        dma0_ch0,
        tx_llipool, (sizeof(tx_llipool) / sizeof(tx_llipool[0])),
        tx_transfers, (sizeof(tx_transfers) / sizeof(tx_transfers[0])));
    bflb_dma_channel_lli_link_head(dma0_ch0, tx_llipool, num);
    // 这是连续循环模式
    printf("tx dma lli num: %d \r\n", num);
    */
    // bflb_dma_channel_start(dma0_ch0);
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

    bflb_gpio_init(gpio, GPIO_PIN_12, GPIO_OUTPUT | GPIO_PULLDOWN | GPIO_SMT_EN | GPIO_DRV_2);
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

uint32_t current_sample_rate = 44100;
uint32_t current_data_width = 16;
double size_per_ms = 0.;
uint8_t pcm_inited = 0;
int32_t cur_base_buffer_size = 0;
int32_t cur_buffer_num = 0;
uint8_t dma_backup_flag = 0;
uint8_t i2s_backup_flag = 0;

int32_t LastTransferSize = 0;
void pcm_open(uint32_t sample_rate, uint32_t data_width, uint32_t sound_channel_num, uint32_t base_buffer_size, uint32_t buffer_num)
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
        i2s0_config.bclk_freq_hz = sample_rate * data_width * sound_channel_num;
        printf("samplerate:%d data_width:%d channel:%d\r\n", sample_rate, data_width, sound_channel_num);
        printf("base_buffer_size:%d buffer_num:%d \r\n", base_buffer_size, buffer_num);
        i2s0_config.tx_fifo_threshold = 8 - 1;
        i2s0_config.rx_fifo_threshold = 8 - 1;
        const bool Used_DMA_BURST = true;
        if (data_width == 16) {
            i2s0_config.frame_width = I2S_SLOT_WIDTH_16;
            i2s0_config.data_width = I2S_SLOT_WIDTH_16;
            if (Used_DMA_BURST) {
                dma0_ch0_config.src_burst_count = DMA_BURST_INCR4;
                dma0_ch0_config.dst_burst_count = DMA_BURST_INCR4;
                dma0_ch0_config.src_width = DMA_DATA_WIDTH_16BIT;
                dma0_ch0_config.dst_width = DMA_DATA_WIDTH_16BIT;
            }
        } else if (data_width == 32) {
            i2s0_config.frame_width = I2S_SLOT_WIDTH_32;
            i2s0_config.data_width = I2S_SLOT_WIDTH_32;
            if (Used_DMA_BURST) {
                dma0_ch0_config.src_burst_count = DMA_BURST_INCR1;
                dma0_ch0_config.dst_burst_count = DMA_BURST_INCR1;
                dma0_ch0_config.src_width = DMA_DATA_WIDTH_32BIT;
                dma0_ch0_config.dst_width = DMA_DATA_WIDTH_32BIT;
            }
        } else {
            printf("data_width error\n");
        }
        size_per_ms = (((double)sample_rate) / 1000.) * (((double)data_width) / 8.) * 2.;
        cur_base_buffer_size = base_buffer_size;
        cur_buffer_num = buffer_num;
        if (base_buffer_size * buffer_num > sizeof(pcm_buffer)) {
            printf("i2s buffer size %d bigger than %d\n", cur_base_buffer_size, sizeof(pcm_buffer));
            return;
        }
        printf("i2s buffer size %d\n", base_buffer_size * buffer_num);

        for (int i = 0; i < buffer_num; i++) {
            tx_transfers[i].src_addr = (uint32_t)pcm_buffer + i * base_buffer_size;
            tx_transfers[i].dst_addr = (uint32_t)DMA_ADDR_I2S_TDR;
            tx_transfers[i].nbytes = (uint32_t)base_buffer_size; // 实际用的
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
    } else {
        return 1;
    }
    last_tick = now;
    return tmp;
}
int64_t writen_count = 0;
static void dma0_transfer_done(void *arg)
{
    writen_count -= (int64_t)cur_base_buffer_size;
    /*
    if (writen_count < 0) {
        i2s_stop();            // 重新同步
        printf("underload\n"); // 欠载
    }
    if (writen_count < cur_base_buffer_size) {
        printf("pre underload\n"); // 这样也会听到卡顿?
    }
    */
    if (writen_count <= 0) {   // writen_count == 0 不一定欠载, 但是还是会听到卡顿
        i2s_stop();            // 重新同步
        printf("underload\n"); // 欠载
    }

    if (writen_count > cur_base_buffer_size * (cur_buffer_num - 1)) {
        i2s_stop();           // 重新同步
        printf("overload\n"); // 过载
    }

    /*
    if (writen_count > cur_base_buffer_size * (cur_buffer_num - 2)) {
        printf("pre overload\n"); // 这样也会听到卡顿?
    }
    */

    if (i2s_log) {
        printf("timeout(ms) %d\n", (int32_t)(((double)(-writen_count)) / size_per_ms));
    }
}

int32_t pcm_data_index = 0;
int32_t reset_index = 0;

void set_start_loc(int32_t index)
{
    reset_index = index;
    if (!get_dma_status()) {
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
    } else { //启动后没有进行传输,证明i2s已经崩溃
        if (LastTransferSize == ((*((int *)(0x2000c10c))) & 0xfff)) {
            //printf("i2s crash\n");
            //这里应该要实现i2s 或 dma的复位,但是目前找不到可用的方法,reset_i2s0()是没有用的
            //所以我只是亮起红灯
            bflb_gpio_set(gpio, GPIO_PIN_12);
        } else {
            bflb_gpio_reset(gpio, GPIO_PIN_12);
        }
        LastTransferSize = ((*((int *)(0x2000c10c))) & 0xfff);
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
    // 越界检查
    if (pcm_data_index >= cur_base_buffer_size * cur_buffer_num) {
        if (pcm_data_index > cur_base_buffer_size * cur_buffer_num) {
            memcpy(&pcm_buffer[0], &pcm_buffer[cur_base_buffer_size * cur_buffer_num], pcm_data_index - cur_base_buffer_size * cur_buffer_num);
        }
        pcm_data_index -= cur_base_buffer_size * cur_buffer_num;
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

static void dma0_ch0_lli_stop();
static void dma0_ch0_stop();
int i2s_stop(void)
{
    // dma0_ch0_lli_stop();
    dma0_ch0_stop();

    return 1;
}

bool i2s_start(void)
{
    if (get_dma_status()) {
        return false;
    }
    //bflb_i2s_link_txdma(i2s0, true);
    uint32_t num = bflb_dma_channel_lli_reload(
        dma0_ch0, tx_llipool, (sizeof(tx_llipool) / sizeof(tx_llipool[0])),
        tx_transfers, cur_buffer_num);
    bflb_dma_channel_lli_link_head(dma0_ch0, tx_llipool, num);

    bflb_dma_channel_start(dma0_ch0);
    printf("i2s_start\r\n");
    return true;
}
bool get_dma_status(void)
{
    // getreg32(channel_base + DMA_CxCONFIG_OFFSET);
    // return (*((uint32_t *)0x2000c01c) & 1); // dma0
    // return !(*((uint32_t *)0x2000AB04) & 1);// i2s
    return (*(uint32_t *)(dma0_ch0->reg_base + (0x10)) & 1) != 0;
}
static void set_i2s_status(bool enable)
{
    if (enable) {
        (*(volatile uint32_t *)(uintptr_t)(dma0_ch0->reg_base + (0x10))) |= 1;
    } else {
        (*(volatile uint32_t *)(uintptr_t)(dma0_ch0->reg_base + (0x10))) &= ~1;
    }
}
// 断开dma传输循环,让i2s自动停止,确保下一次启动dma时dma的传输指针指向缓冲区开头。
static void dma0_ch0_lli_stop()
{
    printf("dma0_ch0_lli_stop\n");
    (*(volatile uint32_t *)(uintptr_t)(dma0_ch0->reg_base + (0x08))) = 0;
}
// 直接停止
static void dma0_ch0_stop()
{
    printf("dma0_ch0_stop\n");
    bflb_dma_channel_stop(dma0_ch0);
}
static void reset_i2s0(void)
{
    uint32_t tmpVal = 0;
    // PERIPHERAL_CLOCK_I2S_ENABLE();
    /**/

    tmpVal = getreg32(GLB_BASE + GLB_SWRST_CFG1_OFFSET);
    tmpVal |= GLB_SWRST_S1AB_MSK; // software reset I2S
    putreg32(tmpVal, GLB_BASE + GLB_SWRST_CFG1_OFFSET);
    tmpVal &= ~GLB_SWRST_S1AB_MSK; // software reset I2S
    putreg32(tmpVal, GLB_BASE + GLB_SWRST_CFG1_OFFSET);

    tmpVal = getreg32(GLB_BASE + GLB_SWRST_CFG1_OFFSET);
    tmpVal |= GLB_SWRST_S1C_MSK; // software reset DMA
    putreg32(tmpVal, GLB_BASE + GLB_SWRST_CFG1_OFFSET);
    tmpVal &= ~GLB_SWRST_S1C_MSK; // software reset DMA
    putreg32(tmpVal, GLB_BASE + GLB_SWRST_CFG1_OFFSET);
}

void i2s_shell(int argc, char **argv)
{
    if (argc >= 2) {
        if (strcmp(argv[1], "start") == 0) {
            uint32_t num = bflb_dma_channel_lli_reload(
                dma0_ch0, tx_llipool, (sizeof(tx_llipool) / sizeof(tx_llipool[0])),
                tx_transfers, cur_buffer_num);
            bflb_dma_channel_lli_link_head(dma0_ch0, tx_llipool, num);

            bflb_dma_channel_start(dma0_ch0);
        } else if (strcmp(argv[1], "stop") == 0) {
            //i2s_stop();
            set_i2s_status(0);
        } else if (strcmp(argv[1], "lli_stop") == 0) {
            dma0_ch0_lli_stop();
        } else if (strcmp(argv[1], "log") == 0) {
            i2s_log = 1;
        } else if (strcmp(argv[1], "unlog") == 0) {
            i2s_log = 0;
        } else if (strcmp(argv[1], "reset") == 0) {
            pcm_inited = 0;
            pcm_open(current_sample_rate, current_data_width, 2, cur_base_buffer_size, cur_buffer_num);
        } else if (strcmp(argv[1], "18db0") == 0) {
            set18dbgain(0);
        } else if (strcmp(argv[1], "18db1") == 0) {
            set18dbgain(3);
        }
    }
}

SHELL_CMD_EXPORT_ALIAS(i2s_shell, i2s, i2s);