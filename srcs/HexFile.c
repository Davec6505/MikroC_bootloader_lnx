
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "HexFile.h"
#include "Types.h"
#include "Utils.h"

// 1 = file size | 2 = address info | 3 = supply the path other than argument
#define DEBUG 0

// boot loader 1st line
const uint8_t boot_line[][16] = {{0x1F, 0xBD, 0x1E, 0x3C, 0x00, 0x40, 0xDE, 0x37, 0x08, 0x00, 0xC0, 0x03, 0x00, 0x00, 0x00, 0x70},
                                 {0x0F, 0xBD, 0x1E, 0x3C, 0x00, 0x40, 0xDE, 0x37, 0x08, 0x00, 0xC0, 0x03, 0x00, 0x00, 0x00, 0x70}};

const uint32_t _PIC32Mn_STARTFLASH = 0x1D000000;
const uint32_t _PIC32Mn_STARTCONF = 0x1FC00000;
const uint32_t vector[] = {_PIC32Mn_STARTFLASH, _PIC32Mn_STARTFLASH, _PIC32Mn_STARTCONF};
int vector_index = 0;

// memory to hold flash data
uint8_t *flash_ptr = NULL;
uint8_t *flash_ptr_conf = 0;
uint8_t *flash_ptr_start = NULL;
uint32_t mem_created = 0;
uint32_t bootaddress_space = 0;

void overwrite_bootflash_program(void);
/*
 * To get chip into bootloader mode to usb needs to interrupt transfer a sequence of packets
 * Packet A : send [STX][cmdSYNC]
 * Packet B : send [STX][cmdINFO]
 * Packet C : send [STX][cmdBOOT]
 * Packet D : send [STX][cmdSYNC]
 * Find the file to send
 */

/***************************************************
 * Open the hex file extract each line and iterate
 * over the data from each line, the data is ASCII,
 * conver each byte to its binary equivilant,
 * Get the address MSW and LSW then use the address
 * to place the data bytes at the index in the
 * ram buffer, this way the file is only iterated
 * through once.
 * 2 buffers are used
 *  1) program data,
 *  2) configuration data
 ***************************************************/
void condition_hexfile_data(char *path)
{
    if (fp == NULL)
    {
        fp = fopen(path, "r");
        if (fp == NULL)
        {
            fprintf(stderr, "Could not find or open a file!!\n");
            return;
        }
    }
    // make sure file starts from begining
    fseek(fp, 0, SEEK_SET);
}

