#include <stdio.h>
#include <stdint.h>

#include "bl616_glb.h"

#include "btstack_config.h"
#include "btstack.h"
#include "classic/a2dp.h"
#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "a2dp_decoder.h"

#include "codecs/SBC/sbc_coder.h"
#include "codecs/AAC/aac_coder.h"

#include "codecs/APTX/aptx_coder.h"
#include "codecs/LDAC/ldac_coder.h"
#include "codecs/LHDCV5/lhdcv5_coder.h"
#include "codecs/OPUS/opus_coder.h"
#include "codecs/LC3/lc3plus_coder.h"
#include "codecs/pcm_bridge.h"

static tA2DP_SBC_CIE a2dp_sbc_sink_cfg;
static const tA2DP_SBC_CIE a2dp_sbc_sink_caps = {
    .config[0] = A2DP_SBC_SAMP_FREQ_48000 |
                 A2DP_SBC_SAMP_FREQ_44100 |
                 A2DP_SBC_CH_MODE_STREO |
                 A2DP_SBC_CH_MODE_DUAL |
                 A2DP_SBC_CH_MODE_JOINT,
    .config[1] = 0xff,
    .min_bitpool = 2,
    .max_bitpool = 53,
};
static tA2DP_AAC_CIE a2dp_aac_sink_cfg;
static const tA2DP_AAC_CIE a2dp_aac_sink_caps = {
    .ObjectType = 0xFF,
    .SamplingFrequency_Channels = 0xFFFF,
    .VBR_Bitrate[0] = 0xFF,
    .VBR_Bitrate[1] = 0xFF,
    .VBR_Bitrate[2] = 0xFF
};
static tA2DP_LDAC_CIE a2dp_ldac_sink_cfg;
static const tA2DP_LDAC_CIE a2dp_ldac_sink_caps = {
    .vendorId = A2DP_LDAC_VENDOR_ID,
    .codecId = A2DP_LDAC_CODEC_ID,
    .config[0] = (A2DP_LDAC_SAMPLING_FREQ_44100 | A2DP_LDAC_SAMPLING_FREQ_48000),
    .config[1] = (A2DP_LDAC_CHANNEL_MODE_DUAL | A2DP_LDAC_CHANNEL_MODE_STEREO)
};
static tA2DP_APTXHD_CIE a2dp_aptxhd_sink_cfg;
static const tA2DP_APTXHD_CIE a2dp_aptxhd_sink_caps = {
    .vendorId = A2DP_APTX_HD_VENDOR_ID,
    .codecId = A2DP_APTX_HD_CODEC_ID_BLUETOOTH,
    .channelMode = A2DP_APTX_HD_CHANNELS_STEREO,
    .sampleRate = (A2DP_APTX_HD_SAMPLERATE_48000 >> 4) |
                  (A2DP_APTX_HD_SAMPLERATE_44100 >> 4),
    .reserved = 0
};
static tA2DP_OPUS_CIE a2dp_opus_sink_cfg;
static const tA2DP_OPUS_CIE a2dp_opus_sink_caps = {
    .vendorId = A2DP_OPUS_VENDOR_ID, // vendorId
    .codecId = A2DP_OPUS_CODEC_ID,   // codecId
    .config[0] = A2DP_OPUS_CHANNEL_MODE_DUAL_MONO |
                 A2DP_OPUS_CHANNEL_MODE_STEREO | A2DP_OPUS_FRAMESIZE_MASK |
                 A2DP_OPUS_SAMPLING_FREQ_MASK
};
static tA2DP_LHDCv5_CIE a2dp_lhdcv5_sink_cfg;
static const tA2DP_LHDCv5_CIE a2dp_lhdcv5_sink_caps = {
    .vendorId = A2DP_LHDC_VENDOR_ID,
    .codecId = A2DP_LHDCV5_CODEC_ID,
    .config[0] = A2DP_LHDCV5_SAMPLING_FREQ_48000,
    .config[1] = A2DP_LHDCV5_BIT_FMT_16 | A2DP_LHDCV5_MAX_BIT_RATE_1000K | A2DP_LHDCV5_MIN_BIT_RATE_400K,
    .config[2] = A2DP_LHDCV5_VER_1 | A2DP_LHDCV5_FRAME_LEN_5MS,
    .config[3] = A2DP_LHDCV5_FEATURE_JAS,
    .config[4] = 0,
};
static tA2DP_LC3PLUS_CIE a2dp_lc3plus_sink_cfg;
static const tA2DP_LC3PLUS_CIE a2dp_lc3plus_sink_caps = {
    .vendorId = A2DP_LC3PLUS_VENDOR_ID,
    .codecId = A2DP_LC3PLUS_CODEC_ID,
    .frame_duration = A2DP_LC3PLUS_FRAME_DURATION_025 |
                      A2DP_LC3PLUS_FRAME_DURATION_050 |
                      A2DP_LC3PLUS_FRAME_DURATION_100,
    .channels = A2DP_LC3PLUS_CHANNELS_2,
    .frequency = A2DP_LC3PLUS_SAMPLING_FREQ_48000 | A2DP_LC3PLUS_SAMPLING_FREQ_96000
};
typedef struct {
    avdtp_stream_endpoint_t *endpoint;
    const tA2DP_DECODER_INTERFACE *itf;
} a2dp_decoder;
a2dp_decoder g_decoder[10];
typedef struct {
    uint8_t cid;
    uint8_t local_seid_priority;
    uint8_t remote_seid;
    void *cfg;
    uint8_t cfg_len;
} a2dp_config_process_info;
a2dp_config_process_info g_a2dp_config_process_info;

