#ifndef HEX_FILE_H
#define HEX_FILE_H

#include <stdio.h>
#include <stdint.h>
#include "USB.h"

#define V2P 0x1FFFFFFF

#define MZ1024 0x100000
#define MZ2048 0x200000

void bootInfo_buffer(void *boot_info, const void *buffer);
void setupChiptoBoot(struct libusb_device_handle *devh, char *path);

// function prototypes file handling
void load_hex_buffer(char *data, uint16_t iterable);
uint32_t file_byte_count(FILE *fp);
void file_extract_line(FILE *fp, char *buf, int fp_result);
int16_t get_data_array(FILE *fp, uint8_t *bytes);

#endif