void setupChiptoBoot(struct libusb_device_handle *devh, char *path)
{

    // utils
    static int8_t trigger = 0;
    int16_t result = 0;
    uint8_t _out_only = 0;
    // flash size
    uint32_t size = 0;
    uint32_t _temp_flash_erase_ = 0;
    uint32_t _boot_flash_start = 0;

    uint16_t _blocks_to_flash_ = 0, modulo = 0; //(uint32_t)(size / bootinfo_t.ulMcuSize.fValue);
    double _blocks_temp = 0.0, fractional = 0.0, integer = 0.0;
    TCmd tcmd_t = cmdINFO;
    TBootInfo bootinfo_t = {0};

    // hex loading
    // uint16_t hex_load_percent = 0;
    uint16_t hex_load_limit = 0;
    uint16_t hex_load_tracking = 0;
    uint16_t hex_load_modulo = 0;

    // file handling
    FILE *fp = NULL;

    // usb specific data
    static char data_in[MAX_INTERRUPT_IN_TRANSFER_SIZE];
    static char data_out[MAX_INTERRUPT_OUT_TRANSFER_SIZE];

    while (tcmd_t != cmdDONE)
    {
        /*
         * main state mc to handle the sequence need by
         * MikroC bootloader firmware, I believe this conforms
         * closely to the UHB standard.
         */
        {
            switch (tcmd_t)
            {

            case cmdSYNC:
            {
                _out_only = 0;
                data_out[0] = 0x0f;
                data_out[1] = (char)cmdSYNC;
                for (int i = 9; i < MAX_INTERRUPT_OUT_TRANSFER_SIZE; i++)
                {
                    data_out[i] = 0x0;
                }
            }
            break;
            case cmdINFO:
            {
                _out_only = 0;
                data_out[0] = 0x0f;
                data_out[1] = (char)cmdINFO;
                for (int i = 2; i < MAX_INTERRUPT_OUT_TRANSFER_SIZE; i++)
                {
                    data_out[i] = 0x0;
                }
            }
            break;
            case cmdBOOT:
            {
                _out_only = 0;
                bootInfo_buffer(&bootinfo_t, data_in);
                data_out[0] = 0x0f;
                data_out[1] = (char)cmdBOOT;
                for (int i = 2; i < MAX_INTERRUPT_OUT_TRANSFER_SIZE; i++)
                {
                    data_out[i] = 0x0;
                }
                // start at address space 1d00
                mem_created = vector_index = 0;
            }
            break;
            case cmdNON:
            {
                // expect a data response back from device
                _out_only = 0;

                if (fp == NULL)
                {
                    fp = fopen(path, "r");
                    if (fp == NULL)
                    {
                        fprintf(stderr, "Could not find or open a file!!\n");
                        return;
                    }
                }
                // make sure file starts from begining
                fseek(fp, 0, SEEK_SET);

                // handle address space from vector array, 1st 1d00 then 1fc0
                if (vector_index == 1)
                {
                    printf("%08x\n", vector[vector_index + 1]);
                    size = locate_address_in_file(fp, vector_index + 1);
                    flash_ptr = flash_ptr_start;
                    overwrite_bootflash_program();
                    flash_ptr = flash_ptr_start;
                    size = bootinfo_t.uiEraseBlock.fValue.intVal; // 0x4000

                    //  Work out the boot start vector for a sanity check, MikroC bootloader uses program flash
                    //  depending on the mcu ie. pic32mz1024efh 0x100000 size
                    _boot_flash_start = bootinfo_t.ulBootStart.fValue & V2P;
                    _boot_flash_start -= bootinfo_t.uiEraseBlock.fValue.intVal;

                    // erase a whole page 0x4000 for configuration vector
                    hex_load_limit = bootinfo_t.uiEraseBlock.fValue.intVal / MAX_INTERRUPT_OUT_TRANSFER_SIZE;

                    _temp_flash_erase_ = (_boot_flash_start); //+ (uint32_t)(1 * bootinfo_t.uiEraseBlock.fValue.intVal)) - 1;
                    printf("%08x : %08x\n", _boot_flash_start, _temp_flash_erase_);
                    _blocks_to_flash_ = 1;

                    bootaddress_space = _boot_flash_start;
                }
                else if (vector_index == 2)
                {
                    size = locate_address_in_file(fp, vector_index);
                    flash_ptr = flash_ptr_start;
                    if (bootinfo_t.ulMcuSize.fValue == MZ2048)
                        memcpy(flash_ptr, boot_line[0], sizeof(boot_line[0]));
                    else
                        memcpy(flash_ptr, boot_line[1], sizeof(boot_line[0]));

                    hex_load_limit = 2048 / MAX_INTERRUPT_OUT_TRANSFER_SIZE;
                    _temp_flash_erase_ = (vector[vector_index]); // + (uint32_t)(1 * bootinfo_t.uiEraseBlock.fValue.intVal)) - 1;
                    _blocks_to_flash_ = 1;

                    bootaddress_space = vector[vector_index];
                }
                else
                {
                    size = locate_address_in_file(fp, vector_index);
                    flash_ptr = flash_ptr_start;
                    hex_load_limit = size / MAX_INTERRUPT_OUT_TRANSFER_SIZE;
                    // erasing preperation
                    _blocks_temp = (float)size / (float)bootinfo_t.uiEraseBlock.fValue.intVal;
                    fractional = modf(_blocks_temp, &integer);
                    _blocks_to_flash_ = (uint16_t)integer;

                    if (fractional > 0.0)
                    {
                        _blocks_to_flash_++;
                    }
                    if (_blocks_to_flash_ == 0)
                    {
                        // erase at least 1 page if there are zero blocks to flash.
                        _blocks_to_flash_ = 1;
                    }

                    bootaddress_space = vector[vector_index];

                    _temp_flash_erase_ = (vector[vector_index]); // +
                                                                 // (uint32_t)((_blocks_to_flash_ * bootinfo_t.uiEraseBlock.fValue.intVal)) - 1);
                }

                printf("trnsfer size:= %d\n", size);
                if (size > 0)
                {
                    trigger = 1;
                    // reset flash pointer to start
                    flash_ptr = flash_ptr_start;
                }
                else
                {
                    // no point in continuing if the file is empty
                    exit(EXIT_FAILURE);
                }

#if DEBUG == 3
                printf("vector indexed at [%02x]\n", vector_index);
#endif

                printf("bootaddress_space [%08x]\tflash erase start [%08x]\tblock to flash [%04x]\n", bootaddress_space, _temp_flash_erase_, _blocks_to_flash_);
            }
            break;
            case cmdERASE:
            {
                // expect a data response back from device
                _out_only = 0;
                // bootloader needs startaddress "page boundry" and quantity of pages to to erase
                // erase for MikroC starts high and subracts from quantity after each page has
                // been erased and quantity == 0
                data_out[0] = 0x0f;
                data_out[1] = (char)cmdERASE;
                memcpy(data_out + 2, &_temp_flash_erase_, sizeof(uint32_t));
                memcpy(data_out + 6, &_blocks_to_flash_, sizeof(int16_t));
                for (int i = 9; i < MAX_INTERRUPT_OUT_TRANSFER_SIZE; i++)
                {
                    data_out[i] = 0x0;
                }
            }
            break;
            case cmdWRITE:
            {
                // expect no data back continously stream data.
                _out_only = 1;

                if (vector_index == 2)
                    size = 2048;
                else if (vector_index == 1)
                    size = 0x4000;

                hex_load_tracking = 0;
                data_out[0] = 0x0f;
                data_out[1] = (char)cmdWRITE;
                memcpy(data_out + 2, &bootaddress_space, sizeof(uint32_t));
                memcpy(data_out + 6, &size, sizeof(int16_t));
                for (int i = 9; i < MAX_INTERRUPT_OUT_TRANSFER_SIZE; i++)
                {
                    data_out[i] = 0x0;
                }

                // reset the pointer position
                flash_ptr = flash_ptr_start;
            }
            break;
            case cmdHEX:
            {
                // expect no data back continously stream data.
                _out_only = 1;

                hex_load_tracking++;
                // use the flash buffer to stream 64 byte slices at a time

                if (hex_load_tracking > hex_load_limit)
                {
                    tcmd_t = cmdREBOOT;
                    _out_only = 0;
                }

                load_hex_buffer(data_out, MAX_INTERRUPT_OUT_TRANSFER_SIZE);
            }
            break;
            case cmdREBOOT:
            {
                _out_only = 2;
                printf("\n");
                // free memory created for flas_pointer

                /*
                 * re-boot command will cause the app to exit due to timeout from
                 * usb response, may want to set _out_only to 1 to sto exception.
                 * extra handling of usb may be needed if _out_only set to 1.
                 */

                vector_index++;
                if (vector_index > 2)
                {
                    if (flash_ptr != NULL)
                    {
                        flash_ptr = flash_ptr_start;
                        // free(flash_ptr);
                        // flash_ptr = flash_ptr_start = NULL;
                    }
                    data_out[0] = 0x0f;
                    data_out[1] = (char)cmdREBOOT;
                    for (int i = 2; i < MAX_INTERRUPT_OUT_TRANSFER_SIZE; i++)
                    {
                        flash_ptr = flash_ptr_start;
                        data_out[i] = 0x0;
                    }
                }
                else
                {
                    // start back at data prep for config vector
                    tcmd_t = cmdNON;
                }
            }
            break;
            default:
                break;
            }
        }
        // Sendin the data via usb
        if (tcmd_t != cmdNON && !(tcmd_t == cmdREBOOT && _out_only == 1))
        {
            if (boot_interrupt_transfers(devh, data_in, data_out, _out_only))
            {
                fprintf(stderr, "Transfered data complete...\n");
                exit(EXIT_FAILURE);
            }
        }

        /*
         * extra state machine to help sequencer? this may be taken away in
         * for refinement.
         */
        {
            switch (tcmd_t)
            {
            case cmdINFO:
                tcmd_t = cmdBOOT;
                break;
            case cmdBOOT:
                tcmd_t = cmdNON;
                break;
            case cmdNON:
                if (trigger == 1)
                {
                    if (vector_index == 0)
                        tcmd_t = cmdSYNC;
                    else
                        tcmd_t = cmdERASE;
                    trigger = 0;
                }
                break;
            case cmdSYNC:
                tcmd_t = cmdERASE;
                printf("Erase\n");
                break;
            case cmdERASE:
                tcmd_t = cmdWRITE /*cmdREBOOT*/;
                printf("Write\n");
                break;
            case cmdWRITE:
                tcmd_t = cmdHEX;
                printf("HEX\n");
                break;
            case cmdHEX:
                break;
            case cmdREBOOT:
                break;
            default:
                break;
            }
        }
    }
}

