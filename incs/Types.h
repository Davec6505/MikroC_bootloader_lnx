#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

// Supported MCU families/types.
enum TMcuType {mtPIC16 = 1, mtPIC18, mtPIC18FJ, mtPIC24, mtDSPIC = 10, mtPIC32 = 20};


// Int splitter
union TIntToChars{
  char bytes[2];
  int ints;
};
// Bootloader info field ID's.
enum TBootInfoField {bifMCUTYPE=1,  // MCU type/family.
                     bifMCUID,      // MCU ID number.
                     bifERASEBLOCK, // MCU flash erase block size.
                     bifWRITEBLOCK, // MCU flash write block size.
                     bifBOOTREV,    // Bootloader revision.
                     bifBOOTSTART,  // Bootloader start address.
                     bifDEVDSC,     // Device descriptor string.
                     bifMCUSIZE     // MCU flash size
                     };


// Suported commands.
typedef enum  {
  cmdDONE = -1,            // EXIT loop
  cmdNON=0,                // 'Idle'.
  cmdSYNC,                 // Synchronize with PC tool.
  cmdINFO,                 // Send bootloader info record.
  cmdBOOT,                 // Go to bootloader mode.
  cmdREBOOT,               // Restart MCU.
  cmdWRITE=11,             // Write to MCU flash.
  cmdERASE=21,             // Erase MCU flash.
  cmdHEX = 31
  }TCmd;            


typedef struct {
  uint8_t bLo;
  uint8_t bHi;
}TBigEndian;


// Byte field (1 byte).
typedef struct {
  uint8_t fFieldType;
  uint8_t fValue;
} TCharField;

// Int field (2 bytes).
typedef struct {
  uint8_t fFieldType;
  union {
    uint16_t intVal;
    TBigEndian bEndian;
  } fValue;
} TUIntField;

// Long field (4 bytes).
typedef struct {
  uint8_t fFieldType;
  uint32_t fValue;
} TULongField;

// String field (MAX_STRING_FIELD_LENGTH bytes).
#define MAX_STRING_FIELD_LENGTH 20
typedef struct {
  uint8_t fFieldType;
  uint8_t fValue[MAX_STRING_FIELD_LENGTH];
} TStringField;


// Bootloader info record (device specific information).
typedef struct {
  uint8_t bSize;
  TCharField   bMcuType;
  TULongField  ulMcuSize;
  TUIntField   uiEraseBlock;
  TUIntField   uiWriteBlock;
  TUIntField   uiBootRev;
  TULongField  ulBootStart;
  TStringField sDevDsc;
} __attribute__((packed))TBootInfo;

/*
* To get chip into bootloader mode to usb needs to interrupt transfer a sequence of packets
* Packet A : send [STX][cmdSYNC]
* Packet B : send [STX][cmdINFO] 
* Packet C : send [STX][cmdBOOT]
* Packet D : send [STX][cmdSYNC]
* Find the file to send
*/
void setupChiptoBoot(struct libusb_device_handle *devh);
void bootInfo_buffer(void *boot_info,const void *buffer);
int16_t swap_bytes(uint8_t *bytes,int16_t num);






#endif