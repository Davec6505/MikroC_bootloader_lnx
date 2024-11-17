#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "Types.h"
#include "Utils.h"

int16_t swap_bytes(uint8_t *bytes, int16_t num)
{

    uint8_t byte_ = 0;
    union TIntToChars int2char_t;
    int2char_t.ints = num;

    bytes[0] = int2char_t.bytes[1];
    bytes[1] = int2char_t.bytes[0];
    return int2char_t.ints;
}

uint16_t swap_wordbytes(uint16_t w_b)
{
    WORD_BYTES wb;
    wb.a_word = w_b;
    uint8_t temp = wb.bytes[0];
    wb.bytes[0] = wb.bytes[1];
    wb.bytes[1] = temp;
    return wb.a_word;
}

uint8_t transform_char_bin(unsigned char c)
{

    if (c >= '0' & c <= '9')
    {
        return c - '0';
    }
    else if (c >= 'a' && c <= 'f')
    {
        return (c - 'a') + 10;
    }
    else if (c >= 'A' && c <= 'F')
    {
        return (c - 'A') + 10;
    }

    return 255;
}

uint8_t transform_2chars_1bin(uint8_t var[])
{
    uint8_t temp = 0;
    temp = (var[0] & 0xf) << 4;
    temp |= (var[1] & 0xf);

    return temp;
}

uint32_t transform_2words_long(uint16_t a, uint16_t b)
{
    uint16_t temp16 = a;
    uint32_t temp32 = (a & 0xffff) << 16;
    return temp32 |= b;
}