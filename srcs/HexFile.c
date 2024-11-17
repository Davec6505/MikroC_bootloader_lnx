
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "HexFile.h"
#include "Types.h"
#include "Utils.h"

#define DEBUG 0

const uint32_t _PIC32Mn_STARTFLASH = 0x1D000000;

// memory to hold flash data
uint8_t *flash_ptr = 0;
uint8_t *flash_ptr_start = 0;

/*
 * To get chip into bootloader mode to usb needs to interrupt transfer a sequence of packets
 * Packet A : send [STX][cmdSYNC]
 * Packet B : send [STX][cmdINFO]
 * Packet C : send [STX][cmdBOOT]
 * Packet D : send [STX][cmdSYNC]
 * Find the file to send
 */

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
    static uint16_t hex_load_percent = 0;
    static uint16_t hex_load_limit = 0;
    static uint16_t hex_load_tracking = 0;
    static uint16_t hex_load_modulo = 0;

    // file handling
    FILE *fp;

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
            }
            break;
            case cmdNON:
            {
                _out_only = 0; // expect a data response back from device
#if DEBUG == 3
                printf("\nEnter the path of the hex file... ");
                fgets(path, sizeof(path), stdin);
#endif
                size_t len = strlen(path);

                // remove the backslashes from path
                if (len > 0 && path[len - 1] == '\n')
                {
                    path[len - 1] = '\0'; // remove \n
                }

                // show the path sanity check
                printf("\n%s\n^z to stop.", path);
                fp = fopen(path, "r");

                if (fp == NULL)
                {
                    fprintf(stderr, "Could not find or open a file!!\n");
                }
                else
                {
                    size = locate_address_in_file(fp);
                    if (size > 0)
                    {
                        trigger = 1;
                        // reset flash pointer to start
                        flash_ptr = flash_ptr_start;
                    }
                    else
                    {
                        return; // no point in continuing if the file is empty
                    }
                }

                // hex loading preperation
                // usb interrupt transfer send 64 bytes at a time
                hex_load_limit = size / MAX_INTERRUPT_OUT_TRANSFER_SIZE;
                // hex_load_modulo = size % MAX_INTERRUPT_OUT_TRANSFER_SIZE;
                hex_load_percent = hex_load_limit / 100;
                printf("percent:= %u\n", hex_load_percent);

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
                    _blocks_to_flash_ = 1;
                }

                // work out the boot start vector for a sanity check, MikroC bootloader uses program flash
                _boot_flash_start = _PIC32Mn_STARTFLASH +
                                    (((bootinfo_t.ulMcuSize.fValue - __BOOT_FLASH_SIZE) /
                                      bootinfo_t.uiEraseBlock.fValue.intVal) *
                                     bootinfo_t.uiEraseBlock.fValue.intVal);
                _temp_flash_erase_ = (_PIC32Mn_STARTFLASH + (uint32_t)((_blocks_to_flash_ * bootinfo_t.uiEraseBlock.fValue.intVal)) - 1);

                // sanity check flash erase start address, sent from chip if result to large the bootloade firmware will
                // be erased and render the chip unusable, to recover a programer tool will be needed.
                if (_temp_flash_erase_ >= _boot_flash_start)
                {
                    fprintf(stderr, "block size:= %u\n", _blocks_to_flash_);
                }
                else
                {
                    printf("flash erase start [%08x]\tbootflash start := [%08x]\n", _temp_flash_erase_, _boot_flash_start);
                }
            }
            break;
            case cmdERASE:
            {
                _out_only = 0; // expect a data response back from device
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
                _out_only = 1;
                // hex_load_percent_last = 0;
                hex_load_tracking = 0;
                data_out[0] = 0x0f;
                data_out[1] = (char)cmdWRITE;
                memcpy(data_out + 2, &_PIC32Mn_STARTFLASH, sizeof(uint32_t));
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
                _out_only = 1; // expect no data back continously stream data.
#if DEBUG == 2
                printf("[%d][%d][%d]\n", hex_load_tracking, hex_load_modulo, iterate);
#endif
                // use the flash buffer to stream 64 byte slices at a time
                load_hex_buffer(data_out, MAX_INTERRUPT_OUT_TRANSFER_SIZE);

                hex_load_tracking++;

                if (hex_load_tracking > hex_load_limit)
                {
                    tcmd_t = cmdREBOOT;
                    _out_only = 0;
                }
            }
            break;
            case cmdREBOOT:
            {
                _out_only = 0;
                printf("\n");
                // free memory created for flas_pointer
                if (flash_ptr != NULL)
                {
                    flash_ptr = flash_ptr_start;
                    free(flash_ptr);
                    flash_ptr = flash_ptr_start = NULL;
                }
                /*
                 * re-boot command will cause the app to exit due to timeout from
                 * usb response, may want to set _out_only to 1 to sto exception.
                 * extra handling of usb may be needed if _out_only set to 1.
                 */

                data_out[0] = 0x0f;
                data_out[1] = (char)cmdREBOOT;
                for (int i = 2; i < MAX_INTERRUPT_OUT_TRANSFER_SIZE; i++)
                {
                    data_out[i] = 0x0;
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
                fprintf(stderr, "Error whilst transfering data...");
                return;
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
                    tcmd_t = cmdSYNC;
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
    }
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
uint32_t locate_address_in_file(FILE *fp)
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
        flash_ptr = (uint8_t *)malloc((sizeof(uint8_t) * size) + 10);
        // keep track of the starting point
        flash_ptr_start = flash_ptr;
        if (flash_ptr_start == NULL)
        {
            fprintf(stderr, "Start address of internal pointer not set!");
        }
    }

    // start at the begining of the file and reset all relevant vars
    fseek(fp, 0, SEEK_SET);

    count = 0;
    inc_lsw_addres = 0x00;

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
        else if ((address == _PIC32Mn_STARTFLASH) && (hex.report.report == 00))
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
                }
            }
#if DEBUG == 2
            printf("\t[%04x] [%04x] [%04x] [%04x] \t[%04x]\n", hex.report.add_lsw, last_lsw_address, last_data_quantity, lsw_address_subtract_result, quantity_diff);
#endif
            //  data resides in this row start to add to data
            for (k = 0; k < hex.report.data_quant; k++)
            {
                *(flash_ptr++) = line[k + sizeof(_HEX_REPORT_)];
                count++;
            }

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
#if DEBUG == 1
            printf("[%02x] ", line[i]);
#endif
            i++;
        }
    }
}
