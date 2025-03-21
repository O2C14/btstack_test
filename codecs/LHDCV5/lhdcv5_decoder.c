#include <stdio.h>
#include "a2dp_decoder.h"
#include "btstack.h"
#include "lhdcv5_coder.h"
#include "lhdc_v5_dec.h"
#include "lhdc_v5_dec_workspace.h"
#include "lhdcv5BT_dec.h"
#include "pcm_bridge.h"

static bool a2dp_lhdcv5_decoder_decode_packet(void *lhdcBT_dec_handle, uint16_t con_handle, uint8_t *packet, uint16_t size)
{
    int32_t out_pcm_szie = 0;

    uint32_t lhdc_total_frame_nb = *packet >> 2;
    uint32_t check_size = 2;
    for (int i = 0; i < lhdc_total_frame_nb && check_size < size; i++) {
        set_start_loc(0);
        uint32_t size_per_ch = *(uint16_t *)(packet + check_size) & 0b1111111111;
        lhdc_v5_dec_decode(get_pcm_tail(), packet + check_size, &out_pcm_szie, lhdcBT_dec_handle);
        check_size += size_per_ch * MAX_CHANNELS + sizeof(uint16_t);
        check_buffer_edge(out_pcm_szie);
    }
    if (check_size != size) {
        // log_info("incomplete frame");
        printf("incomplete frame\n");
    }
    return 1;
}

static void a2dp_lhdcv5_decoder_decoder_cleanup(void *lhdcBT_dec_handle)
{
    free(lhdcBT_dec_handle);
}
static void *a2dp_lhdcv5_decoder_configure(const tA2DP_LHDCv5_CIE *codec_cfg, pcm_info *info_out)
{
    printf("lhdcv5\n");
    if (codec_cfg->vendorId != A2DP_LHDC_VENDOR_ID) {
        printf("lhdcv5 LHDC_VENDOR_ID error %x\n",codec_cfg->vendorId);
    }
    if (codec_cfg->codecId != A2DP_LHDCV5_CODEC_ID) {
        printf("lhdcv5 LHDCV5_CODEC_ID error %x\n",codec_cfg->codecId);
    }
    switch (codec_cfg->config[0] & A2DP_LHDCV5_SAMPLING_FREQ_MASK) {
        case A2DP_LHDCV5_SAMPLING_FREQ_44100:
            info_out->sample_rate = 44100;
            break;
        case A2DP_LHDCV5_SAMPLING_FREQ_48000:
            info_out->sample_rate = 48000;
            break;
        case A2DP_LHDCV5_SAMPLING_FREQ_96000:
            info_out->sample_rate = 96000;
            break;
        case A2DP_LHDCV5_SAMPLING_FREQ_192000:
            info_out->sample_rate = 192000;
            break;
        default:
            printf("unknown lhdcv5 FREQ: %d\n", codec_cfg->config[0] & A2DP_LHDCV5_SAMPLING_FREQ_MASK);
            break;
    }
    switch (codec_cfg->config[1] & A2DP_LHDCV5_BIT_FMT_MASK) {
        case A2DP_LHDCV5_BIT_FMT_16:
            info_out->sampleFormat = 16;
            break;
        case A2DP_LHDCV5_BIT_FMT_24:
            info_out->sampleFormat = 24;
            break;
        case A2DP_LHDCV5_BIT_FMT_32:
            info_out->sampleFormat = 32;
            break;
        default:
            printf("unknown lhdcv5 BIT_FMT: %d\n", codec_cfg->config[1] & A2DP_LHDCV5_BIT_FMT_MASK);
            break;
    }
    info_out->nframes_per_buffer = (info_out->sample_rate * LHDCV5BT_FRAME_DUR_5MS) / (1000 * 10);
    int v5_size = 0;
    lhdc_v5_dec_workspace_get_size(MAX_CHANNELS, info_out->sample_rate, LHDCV5BT_FRAME_DUR_5MS, 0, &v5_size);
    void *lhdcBT_dec_handle = malloc((v5_size + 7) & (~7));
    lhdc_v5_dec_workspace_init(MAX_CHANNELS, info_out->sample_rate, LHDCV5BT_FRAME_DUR_5MS, 0, lhdcBT_dec_handle);
    lhdc_v5_dec_init(MAX_CHANNELS, info_out->sampleFormat, info_out->sample_rate, LHDCV5BT_FRAME_DUR_5MS, lhdcBT_dec_handle);
    printf("%p\n", lhdcBT_dec_handle);
    return lhdcBT_dec_handle;
}
static const tA2DP_DECODER_INTERFACE a2dp_decoder_interface_lhdcv5 = {
    &a2dp_lhdcv5_decoder_decoder_cleanup,
    &a2dp_lhdcv5_decoder_decode_packet,
    NULL,                           // decoder_start
    NULL,                           // decoder_suspend
    &a2dp_lhdcv5_decoder_configure, // decoder_configure
};
const tA2DP_DECODER_INTERFACE *A2DP_GetDecoderInterfaceLhdcV5()
{
    return &a2dp_decoder_interface_lhdcv5;
}