typedef struct {
    uint16_t acl_con_handle;
    uint8_t seid;
    void *codec_handle;
    uint32_t nframes_per_buffer;
} device_part;
static uint8_t num_of_devices = 0;
static device_part devs[4];
device_part* add_dev(uint16_t con_handle) {
    int devices_i = 0;
    bool is_have_valid_cid = false;
    for (; devices_i < sizeof(devs) / sizeof(devs[0]); devices_i++) {
        if (devs[devices_i].acl_con_handle == NULL) {
            is_have_valid_cid = true;
            break;
        }
    }
    if (is_have_valid_cid == true) {
        devs[devices_i].acl_con_handle = con_handle;
        num_of_devices++;
    } else {
        printf("devs no empty idx\n");
        return NULL;
    }
    return &devs[devices_i];
}

device_part* get_dev(uint16_t con_handle) {
    int devices_i;
    bool is_contain = false;
    for (devices_i = 0; devices_i < sizeof(devs) / sizeof(devs[0]); devices_i++) {
        if (devs[devices_i].acl_con_handle == con_handle) {
            is_contain = true;
            break;
        }
    }
    if (!is_contain) {
        return NULL;
    }
    return &devs[devices_i];
}

void init_dec_endpoint()
{
    uint8_t tmp_seid = 0;

    avdtp_stream_endpoint_t *sbc_endpoint =
        a2dp_sink_create_stream_endpoint(AVDTP_AUDIO,
                                         AVDTP_CODEC_SBC,
                                         &a2dp_sbc_sink_caps,
                                         sizeof(tA2DP_SBC_CIE),
                                         &a2dp_sbc_sink_cfg,
                                         sizeof(tA2DP_SBC_CIE));
    btstack_assert(sbc_endpoint != NULL);
    tmp_seid = avdtp_local_seid(sbc_endpoint);
    g_decoder[tmp_seid].endpoint = sbc_endpoint;
    g_decoder[tmp_seid].itf = NULL;
    g_decoder[tmp_seid].itf = A2DP_GetDecoderInterfaceSbc();
    
    avdtp_stream_endpoint_t *aac_endpoint =
        a2dp_sink_create_stream_endpoint(AVDTP_AUDIO,
                                        AVDTP_CODEC_MPEG_2_4_AAC,
                                        &a2dp_aac_sink_caps,
                                        sizeof(tA2DP_AAC_CIE),
                                        &a2dp_aac_sink_cfg,
                                        sizeof(tA2DP_AAC_CIE));
    btstack_assert(aac_endpoint != NULL);
    tmp_seid = avdtp_local_seid(aac_endpoint);
    g_decoder[tmp_seid].endpoint = aac_endpoint;
    g_decoder[tmp_seid].itf = NULL;
    g_decoder[tmp_seid].itf = A2DP_GetDecoderInterfaceAac();
    /*
    avdtp_stream_endpoint_t *aptxhd_endpoint =
        a2dp_sink_create_stream_endpoint(AVDTP_AUDIO,
                                        AVDTP_CODEC_NON_A2DP,
                                        &a2dp_aptxhd_sink_caps, 
                                        sizeof(tA2DP_APTXHD_CIE),
                                        &a2dp_aptxhd_sink_cfg, 
                                        sizeof(tA2DP_APTXHD_CIE));
    btstack_assert(aptxhd_endpoint != NULL);
    */
    avdtp_stream_endpoint_t *ldac_endpoint =
        a2dp_sink_create_stream_endpoint(AVDTP_AUDIO,
                                         AVDTP_CODEC_NON_A2DP,
                                         &a2dp_ldac_sink_caps,
                                         sizeof(tA2DP_LDAC_CIE),
                                         &a2dp_ldac_sink_cfg,
                                         sizeof(tA2DP_LDAC_CIE));
    btstack_assert(ldac_endpoint != NULL);
    tmp_seid = avdtp_local_seid(ldac_endpoint);
    g_decoder[tmp_seid].endpoint = ldac_endpoint;
    g_decoder[tmp_seid].itf = NULL;
    g_decoder[tmp_seid].itf = A2DP_GetDecoderInterfaceLdac();
    
    avdtp_stream_endpoint_t *lhdcv5_endpoint =
        a2dp_sink_create_stream_endpoint(AVDTP_AUDIO,
                                         AVDTP_CODEC_NON_A2DP,
                                         &a2dp_lhdcv5_sink_caps,
                                         sizeof(tA2DP_LHDCv5_CIE),
                                         &a2dp_lhdcv5_sink_cfg,
                                         sizeof(tA2DP_LHDCv5_CIE));
    btstack_assert(lhdcv5_endpoint != NULL);
    tmp_seid = avdtp_local_seid(lhdcv5_endpoint);
    g_decoder[tmp_seid].endpoint = lhdcv5_endpoint;
    g_decoder[tmp_seid].itf = NULL;
    g_decoder[tmp_seid].itf = A2DP_GetDecoderInterfaceLhdcV5();
    /*
    avdtp_stream_endpoint_t *lc3plus_endpoint =
        a2dp_sink_create_stream_endpoint(AVDTP_AUDIO,
                                        AVDTP_CODEC_NON_A2DP,
                                        &a2dp_lc3plus_sink_caps, 
                                        sizeof(tA2DP_LC3PLUS_CIE),
                                        &a2dp_lc3plus_sink_cfg, 
                                        sizeof(tA2DP_LC3PLUS_CIE));
    btstack_assert(lc3plus_endpoint != NULL);
    lc3plus_seid = avdtp_local_seid(lc3plus_endpoint);
    SetISCBySeid(lc3plus_seid, A2DP_GetDecoderInterfaceLc3plus(), &a2dp_lc3plus_sink_cfg);
    */
}