/*Display the boot info need for erase and write data*/
void bootInfo_buffer(void *boot_info, const void *buffer)
{
    TBootInfo *bootinfo_t = boot_info;
    uint8_t *data;
    bootinfo_t->bSize = *((uint8_t *)buffer + 0);
    memcpy(((uint8_t *)bootinfo_t + 1), ((uint8_t *)buffer + 1), sizeof(TCharField));
    memcpy(((uint8_t *)bootinfo_t + 3), ((uint8_t *)buffer + 4), sizeof(TULongField));
    memcpy(((uint8_t *)bootinfo_t + 11), ((uint8_t *)buffer + 12), sizeof(TUIntField));
    memcpy(((uint8_t *)bootinfo_t + 15), ((uint8_t *)buffer + 16), sizeof(TULongField));
    memcpy(((uint8_t *)bootinfo_t + 19), ((uint8_t *)buffer + 20), sizeof(TULongField));
    memcpy(((uint8_t *)bootinfo_t + 23), ((uint8_t *)buffer + 24), sizeof(TULongField));
    memcpy(((uint8_t *)bootinfo_t + 31), ((uint8_t *)buffer + 32), sizeof(TStringField));

    printf("\n%02x\n%02x\t%02x\n%02x\t%08x\n%02x\t%04x\n%02x\t%04x\n%02x\t%04x\n%02x\t%08x\n%02x\t%s\n\n", bootinfo_t->bSize, bootinfo_t->bMcuType.fFieldType, bootinfo_t->bMcuType.fValue, bootinfo_t->ulMcuSize.fFieldType, bootinfo_t->ulMcuSize.fValue, bootinfo_t->uiEraseBlock.fFieldType, bootinfo_t->uiEraseBlock.fValue.intVal, bootinfo_t->uiWriteBlock.fFieldType, bootinfo_t->uiWriteBlock.fValue.intVal, bootinfo_t->uiBootRev.fFieldType, bootinfo_t->uiBootRev.fValue.intVal, bootinfo_t->ulBootStart.fFieldType, bootinfo_t->ulBootStart.fValue, bootinfo_t->sDevDsc.fFieldType, bootinfo_t->sDevDsc.fValue);
}

