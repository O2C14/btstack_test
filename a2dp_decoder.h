#ifndef A2DP_DEOCDER_H
#define A2DP_DEOCDER_H
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "a2dp_codec_api.h"
typedef struct {
  uint8_t *packet;
  uint16_t size;
} __packed dec_msg;
typedef void (*decoded_data_callback_t)(uint8_t *buf, uint32_t len);

typedef struct {
  // Initialize the decoder. Can be called multiple times, will reinitalize.
  bool (*decoder_init)(decoded_data_callback_t decode_callback);

  // Cleanup the A2DP decoder.
  void (*decoder_cleanup)();

  // Decodes |p_buf| and calls |decode_callback| passed into init for the
  // decoded data.
  bool (*decode_packet)(dec_msg *p_buf);

  // Start the A2DP decoder.
  void (*decoder_start)();

  // Suspend the A2DP decoder.
  void (*decoder_suspend)();

  // A2DP decoder configuration.
  uint32_t (*decoder_configure)(const uint8_t *p_codec_info);

} tA2DP_DECODER_INTERFACE;
extern TaskHandle_t xHandle;
void send_to_decoder(uint8_t *packet, uint16_t size);
void init_dec_endpoint();
void init_decoder(uint8_t seid);
void stop_decoder();
void create_decoder_thread();
#endif