#include "ldac_coder.h"
#include "a2dp_codec_api.h"
#include "bflb_mtimer.h"
#include "pcm_bridge.h"
#include "ldacBT.h"
#include "bl616_glb.h"
/* Convert LDAC Error Code to string */
#define CASE_RETURN_STR(const) \
  case const:                  \
    return #const;
static const char *ldac_ErrCode2Str(int ErrCode)
{
  switch (ErrCode) {
    CASE_RETURN_STR(LDACBT_ERR_NONE);
    CASE_RETURN_STR(LDACBT_ERR_NON_FATAL);
    CASE_RETURN_STR(LDACBT_ERR_BIT_ALLOCATION);
    CASE_RETURN_STR(LDACBT_ERR_NOT_IMPLEMENTED);
    CASE_RETURN_STR(LDACBT_ERR_NON_FATAL_ENCODE);
    CASE_RETURN_STR(LDACBT_ERR_FATAL);
    CASE_RETURN_STR(LDACBT_ERR_SYNTAX_BAND);
    CASE_RETURN_STR(LDACBT_ERR_SYNTAX_GRAD_A);
    CASE_RETURN_STR(LDACBT_ERR_SYNTAX_GRAD_B);
    CASE_RETURN_STR(LDACBT_ERR_SYNTAX_GRAD_C);
    CASE_RETURN_STR(LDACBT_ERR_SYNTAX_GRAD_D);
    CASE_RETURN_STR(LDACBT_ERR_SYNTAX_GRAD_E);
    CASE_RETURN_STR(LDACBT_ERR_SYNTAX_IDSF);
    CASE_RETURN_STR(LDACBT_ERR_SYNTAX_SPEC);
    CASE_RETURN_STR(LDACBT_ERR_BIT_PACKING);
    CASE_RETURN_STR(LDACBT_ERR_ALLOC_MEMORY);
    CASE_RETURN_STR(LDACBT_ERR_FATAL_HANDLE);
    CASE_RETURN_STR(LDACBT_ERR_ILL_SYNCWORD);
    CASE_RETURN_STR(LDACBT_ERR_ILL_SMPL_FORMAT);
    CASE_RETURN_STR(LDACBT_ERR_ILL_PARAM);
    CASE_RETURN_STR(LDACBT_ERR_ASSERT_SAMPLING_FREQ);
    CASE_RETURN_STR(LDACBT_ERR_ASSERT_SUP_SAMPLING_FREQ);
    CASE_RETURN_STR(LDACBT_ERR_CHECK_SAMPLING_FREQ);
    CASE_RETURN_STR(LDACBT_ERR_ASSERT_CHANNEL_CONFIG);
    CASE_RETURN_STR(LDACBT_ERR_CHECK_CHANNEL_CONFIG);
    CASE_RETURN_STR(LDACBT_ERR_ASSERT_FRAME_LENGTH);
    CASE_RETURN_STR(LDACBT_ERR_ASSERT_SUP_FRAME_LENGTH);
    CASE_RETURN_STR(LDACBT_ERR_ASSERT_FRAME_STATUS);
    CASE_RETURN_STR(LDACBT_ERR_ASSERT_NSHIFT);
    CASE_RETURN_STR(LDACBT_ERR_ASSERT_CHANNEL_MODE);
    CASE_RETURN_STR(LDACBT_ERR_ENC_INIT_ALLOC);
    CASE_RETURN_STR(LDACBT_ERR_ENC_ILL_GRADMODE);
    CASE_RETURN_STR(LDACBT_ERR_ENC_ILL_GRADPAR_A);
    CASE_RETURN_STR(LDACBT_ERR_ENC_ILL_GRADPAR_B);
    CASE_RETURN_STR(LDACBT_ERR_ENC_ILL_GRADPAR_C);
    CASE_RETURN_STR(LDACBT_ERR_ENC_ILL_GRADPAR_D);
    CASE_RETURN_STR(LDACBT_ERR_ENC_ILL_NBANDS);
    CASE_RETURN_STR(LDACBT_ERR_PACK_BLOCK_FAILED);
    CASE_RETURN_STR(LDACBT_ERR_DEC_INIT_ALLOC);
    CASE_RETURN_STR(LDACBT_ERR_INPUT_BUFFER_SIZE);
    CASE_RETURN_STR(LDACBT_ERR_UNPACK_BLOCK_FAILED);
    CASE_RETURN_STR(LDACBT_ERR_UNPACK_BLOCK_ALIGN);
    CASE_RETURN_STR(LDACBT_ERR_UNPACK_FRAME_ALIGN);
    CASE_RETURN_STR(LDACBT_ERR_FRAME_LENGTH_OVER);
    CASE_RETURN_STR(LDACBT_ERR_FRAME_ALIGN_OVER);
    CASE_RETURN_STR(LDACBT_ERR_ALTER_EQMID_LIMITED);
    CASE_RETURN_STR(LDACBT_ERR_ILL_EQMID);
    CASE_RETURN_STR(LDACBT_ERR_ILL_SAMPLING_FREQ);
    CASE_RETURN_STR(LDACBT_ERR_ILL_NUM_CHANNEL);
    CASE_RETURN_STR(LDACBT_ERR_ILL_MTU_SIZE);
    CASE_RETURN_STR(LDACBT_ERR_HANDLE_NOT_INIT);
    default:
      return "unknown-error-code";
  }
}
static char a_ErrorCodeStr[128];
static const char *get_error_code_string(int error_code)
{
  int errApi, errHdl, errBlk;

  errApi = LDACBT_API_ERR(error_code);
  errHdl = LDACBT_HANDLE_ERR(error_code);
  errBlk = LDACBT_BLOCK_ERR(error_code);

  a_ErrorCodeStr[0] = '\0';
  strcat(a_ErrorCodeStr, "API:");
  strcat(a_ErrorCodeStr, ldac_ErrCode2Str(errApi));
  strcat(a_ErrorCodeStr, " Handle:");
  strcat(a_ErrorCodeStr, ldac_ErrCode2Str(errHdl));
  strcat(a_ErrorCodeStr, " Block:");
  strcat(a_ErrorCodeStr, ldac_ErrCode2Str(errBlk));
  return a_ErrorCodeStr;
}

