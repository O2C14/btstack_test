cmake_minimum_required (VERSION 3.22.1)

include(proj.conf)

find_package(bouffalo_sdk REQUIRED HINTS $ENV{BL_SDK_BASE})

sdk_set_main_file(main.c)
sdk_add_include_directories(.)
sdk_add_include_directories(./btblecontroller/btble_inc)
sdk_add_static_library(btblecontroller/lib/libbtblecontroller_bl616_ble1m0s1sbredr1.a)

SET(BTSTACK_ROOT ${CMAKE_CURRENT_BINARY_DIR}/../btstack)
sdk_add_include_directories(${BTSTACK_ROOT}/3rd-party/micro-ecc)
sdk_add_include_directories(${BTSTACK_ROOT}/3rd-party/bluedroid/decoder/include)
sdk_add_include_directories(${BTSTACK_ROOT}/3rd-party/bluedroid/encoder/include)
#sdk_add_include_directories(${BTSTACK_ROOT}/3rd-party/lc3-google/include)
sdk_add_include_directories(${BTSTACK_ROOT}/3rd-party/md5)
sdk_add_include_directories(${BTSTACK_ROOT}/3rd-party/hxcmod-player)
sdk_add_include_directories(${BTSTACK_ROOT}/3rd-party/hxcmod-player/mod)
sdk_add_include_directories(${BTSTACK_ROOT}/3rd-party/rijndael)
sdk_add_include_directories(${BTSTACK_ROOT}/3rd-party/yxml)
sdk_add_include_directories(${BTSTACK_ROOT}/3rd-party/tinydir)
sdk_add_include_directories(${BTSTACK_ROOT}/src)
sdk_add_include_directories(${BTSTACK_ROOT}/platform/embedded)
sdk_add_include_directories(${BTSTACK_ROOT}/platform/freertos)

file(GLOB SOURCES_SRC       "${BTSTACK_ROOT}/src/*.c" "${BTSTACK_ROOT}/example/sco_demo_util.c")
file(GLOB SOURCES_BLE       "${BTSTACK_ROOT}/src/ble/*.c")
file(GLOB SOURCES_BLUEDROID "${BTSTACK_ROOT}/3rd-party/bluedroid/encoder/srce/*.c" "${BTSTACK_ROOT}/3rd-party/bluedroid/decoder/srce/*.c")
file(GLOB SOURCES_CLASSIC   "${BTSTACK_ROOT}/src/classic/*.c")
file(GLOB SOURCES_MESH      "${BTSTACK_ROOT}/src/mesh/*.c")
file(GLOB SOURCES_GATT      "${BTSTACK_ROOT}/src/ble/gatt-service/*.c")
file(GLOB SOURCES_UECC      "${BTSTACK_ROOT}/3rd-party/micro-ecc/uECC.c")#ecc
file(GLOB SOURCES_RIJNDAEL  "${BTSTACK_ROOT}/3rd-party/rijndael/rijndael.c")#btstack_crypto.c aes
#file(GLOB SOURCES_LC3_GOOGLE "${BTSTACK_ROOT}/3rd-party/lc3-google/src/*.c")
file(GLOB SOURCES_FREERTOS_PORT "${BTSTACK_ROOT}/platform/freertos/btstack_run_loop_freertos.c")
file(GLOB SOURCES_HCI_STDOUT "${BTSTACK_ROOT}/platform/embedded/hci_dump_embedded_stdout.c")
file(GLOB SOURCES_BLE_OFF "${BTSTACK_ROOT}/src/ble/le_device_db_memory.c")
list(REMOVE_ITEM SOURCES_BLE   ${SOURCES_BLE_OFF})
file(GLOB SOURCES_HXCMOD    "${BTSTACK_ROOT}/3rd-party/hxcmod-player/*.c"  "${BTSTACK_ROOT}/3rd-party/hxcmod-player/mods/*.c")

set(SOURCES
    ${SOURCES_BLE}
    ${SOURCES_BLUEDROID}
    ${SOURCES_CLASSIC}
    ${SOURCES_GATT}
    ${SOURCES_MESH}
    ${SOURCES_RIJNDAEL}
    ${SOURCES_SRC}
    ${SOURCES_UECC}
    ${SOURCES_FREERTOS_PORT}
    ${SOURCES_HXCMOD}
#    ${SOURCES_HCI_STDOUT}
)
list(SORT SOURCES)


set(EXAMPLE "le_streamer_client")
find_package (Python REQUIRED COMPONENTS Interpreter)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${EXAMPLE}.h
    DEPENDS ${BTSTACK_ROOT}/example/${EXAMPLE}.gatt
    COMMAND ${Python_EXECUTABLE}
    ARGS ${BTSTACK_ROOT}/tool/compile_gatt.py ${BTSTACK_ROOT}/example/${EXAMPLE}.gatt ${CMAKE_CURRENT_BINARY_DIR}/${EXAMPLE}.h
)
#add_custom_command 可能不会运行,所以我将这个指令输出,第一次编译会输出这个指令,第二次编译之前就可以手动执行这个指令
#add_custom_command may not run, so I will output this instruction. The first compilation will output this instruction, and it can be executed before the second compilation
message("${Python_EXECUTABLE} ${BTSTACK_ROOT}/tool/compile_gatt.py ${BTSTACK_ROOT}/example/${EXAMPLE}.gatt ${CMAKE_CURRENT_BINARY_DIR}/${EXAMPLE}.h")

sdk_add_include_directories(${CMAKE_CURRENT_BINARY_DIR}/)


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
    codecs/LHDC
    codecs/LHDC/inc
)


list(APPEND SOURCES
codecs/AAC/aac_decoder.c
)
list(APPEND SOURCES
codecs/APTX/aptx_decoder.c
codecs/APTX/freeaptx.c
)
list(APPEND SOURCES
codecs/LDAC/ldac_decoder.c
codecs/LDAC/src/ldacBT.c
codecs/LDAC/src/ldaclib.c
)
file(GLOB SOURCES_LC3_GOOGLE "./codecs/LC3/liblc3-1.1.1/src/*.c")
list(APPEND SOURCES
${SOURCES_LC3_GOOGLE}
codecs/LC3/lc3plus_decoder.c
)
list(APPEND SOURCES
codecs/LHDC/src/Arc.o.c
codecs/LHDC/src/cirbuf.o.c
codecs/LHDC/src/dec.o.c
codecs/LHDC/src/DecodeUps2xLPF.o.c
#codecs/LHDC/src/encdec.o.c
codecs/LHDC/src/fast_upsampling_2x.o.c
codecs/LHDC/src/lhdcUtil.o.c
codecs/LHDC/src/lic_check.o.c
codecs/LHDC/src/licdecrypt.o.c
codecs/LHDC/src/lzh_dec.o.c
codecs/LHDC/lhdc_decoder.c
)
list(APPEND SOURCES
codecs/SBC/sbc_decoder.c
codecs/SBC/sbc-2.0/sbc/sbc.c
codecs/pcm_bridge.c
a2dp_decoder.c
a2dp_encoder.c
)
project(btstack_test)#这个影响了sdk_add_static_library的执行,在这之后执行sdk_add_static_library都是无效的

target_sources(app PRIVATE 
${SOURCES}

es9038q2m.c

btstack_port.c
#选择一个例子
#Choose an example
a2dp_sink_demo.c
#${BTSTACK_ROOT}/example/${EXAMPLE}.c
)
