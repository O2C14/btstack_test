#ifndef SBC_DECODER_H
#define SBC_DECODER_H
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "oi_codec_sbc.h"

//#include <statis.h>
#include "a2dp_decoder.h"
#ifdef BIT
#undef BIT
#define BIT(n) (1UL << (n))
#else
#define BIT(n) (1UL << (n))
#endif
/* Sampling Frequency */
#define A2DP_SBC_SAMP_FREQ_16000 BIT(7)
#define A2DP_SBC_SAMP_FREQ_32000 BIT(6)
#define A2DP_SBC_SAMP_FREQ_44100 BIT(5)
#define A2DP_SBC_SAMP_FREQ_48000 BIT(4)

/* Channel Mode */
#define A2DP_SBC_CH_MODE_MONO  BIT(3)
#define A2DP_SBC_CH_MODE_DUAL  BIT(2)
#define A2DP_SBC_CH_MODE_STREO BIT(1)
#define A2DP_SBC_CH_MODE_JOINT BIT(0)

/* Block Length */
#define A2DP_SBC_BLK_LEN_4  BIT(7)
#define A2DP_SBC_BLK_LEN_8  BIT(6)
#define A2DP_SBC_BLK_LEN_12 BIT(5)
#define A2DP_SBC_BLK_LEN_16 BIT(4)

/* Subbands */
#define A2DP_SBC_SUBBAND_4 BIT(3)
#define A2DP_SBC_SUBBAND_8 BIT(2)

/* Allocation Method */
#define A2DP_SBC_ALLOC_MTHD_SNR      BIT(1)
#define A2DP_SBC_ALLOC_MTHD_LOUDNESS BIT(0)

/** @brief SBC Codec */
typedef struct{
    /** First two octets of configuration */
    uint8_t config[2];
    /** Minimum Bitpool Value */
    uint8_t min_bitpool;
    /** Maximum Bitpool Value */
    uint8_t max_bitpool;
}__packed tA2DP_SBC_CIE;

#define A2DP_SBC_MPL_HDR_LEN 1
const tA2DP_DECODER_INTERFACE* A2DP_GetDecoderInterfaceSbc(void);
#endif