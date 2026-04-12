sdk_generate_library(lhdcv5)
set(DEC5_SRC_BASE E:/codec/lhdcv5)
sdk_add_include_directories(
    "${DEC5_SRC_BASE}/lhdcv5/liblhdc-common/inc"
    "${DEC5_SRC_BASE}/lhdcv5/inc"
    "${DEC5_SRC_BASE}/lhdcv5/liblhdcv5dec/include"
    "${DEC5_SRC_BASE}/lhdcv5/liblhdcv5dec/inc"
    )
file(GLOB DEC5SRC 
"${DEC5_SRC_BASE}/lhdcv5/liblhdc-common/src/*.c"
"${DEC5_SRC_BASE}/lhdcv5/liblhdcv5dec/src/dec/lhdc_v5_dec/*.c" 
"${DEC5_SRC_BASE}/lhdcv5/liblhdcv5dec/src/dec/lhdcv5_util_dec.c" 
"${DEC5_SRC_BASE}/lhdcv5/liblhdcv5dec/src/lhdcv5BT_dec.c" 
)
sdk_library_add_sources(${DEC5SRC})