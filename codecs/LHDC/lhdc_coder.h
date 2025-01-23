#ifndef LHDC_DECODER_H
#define LHDC_DECODER_H
#include "a2dp_codec_api.h"
#include <stdint.h>
// [Octet 0-3] Vendor ID
#define A2DP_LHDC_VENDOR_ID 0x0000053a
// [Octet 4-5] Vendor Specific Codec ID
#define A2DP_LHDCV2_CODEC_ID 0x4C32
#define A2DP_LHDCV3_CODEC_ID 0x4C33
#define A2DP_LHDCV1_CODEC_ID 0x484C
#define A2DP_LHDCV1_LL_CODEC_ID 0x4C4C
#define A2DP_LHDCV5_CODEC_ID 0x4C35

// [Octet 6], [Bits 0-3] Sampling Frequency
#define A2DP_LHDC_SAMPLING_FREQ_MASK 0x0F
#define A2DP_LHDC_SAMPLING_FREQ_44100 0x08
#define A2DP_LHDC_SAMPLING_FREQ_48000 0x04
#define A2DP_LHDC_SAMPLING_FREQ_88200 0x02
#define A2DP_LHDC_SAMPLING_FREQ_96000 0x01
// [Octet 6], [Bits 3-4] Bit dipth
#define A2DP_BAD_BITS_PER_SAMPLE    0xff
#define A2DP_LHDC_BIT_FMT_MASK 	 0x30
#define A2DP_LHDC_BIT_FMT_24	 0x10
#define A2DP_LHDC_BIT_FMT_16	 0x20

// [Octet 6], [Bits 6-7] Bit dipth
#define A2DP_LHDC_FEATURE_AR		0x80
#define A2DP_LHDC_FEATURE_JAS		0x40

//[Octet 7:bit0..bit3]
#define A2DP_LHDC_VERSION_MASK 0x0F
//#define A2DP_LHDC_VERSION_2    0x01
//#define A2DP_LHDC_VERSION_3    0x02
//Supported version
typedef enum {
    A2DP_LHDC_VER2_BES  = 0,
    A2DP_LHDC_VER2 = 1,
    A2DP_LHDC_VER3 = 0x01,
    A2DP_LHDC_VER4 = 0x02,
    A2DP_LHDC_VER5 = 0x04,
    A2DP_LHDC_VER6 = 0x08,
    A2DP_LHDC_ERROR_VER,

    A2DP_LHDC_LAST_SUPPORTED_VERSION = A2DP_LHDC_VER4,
} A2DP_LHDC_VERSION;

//[Octet 7:bit4..bit5]
#define A2DP_LHDC_MAX_BIT_RATE_MASK       0x30
#define A2DP_LHDC_MAX_BIT_RATE_900K       0x00
#define A2DP_LHDC_MAX_BIT_RATE_500K       0x10		//500~600K
#define A2DP_LHDC_MAX_BIT_RATE_400K       0x20
//[Octet 7:bit6]
#define A2DP_LHDC_LL_MASK             0x40
#define A2DP_LHDC_LL_NONE             0x00
#define A2DP_LHDC_LL_SUPPORTED        0x40

//[Octet 7:bit7]
#define A2DP_LHDC_FEATURE_LLAC		0x80

//[Octet 8:bit0..bit3]
#define A2DP_LHDC_CH_SPLIT_MSK        0x0f
#define A2DP_LHDC_CH_SPLIT_NONE       0x01
#define A2DP_LHDC_CH_SPLIT_TWS        0x02
#define A2DP_LHDC_CH_SPLIT_TWS_PLUS   0x04

//[Octet 8:bit4..bit7]
#define A2DP_LHDC_FEATURE_META		0x10
#define A2DP_LHDC_FEATURE_MIN_BR	0x20
#define A2DP_LHDC_FEATURE_LARC		0x40
#define A2DP_LHDC_FEATURE_LHDCV4	0x80
typedef struct {
  uint32_t vendorId;
  uint16_t codecId;
  uint8_t config[3];
} __packed tA2DP_LHDC_CIE;

const tA2DP_DECODER_INTERFACE *A2DP_GetDecoderInterfaceLhdc();
// Length of the LHDC Media Payload header
#define A2DP_LHDC_MPL_HDR_LEN 2

#endif