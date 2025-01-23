#include "aacdecoder_lib.h"
#include "a2dp_codec_api.h"
#include "aac_coder.h"
#include "pcm_bridge.h"
#include "bflb_mtimer.h"
#include "bl616_glb.h"
static HANDLE_AACDECODER aac_handle = NULL;
static bool a2dp_aac_decoder_init(decoded_data_callback_t decode_callback)
{
  if (!aac_handle) {
    //a2dp_aac_decoder_cleanup();
    aac_handle = aacDecoder_Open(TT_MP4_LATM_MCP1, 1 /* nrOfLayers */);
    //aacDecoder_SetParam(aac_handle, AAC_PCM_LIMITER_ENABLE, 0);
    //aacDecoder_SetParam(aac_handle, AAC_DRC_ATTENUATION_FACTOR, 0);
    //aacDecoder_SetParam(aac_handle, AAC_DRC_BOOST_FACTOR, 0);
    //aacDecoder_SetParam(aac_handle, AAC_PCM_MAX_OUTPUT_CHANNELS, 2);
  }
  return true;
}
static void a2dp_aac_decoder_cleanup(void)
{
  if (aac_handle) {
    aacDecoder_Close(aac_handle);
    aac_handle = NULL;
  }
}
//output 4096
static bool a2dp_aac_decoder_decode_packet(dec_msg *p_buf)
{
  UCHAR *pBuffer = p_buf->packet;
  int32_t in_len = p_buf->size;
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
      //set_start_loc(0);
      err = aacDecoder_DecodeFrame(aac_handle,
                                      //get_pcm_tail(),
                                   (INT_PCM *)(tmp_pcm_buffer),
                                   4096 / sizeof(INT_PCM),
                                   0);   // flags

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
      pcm_write(tmp_pcm_buffer, out_size);
      //check_buffer_edge(out_size);
    }
  }
  return true;
}

static uint32_t a2dp_aac_decoder_decoder_configure(const uint8_t *codec_cfg)
{
  tA2DP_AAC_CIE *cfg = codec_cfg;
  printf("use AAC\n");
  uint32_t bitrate = ((cfg->VBR_Bitrate[0] << 16) & A2DP_AAC_BIT_RATE_MASK0) |
                     ((cfg->VBR_Bitrate[1] << 8) & A2DP_AAC_BIT_RATE_MASK1) |
                     (cfg->VBR_Bitrate[2] & A2DP_AAC_BIT_RATE_MASK2);
  printf("AAC bitrate%d\n", bitrate);
  if ((bitrate % 1000) || (bitrate == 0)) {
    printf("AAC cfg error\n");
    return DataWidth_16 | SR_44100;
  }
  switch (cfg->SamplingFrequency_Channels & (A2DP_AAC_SAMPLING_FREQ_MASK0 | A2DP_AAC_SAMPLING_FREQ_MASK1)) {
    case A2DP_AAC_SAMPLING_FREQ_44100:
      return DataWidth_16 | SR_44100;
    case A2DP_AAC_SAMPLING_FREQ_48000:
      return DataWidth_16 | SR_48000;
    case A2DP_AAC_SAMPLING_FREQ_88200:
      return DataWidth_16 | SR_88200;
    case A2DP_AAC_SAMPLING_FREQ_96000:
      return DataWidth_16 | SR_96000;
    default:
      printf("AAC cfg error\n");
      return DataWidth_16 | SR_44100;
  }
  return 0;
}
static const tA2DP_DECODER_INTERFACE a2dp_decoder_interface_aac = {
  &a2dp_aac_decoder_init,
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