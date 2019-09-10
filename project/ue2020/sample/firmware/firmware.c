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
#include <string.h>
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

	int serial_no = 12345;
	char mdata[128];
	int count = 0;
	char mdata2[32];

	handle = TM_Open(&error);
	if ( !handle ) {
		fprintf(stderr, "Error finding USB device\n");
		libusb_exit(NULL);
		return 1;
	}

	// Clean exist manufacture data
	TM_CleanManufactureData();
	sleep(2);

	TM_SetSerialNumber(serial_no);

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

	fprintf(stderr, "sent manufacture data\n");
	for (count = 0; count < 64; count++)
		memset(&mdata[count], count, sizeof(char));
	for (count; count < 128; count++)
		memset(&mdata[count], count, sizeof(char));

	TM_SetManufactureData(128, (char *)&mdata);

	memset(&mdata[0], 0x0, sizeof(mdata));
	TM_GetManufactureData((char *)&mdata);
	for (count =0; count<128; count++) {
		fprintf(stderr, "%1x ", mdata[count]);
		if ( ((count+1) >= 8) && ((count+1)%8 == 0) )
			fprintf(stderr, "\n");
	}

	TM_GetTouchFirmwareAuthenticationCode(mdata, mdata2);
	for (count =0; count<32; count++) {
		fprintf(stderr, "%x ", (unsigned char)mdata2[count]);
		if ( ((count+1) >= 8) && ((count+1)%8 == 0) )
			fprintf(stderr, "\n");
	}

exit:
	TM_Close(handle);
	return 0;
}