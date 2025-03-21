#include "pcm_bridge.h"
#include "bl616_glb.h"
#include "sbc_coder.h"
#include "a2dp_decoder.h"
#include "sbc-2.0/sbc/sbc.h"
//output 512
static bool a2dp_sbc_decode_packet(sbc_t *sbc_context, uint16_t con_handle, uint8_t *packet, uint16_t size)
{
    const void *input = packet + A2DP_SBC_MPL_HDR_LEN;
    size_t input_len = size - A2DP_SBC_MPL_HDR_LEN;
    uint8_t frame_count = packet[0] & 0xf;
    size_t written = 0;
    size_t len = 0;
    while (frame_count--) {
        set_start_loc(0);
        len += sbc_decode(&sbc_context,
                          input + len,
                          input_len - len,
                          get_pcm_tail(),
                          TEMP_PCM_BUFFER_MAX_SIZE,
                          &written);
        if (written) {
            check_buffer_edge(written);
            written = 0;
        } else {
            printf("sbc dec error\n");
        }
    }
    return true;
}

static void *a2dp_sbc_decoder_configure(const tA2DP_SBC_CIE *codec_cfg, pcm_info *info_out)
{
    printf("use SBC\n");
    printf("config 0 %d\n", (uint32_t)codec_cfg->config[0]);
    printf("config 1 %d\n", (uint32_t)codec_cfg->config[1]);
    printf("min_bitpool %d\n", (uint32_t)codec_cfg->min_bitpool);
    printf("max_bitpool %d\n", (uint32_t)codec_cfg->max_bitpool);

    sbc_t *sbc_context = malloc(sizeof(sbc_t));
    int func_ret = sbc_init_a2dp(sbc_context, 0, codec_cfg, sizeof(tA2DP_SBC_CIE));
    if (func_ret) {
        printf("sbc init error\n");
    }

    info_out->sampleFormat = 16;
    if (codec_cfg->config[0] & A2DP_SBC_SAMP_FREQ_44100) {
        info_out->sample_rate = 44100;
    } else if (codec_cfg->config[0] & A2DP_SBC_SAMP_FREQ_48000) {
        info_out->sample_rate = 48000;
    }
    info_out->nframes_per_buffer = 128;
    return sbc_context;
}
void a2dp_sbc_decoder_cleanup(void *handle)
{
    sbc_finish(handle);
    free(handle);
}
static const tA2DP_DECODER_INTERFACE a2dp_decoder_interface_sbc = {
    &a2dp_sbc_decoder_cleanup,
    &a2dp_sbc_decode_packet,
    NULL,                        // decoder_start
    NULL,                        // decoder_suspend
    &a2dp_sbc_decoder_configure, // decoder_configure
};
const tA2DP_DECODER_INTERFACE *A2DP_GetDecoderInterfaceSbc(void)
{
    //if (!A2DP_IsSinkCodecValidSbc(p_codec_info)) return NULL;

    return &a2dp_decoder_interface_sbc;
}