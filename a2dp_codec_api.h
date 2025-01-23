#ifndef A2DP_CODEC_API_H
#define A2DP_CODEC_API_H
//A2DP_CODEC_API_H
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "a2dp_decoder.h"


typedef enum PCM_FORMAT{
  DataWidth_16 = (1 << 0),
  DataWidth_24 = (1 << 1),
  DataWidth_32 = (1 << 2),
  SR_44100 = (1 << 3),
  SR_48000 = (1 << 4),
  SR_88200 = (1 << 5),
  SR_96000 = (1 << 6),
};

typedef enum {
  stared,

  stop
} decoder_state;


#endif