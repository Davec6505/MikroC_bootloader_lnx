#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include <linux/types.h>
#include <linux/input.h>
#include <linux/hidraw.h>

#include "USB.h"
#include "Types.h"
#include "HexFile.h"

static const int CONTROL_REQUEST_TYPE_IN = LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE;
static const int CONTROL_REQUEST_TYPE_OUT = LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE;

// From the HID spec:

static const int HID_GET_REPORT = 0x01;
static const int HID_SET_REPORT = 0x09;
static const int HID_REPORT_TYPE_INPUT = 0x01;
static const int HID_REPORT_TYPE_OUTPUT = 0x02;
static const int HID_REPORT_TYPE_FEATURE = 0x03;

// With firmware support, transfers can be > the endpoint's max packet size.

static const int TIMEOUT_MS = 5000;

// Use interrupt transfers to to write data to the device and receive data from the device.
// Returns - zero on success, libusb error code on failure.
int boot_interrupt_transfers(libusb_device_handle *devh, char *data_in, char *data_out, uint8_t out_only)
{
    // Assumes interrupt endpoint 2 IN and OUT:
    static const int INTERRUPT_IN_ENDPOINT = 0x81;
    static const int INTERRUPT_OUT_ENDPOINT = 0x01;

    // With firmware support, transfers can be > the endpoint's max packet size.
    int bytes_transferred;
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

    if (result >= 0 | out_only == 1)
    {
#if DEBUG == 1
        printf("Data sent via interrupt transfer:\n");
        for (i = 0; i < bytes_transferred; i++)
        {
            printf("%02x ", data_out[i] & 0xff);
        }
        printf("\n");
#endif

        if (out_only > 0)
            return result;

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
#if DEBUG == 1
                printf("Data received via interrupt transfer:\n");
                for (i = 0; i < bytes_transferred; i++)
                {
                    printf("%02x ", data_in[i] & 0xff);
                }
                printf("\n");
#endif
            }
            else
            {
                fprintf(stderr, "No data received in interrupt transfer (%d)\n", result);
                return -1;
            }
        }
        else
        {
            fprintf(stderr, "mcu rebooted! %d\n", result); //"Error receiving data via interrupt transfer %d\n", result);
            return result;
        }
    }
    else
    {
        if (out_only != 2)
            fprintf(stderr, "Error sending data via interrupt transfer %d\n", result);
        else
            fprintf(stderr, "Device has been re-booted! %d\n", result);

        return result;
    }
    return 0;
}
