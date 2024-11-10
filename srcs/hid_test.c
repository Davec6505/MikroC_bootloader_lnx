/*
 * generic_hid.c
 *
 *  Created on: Apr 22, 2011
 *      Author: Jan Axelson
 *
 * Demonstrates communicating with a device designed for use with a generic HID-class USB device.
 * Sends and receives 2-byte reports.
 * Requires: an attached HID-class device that supports 2-byte
 * Input, Output, and Feature reports.
 * The device firmware should respond to a received report by sending a report.
 * Change VENDOR_ID and PRODUCT_ID to match your device's Vendor ID and Product ID.
 * See Lvr.com/winusb.htm for example device firmware.
 * This firmware is adapted from code provided by Xiaofan.
 * Note: libusb error codes are negative numbers.

The application uses the libusb 1.0 API from libusb.org.
Compile the application with the -lusb-1.0 option.
Use the -I option if needed to specify the path to the libusb.h header file. For example:
-I/usr/local/angstrom/arm/arm-angstrom-linux-gnueabi/usr/include/libusb-1.0

 */

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <libusb-1.0/libusb.h>
#include <assert.h>

#include <linux/types.h>
#include <linux/input.h>
#include <linux/hidraw.h>

// Values for bmRequestType in the Setup transaction's Data packet.
#include "Types.h"

// Local scope preprocessor defines

/*
 * uncoment to report hex file stripping and buffer conditioning..
 */
#define DEBUG -1

#define MAX_INTERRUPT_IN_TRANSFER_SIZE 64
#define MAX_INTERRUPT_OUT_TRANSFER_SIZE 64

static const int CONTROL_REQUEST_TYPE_IN = LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE;
static const int CONTROL_REQUEST_TYPE_OUT = LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE;

// From the HID spec:

static const int HID_GET_REPORT = 0x01;
static const int HID_SET_REPORT = 0x09;
static const int HID_REPORT_TYPE_INPUT = 0x01;
static const int HID_REPORT_TYPE_OUTPUT = 0x02;
static const int HID_REPORT_TYPE_FEATURE = 0x03;

// With firmware support, transfers can be > the endpoint's max packet size.

static const int MAX_CONTROL_IN_TRANSFER_SIZE = 64;
static const int MAX_CONTROL_OUT_TRANSFER_SIZE = 64;

static const int INTERFACE_NUMBER = 0;
static const int TIMEOUT_MS = 5000;

static const uint32_t _PIC32Mn_STARTFLASH = 0x1D000000;

// memory to hold flash data
static uint8_t *flash_ptr;

// function prototypes usb handling
int exchange_feature_reports_via_control_transfers(libusb_device_handle *devh);
int exchange_input_and_output_reports_via_control_transfers(libusb_device_handle *devh);
int exchange_input_and_output_reports_via_interrupt_transfers(libusb_device_handle *devh);

// function prototypes file handling
static uint32_t locate_address_in_file(FILE *fp);
static uint8_t transform_char_bin(unsigned char c);
static uint8_t transform_2chars_1bin(uint8_t var[]);
static uint32_t transform_2words_long(uint16_t a, uint16_t b);

static int16_t get_data_array(FILE *fp, uint8_t *bytes);
static uint32_t file_byte_count(FILE *fp);

int main(void)
{
	// Change these as needed to match idVendor and idProduct in your device's device descriptor.
	libusb_device ***list_;
	static const int VENDOR_ID = 0x2dbc;
	static const int PRODUCT_ID = 0x0001;

	struct libusb_device_handle *devh = NULL;
	struct libusb_init_option *opts = {0};
	int device_ready = 0;
	int result = 0;

	result = libusb_init_context(NULL, NULL, 0);

	if (result >= 0)
	{

		devh = libusb_open_device_with_vid_pid(NULL, VENDOR_ID, PRODUCT_ID);
		fprintf(stderr, "devh:=  VID%x:PID%x\n", VENDOR_ID, PRODUCT_ID);

		if (devh != NULL)
		{
			// The HID has been detected.
			// Detach the hidusb driver from the HID to enable using libusb.

			libusb_detach_kernel_driver(devh, INTERFACE_NUMBER);
			{
				result = libusb_claim_interface(devh, INTERFACE_NUMBER);
				if (result >= 0)
				{
					device_ready = 1;
				}
				else
				{
					fprintf(stderr, "libusb_claim_interface error %d\n", result);
				}
			}
		}
		else
		{
			fprintf(stderr, "Unable to find the device.\n");
		}
	}
	else
	{
		fprintf(stderr, "Unable to initialize libusb.\n");
	}

	if (device_ready)
	{
		// Send and receive data.
		// exchange_input_and_output_reports_via_interrupt_transfers(devh);
		// exchange_input_and_output_reports_via_control_transfers(devh);
		// exchange_feature_reports_via_control_transfers(devh);
		setupChiptoBoot(devh);
		// Finished using the device.
		libusb_release_interface(devh, 0);
	}
	libusb_close(devh);
	libusb_exit(NULL);
	return 0;
}