void init_decoder(uint8_t cid, uint8_t seid)
{
    if (g_decoder[seid].itf == NULL || g_decoder[seid].itf->decoder_configure == NULL) {
        printf("unknown seid %d\n", seid);
        return;
    }
    uint16_t acl_con_handle = avdtp_get_connection_for_avdtp_cid(cid)->con_handle;
    printf("acl_con_handle %d\n", acl_con_handle);

    device_part* p_dev = get_dev(acl_con_handle);

    bool is_have_valid_cid = false;
    if (p_dev != NULL) {
        decoder_release(cid, seid);
    }
    p_dev = add_dev(acl_con_handle);
    void* handle = NULL;
    pcm_info info;
    handle = g_decoder[seid].itf->decoder_configure(g_decoder[seid].endpoint->media_codec_configuration_info, &info);
    p_dev->codec_handle = handle;
    p_dev->nframes_per_buffer = info.nframes_per_buffer;
    double ms_per_buffer = (double)info.nframes_per_buffer * 1000. / (double)info.sample_rate;
    int base_size = 1;
    while (ms_per_buffer * base_size < 10.) {// 至少10ms检查一次
        base_size++;
    }

    int nbuffer = 1;
    while (ms_per_buffer * base_size * nbuffer < 100.) {// 至少100ms的缓冲区
        nbuffer++;
    }
    double latency_ms = ms_per_buffer * nbuffer * base_size;
    printf("latency ms:%d\n", (int)latency_ms);
    int decoded_audio_storage_size = base_size * info.nframes_per_buffer * (abs(info.sampleFormat) / 8) * MAX_CHANNELS;

    pcm_open(info.sample_rate, info.sampleFormat, 2, decoded_audio_storage_size, nbuffer);
}

avdtp_configuration_sbc_t sbc_configuration;
avdtp_configuration_mpeg_aac_t aac_configuration;

