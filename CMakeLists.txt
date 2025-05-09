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

include(btstack.cmake)

#选择一个例子
#Choose an example
set(EXAMPLE "gatt_streamer_server")

find_package(Python REQUIRED COMPONENTS Interpreter)
if (EXISTS ${BTSTACK_ROOT}/example/${EXAMPLE}.gatt)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${EXAMPLE}.h
    DEPENDS ${BTSTACK_ROOT}/example/${EXAMPLE}.gatt
    COMMAND ${Python_EXECUTABLE} ${BTSTACK_ROOT}/tool/compile_gatt.py ${BTSTACK_ROOT}/example/${EXAMPLE}.gatt ${CMAKE_CURRENT_BINARY_DIR}/${EXAMPLE}.h
    VERBATIM 
)
message(${Python_EXECUTABLE} ${BTSTACK_ROOT}/tool/compile_gatt.py ${BTSTACK_ROOT}/example/${EXAMPLE}.gatt ${CMAKE_CURRENT_BINARY_DIR}/${EXAMPLE}.h)
target_sources(app PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/${EXAMPLE}.h)
sdk_add_include_directories(${CMAKE_CURRENT_BINARY_DIR})
endif()

target_sources(app PRIVATE 
    btstack_port.c
    ${BTSTACK_ROOT}/example/${EXAMPLE}.c
)

project(btstack_test)