// Uses control transfers to write a Feature report to the HID
// and receive a Feature report from the HID.
// Returns - zero on success, libusb error code on failure.

int exchange_feature_reports_via_control_transfers(libusb_device_handle *devh)
{
	int bytes_received;
	int bytes_sent;
	char data_in[MAX_CONTROL_IN_TRANSFER_SIZE];
	char data_out[MAX_CONTROL_OUT_TRANSFER_SIZE];
	int i = 0;
	int result = 0;

	// Store example data in the output buffer for sending.
	// This example uses binary data.

	for (i = 0; i < MAX_CONTROL_OUT_TRANSFER_SIZE; i++)
	{
		data_out[i] = 0x30 + i;
	}

	// Send data to the device.

	bytes_sent = libusb_control_transfer(
		devh,
		CONTROL_REQUEST_TYPE_OUT,
		HID_SET_REPORT,
		(HID_REPORT_TYPE_FEATURE << 8) | 0x00,
		INTERFACE_NUMBER,
		data_out,
		sizeof(data_out),
		TIMEOUT_MS);

	if (bytes_sent >= 0)
	{
		printf("Feature report data sent:\n");
		for (i = 0; i < bytes_sent; i++)
		{
			printf("%02x ", data_out[i]);
		}
		printf("\n");

		// Request data from the device.

		bytes_received = libusb_control_transfer(
			devh,
			CONTROL_REQUEST_TYPE_IN,
			HID_GET_REPORT,
			(HID_REPORT_TYPE_FEATURE << 8) | 0x00,
			INTERFACE_NUMBER,
			data_in,
			MAX_CONTROL_IN_TRANSFER_SIZE,
			TIMEOUT_MS);

		if (bytes_received >= 0)
		{
			printf("Feature report data received:\n");
			for (i = 0; i < bytes_received; i++)
			{
				printf("%02x ", data_in[i]);
			}
			printf("\n");
		}
		else
		{
			fprintf(stderr, "Error receiving Feature report %d\n", result);
			return result;
		}
	}
	else
	{
		fprintf(stderr, "Error sending Feature report %d\n", result);
		return result;
	}
	return 0;
}

// Uses control transfers to write an Output report to the HID
// and receive an Input report from the HID.
// Returns - zero on success, libusb error code on failure.

int exchange_input_and_output_reports_via_control_transfers(libusb_device_handle *devh)
{
	int bytes_received;
	int bytes_sent;
	char data_in[MAX_CONTROL_IN_TRANSFER_SIZE];
	char data_out[MAX_CONTROL_OUT_TRANSFER_SIZE];
	int i = 0;
	int result = 0;

	// Store example data in the output buffer for sending.
	// This example uses binary data.

	for (i = 0; i < MAX_CONTROL_OUT_TRANSFER_SIZE; i++)
	{
		data_out[i] = 0x40 + i;
	}

	// Send data to the device.

	bytes_sent = libusb_control_transfer(
		devh,
		CONTROL_REQUEST_TYPE_OUT,
		HID_SET_REPORT,
		(HID_REPORT_TYPE_OUTPUT << 8) | 0x00,
		INTERFACE_NUMBER,
		data_out,
		sizeof(data_out),
		TIMEOUT_MS);

	if (bytes_sent >= 0)
	{
		printf("Output report data sent:\n");
		for (i = 0; i < bytes_sent; i++)
		{
			printf("%02x ", data_out[i]);
		}
		printf("\n");

		// Request data from the device.

		bytes_received = libusb_control_transfer(
			devh,
			CONTROL_REQUEST_TYPE_IN,
			HID_GET_REPORT,
			(HID_REPORT_TYPE_INPUT << 8) | 0x00,
			INTERFACE_NUMBER,
			data_in,
			MAX_CONTROL_IN_TRANSFER_SIZE,
			TIMEOUT_MS);

		if (bytes_received >= 0)
		{
			printf("Input report data received:\n");
			for (i = 0; i < bytes_received; i++)
			{
				printf("%02x ", data_in[i]);
			}
			printf("\n");
		}
		else
		{
			fprintf(stderr, "Error receiving Input report %d\n", result);
			return result;
		}
	}
	else
	{
		fprintf(stderr, "Error sending Input report %d\n", result);
		return result;
	}
	return 0;
}