static int32_t samplerate = 0;
static HANDLE_LDAC_BT ldacBT_dec_handle = NULL;
//512 1024
static bool a2dp_vendor_ldac_decoder_decode_packet(dec_msg *p_buf)
{
  const uint8_t frame_number = p_buf->packet[0]&0b1111;
  const int32_t streamSize = p_buf->size - A2DP_LDAC_MPL_HDR_LEN;
  const uint8_t *pStream = p_buf->packet + A2DP_LDAC_MPL_HDR_LEN;
  int32_t used_Stream_count = 0;
  
  for (int32_t i = 0; i < frame_number; i++)
  {
    int32_t out_pcm_szie = 0, streamUsed = 0;
    set_start_loc(0);
    //set_start_loc(CURRENT_USED_BYTES - 4096);

    int result = ldacBT_decode(ldacBT_dec_handle, &pStream[used_Stream_count],
                               get_pcm_tail(), LDACBT_SMPL_FMT_S16,
                               streamSize - used_Stream_count, &streamUsed,
                               &out_pcm_szie);
    
    if (result != 0) {
      printf("ldacBT_decode error: %s\r\n", get_error_code_string(ldacBT_get_error_code(ldacBT_dec_handle)));
      printf("Error generated during decoding in stage %d\r\n", result);
    }
    used_Stream_count += streamUsed;
    //write_pcm(tmp_pcm_buffer, out_pcm_szie);
    check_buffer_edge(out_pcm_szie);
  }
  
  return 1;
}

static bool a2dp_vendor_ldac_decoder_init(decoded_data_callback_t decode_callback)
{

  if (ldacBT_dec_handle == NULL) {
    ldacBT_dec_handle = ldacBT_get_handle();
    ldacBT_init_handle_decode(ldacBT_dec_handle,
                              LDACBT_CHANNEL_MODE_DUAL_CHANNEL,
                              samplerate, 0, 0, 0);
  }
}
static void a2dp_vendor_ldac_decoder_decoder_cleanup()
{
  if (ldacBT_dec_handle) {
    ldacBT_close_handle(ldacBT_dec_handle);
    ldacBT_dec_handle = NULL;
  }
}
static uint32_t a2dp_vendor_ldac_decoder_configure(const uint8_t *codec_cfg)
{
  tA2DP_LDAC_CIE *cfg = codec_cfg;
  if ((cfg->vendorId != A2DP_LDAC_VENDOR_ID) ||
      (cfg->codecId != A2DP_LDAC_CODEC_ID)) {
    printf("LDAC cfg error\n");
    samplerate = 48000;
    return DataWidth_16 | SR_48000;
  }
  printf("use LDAC\n");
  switch (cfg->config[0] & A2DP_LDAC_SAMPLING_FREQ_MASK) {
    case A2DP_LDAC_SAMPLING_FREQ_96000: {
      samplerate = 96000;
      return DataWidth_16 | SR_96000;
    }
    case A2DP_LDAC_SAMPLING_FREQ_88200: {
      samplerate = 88200;
      return DataWidth_16 | SR_88200;
    }
    case A2DP_LDAC_SAMPLING_FREQ_48000: {
      samplerate = 48000;
      return DataWidth_16 | SR_48000;
    }
    case A2DP_LDAC_SAMPLING_FREQ_44100: {
      samplerate = 44100;
      return DataWidth_16 | SR_44100;
    }
    default:
      return 0;
  }
}
static const tA2DP_DECODER_INTERFACE a2dp_decoder_interface_ldac = {
  &a2dp_vendor_ldac_decoder_init,
  &a2dp_vendor_ldac_decoder_decoder_cleanup,
  &a2dp_vendor_ldac_decoder_decode_packet,
  NULL,                                // decoder_start
  NULL,                                // decoder_suspend
  &a2dp_vendor_ldac_decoder_configure, // decoder_configure
};
const tA2DP_DECODER_INTERFACE *A2DP_GetDecoderInterfaceLdac()
{
  return &a2dp_decoder_interface_ldac;
}