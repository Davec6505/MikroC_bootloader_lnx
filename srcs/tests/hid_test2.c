/*
 * Test list usb devices works
*/

#ifdef TEST
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
//#include "Types.h"
/*
 * HID example
 */
libusb_device_handle * discover1(){
char *str;
// discover devices
libusb_device **list;
libusb_device *found = NULL;
libusb_device_handle *handle = NULL;
ssize_t i = 0;
int err = 0;
	
	ssize_t cnt = libusb_get_device_list(NULL, &list);
	
	if (cnt < 0)
		fprintf(stderr,"count = %d\n",err);
	
	for (i = 0; i < cnt; i++) {
		libusb_device *device = list[i];
	}
	
	if (found) {
	
		err = libusb_open(found, &handle);
		if (err)
			fprintf(stderr,"%d\n",err);

		// etc
	}
	
	libusb_free_device_list(list, 1);

	return handle;
}

void discover2(){
	libusb_context *context = NULL;
    libusb_device **list = NULL;
	struct libusb_device_descriptor *desc = {0};
    int rc = 0;
    long count = 0;

    rc = libusb_init_context(&context,NULL,0);
    assert(rc == 0);
    fprintf(stderr,"%d\n",rc);

    count = libusb_get_device_list(context, &list);
    assert(count > 0);

    for (long idx = 0; idx < count-1; ++idx) {
        libusb_device *device = list[idx];

        rc = libusb_get_device_descriptor(device, desc);
        assert(rc == 0);

        printf("Vendor:Device = %04x:%04x\n", desc->idVendor, desc->idProduct);
    }

    libusb_free_device_list(list, 1);
    libusb_exit(context);
}



static void print_devs(libusb_device **devs)
{
	libusb_device *dev;
	int i = 0, j = 0;
	uint8_t path[8]; 

	while ((dev = devs[i++]) != NULL) {
		struct libusb_device_descriptor desc;
		int r = libusb_get_device_descriptor(dev, &desc);
		if (r < 0) {
			fprintf(stderr, "failed to get device descriptor");
			return;
		}

		printf("%04x:%04x (bus %d, device %d)",
			desc.idVendor, desc.idProduct,
			libusb_get_bus_number(dev), libusb_get_device_address(dev));

		r = libusb_get_port_numbers(dev, path, sizeof(path));
		if (r > 0) {
			printf(" path: %d", path[0]);
			for (j = 1; j < r; j++)
				printf(".%d", path[j]);
		}
		printf("\n");
	}
}

int _main1_(void)
{
	libusb_device **devs;
	static const int VENDOR_ID = 0x2dbc;
	static const int PRODUCT_ID = 0x0001;

	struct libusb_device_handle *devh = NULL;
	struct libusb_init_option *opts  = {0};
	int device_ready = 0;
	int result;


	int r;
	ssize_t cnt;

	r = libusb_init_context(/*ctx=*/NULL, /*options=*/NULL, /*num_options=*/0);
	if (r < 0)
		return r;

	cnt = libusb_get_device_list(NULL, &devs);
	if (cnt < 0){
		libusb_exit(NULL);
		return (int) cnt;
	}

	print_devs(devs);


	libusb_free_device_list(devs, 1);

	libusb_exit(NULL);
	return 0;
}


#endif