/*
 * @param uint32_t size
 *
 * Stream the data 64 byte slices using
 *
 * return none
 */
void load_hex_buffer(char *data, uint16_t iterable)
{
    uint32_t i = 0;
    for (i = 0; i < iterable; i++)
    {
        *(data + i) = *(flash_ptr++);
#if DEBUG == 3
        printf("%02x", *(data + i) & 0xff);
#endif
    }
#if DEBUG == 3
    printf("\n");
#endif
}

/*
 * @param FILE pointer t
 *
 * stips each line of the hex file into
 * [data count][address][report type][data / extended address][checksum]
 * add all data bytes to the flash buffer
 *
 * @return  count.of data bytes
 */
uint32_t locate_address_in_file(FILE *fp, int index)
{
    // function vars
    uint16_t i = 0, j = 0;

    uint16_t k = 0;
    uint16_t lsw_address_max = 0;
    uint16_t inc_lsw_addres = 0;
    uint16_t last_lsw_address = 0;
    uint16_t lsw_address_subtract_result = 0;
    uint16_t last_data_quantity = 0;
    uint16_t quantity_diff = 0;

    static uint32_t count = 0;
    static uint32_t address = 0;
    uint32_t size = 0;

    int c_ = 0;
    unsigned char c = '\0';

    // temp buffers
    uint8_t line[64] = {0};

    // temp struct of type hex descriptors
    _HEX_ hex = {0};

    // sanity check on file
    if (fp == NULL)
    {
        printf("No file found...\n");
        exit(EXIT_FAILURE);
    }

    // get file size to allocate memory
    size = file_byte_count(fp);
    if (size > 0)
    {
        // don't want to keep recreating memory, use existing
        if (mem_created == 0)
        {
            mem_created = 65535; //((sizeof(uint8_t) * size) + 10);
            flash_ptr = (uint8_t *)malloc(mem_created);
            flash_ptr_start = flash_ptr;

            if (flash_ptr != NULL)
            {
                memset(flash_ptr, 0xff, mem_created - 1);
            }

            if (flash_ptr_start == NULL)
            {
                fprintf(stderr, "Start address of internal pointer not set!");
            }
        }
        else
        {
            flash_ptr = flash_ptr_start;
#if DEBUG == 3
            printf("Memory already created!\n");
#endif
        }
    }

    // start at the begining of the file and reset all relevant vars
    fseek(fp, 0, SEEK_SET);

    count = 0;
    inc_lsw_addres = 0x00;

    printf("Vector [%08x]\n", vector[index]);

    // run until end of file unless told otherwise.
    // The purpose is to strip out data bytes after the
    // address has been found.
    while (c_ != EOF)
    {
        file_extract_line(fp, line, c_);
        // extract byte count and address and report type
        memcpy((uint8_t *)&hex, &line, sizeof(_HEX_));

        hex.report.add_lsw = swap_wordbytes(hex.report.add_lsw);

        if (hex.report.report == 0x02 | hex.report.report == 0x04)
        {
            hex.add_msw = swap_wordbytes(hex.add_msw);
            address = transform_2words_long(hex.add_msw, hex.report.add_lsw);
        }
        else if ((address == vector[index]) && (hex.report.report == 00))
        {

            if (hex.report.add_lsw > lsw_address_max)
            {
                lsw_address_max = hex.report.add_lsw;
            }

            if (hex.report.add_lsw != inc_lsw_addres)
            {
                continue;
            }

            /*
             * Last address subtracted from current address must equal
             * previous rows data count, if the result is not equal to
             * zero then pad the buffer with default flash values 0xff.
             */
            lsw_address_subtract_result = inc_lsw_addres - last_lsw_address;
            quantity_diff = lsw_address_subtract_result - last_data_quantity;
            if (quantity_diff > 0)
            {
                for (k = 0; k < quantity_diff; k++)
                {
                    *(flash_ptr++) = 0xff;
                    count++;
                }
            }
#if DEBUG == 1
            printf("\t[%04x] [%04x] [%04x] [%04x] \t[%04x]\n", hex.report.add_lsw, last_lsw_address, last_data_quantity, lsw_address_subtract_result, quantity_diff);
#endif
            //  data resides in this row start to add to data
            for (k = 0; k < hex.report.data_quant; k++)
            {
                *(flash_ptr++) = line[k + sizeof(_HEX_REPORT_)];
#if DEBUG == 1
                printf("%02x ", *flash_ptr);
#endif
                count++;
            }
#if DEBUG == 1
            printf("\n");
#endif
            // Save a copy of the data quantity and the last current
            // address for next comparison.
            last_data_quantity = hex.report.data_quant;
            last_lsw_address = inc_lsw_addres;
        }

        // hex file report 01 is end of file EXIT loop
        if (hex.report.report == 0x01)
        {
            // start at the begining of the file
            fseek(fp, 0, SEEK_SET);
            // printf("Starting over...\n");
            inc_lsw_addres += 2;
            if (inc_lsw_addres > lsw_address_max)
            {
                break;
            }
        }
    }
    printf("Buffer count:= %u | file length = %u\n", count, size);

    return count;
}

