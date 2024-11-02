#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>



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


void swap_byteofint_buffer(void *boot_info,const void *buffer);
void swap_bytes(uint8_t *bytes,int num);






#endif