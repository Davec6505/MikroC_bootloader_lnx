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
//#include </usr/include/libusb-1.0/libusb.h>
#include <libusb-1.0/libusb.h>
#include <assert.h>

#include <linux/types.h>
#include <linux/input.h>
#include <linux/hidraw.h>
///usr/include/libusb-1.0/libusb.h
// Values for bmRequestType in the Setup transaction's Data packet.
#include "Types.h"

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

#define MAX_INTERRUPT_IN_TRANSFER_SIZE 64
#define MAX_INTERRUPT_OUT_TRANSFER_SIZE 64



static const int INTERFACE_NUMBER = 0;
static const int TIMEOUT_MS = 5000;

static const uint32_t _PIC32Mn_STARTFLASH = 0x1D000000;

int exchange_feature_reports_via_control_transfers(libusb_device_handle *devh);
int exchange_input_and_output_reports_via_control_transfers(libusb_device_handle *devh);
int exchange_input_and_output_reports_via_interrupt_transfers(libusb_device_handle *devh);





int main(void)
{
	// Change these as needed to match idVendor and idProduct in your device's device descriptor.
	libusb_device ***list_;
	static const int VENDOR_ID = 0x2dbc;
	static const int PRODUCT_ID = 0x0001;

	struct libusb_device_handle *devh = NULL;
	struct libusb_init_option *opts  = {0};
	int device_ready = 0;
	int result = 0;

   result = libusb_init_context(NULL,NULL,0);

	if (result >= 0)
	{

		devh = libusb_open_device_with_vid_pid(NULL, VENDOR_ID, PRODUCT_ID);
		fprintf(stderr,"devh:=  VID%x:PID%x\n",VENDOR_ID,PRODUCT_ID);

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
		//exchange_input_and_output_reports_via_interrupt_transfers(devh);
		//exchange_input_and_output_reports_via_control_transfers(devh);
		//exchange_feature_reports_via_control_transfers(devh);
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
	char data_out[MAX_CONTROL_OUT_TRANSFER_SIZE];	int i = 0;
	int result = 0;

	// Store example data in the output buffer for sending.
	// This example uses binary data.

	for (i=0;i < MAX_CONTROL_OUT_TRANSFER_SIZE; i++)
	{
		data_out[i]=0x30+i;
	}

	// Send data to the device.

	bytes_sent = libusb_control_transfer(
			devh,
			CONTROL_REQUEST_TYPE_OUT ,
			HID_SET_REPORT,
			(HID_REPORT_TYPE_FEATURE<<8)|0x00,
			INTERFACE_NUMBER,
			data_out,
			sizeof(data_out),
			TIMEOUT_MS);

	if (bytes_sent >= 0)
	{
	 	printf("Feature report data sent:\n");
	 	for(i = 0; i < bytes_sent; i++)
	 	{
	 		printf("%02x ",data_out[i]);
	 	}
	 	printf("\n");

		// Request data from the device.

		bytes_received = libusb_control_transfer(
				devh,
				CONTROL_REQUEST_TYPE_IN ,
				HID_GET_REPORT,
				(HID_REPORT_TYPE_FEATURE<<8)|0x00,
				INTERFACE_NUMBER,
				data_in,
				MAX_CONTROL_IN_TRANSFER_SIZE,
				TIMEOUT_MS);

		if (bytes_received >= 0)
		{
		 	printf("Feature report data received:\n");
		 	for(i = 0; i < bytes_received; i++)
		 	{
		 		printf("%02x ",data_in[i]);
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
	char data_out[MAX_CONTROL_OUT_TRANSFER_SIZE];	int i = 0;
	int result = 0;

	// Store example data in the output buffer for sending.
	// This example uses binary data.

	for (i=0;i < MAX_CONTROL_OUT_TRANSFER_SIZE; i++)
	{
		data_out[i]=0x40+i;
	}

	// Send data to the device.

	bytes_sent = libusb_control_transfer(
			devh,
			CONTROL_REQUEST_TYPE_OUT ,
			HID_SET_REPORT,
			(HID_REPORT_TYPE_OUTPUT<<8)|0x00,
			INTERFACE_NUMBER,
			data_out,
			sizeof(data_out),
			TIMEOUT_MS);

	if (bytes_sent >= 0)
	{
	 	printf("Output report data sent:\n");
	 	for(i = 0; i < bytes_sent; i++)
	 	{
	 		printf("%02x ",data_out[i]);
	 	}
	 	printf("\n");

		// Request data from the device.

		bytes_received = libusb_control_transfer(
				devh,
				CONTROL_REQUEST_TYPE_IN ,
				HID_GET_REPORT,
				(HID_REPORT_TYPE_INPUT<<8)|0x00,
				INTERFACE_NUMBER,
				data_in,
				MAX_CONTROL_IN_TRANSFER_SIZE,
				TIMEOUT_MS);

		if (bytes_received >= 0)
		{
		 	printf("Input report data received:\n");
		 	for(i = 0; i < bytes_received; i++)
		 	{
		 		printf("%02x ",data_in[i]);
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


	int bytes_transferred=0;
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
	  	for(i = 0; i < bytes_transferred; i++)
	  	{
	  		printf("%02x ",data_out[i]);
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
			  	for(i = 0; i < bytes_transferred; i++)
			  	{
			  		printf("%02x ",data_in[i]);
			  	}
			  	printf("\n");

				TBootInfo bootinfo_t = {0};

		        bootInfo_buffer(&bootinfo_t,data_in);	


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



int boot_interrupt_transfers(libusb_device_handle *devh,char *data_in,char *data_out)
{
    // Assumes interrupt endpoint 2 IN and OUT:
    static const int INTERRUPT_IN_ENDPOINT = 0x81;
    static const int INTERRUPT_OUT_ENDPOINT = 0x01;

	// With firmware support, transfers can be > the endpoint's max packet size.

	int bytes_transferred;
	int i = 0;
	int result = 0;;


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
	  	for(i = 0; i < bytes_transferred; i++)
	  	{
	  		printf("%02x ",data_out[i]);
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
			  	for(i = 0; i < bytes_transferred; i++)
			  	{
			  		printf("%02x ",data_in[i]);
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

void setupChiptoBoot(struct libusb_device_handle *devh){
static int8_t trigger = 0;
int16_t result = 0;

uint32_t size = 0;
TCmd tcmd_t = cmdINFO;
TBootInfo bootinfo_t = {0};
FILE *fp;
char path[250] = {0};  //path to hex file

static char data_in[MAX_INTERRUPT_IN_TRANSFER_SIZE];
static char data_out[MAX_INTERRUPT_OUT_TRANSFER_SIZE]; 

int32_t _blocks_to_flash_ = 0;//(uint32_t)(size / bootinfo_t.ulMcuSize.fValue);
   while(tcmd_t != cmdDONE)
   {	
		switch (tcmd_t)
		{
	
			case cmdSYNC:
				data_out[0] = 0x0f;data_out[1] = (char)cmdSYNC;		
				for (int i=9;i < MAX_INTERRUPT_OUT_TRANSFER_SIZE; i++)
				{
					data_out[i]=0x0;
				}			
				break;
			case cmdINFO:
				data_out[0] = 0x0f;data_out[1] = (char)cmdINFO;
				for (int i=2;i < MAX_INTERRUPT_OUT_TRANSFER_SIZE; i++)
				{
					data_out[i]=0x0;
				}
				break;
			case cmdBOOT:
		        bootInfo_buffer(&bootinfo_t,data_in);
				data_out[0] = 0x0f;data_out[1] = (char)cmdBOOT;
				for (int i=2;i < MAX_INTERRUPT_OUT_TRANSFER_SIZE; i++)
				{
					data_out[i]=0x0;
				}
				break;
			case cmdNON:
				printf("\n\tEnter the path of the hex file... ");
				fgets(path,sizeof(path),stdin);
				size_t len = strlen(path);
				if(len > 0 && path[len-1] == '\n'){
					path[len -1] = '\0'; //remove \n
				}
				printf("\n%s\n",path);
				fp = fopen(path,"rb");
				if(fp == NULL){
				  fprintf(stderr,"Could not find or open a file!!\n");
				}
				else{					
					for(size = 0; getc(fp) != EOF;size++);
					printf("file length = %u\n",size);
					trigger = 1;
				}
				_blocks_to_flash_ = (int32_t)(size / bootinfo_t.ulMcuSize.fValue);
				if(_blocks_to_flash_ == 0)_blocks_to_flash_ = 1;
				 printf("block size:= %u\n",_blocks_to_flash_);
				break;
			case cmdERASE:	
				data_out[0] = 0x0f;data_out[1] = (char)cmdERASE;		
				memcpy(data_out+3,&_PIC32Mn_STARTFLASH,sizeof(uint32_t));	
				memcpy(data_out+8,&_blocks_to_flash_,sizeof(int16_t));	
				for (int i=9;i < MAX_INTERRUPT_OUT_TRANSFER_SIZE; i++)
				{
					data_out[i]=0x0;
				}			
				break;	
			case cmdWRITE:
				data_out[0] = 0x0f;data_out[1] = (char)cmdWRITE;		
				memcpy(data_out+3,&_PIC32Mn_STARTFLASH,sizeof(uint32_t));	
				memcpy(data_out+8,&_blocks_to_flash_,sizeof(int16_t));	
				for (int i=9;i < MAX_INTERRUPT_OUT_TRANSFER_SIZE; i++)
				{
					data_out[i]=0x0;
				}
				break;
			case cmdHEX:
				break;
			default:		    
				break;
		}

		// Send and receive data.
        if(tcmd_t != cmdNON)
		{
			if(boot_interrupt_transfers(devh,data_in,data_out))
			{
				fprintf(stderr,"Error whilst transfering data...");
				return;
			}
		}

		switch(tcmd_t)
		{
			case cmdINFO:tcmd_t = cmdBOOT;break;
			case cmdBOOT:tcmd_t = cmdNON;break;
			case cmdNON:
				if(trigger == 1)
				{
					tcmd_t = cmdSYNC;					
				}
				break;
			case cmdSYNC:tcmd_t = cmdERASE;break;
			case cmdERASE:tcmd_t = cmdWRITE;break;
			case cmdWRITE:tcmd_t = cmdHEX;break;
			default:break;
		}
		
   }
}


/*Display the boot info need for erase and write data*/
void bootInfo_buffer(void *boot_info,const void *buffer){
TBootInfo *bootinfo_t = boot_info;
uint8_t *data;
	bootinfo_t->bSize = *((uint8_t*)buffer+0);
	memcpy(((uint8_t*)bootinfo_t+1),((uint8_t*)buffer+1),sizeof(TCharField));
	memcpy(((uint8_t*)bootinfo_t+3),((uint8_t*)buffer+4),sizeof(TULongField));
	memcpy(((uint8_t*)bootinfo_t+11),((uint8_t*)buffer+12),sizeof(TUIntField));
	memcpy(((uint8_t*)bootinfo_t+15),((uint8_t*)buffer+16),sizeof(TULongField));
	memcpy(((uint8_t*)bootinfo_t+19),((uint8_t*)buffer+20),sizeof(TULongField));
	memcpy(((uint8_t*)bootinfo_t+23),((uint8_t*)buffer+24),sizeof(TULongField));
	memcpy(((uint8_t*)bootinfo_t+31),((uint8_t*)buffer+32),sizeof(TStringField));
	
	printf("\n%02x\n%02x\t%02x\n%02x\t%08x\n%02x\t%04x\n%02x\t%04x\n%02x\t%04x\n%02x\t%08x\n%02x\t%s\n\n"
		,bootinfo_t->bSize
		,bootinfo_t->bMcuType.fFieldType
		,bootinfo_t->bMcuType.fValue
		,bootinfo_t->ulMcuSize.fFieldType
		,bootinfo_t->ulMcuSize.fValue
		,bootinfo_t->uiEraseBlock.fFieldType
		,bootinfo_t->uiEraseBlock.fValue.intVal
		,bootinfo_t->uiWriteBlock.fFieldType
		,bootinfo_t->uiWriteBlock.fValue.intVal
		,bootinfo_t->uiBootRev.fFieldType
		,bootinfo_t->uiBootRev.fValue.intVal
		,bootinfo_t->ulBootStart.fFieldType
		,bootinfo_t->ulBootStart.fValue
		,bootinfo_t->sDevDsc.fFieldType
		,bootinfo_t->sDevDsc.fValue);

}

void swap_bytes(uint8_t *bytes,int num){
uint8_t byte_ = 0;

  for(uint32_t i = 1;i < sizeof(bytes);i+=2){
	byte_ = bytes[i];
	bytes[i] = bytes[i+1];
	bytes[i+1] = byte_;
  }
  
}