/*
 * Utils
 */

uint32_t file_byte_count(FILE *fp)
{
    uint32_t size = 0;

    fseek(fp, 0, SEEK_SET);

    for (size = 0; getc(fp) != EOF; size++)
        ;

    return size;
}

void file_extract_line(FILE *fp, char *buf, int fp_result)
{
    int i = 0, j = 0;
    char c;
    uint8_t temp_[3] = {0};

    while (fp_result = fgetc(fp))
    {

        c = (unsigned char)fp_result;

        // make sure we dont capture new line
        if (c == '\n')
        {
            break;
        }

        // start char of a new line in a hex file is always a ':'
        if (c == ':')
        {
            continue;
        }

        // extract each ascii char from hex file line and convert to bin data
        temp_[j++] = transform_char_bin(c);

        if (j > 1)
        {
            *(buf + i) = transform_2chars_1bin(temp_);
            j = 0;
#if DEBUG == 4
            printf("[%02x] ", buf[i]);
#endif
            i++;
        }
    }
}

void overwrite_bootflash_program(void)
{
    int i = 0;
    uint8_t line[16];
    memcpy(line, flash_ptr, 16);
    for (i = 0; i < (0x4000 - 16); i++)
    {
        *(flash_ptr++) = 0xff;
    }
    memcpy(flash_ptr, line, 16);
}
