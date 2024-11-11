#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

// Bootloader physical start address equasion:
//  __FLASH_SIZE = 0x00200000;  & __BOOT_FLASH_SIZE = 0x00028000;   1d000000 + ((200000 - 9858)/4000)*4000 == 1d1f4000
// Boot flash size can be aquired after a compiation of Boot firmware in MikroC Pro
#define __BOOT_FLASH_SIZE 0x9858

// Supported MCU families/types.
enum TMcuType
{
  mtPIC16 = 1,
  mtPIC18,
  mtPIC18FJ,
  mtPIC24,
  mtDSPIC = 10,
  mtPIC32 = 20
};

// Int splitter
union TIntToChars
{
  char bytes[2];
  int ints;
};
// Bootloader info field ID's.
enum TBootInfoField
{
  bifMCUTYPE = 1, // MCU type/family.
  bifMCUID,       // MCU ID number.
  bifERASEBLOCK,  // MCU flash erase block size.
  bifWRITEBLOCK,  // MCU flash write block size.
  bifBOOTREV,     // Bootloader revision.
  bifBOOTSTART,   // Bootloader start address.
  bifDEVDSC,      // Device descriptor string.
  bifMCUSIZE      // MCU flash size
};

// Suported commands.
typedef enum
{
  cmdDONE = -1,  // EXIT loop
  cmdNON = 0,    // 'Idle'.
  cmdSYNC,       // Synchronize with PC tool.
  cmdINFO,       // Send bootloader info record.
  cmdBOOT,       // Go to bootloader mode.
  cmdREBOOT,     // Restart MCU.
  cmdWRITE = 11, // Write to MCU flash.
  cmdERASE = 21, // Erase MCU flash.
  cmdHEX = 31
} TCmd;

typedef struct
{
  uint8_t bLo;
  uint8_t bHi;
} TBigEndian;

// Byte field (1 byte).
typedef struct
{
  uint8_t fFieldType;
  uint8_t fValue;
} TCharField;

// Int field (2 bytes).
typedef struct
{
  uint8_t fFieldType;
  union
  {
    uint16_t intVal;
    TBigEndian bEndian;
  } fValue;
} TUIntField;

// Long field (4 bytes).
typedef struct
{
  uint8_t fFieldType;
  uint32_t fValue;
} TULongField;

// String field (MAX_STRING_FIELD_LENGTH bytes).
#define MAX_STRING_FIELD_LENGTH 20
typedef struct
{
  uint8_t fFieldType;
  uint8_t fValue[MAX_STRING_FIELD_LENGTH];
} TStringField;

// Bootloader info record (device specific information).
typedef struct
{
  uint8_t bSize;
  TCharField bMcuType;
  TULongField ulMcuSize;
  TUIntField uiEraseBlock;
  TUIntField uiWriteBlock;
  TUIntField uiBootRev;
  TULongField ulBootStart;
  TStringField sDevDsc;
} __attribute__((packed)) TBootInfo;

/*
 * Structure of hex file lines data
 * Start line: [0]  "dont care"
 * Byte count: [1]  "number of data bytes in the line"
 * Address     [2][3][4][5] "Address field for the extend segment address, we llok for 0000 here"
 * Record Type [6]  "This tells the record type,[00 data] [01 end] [02 / 04 address][05 start of linear address record 'Arm only'] "
 */

typedef union
{
  uint16_t a_word;
  uint8_t bytes[2];
} __attribute__((packed)) WORD_BYTES;

typedef struct
{
  uint8_t data_quant;
  uint16_t add_lsw;
  uint8_t report;
} __attribute__((packed)) _HEX_REPORT_;
typedef struct
{
  _HEX_REPORT_ report;
  uint16_t add_msw;
} __attribute__((packed)) _HEX_;

/*
 * To get chip into bootloader mode to usb needs to interrupt transfer a sequence of packets
 * Packet A : send [STX][cmdSYNC]
 * Packet B : send [STX][cmdINFO]
 * Packet C : send [STX][cmdBOOT]
 * Packet D : send [STX][cmdSYNC]
 * Find the file to send
 */

// chip communication handling
void bootInfo_buffer(void *boot_info, const void *buffer);
void load_hex_buffer(char *data, uint16_t iterable);
void setupChiptoBoot(struct libusb_device_handle *devh);

#endif