void decoder_save_sbc_config(uint8_t cid, uint8_t *packet, uint8_t remote_seid)
{
    size_t i;
    printf("A2DP  Sink      : Received SBC codec capability\n");
    for (i = 1; g_decoder[i].endpoint != NULL; i++) {
        if (g_decoder[i].endpoint->sep.capabilities.media_codec.media_codec_type == AVDTP_CODEC_SBC) {
            break;
        }
    }
    if (g_decoder[i].endpoint == NULL || g_a2dp_config_process_info.local_seid_priority > i) {
        return;
    }

    sbc_configuration.sampling_frequency = avdtp_choose_sbc_sampling_frequency(
        g_decoder[i].endpoint,
        avdtp_subevent_signaling_media_codec_sbc_capability_get_sampling_frequency_bitmap(packet));
    sbc_configuration.channel_mode = avdtp_choose_sbc_channel_mode(
        g_decoder[i].endpoint, avdtp_subevent_signaling_media_codec_sbc_capability_get_channel_mode_bitmap(packet));
    sbc_configuration.block_length = avdtp_choose_sbc_block_length(
        g_decoder[i].endpoint, avdtp_subevent_signaling_media_codec_sbc_capability_get_block_length_bitmap(packet));
    sbc_configuration.subbands = avdtp_choose_sbc_subbands(
        g_decoder[i].endpoint, avdtp_subevent_signaling_media_codec_sbc_capability_get_subbands_bitmap(packet));
    sbc_configuration.allocation_method = avdtp_choose_sbc_allocation_method(
        g_decoder[i].endpoint,
        avdtp_subevent_signaling_media_codec_sbc_capability_get_allocation_method_bitmap(packet));
    sbc_configuration.max_bitpool_value = avdtp_choose_sbc_max_bitpool_value(
        g_decoder[i].endpoint, avdtp_subevent_signaling_media_codec_sbc_capability_get_max_bitpool_value(packet));
    sbc_configuration.min_bitpool_value = avdtp_choose_sbc_min_bitpool_value(
        g_decoder[i].endpoint, avdtp_subevent_signaling_media_codec_sbc_capability_get_min_bitpool_value(packet));
    g_a2dp_config_process_info.cfg = &sbc_configuration;
    g_a2dp_config_process_info.cid = cid;
    g_a2dp_config_process_info.local_seid_priority = i;
    g_a2dp_config_process_info.remote_seid = remote_seid;
}

void decoder_save_aac_config(uint8_t cid, uint8_t *capa, uint8_t remote_seid)
{
    size_t i;
    printf("A2DP  Sink      : Received AAC codec capability\n");
    for (i = 1; g_decoder[i].endpoint != NULL; i++) {
        if (g_decoder[i].endpoint->sep.capabilities.media_codec.media_codec_type == AVDTP_CODEC_MPEG_2_4_AAC) {
            break;
        }
    }
    if (g_decoder[i].endpoint == NULL || g_a2dp_config_process_info.local_seid_priority > i) {
        return;
    }

    g_a2dp_config_process_info.remote_seid = remote_seid;
    g_a2dp_config_process_info.local_seid_priority = i;
    g_a2dp_config_process_info.cid = cid;
    g_a2dp_config_process_info.cfg = &aac_configuration;
    uint8_t remote_object_type = avdtp_subevent_signaling_media_codec_mpeg_aac_capability_get_object_type_bitmap(capa);
    for (int j = 1; j < 8; j++) {
        if (((remote_object_type & a2dp_aac_sink_caps.ObjectType) & (1 << j)) == 0) {
            continue;
        }
        aac_configuration.object_type = 8 - j;
        break;
    }
    uint16_t remote_sampling_frequency =
        avdtp_subevent_signaling_media_codec_mpeg_aac_capability_get_sampling_frequency_bitmap(capa);
    const uint32_t aac_sampling_frequency_table[] = { 96000, 88200, 64000, 48000, 44100, 32000,
                                                      24000, 22050, 16000, 12000, 11025, 8000 };
    for (size_t j = 0; j < (sizeof(aac_sampling_frequency_table) / sizeof(aac_sampling_frequency_table[0])); j++) {
        if ((remote_sampling_frequency & (1 << j)) != 0) {
            aac_configuration.sampling_frequency = aac_sampling_frequency_table[j];
            break;
        }
    }
    // printf("aac sr %d\n", aac_configuration.sampling_frequency);

    aac_configuration.channels = 2;
    // avdtp_subevent_signaling_media_codec_mpeg_aac_capability_get_channels_bitmap(capa);
    // aac_configuration.bit_rate = avdtp_subevent_signaling_media_codec_mpeg_aac_capability_get_bit_rate(capa);
    aac_configuration.bit_rate = 256000;
    aac_configuration.vbr = avdtp_subevent_signaling_media_codec_mpeg_aac_capability_get_vbr(capa);
}

