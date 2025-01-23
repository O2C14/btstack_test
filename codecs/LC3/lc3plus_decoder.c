#include "lc3plus_coder.h"
#include "a2dp_codec_api.h"
#include "bflb_mtimer.h"
#include "pcm_bridge.h"
#include "lc3.h"
#include "bl616_glb.h"
static decoded_data_callback_t write_pcm;
static int frame_us, srate_hz, nchannels, nsamples;
static bool hrmode;
static int32_t pcm_srate_hz = 0;
static lc3_decoder_t dec[2];
//512 1024
static bool a2dp_vendor_lc3plus_decoder_decode_packet(dec_msg *p_buf)
{
  int pcm_sbits = 24;
  int pcm_sbytes = pcm_sbits / 8;
  const uint8_t *in_ptr = p_buf->packet;
  enum lc3_pcm_format pcm_fmt = pcm_sbits == 24 ? LC3_PCM_FORMAT_S24 : LC3_PCM_FORMAT_S16;
  int block_bytes = p_buf->size;
  int nerr = 0;
  for (int ich = 0; ich < nchannels; ich++) {
    int frame_bytes = block_bytes / nchannels + (ich < block_bytes % nchannels);

    int res = lc3_decode(dec[ich], in_ptr, frame_bytes,
                         pcm_fmt, get_pcm_tail() + ich * pcm_sbytes, nchannels);

    nerr += (res != 0);
    in_ptr += frame_bytes;
  }
  return 1;
}

static bool a2dp_vendor_lc3plus_decoder_init(decoded_data_callback_t decode_callback)
{
  write_pcm = decode_callback;
  int frame_samples = lc3_hr_frame_samples(hrmode, frame_us, pcm_srate_hz);

  for (int ich = 0; ich < nchannels; ich++) {
    dec[ich] = lc3_hr_setup_decoder(
        hrmode, frame_us, srate_hz, 48000,
        malloc(lc3_hr_decoder_size(hrmode, frame_us, pcm_srate_hz)));

    if (!dec[ich])
      printf("Decoder initialization failed\n");
  }
}
static void a2dp_vendor_lc3plus_decoder_decoder_cleanup()
{
  for (int ich = 0; ich < nchannels; ich++) {
    if (dec[ich]) {
      free(dec[ich]);
    }
  }
}
static uint32_t a2dp_vendor_lc3plus_decoder_configure(const uint8_t *codec_cfg)
{
  tA2DP_LC3PLUS_CIE *cfg = codec_cfg;
  if ((cfg->vendorId != A2DP_LC3PLUS_VENDOR_ID) ||
      (cfg->codecId != A2DP_LC3PLUS_CODEC_ID)) {
    printf("lc3plus configure error\n");
    pcm_srate_hz = 48000;
    return DataWidth_16 | SR_48000;
  }
  switch (cfg->frame_duration & A2DP_LC3PLUS_FRAME_DURATION_MASK) {
    case A2DP_LC3PLUS_FRAME_DURATION_025:
      frame_us = 2500;
      break;
    case A2DP_LC3PLUS_FRAME_DURATION_050:
      frame_us = 5000;
      break;
    case A2DP_LC3PLUS_FRAME_DURATION_100:
      frame_us = 10000;
      break;
    default:
      break;
  }
  if (cfg->frequency & A2DP_LC3PLUS_SAMPLING_FREQ_48000) {
    srate_hz = 48000;
  } else if (cfg->frequency & A2DP_LC3PLUS_SAMPLING_FREQ_96000) {
    srate_hz = 96000;
  }
  if (cfg->channels & A2DP_LC3PLUS_CHANNELS_2) {
    nchannels = 2;
  } else {
    nchannels = 1;
  }
}
static const tA2DP_DECODER_INTERFACE a2dp_decoder_interface_lc3plus = {
  &a2dp_vendor_lc3plus_decoder_init,
  &a2dp_vendor_lc3plus_decoder_decoder_cleanup,
  &a2dp_vendor_lc3plus_decoder_decode_packet,
  NULL,                                   // decoder_start
  NULL,                                   // decoder_suspend
  &a2dp_vendor_lc3plus_decoder_configure, // decoder_configure
};
const tA2DP_DECODER_INTERFACE *A2DP_GetDecoderInterfaceLc3plus()
{
  return &a2dp_decoder_interface_lc3plus;
}