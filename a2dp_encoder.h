#ifndef A2DP_ENCODER_H
#define A2DP_ENCODER_H
#include <stdint.h>
//
// A2DP encoder callbacks interface.
//
typedef struct {
  // Initialize the A2DP encoder.
  // |p_peer_params| contains the A2DP peer information
  // The current A2DP codec config is in |a2dp_codec_config|.
  // |read_callback| is the callback for reading the input audio data.
  // |enqueue_callback| is the callback for enqueueing the encoded audio data.
  void (*encoder_init)(const uint8_t *p_codec_info);

  // Cleanup the A2DP encoder.
  void (*encoder_cleanup)(void);

  // Reset the feeding for the A2DP encoder.
  void (*feeding_reset)(void);

  // Flush the feeding for the A2DP encoder.
  void (*feeding_flush)(void);

  // Get the A2DP encoder interval (in milliseconds).
  uint64_t (*get_encoder_interval_ms)(void);

  // Get the A2DP encoded maximum frame size (similar to MTU).
  int (*get_effective_frame_size)(void);

  // Prepare and send A2DP encoded frames.
  // |timestamp_us| is the current timestamp (in microseconds).
  void (*send_frames)(uint64_t timestamp_us);

  // Set transmit queue length for the A2DP encoder.
  void (*set_transmit_queue_length)(uint32_t transmit_queue_length);
} tA2DP_ENCODER_INTERFACE;

extern avdtp_stream_endpoint_t *sbc_enc_endpoint;
void init_enc_endpoint();
#endif
