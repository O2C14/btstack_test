#ifndef A2DP_DEOCDER_H
#define A2DP_DEOCDER_H
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#define MAX_CHANNELS (2)
typedef struct {
  int8_t sampleFormat;
  uint32_t sample_rate;
  uint32_t nframes_per_buffer;
} pcm_info;
// typedef void (*decoded_data_callback_t)(uint8_t cid,uint8_t *buf, uint32_t len);

typedef struct {
  // Initialize the decoder. Can be called multiple times, will reinitalize.
  // void* (*decoder_init)(decoded_data_callback_t decode_callback);

  // Cleanup the A2DP decoder.
  void (*decoder_cleanup)(void *handle);

  // Decodes |p_buf| and calls |decode_callback| passed into init for the
  // decoded data.
  bool (*decode_packet)(void *handle, uint16_t con_handle, uint8_t *packet, uint16_t size);

  // Start the A2DP decoder.
  void (*decoder_start)();

  // Suspend the A2DP decoder.
  void (*decoder_suspend)();

  // A2DP decoder configuration.
  void *(*decoder_configure)(const uint8_t *p_codec_info, pcm_info *info_out);

} tA2DP_DECODER_INTERFACE;

void send_to_decoder(uint16_t con_handle, uint8_t seid, uint8_t *packet, uint16_t size);
void init_dec_endpoint();
void init_decoder(uint8_t cid, uint8_t seid);
void stop_decoder();

void decoder_save_sbc_config(uint8_t cid, uint8_t *packet, uint8_t remote_seid);
void decoder_save_aac_config(uint8_t cid, uint8_t *capa, uint8_t remote_seid);
void decoder_save_vendor_config(uint8_t cid, uint8_t *capa, uint16_t len, uint8_t remote_seid);
void decoder_submit_config();
void decoder_release(uint8_t cid, uint8_t seid);
void decoder_close(uint8_t cid, uint8_t seid);
#endif