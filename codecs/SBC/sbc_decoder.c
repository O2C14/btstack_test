#include "pcm_bridge.h"
#include "bl616_glb.h"
#include "sbc_coder.h"
#include "a2dp_codec_api.h"
#include "sbc-2.0/sbc/sbc.h"
static sbc_t sbc_context;
//output 512
static bool a2dp_sbc_decode_packet(dec_msg *p_buf)
{
  const void *input = p_buf->packet + A2DP_SBC_MPL_HDR_LEN;
  size_t input_len = p_buf->size - A2DP_SBC_MPL_HDR_LEN;
  uint8_t frame_count = p_buf->packet[0] & 0xf;
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
static const tA2DP_SBC_CIE *sbc_cfg;
static int a2dp_sbc_decode_init(decoded_data_callback_t decode_callback)
{

  if (sbc_init_a2dp(&sbc_context, 0, sbc_cfg, sizeof(tA2DP_SBC_CIE))) {
    printf("sbc init error\n");
  }
}

static uint32_t a2dp_sbc_decoder_configure(const uint8_t *codec_cfg)
{
  printf("use SBC\n");
  sbc_cfg = codec_cfg;
  printf("config 0 %d\n", (uint32_t)sbc_cfg->config[0]);
  printf("config 1 %d\n", (uint32_t)sbc_cfg->config[1]);
  printf("min_bitpool %d\n", (uint32_t)sbc_cfg->min_bitpool);
  printf("max_bitpool %d\n", (uint32_t)sbc_cfg->max_bitpool);
  if (sbc_cfg->config[0] & A2DP_SBC_SAMP_FREQ_44100) {
    return DataWidth_16 | SR_44100;

  } else if (sbc_cfg->config[0] & A2DP_SBC_SAMP_FREQ_48000) {
    return DataWidth_16 | SR_48000;
  }

  return 0;
}
void a2dp_sbc_decoder_cleanup()
{
  sbc_finish(&sbc_context);
}
static const tA2DP_DECODER_INTERFACE a2dp_decoder_interface_sbc = {
  &a2dp_sbc_decode_init,
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