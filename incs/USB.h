#ifndef USB_H
#define USB_H

#include <libusb-1.0/libusb.h>

#define MAX_CONTROL_IN_TRANSFER_SIZE 64
#define MAX_CONTROL_OUT_TRANSFER_SIZE 64
#define MAX_INTERRUPT_IN_TRANSFER_SIZE 64
#define MAX_INTERRUPT_OUT_TRANSFER_SIZE 64

extern const int INTERFACE_NUMBER;

// function prototypes usb handling
int exchange_feature_reports_via_control_transfers(libusb_device_handle *devh);
int exchange_input_and_output_reports_via_control_transfers(libusb_device_handle *devh);
int exchange_input_and_output_reports_via_interrupt_transfers(libusb_device_handle *devh);

#endif