void decoder_save_vendor_config(uint8_t cid, uint8_t *capa, uint16_t len, uint8_t remote_seid)
{
    size_t i;
    uint8_t *self_capa = NULL;
    bool have_valid_capa = false;
    for (i = 1; i != (sizeof(g_decoder) / sizeof(g_decoder[0])); i++) {
        if (g_decoder[i].endpoint == NULL) {
            continue;
        }
        self_capa = g_decoder[i].endpoint->sep.capabilities.media_codec.media_codec_information;
        if (*((uint32_t *)(self_capa)) == *((uint32_t *)(capa)) &&
            *((uint16_t *)(self_capa) + 2) == *((uint16_t *)(capa) + 2)) {
            have_valid_capa = true;
            break;
        }
    }
    if (!have_valid_capa) {
        return;
    }
    switch (*((uint32_t *)(capa))) {
        case A2DP_LDAC_VENDOR_ID: {
            if (g_a2dp_config_process_info.local_seid_priority > i) {
                return;
            }
            tA2DP_LDAC_CIE *t = capa;
            if (t->codecId != A2DP_LDAC_CODEC_ID) {
                return;
            }
            printf("A2DP_LDAC_VENDOR_ID\n");
            a2dp_ldac_sink_cfg.vendorId = A2DP_LDAC_VENDOR_ID;
            a2dp_ldac_sink_cfg.codecId = A2DP_LDAC_CODEC_ID;

            g_a2dp_config_process_info.cid = cid;
            g_a2dp_config_process_info.remote_seid = remote_seid;
            g_a2dp_config_process_info.local_seid_priority = i;
            g_a2dp_config_process_info.cfg = &a2dp_ldac_sink_cfg;
            g_a2dp_config_process_info.cfg_len = sizeof(a2dp_ldac_sink_cfg);
            do {
                if (t->config[0] & A2DP_LDAC_SAMPLING_FREQ_48000) {
                    a2dp_ldac_sink_cfg.config[0] = A2DP_LDAC_SAMPLING_FREQ_48000;
                    break;
                }
                if (t->config[0] & A2DP_LDAC_SAMPLING_FREQ_96000) {
                    a2dp_ldac_sink_cfg.config[0] = A2DP_LDAC_SAMPLING_FREQ_96000;
                    break;
                }
                if (t->config[0] & A2DP_LDAC_SAMPLING_FREQ_44100) {
                    a2dp_ldac_sink_cfg.config[0] = A2DP_LDAC_SAMPLING_FREQ_44100;
                    break;
                }
                if (t->config[0] & A2DP_LDAC_SAMPLING_FREQ_88200) {
                    a2dp_ldac_sink_cfg.config[0] = A2DP_LDAC_SAMPLING_FREQ_88200;
                    break;
                }
            } while (0);
            do {
                if (t->config[1] & A2DP_LDAC_CHANNEL_MODE_DUAL) {
                    a2dp_ldac_sink_cfg.config[1] = A2DP_LDAC_CHANNEL_MODE_DUAL;
                    break;
                }
                if (t->config[1] & A2DP_LDAC_CHANNEL_MODE_STEREO) {
                    a2dp_ldac_sink_cfg.config[1] = A2DP_LDAC_CHANNEL_MODE_STEREO;
                    break;
                }
            } while (0);
        } break;
        case A2DP_APTX_HD_VENDOR_ID:
            printf("A2DP_APTX_HD_VENDOR_ID\n");
            break;
        case A2DP_LHDC_VENDOR_ID:
            switch (*((uint16_t *)(capa + 4))) {
                case A2DP_LHDCV1_CODEC_ID:
                    printf("A2DP_LHDCV1_CODEC_ID \n");
                    break;
                case A2DP_LHDCV2_CODEC_ID:
                    printf("A2DP_LHDCV2_CODEC_ID \n");
                    break;
                case A2DP_LHDCV3_CODEC_ID:
                    printf("A2DP_LHDCV3_CODEC_ID \n");
                    break;
                case A2DP_LHDCV5_CODEC_ID:
                    printf("A2DP_LHDCV5_CODEC_ID \n");

                    tA2DP_LHDCv5_CIE* lhdcv5_capa = capa;
                    tA2DP_LHDCv5_CIE* lhdcv5_cfg = self_capa;
                    if (g_a2dp_config_process_info.local_seid_priority > i) {
                        break;
                    }
                    g_a2dp_config_process_info.local_seid_priority = i;
                    lhdcv5_cfg->vendorId = A2DP_LHDC_VENDOR_ID;
                    lhdcv5_cfg->codecId = A2DP_LHDCV5_CODEC_ID;

                    memset(lhdcv5_cfg->config, 0, sizeof(lhdcv5_cfg->config));
                    if (lhdcv5_capa->config[0] & A2DP_LHDCV5_SAMPLING_FREQ_48000) {
                        lhdcv5_cfg->config[0] |= A2DP_LHDCV5_SAMPLING_FREQ_48000;
                    } else if (lhdcv5_capa->config[0] & A2DP_LHDCV5_SAMPLING_FREQ_44100) {
                        lhdcv5_cfg->config[0] |= A2DP_LHDCV5_SAMPLING_FREQ_44100;
                    } else if (lhdcv5_capa->config[0] & A2DP_LHDCV5_SAMPLING_FREQ_96000) {
                        lhdcv5_cfg->config[0] |= A2DP_LHDCV5_SAMPLING_FREQ_96000;
                    } else if (lhdcv5_capa->config[0] & A2DP_LHDCV5_SAMPLING_FREQ_192000) {
                        lhdcv5_cfg->config[0] |= A2DP_LHDCV5_SAMPLING_FREQ_192000;
                    }
                    if (lhdcv5_capa->config[1] & A2DP_LHDCV5_BIT_FMT_16) {
                        lhdcv5_cfg->config[1] |= A2DP_LHDCV5_BIT_FMT_16;
                    } else if (lhdcv5_capa->config[1] & A2DP_LHDCV5_BIT_FMT_24) {
                        lhdcv5_cfg->config[1] |= A2DP_LHDCV5_BIT_FMT_24;
                    } else if (lhdcv5_capa->config[1] & A2DP_LHDCV5_BIT_FMT_32) {
                        lhdcv5_cfg->config[1] |= A2DP_LHDCV5_BIT_FMT_32;
                    }
                    if (lhdcv5_capa->config[1] & A2DP_LHDCV5_MAX_BIT_RATE_1000K) {
                        lhdcv5_cfg->config[1] |= A2DP_LHDCV5_MAX_BIT_RATE_1000K;
                    } else if (lhdcv5_capa->config[1] & A2DP_LHDCV5_MAX_BIT_RATE_900K) {
                        lhdcv5_cfg->config[1] |= A2DP_LHDCV5_MAX_BIT_RATE_900K;
                    } else if (lhdcv5_capa->config[1] & A2DP_LHDCV5_MAX_BIT_RATE_500K) {
                        lhdcv5_cfg->config[1] |= A2DP_LHDCV5_MAX_BIT_RATE_500K;
                    } else if (lhdcv5_capa->config[1] & A2DP_LHDCV5_MAX_BIT_RATE_400K) {
                        lhdcv5_cfg->config[1] |= A2DP_LHDCV5_MAX_BIT_RATE_400K;
                    }
                    if (lhdcv5_capa->config[1] & A2DP_LHDCV5_MIN_BIT_RATE_400K) {
                        lhdcv5_cfg->config[1] |= A2DP_LHDCV5_MIN_BIT_RATE_400K;
                    } else if (lhdcv5_capa->config[1] & A2DP_LHDCV5_MIN_BIT_RATE_256K) {
                        lhdcv5_cfg->config[1] |= A2DP_LHDCV5_MIN_BIT_RATE_256K;
                    } else if (lhdcv5_capa->config[1] & A2DP_LHDCV5_MIN_BIT_RATE_128K) {
                        lhdcv5_cfg->config[1] |= A2DP_LHDCV5_MIN_BIT_RATE_128K;
                    } else if (lhdcv5_capa->config[1] & A2DP_LHDCV5_MIN_BIT_RATE_64K) {
                        lhdcv5_cfg->config[1] |= A2DP_LHDCV5_MIN_BIT_RATE_64K;
                    }
                    if (lhdcv5_capa->config[3] & A2DP_LHDCV5_FEATURE_JAS) {
                        lhdcv5_cfg->config[3] |= A2DP_LHDCV5_FEATURE_JAS;
                    }
                    g_a2dp_config_process_info.cfg = &a2dp_lhdcv5_sink_cfg;
                    g_a2dp_config_process_info.cfg_len = sizeof(a2dp_lhdcv5_sink_cfg);
                    g_a2dp_config_process_info.cid = cid;
                    g_a2dp_config_process_info.remote_seid = remote_seid;
                    break;
                default:
                    break;
            }
            break;

        default:
            break;
    }
}