// Use interrupt transfers to to write data to the device and receive data from the device.
// Returns - zero on success, libusb error code on failure.

int exchange_input_and_output_reports_via_interrupt_transfers(libusb_device_handle *devh)
{
	// Assumes interrupt endpoint 2 IN and OUT:
	static const int INTERRUPT_IN_ENDPOINT = 0x81;
	static const int INTERRUPT_OUT_ENDPOINT = 0x01;

	// With firmware support, transfers can be > the endpoint's max packet size.
	static char data_in[MAX_INTERRUPT_IN_TRANSFER_SIZE];
	static char data_out[MAX_INTERRUPT_OUT_TRANSFER_SIZE];

	int bytes_transferred = 0;
	int i = 0;
	int result = 0;

	// Write data to the device.

	result = libusb_interrupt_transfer(
		devh,
		INTERRUPT_OUT_ENDPOINT,
		data_out,
		MAX_INTERRUPT_OUT_TRANSFER_SIZE,
		&bytes_transferred,
		TIMEOUT_MS);

	if (result >= 0)
	{
		printf("Data sent via interrupt transfer:\n");
		for (i = 0; i < bytes_transferred; i++)
		{
			printf("%02x ", data_out[i]);
		}
		printf("\n");

		// Read data from the device.

		result = libusb_interrupt_transfer(
			devh,
			INTERRUPT_IN_ENDPOINT,
			data_in,
			MAX_INTERRUPT_OUT_TRANSFER_SIZE,
			&bytes_transferred,
			TIMEOUT_MS);

		if (result >= 0)
		{
			if (bytes_transferred > 0)
			{
				printf("Data received via interrupt transfer:\n");
				for (i = 0; i < bytes_transferred; i++)
				{
					printf("%02x ", data_in[i]);
				}
				printf("\n");

				TBootInfo bootinfo_t = {0};

				bootInfo_buffer(&bootinfo_t, data_in);
			}
			else
			{
				fprintf(stderr, "No data received in interrupt transfer (%d)\n", result);
				return -1;
			}
		}
		else
		{
			fprintf(stderr, "Error receiving data via interrupt transfer %d\n", result);
			return result;
		}
	}
	else
	{
		fprintf(stderr, "Error sending data via interrupt transfer %d\n", result);
		return result;
	}
	return 0;
}

int boot_interrupt_transfers(libusb_device_handle *devh, char *data_in, char *data_out)
{
	// Assumes interrupt endpoint 2 IN and OUT:
	static const int INTERRUPT_IN_ENDPOINT = 0x81;
	static const int INTERRUPT_OUT_ENDPOINT = 0x01;

	// With firmware support, transfers can be > the endpoint's max packet size.

	int bytes_transferred;
	int i = 0;
	int result = 0;
	;

	// Write data to the device.

	result = libusb_interrupt_transfer(
		devh,
		INTERRUPT_OUT_ENDPOINT,
		data_out,
		MAX_INTERRUPT_OUT_TRANSFER_SIZE,
		&bytes_transferred,
		TIMEOUT_MS);

	if (result >= 0)
	{
		printf("Data sent via interrupt transfer:\n");
		for (i = 0; i < bytes_transferred; i++)
		{
			printf("%02x ", data_out[i] & 0xff);
		}
		printf("\n");

		// Read data from the device.

		result = libusb_interrupt_transfer(
			devh,
			INTERRUPT_IN_ENDPOINT,
			data_in,
			MAX_INTERRUPT_OUT_TRANSFER_SIZE,
			&bytes_transferred,
			TIMEOUT_MS);

		if (result >= 0)
		{
			if (bytes_transferred > 0)
			{
				printf("Data received via interrupt transfer:\n");
				for (i = 0; i < bytes_transferred; i++)
				{
					printf("%02x ", data_in[i] & 0xff);
				}
				printf("\n");
			}
			else
			{
				fprintf(stderr, "No data received in interrupt transfer (%d)\n", result);
				return -1;
			}
		}
		else
		{
			fprintf(stderr, "Error receiving data via interrupt transfer %d\n", result);
			return result;
		}
	}
	else
	{
		fprintf(stderr, "Error sending data via interrupt transfer %d\n", result);
		return result;
	}
	return 0;
}

