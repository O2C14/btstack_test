#include <stdio.h>
#include <stdint.h>

#include "bl616_glb.h"

#include "btstack_config.h"
#include "btstack.h"

#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "a2dp_decoder.h"
#include "a2dp_codec_api.h"

#include "codecs/SBC/sbc_coder.h"
#include "codecs/AAC/aac_coder.h"

#include "codecs/APTX/aptx_coder.h"
#include "codecs/LDAC/ldac_coder.h"
#include "codecs/LHDC/lhdc_coder.h"
#include "codecs/OPUS/opus_coder.h"
#include "codecs/LC3/lc3plus_coder.h"
#include "codecs/pcm_bridge.h"

static tA2DP_SBC_CIE a2dp_sbc_sink_cfg;
static const tA2DP_SBC_CIE a2dp_sbc_sink_caps = {
  .config[0] = A2DP_SBC_SAMP_FREQ_48000 |
               A2DP_SBC_SAMP_FREQ_44100 |
               A2DP_SBC_CH_MODE_STREO |
               A2DP_SBC_CH_MODE_DUAL |
               A2DP_SBC_CH_MODE_JOINT,
  .config[1] = 0xff,
  .min_bitpool = 2,
  .max_bitpool = 53,
};
static tA2DP_AAC_CIE a2dp_aac_sink_cfg;
static const tA2DP_AAC_CIE a2dp_aac_sink_caps = {
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
typedef struct {
  tA2DP_DECODER_INTERFACE *inter;
  uint8_t seid;
  uint8_t *cfg;
} InterfaceSeidCFG;
static InterfaceSeidCFG ISC[4] = { 0 };
static void SetISCBySeid(uint8_t seid, tA2DP_DECODER_INTERFACE *inter, uint8_t *cfg)
{
  if (!inter) {
    return;
  }
  for (size_t i = 0; i < 4; i++) {
    if (!(ISC[i].inter)) {
      ISC[i].inter = inter;
      ISC[i].seid = seid;
      ISC[i].cfg = cfg;
      break;
    }
  }
}
static InterfaceSeidCFG *GetISCBySeid(uint8_t seid)
{
  for (size_t i = 0; i < 4; i++) {
    if (ISC[i].seid == seid) {
      return &ISC[i];
    }
  }
  return NULL;
}

void init_dec_endpoint()
{
  uint8_t sbc_seid = 0,
          aac_seid = 0,
          aptxhd_seid = 0,
          ldac_seid = 0,
          lhdc_seid = 0,
          lc3plus_seid = 0;

  avdtp_stream_endpoint_t *sbc_endpoint =
      a2dp_sink_create_stream_endpoint(AVDTP_AUDIO,
                                       AVDTP_CODEC_SBC,
                                       &a2dp_sbc_sink_caps,
                                       sizeof(tA2DP_SBC_CIE),
                                       &a2dp_sbc_sink_cfg,
                                       sizeof(tA2DP_SBC_CIE));
  btstack_assert(sbc_endpoint != NULL);
  sbc_seid = avdtp_local_seid(sbc_endpoint);
  SetISCBySeid(sbc_seid, A2DP_GetDecoderInterfaceSbc(), &a2dp_sbc_sink_cfg);

  avdtp_stream_endpoint_t *aac_endpoint =
      a2dp_sink_create_stream_endpoint(AVDTP_AUDIO,
                                       AVDTP_CODEC_MPEG_2_4_AAC,
                                       &a2dp_aac_sink_caps,
                                       sizeof(tA2DP_AAC_CIE),
                                       &a2dp_aac_sink_cfg,
                                       sizeof(tA2DP_AAC_CIE));
  btstack_assert(aac_endpoint != NULL);
  aac_seid = avdtp_local_seid(aac_endpoint);
  SetISCBySeid(aac_seid, A2DP_GetDecoderInterfaceAac(), &a2dp_aac_sink_cfg);
  /*
  avdtp_stream_endpoint_t *aptxhd_endpoint =
      a2dp_sink_create_stream_endpoint(AVDTP_AUDIO,
                                       AVDTP_CODEC_NON_A2DP,
                                       &a2dp_aptxhd_sink_caps, 
                                       sizeof(tA2DP_APTXHD_CIE),
                                       &a2dp_aptxhd_sink_cfg, 
                                       sizeof(tA2DP_APTXHD_CIE));
  btstack_assert(aptxhd_endpoint != NULL);
  aptxhd_seid = avdtp_local_seid(aptxhd_endpoint);
  SetISCBySeid(aptxhd_seid, A2DP_GetDecoderInterfaceAptxHd(), &a2dp_aptxhd_sink_cfg);
  */

  avdtp_stream_endpoint_t *ldac_endpoint =
      a2dp_sink_create_stream_endpoint(AVDTP_AUDIO,
                                       AVDTP_CODEC_NON_A2DP,
                                       &a2dp_ldac_sink_caps,
                                       sizeof(tA2DP_LDAC_CIE),
                                       &a2dp_ldac_sink_cfg,
                                       sizeof(tA2DP_LDAC_CIE));
  btstack_assert(ldac_endpoint != NULL);
  ldac_seid = avdtp_local_seid(ldac_endpoint);
  SetISCBySeid(ldac_seid, A2DP_GetDecoderInterfaceLdac(), &a2dp_ldac_sink_cfg);
  /*
  avdtp_stream_endpoint_t *lhdc_endpoint =
      a2dp_sink_create_stream_endpoint(AVDTP_AUDIO,
                                       AVDTP_CODEC_NON_A2DP,
                                       &a2dp_lhdc_sink_caps, 
                                       sizeof(tA2DP_LHDC_CIE),
                                       &a2dp_lhdc_sink_cfg, 
                                       sizeof(tA2DP_LHDC_CIE));
  btstack_assert(lhdc_endpoint != NULL);
  lhdc_seid = avdtp_local_seid(lhdc_endpoint);
  SetISCBySeid(lhdc_seid, A2DP_GetDecoderInterfaceLhdc(), &a2dp_lhdc_sink_cfg);
  
  avdtp_stream_endpoint_t *lc3plus_endpoint =
      a2dp_sink_create_stream_endpoint(AVDTP_AUDIO,
                                       AVDTP_CODEC_NON_A2DP,
                                       &a2dp_lc3plus_sink_caps, 
                                       sizeof(tA2DP_LC3PLUS_CIE),
                                       &a2dp_lc3plus_sink_cfg, 
                                       sizeof(tA2DP_LC3PLUS_CIE));
  btstack_assert(lc3plus_endpoint != NULL);
  lc3plus_seid = avdtp_local_seid(lc3plus_endpoint);
  SetISCBySeid(lc3plus_seid, A2DP_GetDecoderInterfaceLc3plus(), &a2dp_lc3plus_sink_cfg);
  */
}

static tA2DP_DECODER_INTERFACE *old_inter = NULL;
void init_decoder(uint8_t seid)
{
  uint32_t samplerate = 0;
  uint8_t data_width = 0;
  InterfaceSeidCFG *p = GetISCBySeid(seid);
  if (p == NULL) {
    printf("no match seid\n");
    return;
  }
  if (!p->inter) {
    return;
  }
  if (old_inter) {
    if (p->inter == old_inter) {
      return;
    }
    if (old_inter->decoder_cleanup) {
      old_inter->decoder_cleanup();
      old_inter = NULL;
    }
  }
  old_inter = p->inter;

  uint32_t pcm_cfg = p->inter->decoder_configure(p->cfg);
  if (pcm_cfg & DataWidth_16) {
    data_width = 16;
  } else if (pcm_cfg & DataWidth_24) {
    data_width = 24;
  } else if (pcm_cfg & DataWidth_32) {
    data_width = 32;
  }
  if (pcm_cfg & SR_44100) {
    samplerate = 44100;
  } else if (pcm_cfg & SR_48000) {
    samplerate = 48000;
  } else if (pcm_cfg & SR_88200) {
    samplerate = 88200;
  } else if (pcm_cfg & SR_96000) {
    samplerate = 96000;
  }

  p->inter->decoder_init(&pcm_write);

  pcm_open(samplerate, data_width, 2);
}
void stop_decoder()
{
  i2s_stop();
}
#ifdef USED_DEC_THREAD
#define MAX_RCV_BLOCK_NUM (8)
static QueueHandle_t dec_queue;
void send_to_decoder(uint8_t *packet, uint16_t size)
{
  dec_msg msg = {
    .packet = packet,
    .size = size,
  };
  //memcpy(msg.packet, packet, size);
  xQueueSend(dec_queue, &msg, 0);
}
TaskHandle_t xHandle;

void handle_stream_data()
{
  dec_msg msg = { 0 };
  while (1) {
    xQueueReceive(dec_queue, &msg, portMAX_DELAY);
    if (msg.packet != NULL) {
      old_inter->decode_packet(&msg);
    }
    msg.packet = NULL;
  }
}
void create_decoder_thread()
{
  //xTaskCreate(handle_stream_data, "A2dpDecoder", 2048, NULL, configMAX_PRIORITIES - 29, &xHandle);

  xTaskCreate(handle_stream_data, "A2dpDecoder", 1024 * 16, NULL, configMAX_PRIORITIES - 29, &xHandle);

  dec_queue = xQueueCreate(MAX_RCV_BLOCK_NUM, sizeof(dec_msg));
}
#else
void send_to_decoder(uint8_t *packet, uint16_t size)
{
  dec_msg msg = {
    .packet = packet,
    .size = size,
  };
  old_inter->decode_packet(&msg);
}
void create_decoder_thread()
{
}
#endif