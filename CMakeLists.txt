cmake_minimum_required (VERSION 3.22.1)

include(proj.conf)

find_package(bouffalo_sdk REQUIRED HINTS $ENV{BL_SDK_BASE})

sdk_set_main_file(main.c)
sdk_add_include_directories(.)
sdk_add_include_directories(./btblecontroller/btble_inc)
sdk_add_static_library(btblecontroller/lib/libbtblecontroller_bl616_ble1m0s1sbredr1.a)
project(btstack_test)

SET(BTSTACK_ROOT ${CMAKE_CURRENT_BINARY_DIR}/../btstack)
sdk_add_include_directories(${BTSTACK_ROOT}/3rd-party/micro-ecc)
sdk_add_include_directories(${BTSTACK_ROOT}/3rd-party/bluedroid/decoder/include)
sdk_add_include_directories(${BTSTACK_ROOT}/3rd-party/bluedroid/encoder/include)
sdk_add_include_directories(${BTSTACK_ROOT}/3rd-party/lc3-google/include)
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
file(GLOB SOURCES_LC3_GOOGLE "${BTSTACK_ROOT}/3rd-party/lc3-google/src/*.c")
file(GLOB SOURCES_FREERTOS_PORT "${BTSTACK_ROOT}/platform/freertos/btstack_run_loop_freertos.c")
file(GLOB SOURCES_HCI_STDOUT "${BTSTACK_ROOT}/platform/embedded/hci_dump_embedded_stdout.c")
file(GLOB SOURCES_BLE_OFF "${BTSTACK_ROOT}/src/ble/le_device_db_memory.c")
list(REMOVE_ITEM SOURCES_BLE   ${SOURCES_BLE_OFF})

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
    ${SOURCES_HCI_STDOUT}
)
list(SORT SOURCES)


set(EXAMPLE "a2dp_sink_demo")
find_package (Python REQUIRED COMPONENTS Interpreter)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${EXAMPLE}.h
    DEPENDS ${BTSTACK_ROOT}/example/${EXAMPLE}.gatt
    COMMAND ${Python_EXECUTABLE} ${BTSTACK_ROOT}/tool/compile_gatt.py ${BTSTACK_ROOT}/example/${EXAMPLE}.gatt ${CMAKE_CURRENT_BINARY_DIR}/${EXAMPLE}.h
    VERBATIM 
)
#add_custom_command 可能不会运行,所以我将这个指令输出,第一次编译会输出这个指令,第二次编译之前就可以执行这个指令
#add_custom_command may not run, so I will output this instruction. The first compilation will output this instruction, and it can be executed before the second compilation
message("${Python_EXECUTABLE} ${BTSTACK_ROOT}/tool/compile_gatt.py ${BTSTACK_ROOT}/example/${EXAMPLE}.gatt ${CMAKE_CURRENT_BINARY_DIR}/${EXAMPLE}.h")

sdk_add_include_directories(${CMAKE_CURRENT_BINARY_DIR}/)

target_sources(app PRIVATE 
${SOURCES}
btstack_port.c
#选择一个例子
#Choose an example
${BTSTACK_ROOT}/example/${EXAMPLE}.c
)