/*
 * To get chip into bootloader mode to usb needs to interrupt transfer a sequence of packets
 * Packet A : send [STX][cmdSYNC]
 * Packet B : send [STX][cmdINFO]
 * Packet C : send [STX][cmdBOOT]
 * Packet D : send [STX][cmdSYNC]
 * Find the file to send
 */

void setupChiptoBoot(struct libusb_device_handle *devh)
{
	// utils
	static int8_t trigger = 0;
	int16_t result = 0;

	// flash size
	uint32_t size = 0;
	uint32_t _temp_flash_erase_ = 0;
	uint16_t _blocks_to_flash_ = 0, modulo = 0; //(uint32_t)(size / bootinfo_t.ulMcuSize.fValue);
	double _blocks_temp = 0.0, fractional = 0.0, integer = 0.0;
	TCmd tcmd_t = cmdINFO;
	TBootInfo bootinfo_t = {0};

	// file handling
	FILE *fp;
	char path[250] = {0}; // path to hex file

	// usb specific data
	static char data_in[MAX_INTERRUPT_IN_TRANSFER_SIZE];
	static char data_out[MAX_INTERRUPT_OUT_TRANSFER_SIZE];

	while (tcmd_t != cmdDONE)
	{
		switch (tcmd_t)
		{

		case cmdSYNC:
			data_out[0] = 0x0f;
			data_out[1] = (char)cmdSYNC;
			for (int i = 9; i < MAX_INTERRUPT_OUT_TRANSFER_SIZE; i++)
			{
				data_out[i] = 0x0;
			}
			break;
		case cmdINFO:
			data_out[0] = 0x0f;
			data_out[1] = (char)cmdINFO;
			for (int i = 2; i < MAX_INTERRUPT_OUT_TRANSFER_SIZE; i++)
			{
				data_out[i] = 0x0;
			}
			break;
		case cmdBOOT:
			bootInfo_buffer(&bootinfo_t, data_in);
			data_out[0] = 0x0f;
			data_out[1] = (char)cmdBOOT;
			for (int i = 2; i < MAX_INTERRUPT_OUT_TRANSFER_SIZE; i++)
			{
				data_out[i] = 0x0;
			}
			break;
		case cmdNON:
			printf("\n\tEnter the path of the hex file... ");
			fgets(path, sizeof(path), stdin);
			size_t len = strlen(path);

			if (len > 0 && path[len - 1] == '\n')
			{
				path[len - 1] = '\0'; // remove \n
			}

			printf("\n%s\n", path);
			fp = fopen(path, "r");

			if (fp == NULL)
			{
				fprintf(stderr, "Could not find or open a file!!\n");
			}
			else
			{
				size = locate_address_in_file(fp);
				if (size > 0)
					trigger = 1;
				else
					return; // no point in continuing if the file is empty
			}

			_blocks_temp = 10; //((float)bootinfo_t.ulMcuSize.fValue / (float)size);
			fractional = modf(_blocks_temp, &integer);
			_blocks_to_flash_ = (uint16_t)integer;

			if (fractional > 0.0)
				_blocks_to_flash_++;
			if (_blocks_to_flash_ == 0)
				_blocks_to_flash_ = 1;
			//_blocks_to_flash_ = swap_bytes(temp_byte_int,_blocks_to_flash_);
			printf("block size:= %u\n", _blocks_to_flash_);

			break;
		case cmdERASE: // 1d000000

			_temp_flash_erase_ = (_PIC32Mn_STARTFLASH + (uint32_t)((_blocks_to_flash_ * bootinfo_t.uiEraseBlock.fValue.intVal)) - 1);
			printf("%u\n", _temp_flash_erase_);
			data_out[0] = 0x0f;
			data_out[1] = (char)cmdERASE;
			memcpy(data_out + 2, &_temp_flash_erase_, sizeof(uint32_t));
			memcpy(data_out + 6, &_blocks_to_flash_, sizeof(int16_t));
			for (int i = 9; i < MAX_INTERRUPT_OUT_TRANSFER_SIZE; i++)
			{
				data_out[i] = 0x0;
			}
			break;
		case cmdWRITE:
			data_out[0] = 0x0f;
			data_out[1] = (char)cmdWRITE;
			memcpy(data_out + 2, &_PIC32Mn_STARTFLASH, sizeof(uint32_t));
			memcpy(data_out + 6, &_blocks_to_flash_, sizeof(int16_t));
			for (int i = 9; i < MAX_INTERRUPT_OUT_TRANSFER_SIZE; i++)
			{
				data_out[i] = 0x0;
			}
			break;
		case cmdHEX:
			break;
		case cmdREBOOT:

			data_out[0] = 0x0f;
			data_out[1] = (char)cmdREBOOT;
			for (int i = 2; i < MAX_INTERRUPT_OUT_TRANSFER_SIZE; i++)
			{
				data_out[i] = 0x0;
			}
			break;
		default:
			break;
		}

		// Send and receive data.
		if (tcmd_t != cmdNON)
		{
			if (boot_interrupt_transfers(devh, data_in, data_out))
			{
				fprintf(stderr, "Error whilst transfering data...");
				return;
			}
		}

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
			break;
		case cmdERASE:
			tcmd_t = /*cmdWRITE*/ cmdREBOOT;
			break;
		case cmdWRITE:
			tcmd_t = cmdHEX;
			break;
		case cmdREBOOT:
			tcmd_t = cmdNON;
			break;
		default:
			break;
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
 * Utils
 */

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

/// @brief
/// @param FILE pointer t
/// @return
static uint32_t locate_address_in_file(FILE *fp)
{
	// function vars
	uint8_t _have_data_ = 0;
	uint16_t i = 0, j = 0;
	uint16_t k = 0;
	uint32_t count = 0;
	uint32_t size = 0;
	int c_ = 0;
	unsigned char c = '\0';

	// temp buffers
	uint8_t temp_[3] = {0};
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
		flash_ptr = (uint8_t *)malloc(sizeof(uint8_t) * size);
	}

	// start at the begining of the file
	fseek(fp, 0, SEEK_SET);
	_have_data_ = 0;
	count = 0;

	// run until end of file unless told otherwise.
	// The purpose is to strip out data bytes after the
	// address has been found.
	while (c_ != EOF)
	{
		i = j = 0;
		// ¬k = 0;
		while (c_ = fgetc(fp))
		{
			// sanity checks
			if (c_ == EOF)
			{
				break;
			}

			c = (unsigned char)c_;

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
				line[i] = transform_2chars_1bin(temp_);
				j = 0;
#if DEBUG == 1
				printf("[%02x] ", line[i]);
#endif
				i++;
			}
		}

		// extract byte count and address and report type
		memcpy((uint8_t *)&hex, &line, sizeof(_HEX_));
		// hex file report 01 is end of file EXIT loop
		if (hex.report == 0x01)
		{
			printf("End of hex!\n");
			break;
		}

		if (hex.report == 0x02 | hex.report == 0x04)
		{
			hex.add_lsw = swap_wordbytes(hex.add_lsw);
			hex.add_msw = swap_wordbytes(hex.add_msw);
			uint32_t address = transform_2words_long(hex.add_msw, hex.add_lsw);

			if (address == _PIC32Mn_STARTFLASH)
			{
				_have_data_ = 1;
			}

#if DEBUG == 1
			printf("[%02x][%04x][%02x][%04x] = [%08x] ", hex.data_quant, hex.add_lsw, hex.report, hex.add_msw, address);
#endif
		}
		else if (hex.report == 00 & _have_data_)
		{
			// memcpy(flash_buffer, line + 4, hex.data_quant);
			//  data resides in this row start to add to data
			for (k = 0; k < hex.data_quant; k++)
			{
#if DEBUG == 1
				*(flash_ptr) = line[k + 4];
				printf("[%02x]", *(flash_ptr++));
#else
				*(flash_ptr++) = line[k + 4];
#endif
				count++;
			}
		}
#if DEBUG == 1
		printf("\n");
#endif

		if (count > size)
			break;
	}

	printf("Buffer count:= %u | file length = %u\n", count, size);

	return count;
}

static uint32_t file_byte_count(FILE *fp)
{
	uint32_t size = 0;

	fseek(fp, 0, SEEK_SET);

	for (size = 0; getc(fp) != EOF; size++)
		;

	return size;
}

static uint8_t transform_char_bin(unsigned char c)
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

static uint8_t transform_2chars_1bin(uint8_t var[])
{
	uint8_t temp = 0;
	temp = (var[0] & 0xf) << 4;
	temp |= (var[1] & 0xf);

	return temp;
}

static uint32_t transform_2words_long(uint16_t a, uint16_t b)
{
	uint16_t temp16 = a;
	uint32_t temp32 = (a & 0xffff) << 16;
	return temp32 |= b;
}
