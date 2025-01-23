#include <stdio.h>
#include <stdint.h>

#include "bl616_glb.h"

#include "btstack_config.h"
#include "btstack.h"

#include "a2dp_encoder.h"

#include "codecs/SBC/sbc_coder.h"
#include "codecs/AAC/aac_coder.h"

#include "codecs/APTX/aptx_coder.h"
#include "codecs/LDAC/ldac_coder.h"
#include "codecs/LHDC/lhdc_coder.h"
#include "codecs/OPUS/opus_coder.h"
#include "codecs/LC3/lc3plus_coder.h"

static tA2DP_SBC_CIE a2dp_sbc_sink_cfg;
static const tA2DP_SBC_CIE a2dp_sbc_sink_caps = {
  .config[0] = 0xFF,
  .config[1] = 0xFF,
  .min_bitpool = 2,
  .max_bitpool = 250,
};
static tA2DP_AAC_CIE a2dp_aac_sink_cfg;
static const tA2DP_AAC_CIE a2dp_aac_sink_caps = {
  /*
Position    Object Type        Support in SRC Support in SNK
Octet0; b7  MPEG2_AAC_LC       M              M
Octet0; b6  MPEG4_AAC_LC       O              O
Octet0; b5  MPEG4_AAC_LTP      O              O
Octet0; b4  MPEG4_AAC_scalable O              O
Octet0; b3  MPEG4_HE_AAC       O              O
Octet0; b2  MPEG4_HE_AACv2     O              O
Octet0; b1  MPEG4_AAC_ELDv2    O              O
*/
  .ObjectType = 0xFF,
  .SamplingFrequency_Channels = 0xFFFF,
  .VBR_Bitrate[0] = 0xFF,
  .VBR_Bitrate[1] = 0xFF,
  .VBR_Bitrate[2] = 0xFF
};
static tA2DP_LDAC_CIE a2dp_ldac_sink_cfg;
static const tA2DP_LDAC_CIE a2dp_ldac_sink_caps = {
  .vendorId = A2DP_LDAC_VENDOR_ID,
  .codecId = A2DP_LDAC_CODEC_ID,
  .config[0] = (A2DP_LDAC_SAMPLING_FREQ_44100 | A2DP_LDAC_SAMPLING_FREQ_48000),
  .config[1] = (A2DP_LDAC_CHANNEL_MODE_DUAL | A2DP_LDAC_CHANNEL_MODE_STEREO)
};
static tA2DP_APTXHD_CIE a2dp_aptxhd_sink_cfg;
static const tA2DP_APTXHD_CIE a2dp_aptxhd_sink_caps = {
  .vendorId = A2DP_APTX_HD_VENDOR_ID,
  .codecId = A2DP_APTX_HD_CODEC_ID_BLUETOOTH,
  .channelMode = A2DP_APTX_HD_CHANNELS_STEREO,
  .sampleRate = (A2DP_APTX_HD_SAMPLERATE_48000 >> 4) |
                (A2DP_APTX_HD_SAMPLERATE_44100 >> 4),
  .reserved = 0
};

/* Opus Sink codec capabilities */
static const tA2DP_OPUS_CIE a2dp_opus_sink_caps = {
  .vendorId = A2DP_OPUS_VENDOR_ID, // vendorId
  .codecId = A2DP_OPUS_CODEC_ID,   // codecId
  .config[0] = A2DP_OPUS_CHANNEL_MODE_DUAL_MONO |
               A2DP_OPUS_CHANNEL_MODE_STEREO | A2DP_OPUS_FRAMESIZE_MASK |
               A2DP_OPUS_SAMPLING_FREQ_MASK
};
static tA2DP_LHDC_CIE a2dp_lhdc_sink_cfg;
static const tA2DP_LHDC_CIE a2dp_lhdc_sink_caps = {
  .vendorId = A2DP_LHDC_VENDOR_ID,
  .codecId = A2DP_LHDCV3_CODEC_ID,
  .config[0] = A2DP_LHDC_SAMPLING_FREQ_48000 | A2DP_LHDC_BIT_FMT_16,
  .config[1] = A2DP_LHDC_MAX_BIT_RATE_900K | A2DP_LHDC_VER3,
  .config[2] = A2DP_LHDC_CH_SPLIT_NONE
};
static tA2DP_LC3PLUS_CIE a2dp_lc3plus_sink_cfg;
static const tA2DP_LC3PLUS_CIE a2dp_lc3plus_sink_caps = {
  .vendorId = A2DP_LC3PLUS_VENDOR_ID,
  .codecId = A2DP_LC3PLUS_CODEC_ID,
  .frame_duration = A2DP_LC3PLUS_FRAME_DURATION_025 |
                    A2DP_LC3PLUS_FRAME_DURATION_050 |
                    A2DP_LC3PLUS_FRAME_DURATION_100,
  .channels = A2DP_LC3PLUS_CHANNELS_2,
  .frequency = A2DP_LC3PLUS_SAMPLING_FREQ_48000 | A2DP_LC3PLUS_SAMPLING_FREQ_96000
};
avdtp_stream_endpoint_t *sbc_enc_endpoint;
void init_enc_endpoint()
{
  /*
  sbc_enc_endpoint = 
      a2dp_source_create_stream_endpoint(AVDTP_AUDIO,
                                         AVDTP_CODEC_SBC,
                                         &a2dp_sbc_sink_caps, sizeof(tA2DP_SBC_CIE),
                                         &a2dp_sbc_sink_cfg, sizeof(tA2DP_SBC_CIE));
  avdtp_local_seid(sbc_enc_endpoint);*/
  //avdtp_set_preferred_sampling_frequency(sbc_enc_endpoint, 48000);
  avdtp_stream_endpoint_t *ldac_enc_endpoint =
      a2dp_source_create_stream_endpoint(AVDTP_AUDIO,
                                         AVDTP_CODEC_NON_A2DP,
                                         &a2dp_ldac_sink_caps, sizeof(tA2DP_LDAC_CIE),
                                         &a2dp_ldac_sink_cfg, sizeof(tA2DP_LDAC_CIE));

  avdtp_local_seid(ldac_enc_endpoint);
}

void select_enc()
{



}