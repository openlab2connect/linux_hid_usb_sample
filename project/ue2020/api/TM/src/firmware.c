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

extern HANDLE g_hd;

void static update_fw_identification(TM_DEVICEINFO *pdev, unsigned char *resp)
{
	char *src;
	char *dest;
	int *integer;

	pdev->iBoardVersion = (int)resp[5]; // 1 byte

	src = (char	*)&resp[6];
	dest = pdev->cHDMIFwVersion;
	memcpy(dest, src,
		sizeof(pdev->cHDMIFwVersion)    // 3 bytes
		+ sizeof(pdev->cHDMIFwDate)     // 4 bytes
		+ sizeof(pdev->cTouchFwVersion) // 3 bytes
		+ sizeof(pdev->cTouchFwDate) ); // 4 bytes

	integer = (int *)&resp[20];          // 4 bytes
	pdev->iuC_ID = *integer;

	integer = (int *)&resp[24];          // 4 bytes
	pdev->iFlashSize = *integer;

	integer = (int *)&resp[28];          // 4 bytes 
	pdev->iFlashRes = *integer;
	
	pdev->iVID = (uint16_t)((resp[33]<<8)|resp[32]);
	pdev->iPID = (uint16_t)((resp[35]<<8)|resp[34]);

	pdev->vln = resp[36];               // 1 byte vnedor name
	src = (char *)&resp[37];
	dest = pdev->cVendorName;
	memcpy(dest, src, pdev->vln);       // max 255 bytes

	pdev->pln = src[255];               // 1 byte product name
	src = (char *)&src[256];
	dest = pdev->cProductName;
	memcpy(dest, src, pdev->pln);        // max 255 bytes
}

int TM_Who (TM_DEVICEINFO * pDeviceInfo)
{
	int rc = TM_SUCCESS;
	HANDLE hd = g_hd;
	CMDBUFFER *cbuf = (CMDBUFFER *)malloc(sizeof(CMDBUFFER));
	RESPBUFFER *rbuf = (RESPBUFFER *)malloc(sizeof(RESPBUFFER));

	if (hd == NULL)
		return TM_DEVICE_NO_OPEN;
	if (cbuf == NULL || rbuf == NULL)
		return TM_FAIL;

	cbuf->cmd[0] = 'C';
	cbuf->cmd[1] = '0';
	cbuf->cmd[2] = '2';

	rc = usb_sync_transfer_get(cbuf->cmd, rbuf->resp, sizeof(CMDBUFFER)-1, 1);
	if ( rc < 0)
		fprintf(stderr, "%s: rc %d\n", __func__, rc);
	else
		update_fw_identification(pDeviceInfo, rbuf->resp);

	libusb_release_interface(hd, 0);
	free(cbuf);
	free(rbuf);
	return rc;
}

int TM_FirmwareReset(void)
{
	int rc = TM_SUCCESS;
	HANDLE hd = g_hd;
	CMDBUFFER *cbuf = (CMDBUFFER *)malloc(sizeof(CMDBUFFER));
	RESPBUFFER *rbuf = (RESPBUFFER *)malloc(sizeof(RESPBUFFER));
	uint16_t retcode;

	if (hd == NULL)
		return TM_DEVICE_NO_OPEN;
	if (cbuf == NULL || rbuf == NULL)
		return TM_FAIL;

	cbuf->cmd[0] = 'C';
	cbuf->cmd[1] = '0';
	cbuf->cmd[2] = '1';

	rc = usb_sync_transfer_set(cbuf->cmd, rbuf->resp, sizeof(CMDBUFFER)-1, 1);
	if ( rc < 0)
		fprintf(stderr, "%s: rc %d\n", __func__, rc);

	retcode = (uint16_t)((rbuf->resp[4] << 8)|rbuf->resp[3]);

	libusb_release_interface(hd, 0);
	free(cbuf);
	free(rbuf);

	if (retcode == 0x0003)
		return TM_INVALID_STATE;
	return rc;
}