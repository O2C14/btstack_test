#sdk_generate_library(btstack)

SET(BTSTACK_ROOT E:/codec/btstack)
include_directories(${BTSTACK_ROOT}/3rd-party/micro-ecc)
include_directories(${BTSTACK_ROOT}/3rd-party/bluedroid/decoder/include)
include_directories(${BTSTACK_ROOT}/3rd-party/bluedroid/encoder/include)
include_directories(${BTSTACK_ROOT}/3rd-party/md5)
include_directories(${BTSTACK_ROOT}/3rd-party/hxcmod-player)
include_directories(${BTSTACK_ROOT}/3rd-party/hxcmod-player/mod)
include_directories(${BTSTACK_ROOT}/3rd-party/lc3-google/include)
include_directories(${BTSTACK_ROOT}/3rd-party/lwip/core/src/include)
include_directories(${BTSTACK_ROOT}/3rd-party/lwip/dhcp-server)
include_directories(${BTSTACK_ROOT}/3rd-party/rijndael)
include_directories(${BTSTACK_ROOT}/3rd-party/yxml)
include_directories(${BTSTACK_ROOT}/3rd-party/tinydir)
include_directories(${BTSTACK_ROOT}/src)
include_directories(${BTSTACK_ROOT}/example)
include_directories(${BTSTACK_ROOT}/chipset/zephyr)
include_directories(${BTSTACK_ROOT}/chipset/bcm)
include_directories(${BTSTACK_ROOT}/platform/freertos)
include_directories(${BTSTACK_ROOT}/platform/embedded)
include_directories(${BTSTACK_ROOT}/platform/lwip)
include_directories(${BTSTACK_ROOT}/platform/lwip/port)

file(GLOB SOURCES_SRC       "${BTSTACK_ROOT}/src/*.c" "${BTSTACK_ROOT}/src/*.cpp" "${BTSTACK_ROOT}/example/sco_demo_util.c")
file(GLOB SOURCES_BLE       "${BTSTACK_ROOT}/src/ble/*.c")
file(GLOB SOURCES_GATT      "${BTSTACK_ROOT}/src/ble/gatt-service/*.c")
file(GLOB SOURCES_CLASSIC   "${BTSTACK_ROOT}/src/classic/*.c")
file(GLOB SOURCES_LE_AUDIO  "${BTSTACK_ROOT}/src/le-audio/*.c" "${BTSTACK_ROOT}/src/le-audio/gatt-service/*.c" "${BTSTACK_ROOT}/example/le_audio_demo_util_*.c")
file(GLOB SOURCES_MESH      "${BTSTACK_ROOT}/src/mesh/*.c")
file(GLOB SOURCES_MD5       "${BTSTACK_ROOT}/3rd-party/md5/md5.c")
file(GLOB SOURCES_UECC      "${BTSTACK_ROOT}/3rd-party/micro-ecc/uECC.c")
file(GLOB SOURCES_YXML      "${BTSTACK_ROOT}/3rd-party/yxml/yxml.c")
file(GLOB SOURCES_HXCMOD    "${BTSTACK_ROOT}/3rd-party/hxcmod-player/*.c"  "${BTSTACK_ROOT}/3rd-party/hxcmod-player/mods/*.c")
file(GLOB SOURCES_RIJNDAEL  "${BTSTACK_ROOT}/3rd-party/rijndael/rijndael.c")
file(GLOB SOURCES_LC3_GOOGLE "${BTSTACK_ROOT}/3rd-party/lc3-google/src/*.c")
file(GLOB SOURCES_FREERTOS_PORT "${BTSTACK_ROOT}/platform/freertos/btstack_run_loop_freertos.c")
file(GLOB SOURCES_BLE_OFF "${BTSTACK_ROOT}/src/ble/le_device_db_memory.c")
list(REMOVE_ITEM SOURCES_BLE   ${SOURCES_BLE_OFF})

set(SOURCES 
	${SOURCES_MD5}
	${SOURCES_YXML}
	${SOURCES_LC3_GOOGLE}
	${SOURCES_RIJNDAEL}
	${SOURCES_SRC}
	${SOURCES_BLE}
	${SOURCES_GATT}
	${SOURCES_LE_AUDIO}
	${SOURCES_MESH}
	${SOURCES_CLASSIC} 
	${SOURCES_UECC}
	${SOURCES_HXCMOD}
    ${SOURCES_FREERTOS_PORT}

)
list(SORT SOURCES)
#sdk_library_add_sources(${SOURCES})
