#include "lhdc_coder.h"
#include "a2dp_codec_api.h"
#include "bflb_mtimer.h"
#include "pcm_bridge.h"
#include "lhdcUtil.h"
static decoded_data_callback_t write2pcm;
static int32_t samplerate = 0;
static int8_t bits_per_sample = 16;
static bool a2dp_vendor_lhdc_decoder_decode_packet(dec_msg *p_buf)
{
  const int32_t streamSize = p_buf->size - A2DP_LHDC_MPL_HDR_LEN;
  const uint8_t *pStream = p_buf->packet + A2DP_LHDC_MPL_HDR_LEN;

  int32_t used_Stream_count = 0;
  lhdcPutData(pStream, streamSize);

  uint32_t loop_cnt = 0;
  int32_t lhdc_out_len = 0;
  uint8_t frame_count = p_buf->packet[0];
  while (frame_count--) {
    lhdc_out_len = lhdcDecodeProcess(tmp_pcm_buffer);
    if (lhdc_out_len > 0) {
      write2pcm(tmp_pcm_buffer, lhdc_out_len);
      //printf("writen %d\n",lhdc_out_len);
    } else {
      break;
    }
  }
  return 1;
}

static bool a2dp_vendor_lhdc_decoder_init(decoded_data_callback_t decode_callback)
{
  write2pcm = decode_callback;
  lhdcInit(bits_per_sample, samplerate, 0, VERSION_3);
  return 1;
}
static void a2dp_vendor_lhdc_decoder_decoder_cleanup()
{
}
static uint32_t a2dp_vendor_lhdc_decoder_configure(const uint8_t *codec_cfg)
{
  tA2DP_LHDC_CIE *cfg = codec_cfg;
  uint8_t tmp = 0;
  if ((cfg->vendorId != A2DP_LHDC_VENDOR_ID) ||
      (cfg->codecId != A2DP_LHDCV3_CODEC_ID)) {
    printf("LHDC cfg error\n");
    samplerate = 48000;
    return DataWidth_16 | SR_48000;
  }
  switch (cfg->config[0] & A2DP_LHDC_SAMPLING_FREQ_MASK) {
    case A2DP_LHDC_SAMPLING_FREQ_96000:
      samplerate = 96000;
      tmp |= SR_96000;
      break;
    case A2DP_LHDC_SAMPLING_FREQ_88200:
      samplerate = 88200;
      tmp |= SR_88200;
      break;
    case A2DP_LHDC_SAMPLING_FREQ_48000:
      samplerate = 48000;
      tmp |= SR_48000;
      break;
    case A2DP_LHDC_SAMPLING_FREQ_44100:
      samplerate = 44100;
      tmp |= SR_44100;
      break;
    default:
      break;
  }
  if (cfg->config[0] & A2DP_LHDC_BIT_FMT_24) {
    bits_per_sample = 24;
    tmp |= DataWidth_24;
  } else if (cfg->config[0] & A2DP_LHDC_BIT_FMT_16) {
    bits_per_sample = 16;
    tmp |= DataWidth_16;
  }
  return tmp;
}
static const tA2DP_DECODER_INTERFACE a2dp_decoder_interface_lhdc = {
  &a2dp_vendor_lhdc_decoder_init,
  &a2dp_vendor_lhdc_decoder_decoder_cleanup,
  &a2dp_vendor_lhdc_decoder_decode_packet,
  NULL,                                // decoder_start
  NULL,                                // decoder_suspend
  &a2dp_vendor_lhdc_decoder_configure, // decoder_configure
};
const tA2DP_DECODER_INTERFACE *A2DP_GetDecoderInterfaceLhdc()
{
  return &a2dp_decoder_interface_lhdc;
}
