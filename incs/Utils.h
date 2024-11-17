#ifndef UTILS_H
#define UTILS_H

int16_t swap_bytes(uint8_t *bytes, int16_t num);
uint16_t swap_wordbytes(uint16_t wb);
uint8_t transform_char_bin(unsigned char c);
uint8_t transform_2chars_1bin(uint8_t var[]);
uint32_t transform_2words_long(uint16_t a, uint16_t b);

#endif
