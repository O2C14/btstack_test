cmake_minimum_required(VERSION 3.15)
set(PROJECT_NAME aac_encoder)
project(aac_encoder)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=rv32imafcpzpsfoperand_xtheade -mabi=ilp32f -mtune=e907")

include_directories(
./aac/libAACdec/include
./aac/libAACdec/src  
./aac/libAACenc/include
./aac/libAACenc/src
./aac/libArithCoding/include
./aac/libArithCoding/src
./aac/libDRCdec/include
./aac/libDRCdec/src
./aac/libFDK/include
./aac/libFDK/include/x86
./aac/libFDK/src
./aac/libMpegTPDec/include
./aac/libMpegTPDec/src
./aac/libMpegTPEnc/include
./aac/libMpegTPEnc/src
./aac/libPCMutils/include
./aac/libPCMutils/src
./aac/libSACdec/include
./aac/libSACdec/src
./aac/libSACenc/include
./aac/libSACenc/src
./aac/libSBRdec/include
./aac/libSBRdec/src
./aac/libSBRenc/include
./aac/libSBRenc/src
./aac/libSYS/include
./aac/libSYS/src
)


file(GLOB_RECURSE AACdec_src "./aac/libAACdec/src/*.cpp" "./aac/libAACdec/src/*.c")
file(GLOB_RECURSE REMOVE_SOURCES "./aac/libAACdec/src/arm/*.cpp" "./aac/libAACdec/src/mips/*.cpp")
list(REMOVE_ITEM AACdec_src ${REMOVE_SOURCES})
add_library(
  AACdec STATIC
  ${AACdec_src}
)
file(GLOB_RECURSE AACenc_src "./aac/libAACenc/src/*.cpp" "./aac/libAACenc/src/*.c")

add_library(
  AACenc STATIC
  ${AACenc_src}
)
file(GLOB_RECURSE ArithCoding_src "./aac/libArithCoding/src/*.cpp" "./aac/libArithCoding/src/*.c")

add_library(
  ArithCoding STATIC
  ${ArithCoding_src}
)
file(GLOB_RECURSE DRCdec_src "./aac/libDRCdec/src/*.cpp" "./aac/libDRCdec/src/*.c")

add_library(
  DRCdec STATIC
  ${DRCdec_src}
)
file(GLOB_RECURSE FDK_src "./aac/libFDK/src/*.cpp" "./aac/libFDK/src/*.c" 
)
file(GLOB_RECURSE REMOVE_SOURCES "./aac/libFDK/src/arm/*.cpp" "./aac/libFDK/src/mips/*.cpp")
list(REMOVE_ITEM FDK_src ${REMOVE_SOURCES})
add_library(
  FDK STATIC
  ${FDK_src}
)
file(GLOB_RECURSE MpegTPDec_src "./aac/libMpegTPDec/src/*.cpp" "./aac/libMpegTPDec/src/*.c")
add_library(
  MpegTPDec STATIC
  ${MpegTPDec_src}
)
file(GLOB_RECURSE MpegTPEnc_src "./aac/libMpegTPEnc/src/*.cpp" "./aac/libMpegTPEnc/src/*.c")
add_library(
  MpegTPEnc STATIC
  ${MpegTPEnc_src}
)
file(GLOB_RECURSE PCMutils_src "./aac/libPCMutils/src/*.cpp" "./aac/libPCMutils/src/*.c")
add_library(
  PCMutils STATIC
  ${PCMutils_src}
)
file(GLOB_RECURSE SACdec_src "./aac/libSACdec/src/*.cpp" "./aac/libSACdec/src/*.c")
add_library(
  SACdec STATIC
  ${SACdec_src}
)
file(GLOB_RECURSE SACenc_src "./aac/libSACenc/src/*.cpp" "./aac/libSACenc/src/*.c")
add_library(
  SACenc STATIC
  ${SACenc_src}
)
file(GLOB_RECURSE SBRdec_src "./aac/libSBRdec/src/*.cpp" "./aac/libSBRdec/src/*.c")
add_library(
  SBRdec STATIC
  ${SBRdec_src}
)
file(GLOB_RECURSE SBRenc_src "./aac/libSBRenc/src/*.cpp" "./aac/libSBRenc/src/*.c")
add_library(
  SBRenc STATIC
  ${SBRenc_src}
)
file(GLOB_RECURSE SYS_src "./aac/libSYS/src/*.cpp" "./aac/libSYS/src/*.c")
add_library(
  SYS STATIC
  ${SYS_src}
)
add_library(
  aac_enc STATIC
  ${AACenc_src}
  ${MpegTPEnc_src}
  ${SACenc_src}
  ${SBRenc_src}
)
add_library(
  aac_dec STATIC
  ${AACdec_src}
  ${DRCdec_src}
  ${MpegTPDec_src}
  ${SACdec_src}
  ${SBRdec_src}

)
add_library(
  aac_other STATIC
  ${ArithCoding_src}
  ${FDK_src}
  ${PCMutils_src}
  ${SYS_src}
)
#target_compile_options(libAACdec PUBLIC -march=rv32imafc -mabi=ilp32f)