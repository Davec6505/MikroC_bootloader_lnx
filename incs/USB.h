#ifndef USB_H
#define USB_H

#include <libusb-1.0/libusb.h>

#define MAX_CONTROL_IN_TRANSFER_SIZE 64
#define MAX_CONTROL_OUT_TRANSFER_SIZE 64
#define MAX_INTERRUPT_IN_TRANSFER_SIZE 64
#define MAX_INTERRUPT_OUT_TRANSFER_SIZE 64

extern const int INTERFACE_NUMBER;

// function prototypes usb handling
int boot_interrupt_transfers(libusb_device_handle *devh, char *data_in, char *data_out, uint8_t out_only);
#endif