void decoder_submit_config()
{
    avdtp_media_codec_type_t media_codec_type = g_decoder[g_a2dp_config_process_info.local_seid_priority]
                                                    .endpoint->sep.capabilities.media_codec.media_codec_type;
    switch (media_codec_type) {
        case AVDTP_CODEC_SBC:
            printf("send sbc cfg sied %d\n", g_a2dp_config_process_info.local_seid_priority);
            a2dp_config_process_set_sbc(AVDTP_ROLE_SINK, g_a2dp_config_process_info.cid,
                                        g_a2dp_config_process_info.local_seid_priority,
                                        g_a2dp_config_process_info.remote_seid, g_a2dp_config_process_info.cfg);
            break;
        case AVDTP_CODEC_MPEG_2_4_AAC:
            printf("send aac cfg seid %d\n", g_a2dp_config_process_info.local_seid_priority);
            a2dp_config_process_set_mpeg_aac(AVDTP_ROLE_SINK, g_a2dp_config_process_info.cid,
                                             g_a2dp_config_process_info.local_seid_priority,
                                             g_a2dp_config_process_info.remote_seid, g_a2dp_config_process_info.cfg);
            break;
        case AVDTP_CODEC_NON_A2DP:
            printf("send vendor cfg seid %d\n", g_a2dp_config_process_info.local_seid_priority);
            a2dp_config_process_set_other(AVDTP_ROLE_SINK, g_a2dp_config_process_info.cid,
                                          g_a2dp_config_process_info.local_seid_priority,
                                          g_a2dp_config_process_info.remote_seid, g_a2dp_config_process_info.cfg,
                                          g_a2dp_config_process_info.cfg_len);
            break;
        default:
            break;
    }
    memset(&g_a2dp_config_process_info, 0, sizeof(g_a2dp_config_process_info));
}

