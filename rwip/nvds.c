#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nvds.h"

uint8_t co_bdaddr[6] = { 0 };
uint8_t nvds_get(uint8_t tag, nvds_tag_len_t *lengthPtr, uint8_t *buf)
{
    if (tag == 1) // bdaddr
    {
        if (*lengthPtr < 6) {
            return 1;
        }
        memcpy(buf, co_bdaddr, 6);
        return 0;
    }
    return 1;
}

uint8_t nvds_del(uint8_t tag)
{
    return 1;
}

uint8_t nvds_put(uint8_t tag, nvds_tag_len_t length, uint8_t *buf)
{
    return 1;
}