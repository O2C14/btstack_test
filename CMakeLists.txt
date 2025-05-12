cmake_minimum_required (VERSION 3.22.1)

include(proj.conf)

find_package(bouffalo_sdk REQUIRED HINTS $ENV{BL_SDK_BASE})

option(CONFIG_BTSTACK_LOG "CONFIG BTSTACK LOG" FALSE)
option(BUILD_BTSTACK "build BTstack" FALSE)
include(btstack.cmake)

sdk_set_main_file(main.c)
sdk_add_include_directories(.)
sdk_add_include_directories(rwip)
#include(rwip/rwip.cmake)
#sdk_add_static_library($ENV{BL_SDK_BASE}/examples/rwip/build/build_out/lib/librwip.a)
sdk_add_static_library(rwip/librwip.a)
sdk_add_static_library(rwip/librwip_rf.a)
target_sources(app PRIVATE 
    rwip/arch_main.c
    rwip/nvds.c
    rwip/rwip_rf_bl616.c
)
include(lhdcv5.cmake)

sdk_add_static_library(codecs/AAC/build/libaac_dec.a)
#sdk_add_static_library(codecs/AAC/build/libaac_enc.a)
sdk_add_static_library(codecs/AAC/build/libaac_other.a)

sdk_add_include_directories(
    codecs
    codecs/SBC
    codecs/AAC
    codecs/AAC/aac/libAACdec/include
    codecs/AAC/aac/libAACenc/include
    codecs/AAC/aac/libSYS/include
    codecs/libopenaptx
    codecs/LDAC
    codecs/LDAC/inc
    codecs/LC3
    codecs/LC3/liblc3-1.1.1/include
)
set(APP_SOURCES
es9038q2m.c
codecs/pcm_bridge.c
a2dp_decoder.c
a2dp_encoder.c
a2dp_sink_demo.c
)
set(NOP_SOURCES
a2dp_sink_demo.c
)
list(APPEND APP_SOURCES
codecs/AAC/aac_decoder.c
)
list(APPEND NOP_SOURCES
codecs/APTX/aptx_decoder.c
codecs/APTX/freeaptx.c
)
list(APPEND APP_SOURCES
codecs/LDAC/ldac_decoder.c
codecs/LDAC/src/ldacBT.c
codecs/LDAC/src/ldaclib.c
)
file(GLOB SOURCES_LC3_GOOGLE "./codecs/LC3/liblc3-1.1.1/src/*.c")
list(APPEND NOP_SOURCES
${SOURCES_LC3_GOOGLE}
codecs/LC3/lc3plus_decoder.c
)
#sdk_add_include_directories(E:/codec/lhdcv5/lhdcv5/liblhdcv5dec/inc)
#sdk_add_include_directories(E:/codec/lhdcv5/lhdcv5/liblhdcv5dec/include)
sdk_add_include_directories(E:/codec/lhdcv5/lhdcv5/liblhdcv5dec/src/dec/lhdc_v5_dec)
sdk_add_include_directories(E:/codec/lhdcv5/lhdcv5/liblhdcv5dec/src/dec)
list(APPEND APP_SOURCES
codecs/LHDCV5/lhdcv5_decoder.c
)
list(APPEND APP_SOURCES
codecs/SBC/sbc_decoder.c
codecs/SBC/sbc-2.0/sbc/sbc.c
)

target_sources(app PRIVATE 
    ${APP_SOURCES}
    btstack_port.c
    libqcc743_on_bl616.c
)
project(btstack_test)#这个影响了sdk_add_static_library的执行,在这之后执行sdk_add_static_library都是无效的