void decoder_close(uint8_t cid, uint8_t seid)
{
    uint16_t acl_con_handle = avdtp_get_connection_for_avdtp_cid(cid)->con_handle;
    device_part* p_dev = get_dev(acl_con_handle);
    if (p_dev == NULL) {
        return;
    }

    if (g_decoder[seid].itf != NULL && g_decoder[seid].itf->decoder_cleanup != NULL) {
        if (p_dev->codec_handle != NULL) {
            g_decoder[seid].itf->decoder_cleanup(p_dev->codec_handle);
            p_dev->codec_handle = NULL;
        }
    }

    // free(decoded_audio_storage);
    // decoded_audio_storage = NULL;
}

void decoder_release(uint8_t cid, uint8_t seid)
{
    uint16_t acl_con_handle = avdtp_get_connection_for_avdtp_cid(cid)->con_handle;
    device_part* p_dev = get_dev(acl_con_handle);

    if (p_dev == NULL) {
        return;
    }

    decoder_close(cid, seid);
    p_dev->acl_con_handle = 0;
    num_of_devices--;
}

void stop_decoder()
{
    // i2s_stop();
}
// con_handle: multi devices play
void send_to_decoder(uint16_t con_handle, uint8_t seid, uint8_t* packet, uint16_t size) {
    if (g_decoder[seid].itf == NULL || g_decoder[seid].itf->decode_packet == NULL) {
        return;
    }
    device_part* p_dev = get_dev(con_handle);

    if (p_dev == NULL) {
        printf("send_to_decoder no cid\n");
        return;
    }
    if (p_dev->codec_handle == NULL) {
        // init_decoder(con_handle, seid);
        printf("send_to_decoder no codec\n");
        return;
        // printf("send_to_decoder no codec\n");
    }

    g_decoder[seid].itf->decode_packet(p_dev->codec_handle, con_handle, packet, size);
}