/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <tm.h>

int main(void)
{
	int rc;
	HANDLE handle;
	int data;
	int error;
	int i;
	char *pvname, *ppname;
	TM_DEVICEINFO dev;

	uint16_t * hdmiFwData;
	uint16_t * mcuFwData;

	handle = TM_Open(&error);
	if ( !handle ) {
		fprintf(stderr, "Error finding USB device\n");
		libusb_exit(NULL);
		return 1;
	}

	TM_Who(&dev);

	hdmiFwData = (uint16_t *)&dev.cHDMIFwDate[0];
	fprintf(stderr, "HDMI Fw Date: %d-%d-%d\n", *hdmiFwData, dev.cHDMIFwDate[1], dev.cHDMIFwDate[2]);

	mcuFwData = (uint16_t *)&dev.cTouchFwDate[0];
	fprintf(stderr, "MCU Fw Date: %d-%d-%d\n", *mcuFwData, dev.cTouchFwDate[1], dev.cTouchFwDate[2]);

	fprintf(stderr, "ID: %d\n", dev.iuC_ID);
	fprintf(stderr, "USB: Vid,0x%x Pid,0x%x\n", dev.iVID, dev.iPID);

	fprintf(stderr, "Vendor Name length in bytes %d\n", dev.vln);
	pvname = (char *)&dev.cVendorName;
	for (i=0; i<dev.vln; i++)
		fprintf(stderr, "%c", pvname[i]);
	fprintf(stderr, "\n");

	fprintf(stderr, "Product Name length in bytes %d\n", dev.pln);
	ppname = (char *)&dev.cProductName;
	for (i=0; i<dev.pln; i++)
		fprintf(stderr, "%c", ppname[i]);
	fprintf(stderr, "\n");

	fprintf(stderr, "Serial Number %u\n", dev.SerialNumber);
	fprintf(stderr, "CRC32 Number %x\n", dev.FirmwareCRC32);
	for (i=0; i<32; i++)
		fprintf(stderr, "0x%1x,", (unsigned char)dev.FirmwareHash[i]);
	fprintf(stderr, "\n");

	// just in case if device needs te be reset
	// TM_FirmwareReset();
	// sleep(3);

exit:
	TM_Close(handle);
	return 0;
}