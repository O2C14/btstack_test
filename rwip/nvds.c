#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nvds.h"
uint8_t * g_local_private_key;
uint8_t nvds_get(uint8_t tag, nvds_tag_len_t *lengthPtr, uint8_t *buf)
{
    int v3; // a5

    if (tag == 0x82) {
        v3 = 0;
        if (*lengthPtr == 1)
            *buf = g_local_private_key != 0;
        return v3;
    }
    v3 = 1;
    if (tag != 0x80 || !g_local_private_key || *lengthPtr != 0x20)
        return v3;
    memcpy(buf,g_local_private_key, 0x20u);
    return 0;
}

uint8_t nvds_del(uint8_t tag)
{
    return 1;
}

uint8_t nvds_put(uint8_t tag, nvds_tag_len_t length, uint8_t *buf)
{
    return 1;
}