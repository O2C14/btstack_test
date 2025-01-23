#ifndef AAC_DECODER_H
#define AAC_DECODER_H
#include <stdint.h>
#include "a2dp_codec_api.h"

// AAC codec specific settings
#define A2DP_AAC_CODEC_LEN 8

// [Octet 0] Object Type
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
#define A2DP_AAC_OBJECT_TYPE_MPEG2_LC 0x80  /* MPEG-2 Low Complexity */
#define A2DP_AAC_OBJECT_TYPE_MPEG4_LC 0x40  /* MPEG-4 Low Complexity */
#define A2DP_AAC_OBJECT_TYPE_MPEG4_LTP 0x20 /* MPEG-4 Long Term Prediction */
#define A2DP_AAC_OBJECT_TYPE_MPEG4_SCALABLE 0x10
#define A2DP_AAC_OBJECT_TYPE_MPEG4_HE_AAC       0x08
#define A2DP_AAC_OBJECT_TYPE_MPEG4_HE_AACv2     0x04
#define A2DP_AAC_OBJECT_TYPE_MPEG4_AAC_ELDv2    0x02


// [Octet 1] Sampling Frequency - 8000 to 44100
#define A2DP_AAC_SAMPLING_FREQ_MASK0 0xFF
#define A2DP_AAC_SAMPLING_FREQ_8000 0x80
#define A2DP_AAC_SAMPLING_FREQ_11025 0x40
#define A2DP_AAC_SAMPLING_FREQ_12000 0x20
#define A2DP_AAC_SAMPLING_FREQ_16000 0x10
#define A2DP_AAC_SAMPLING_FREQ_22050 0x08
#define A2DP_AAC_SAMPLING_FREQ_24000 0x04
#define A2DP_AAC_SAMPLING_FREQ_32000 0x02
#define A2DP_AAC_SAMPLING_FREQ_44100 0x01
// [Octet 2], [Bits 4-7] Sampling Frequency - 48000 to 96000
// NOTE: Bits offset for the higher-order octet 16-bit integer
#define A2DP_AAC_SAMPLING_FREQ_MASK1 (0xF0 << 8)
#define A2DP_AAC_SAMPLING_FREQ_48000 (0x80 << 8)
#define A2DP_AAC_SAMPLING_FREQ_64000 (0x40 << 8)
#define A2DP_AAC_SAMPLING_FREQ_88200 (0x20 << 8)
#define A2DP_AAC_SAMPLING_FREQ_96000 (0x10 << 8)
// [Octet 2], [Bits 2-3] Channel Mode
#define A2DP_AAC_CHANNEL_MODE_MASK 0x0C
#define A2DP_AAC_CHANNEL_MODE_MONO 0x08
#define A2DP_AAC_CHANNEL_MODE_STEREO 0x04
// [Octet 2], [Bits 0-1] RFA
// [Octet 3], [Bit 7] Variable Bit Rate Supported
#define A2DP_AAC_VARIABLE_BIT_RATE_MASK 0x80
#define A2DP_AAC_VARIABLE_BIT_RATE_ENABLED 0x80
#define A2DP_AAC_VARIABLE_BIT_RATE_DISABLED 0x00
// [Octet 3], [Bits 0-6] Bit Rate - Bits 16-22 in the 23-bit UiMsbf
#define A2DP_AAC_BIT_RATE_MASK0 (0x7F << 16)
#define A2DP_AAC_BIT_RATE_MASK1 (0xFF << 8)
#define A2DP_AAC_BIT_RATE_MASK2 0xFF
// [Octet 4], [Bits 0-7] Bit Rate - Bits 8-15 in the 23-bit UiMsfb
// [Octet 5], [Bits 0-7] Bit Rate - Bits 0-7 in the 23-bit UiMsfb

typedef struct {
  uint8_t ObjectType;
  uint16_t SamplingFrequency_Channels;
  uint8_t VBR_Bitrate[3];
} __packed tA2DP_AAC_CIE;
struct bt_a2dp_codec_aac_params_0 {
  uint32_t DRC                : 1;
  uint32_t ObjectType         : 7;
  uint32_t SamplingFrequency0 : 8;
  uint32_t Channels           : 4;
  uint32_t SamplingFrequency1 : 4;
  uint32_t Bitrate0           : 7;
  uint32_t VBR                : 1;
  uint32_t Bitrate1           : 16;
} __packed;

const tA2DP_DECODER_INTERFACE* A2DP_GetDecoderInterfaceAac();

#endif