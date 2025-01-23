#ifndef OPUS_DECODER_H
#define OPUS_DECODER_H
#include <stdint.h>
// [Octet 0-3] Vendor ID
#define A2DP_OPUS_VENDOR_ID              0x000000E0
// [Octet 4-5] Vendor Specific Codec ID
#define A2DP_OPUS_CODEC_ID               0x0001
// [Octet 6], [Bits 0,1,2] Channel Mode
#define A2DP_OPUS_CHANNEL_MODE_MASK      0x07
#define A2DP_OPUS_CHANNEL_MODE_MONO      0x01
#define A2DP_OPUS_CHANNEL_MODE_STEREO    0x02
#define A2DP_OPUS_CHANNEL_MODE_DUAL_MONO 0x04
// [Octet 6], [Bits 3,4] Future 2, FrameSize
#define A2DP_OPUS_FRAMESIZE_MASK         0x18
#define A2DP_OPUS_10MS_FRAMESIZE         0x08
#define A2DP_OPUS_20MS_FRAMESIZE         0x10
// [Octet 6], [Bits 5] Sampling Frequency
#define A2DP_OPUS_SAMPLING_FREQ_MASK     0x80
#define A2DP_OPUS_SAMPLING_FREQ_48000    0x80
// [Octet 6], [Bits 6,7] Reserved
#define A2DP_OPUS_FUTURE_3               0x40
#define A2DP_OPUS_FUTURE_4               0x80
typedef struct {
  uint32_t vendorId;
  uint16_t codecId; /* Codec ID for Opus */
  uint8_t config[1];
} tA2DP_OPUS_CIE;
#endif