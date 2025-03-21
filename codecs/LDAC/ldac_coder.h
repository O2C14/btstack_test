#ifndef LDAC_DECODER_H
#define LDAC_DECODER_H
#include "a2dp_decoder.h"
#include <stdint.h>
// LDAC codec specific settings
#define A2DP_LDAC_CODEC_LEN            10
// [Octet 0-3] Vendor ID
#define A2DP_LDAC_VENDOR_ID            0x0000012D
// [Octet 4-5] Vendor Specific Codec ID
#define A2DP_LDAC_CODEC_ID             0x00AA
// [Octet 6], [Bits 0-5] Sampling Frequency
#define A2DP_LDAC_SAMPLING_FREQ_MASK   0x3F
#define A2DP_LDAC_SAMPLING_FREQ_44100  0x20
#define A2DP_LDAC_SAMPLING_FREQ_48000  0x10
#define A2DP_LDAC_SAMPLING_FREQ_88200  0x08
#define A2DP_LDAC_SAMPLING_FREQ_96000  0x04
#define A2DP_LDAC_SAMPLING_FREQ_176400 0x02
#define A2DP_LDAC_SAMPLING_FREQ_192000 0x01
// [Octet 7], [Bits 0-2] Channel Mode
#define A2DP_LDAC_CHANNEL_MODE_MASK    0x07
#define A2DP_LDAC_CHANNEL_MODE_MONO    0x04
#define A2DP_LDAC_CHANNEL_MODE_DUAL    0x02
#define A2DP_LDAC_CHANNEL_MODE_STEREO  0x01



typedef struct {
  uint32_t vendorId;
  uint16_t codecId;
  uint8_t config[2];
} __packed tA2DP_LDAC_CIE;










const tA2DP_DECODER_INTERFACE *A2DP_GetDecoderInterfaceLdac();
// Length of the LDAC Media Payload header
#define A2DP_LDAC_MPL_HDR_LEN 1

#endif