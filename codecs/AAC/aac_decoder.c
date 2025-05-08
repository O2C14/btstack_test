#include "aacdecoder_lib.h"
#include "a2dp_decoder.h"
#include "aac_coder.h"
#include "pcm_bridge.h"
#include "bflb_mtimer.h"
#include "bl616_glb.h"

static void a2dp_aac_decoder_cleanup(HANDLE_AACDECODER aac_handle)
{
    if (aac_handle) {
        aacDecoder_Close(aac_handle);
    }
}
//output 4096
static bool a2dp_aac_decoder_decode_packet(HANDLE_AACDECODER aac_handle, uint16_t con_handle, uint8_t *packet, uint16_t size)
{
    UCHAR *pBuffer = packet;
    int32_t in_len = size;
    UINT bufferSize = in_len;
    UINT bytesValid = in_len;
    int32_t out_size = 0;
    while (bytesValid > 0) {
        AAC_DECODER_ERROR err = aacDecoder_Fill(aac_handle, &pBuffer, &bufferSize, &bytesValid);
        if (err != AAC_DEC_OK) {
            printf("aacDecoder_Fill failed: 0x%04x\r\n", err);
            return false;
        }

        while (true) {
            // 2 * 1024 / (sr/1000) ms
            set_start_loc(1024 * MAX_CHANNELS * sizeof(INT_PCM) * 2);
            err = aacDecoder_DecodeFrame(aac_handle,
                                         get_pcm_tail(),
                                         //(INT_PCM *)(tmp_pcm_buffer),
                                         4096 / sizeof(INT_PCM),
                                         0); // flags

            if (err == AAC_DEC_NOT_ENOUGH_BITS) {
                break;
            }
            if (err != AAC_DEC_OK) {
                printf("aacDecoder_DecodeFrame failed: 0x%04x\r\n", err);
                break;
            }
            //printf("offset:%d\r\n", offset);
            CStreamInfo *info = aacDecoder_GetStreamInfo(aac_handle);
            if (!info || info->sampleRate <= 0) {
                printf("Invalid stream info\r\n");
                break;
            }
            out_size = info->frameSize * info->numChannels * sizeof(INT_PCM);
            // pcm_write(tmp_pcm_buffer, out_size);
            check_buffer_edge(out_size);
        }
    }
    return true;
}

static void *a2dp_aac_decoder_decoder_configure(const tA2DP_AAC_CIE *codec_cfg, pcm_info *info_out)
{
    info_out->nframes_per_buffer = 1024;
    HANDLE_AACDECODER aac_handle = aacDecoder_Open(TT_MP4_LATM_MCP1, 1 /* nrOfLayers */);
    uint32_t bitrate = ((codec_cfg->VBR_Bitrate[0] << 16) & A2DP_AAC_BIT_RATE_MASK0) |
                       ((codec_cfg->VBR_Bitrate[1] << 8) & A2DP_AAC_BIT_RATE_MASK1) |
                       (codec_cfg->VBR_Bitrate[2] & A2DP_AAC_BIT_RATE_MASK2);
    printf("use AAC\n");
    info_out->sampleFormat = 16;

    printf("AAC bitrate%d\n", bitrate);
    if ((bitrate % 1000) || (bitrate == 0)) {
        printf("AAC cfg error\n");
        return 0;
    }

    switch (codec_cfg->SamplingFrequency_Channels & (A2DP_AAC_SAMPLING_FREQ_MASK0 | A2DP_AAC_SAMPLING_FREQ_MASK1)) {
        case A2DP_AAC_SAMPLING_FREQ_44100:
            info_out->sample_rate = 44100;
            break;
        case A2DP_AAC_SAMPLING_FREQ_48000:
            info_out->sample_rate = 48000;
            break;
        case A2DP_AAC_SAMPLING_FREQ_88200:
            info_out->sample_rate = 88200;
            break;
        case A2DP_AAC_SAMPLING_FREQ_96000:
            info_out->sample_rate = 96000;
            break;
        default:
            printf("AAC cfg error\n");
            break;
    }

    return aac_handle;
}
static const tA2DP_DECODER_INTERFACE a2dp_decoder_interface_aac = {
    &a2dp_aac_decoder_cleanup,
    &a2dp_aac_decoder_decode_packet,
    NULL,                                // decoder_start
    NULL,                                // decoder_suspend
    &a2dp_aac_decoder_decoder_configure, // decoder_configure
};
const tA2DP_DECODER_INTERFACE *A2DP_GetDecoderInterfaceAac()
{
    //if (!A2DP_IsSinkCodecValidAac(p_codec_info)) return NULL;

    return &a2dp_decoder_interface_aac;
}