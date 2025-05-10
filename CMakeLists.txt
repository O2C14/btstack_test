cmake_minimum_required (VERSION 3.22.1)

include(proj.conf)

find_package(bouffalo_sdk REQUIRED HINTS $ENV{BL_SDK_BASE})

option(CONFIG_BTSTACK_LOG "CONFIG BTSTACK LOG" FALSE)
if (CONFIG_BTSTACK_LOG)
sdk_add_compile_definitions(-DCONFIG_BTSTACK_LOG)
sdk_add_compile_definitions(-DENABLE_LOG_ERROR)
sdk_add_compile_definitions(-DENABLE_LOG_INFO)
sdk_add_compile_definitions(-DENABLE_PRINTF_HEXDUMP)
endif()

include(btstack.cmake)
#include(multitarget.cmake)

sdk_set_main_file(main.c)
sdk_add_include_directories(.)
sdk_add_include_directories(${BTSTACK_ROOT}/example)
sdk_add_include_directories(rwip)
#include(rwip/rwip.cmake)
#sdk_add_static_library($ENV{BL_SDK_BASE}/examples/rwip/build/build_out/lib/librwip.a)
sdk_add_static_library(rwip/librwip.a)
sdk_add_static_library(rwip/librwip_rf.a)
sdk_add_static_library($ENV{BL_SDK_BASE}/examples/btstack_build/build/build_out/lib/libbtstack.a)
target_sources(app PRIVATE 
    rwip/arch_main.c
    rwip/nvds.c
    rwip/rwip_rf_bl616.c
)


#选择一个例子
#Choose an example

find_package(Python REQUIRED COMPONENTS Interpreter)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/le_audio_broadcast_sink.h
    DEPENDS E:/codec/bouffalo_sdk/examples/btstack_test/le_audio/le_audio_broadcast_sink.gatt
    COMMAND ${Python_EXECUTABLE} ${BTSTACK_ROOT}/tool/compile_gatt.py E:/codec/bouffalo_sdk/examples/btstack_test/le_audio/le_audio_broadcast_sink.gatt ${CMAKE_CURRENT_BINARY_DIR}/le_audio_broadcast_sink.h
    VERBATIM 
)
target_sources(app PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/le_audio_broadcast_sink.h)

sdk_add_include_directories(${CMAKE_CURRENT_BINARY_DIR})


target_sources(app PRIVATE 
    btstack_port.c
    le_audio/le_audio_broadcast_sink.c
    #le_audio/le_audio_broadcast_source.c
)

project(btstack_test)