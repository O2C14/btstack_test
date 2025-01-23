#ifndef LC3PLUS_DECODER_H
#define LC3PLUS_DECODER_H
#include "a2dp_codec_api.h"
#include <stdint.h>
// LC3plus codec specific settings
#define A2DP_LC3PLUS_CODEC_LEN            10
// [Octet 0-3] Vendor ID
#define A2DP_LC3PLUS_VENDOR_ID            0x000008A9
// [Octet 4-5] Vendor Specific Codec ID
#define A2DP_LC3PLUS_CODEC_ID             0x0001
// [Octet 6], [Bits 0-2] Frame Duration
#define A2DP_LC3PLUS_FRAME_DURATION_025      (1 << 0)
#define A2DP_LC3PLUS_FRAME_DURATION_050      (1 << 1)
#define A2DP_LC3PLUS_FRAME_DURATION_100      (1 << 2)
#define A2DP_LC3PLUS_FRAME_DURATION_MASK     3
// [Octet 7], [Bits 6-7] Channel Mode
#define A2DP_LC3PLUS_CHANNELS_1              (1 << 7)
#define A2DP_LC3PLUS_CHANNELS_2              (1 << 6)
#define A2DP_LC3PLUS_CHANNELS_MASK    		 0xC0
// [Octet 8-9], [Bits 7-8] Sampling Frequency
#define A2DP_LC3PLUS_SAMPLING_FREQ_48000     (1 << 8)
#define A2DP_LC3PLUS_SAMPLING_FREQ_96000     (1 << 7)
#define A2DP_LC3PLUS_SAMPLING_FREQ_MASK     0x180
typedef struct {
    uint32_t vendorId;
    uint16_t codecId;
	uint8_t rfa:4;
	uint8_t frame_duration:4;
	uint8_t channels;
	uint16_t frequency;
}__packed tA2DP_LC3PLUS_CIE;
const tA2DP_DECODER_INTERFACE *A2DP_GetDecoderInterfaceLc3plus();
#endif