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
#include <arpa/inet.h>
#include <tm.h>

extern HANDLE g_hd;
extern event_cb g_callback;

void static update_dev_firmware_hash(TM_DEVICEINFO *pdev, unsigned char *resp)
{
	int i;
	for (i=0; i<32; i++)
		fprintf(stderr, "0x%1x,", resp[5+i]);
	fprintf(stderr, "\n");

	memcpy(pdev->FirmwareHash, (char *)&resp[5], sizeof(char)*32);
}
void static update_dev_firmware_crc(TM_DEVICEINFO *pdev, unsigned char *resp)
{
	int *lt32;

	lt32 = (int *)&resp[5];
	pdev->FirmwareCRC32 = *lt32;
}
void static update_dev_serial(TM_DEVICEINFO *pdev, unsigned char *resp)
{
	int *lt32;

	lt32 = (int *)&resp[5];
	pdev->SerialNumber = *lt32;
}

void static update_fw_identification(TM_DEVICEINFO *pdev, unsigned char *resp)
{
	char *src;
	char *dest;
	int *lt32;
	uint16_t *lt16;
	int startBytes = 5;
	unsigned char resp2[64];
	//int actual_length;
	int i,j;
	event_cb * callback = &g_callback;

	// board version; 1 byte;
	pdev->iBoardVersion = (int)resp[startBytes++];

	// total buffer size calculate start from 6.

	// HDMI firmware version, 3-byte array
	// HDMI firmware date; 4 bytes
	// MCU firmware version, 3-byte array
	// MCU firmware date; 4 bytes
	src = (char*)&resp[startBytes];
	dest = pdev->cHDMIFwVersion;
	memcpy(dest, src,
		sizeof(pdev->cHDMIFwVersion)    // 3 bytes
		+ sizeof(pdev->cHDMIFwDate)     // 4 bytes
		+ sizeof(pdev->cTouchFwVersion) // 3 bytes
		+ sizeof(pdev->cTouchFwDate) ); // 4 bytes

	// 6 +_14
	startBytes +=
		sizeof(pdev->cHDMIFwVersion)    // 3 bytes
		+ sizeof(pdev->cHDMIFwDate)     // 4 bytes
		+ sizeof(pdev->cTouchFwVersion) // 3 bytes
		+ sizeof(pdev->cTouchFwDate);   // 4 bytes

	// little-endian 4 bytes
	// fprintf(stderr, "%s: 0x%x,0x%x,0x%x,0x%x\n",
	// 	__func__, resp[20], resp[21], resp[22], resp[23]);

	// 20 + 4
	lt32 = (int *)&resp[startBytes];
	pdev->iuC_ID = *lt32;
	startBytes += 4;

	// 24 + 4
	// 4 bytes
	lt32 = (int *)&resp[startBytes];
	pdev->iFlashSize = *lt32;
	startBytes += 4;

	// 28 + 4
	// 4 bytes
	lt32 = (int *)&resp[startBytes];
	pdev->iFlashRes = *lt32;
	startBytes += 4;

	// 32 + 2
	// 0x1662,0x7001
	lt16 = (uint16_t *)&resp[startBytes];
	pdev->iVID = (int)*lt16;
	startBytes += 2;

	// 34 + 2
	lt16 = (uint16_t *)&resp[startBytes];
	pdev->iPID = (int)*lt16;
	startBytes += 2;

	// 36
	// vnedor name length 1 byte
	pdev->vln = resp[startBytes];
	// totoal bytes = 36+1
	// printf("%s:total fixed size bytes:%d\n", __func__, startBytes+1);

	// over max ep size
	// total bytes = (start bytes + 1) + lenght of bytes
	if ((startBytes + 1 + pdev->vln) > 64) {
		// libusb_bulk_transfer(g_hd,
		// ENPPOINT_IN, &resp2[0], sizeof(resp2), &actual_length, 0);
		memcpy(&resp2[0], callback->resp3, sizeof(resp2));
	}

	// fill vendor name
	// 36 + 1 point to vendor name
	j = 0; startBytes++;
	src = (char *)&resp[startBytes];
	dest = pdev->cVendorName;
	for (i = 0; i<pdev->vln; i++) {
		// make sure resp buffer is not over size of 64 bytes
		// if it does, move to next resp buffer
		if ((startBytes + 1 + i) > 64)
			dest[i] = resp2[j++];
		else
			dest[i] = src[i];
	}

	// 37 + 23
	// point to proudct length
	startBytes += pdev->vln;

	// proudct name lenght 1 byte
	if ( j != 0 )
		pdev->pln = resp2[j++];
	else {
		pdev->pln = resp[startBytes];
	}

	// over max ep size
	// total bytes = (start bytes + 1) + lenght of other bytes
	if ( j == 0 && (startBytes + 1 + pdev->pln) > 64) {
		// libusb_bulk_transfer(g_hd,
		// ENPPOINT_IN, &resp2[0], sizeof(resp2), &actual_length, 0);
		memcpy(&resp2[0], callback->resp3, sizeof(resp2));
	}

	// 60 + 1
	// point to product name
	startBytes++;
	src = (char *)&resp[startBytes];
	dest = pdev->cProductName;
	for (i = 0; i<pdev->pln; i++) {
		// make sure resp buffer is not over size of 64 bytes
		// if it does, move to next resp buffer
		if ((startBytes + 1 + i) > 64)
			dest[i] = resp2[j++];
		else
			dest[i] = src[i];
	}
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

	// read serial number
	cbuf->cmd[0] = 'C';
	cbuf->cmd[1] = '0';
	cbuf->cmd[2] = '3';

	rc = usb_sync_transfer_get(cbuf->cmd, rbuf->resp, sizeof(CMDBUFFER)-1, 1);
	if ( rc < 0)
		fprintf(stderr, "%s: rc %d\n", __func__, rc);
	else
		update_dev_serial(pDeviceInfo, rbuf->resp);

	// read firmware crc
	cbuf->cmd[0] = 'C';
	cbuf->cmd[1] = '0';
	cbuf->cmd[2] = '4';

	rc = usb_sync_transfer_get(cbuf->cmd, rbuf->resp, sizeof(CMDBUFFER)-1, 1);
	if ( rc < 0)
		fprintf(stderr, "%s: rc %d\n", __func__, rc);
	else
		update_dev_firmware_crc(pDeviceInfo, rbuf->resp);

	// read firmware hash
	cbuf->cmd[0] = 'C';
	cbuf->cmd[1] = '0';
	cbuf->cmd[2] = '5';

	rc = usb_sync_transfer_get(cbuf->cmd, rbuf->resp, sizeof(CMDBUFFER)-1, 1);
	if ( rc < 0)
		fprintf(stderr, "%s: rc %d\n", __func__, rc);
	else
		update_dev_firmware_hash(pDeviceInfo, rbuf->resp);

	//libusb_release_interface(hd, 0);
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
	uint16_t *retcode;

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

	retcode = (uint16_t *)&rbuf->resp[3];

	//libusb_release_interface(hd, 0);
	free(cbuf);
	free(rbuf);

	if (*retcode == 0x0003)
		return TM_INVALID_STATE;
	return rc;
}

int TM_SetSerialNumber (unsigned int iSerialNumber)
{
	int rc = TM_SUCCESS;
	HANDLE hd = g_hd;
	unsigned char cmd[7];
	RESPBUFFER *rbuf = (RESPBUFFER *)malloc(sizeof(RESPBUFFER));
	uint16_t *retcode;

	if (hd == NULL)
		return TM_DEVICE_NO_OPEN;
	if (rbuf == NULL)
		return TM_FAIL;

	cmd[0] = 'C';
	cmd[1] = '0';
	cmd[2] = '8';
	cmd[3] = (unsigned char )(iSerialNumber & 0xff);
	cmd[4] = (unsigned char )((iSerialNumber & 0xff00) >> 8);
	cmd[5] = (unsigned char )((iSerialNumber & 0xff0000) >> 16);
	cmd[6] = (unsigned char )((iSerialNumber & 0xff000000) >> 24);

	rc = usb_sync_transfer_set(cmd, rbuf->resp, sizeof(cmd), 1);
	if ( rc < 0)
		fprintf(stderr, "%s: rc %d\n", __func__, rc);

	retcode = (uint16_t *)&rbuf->resp[3];
	free(rbuf);

	if (*retcode == 0x0008)
		return TM_INVALID_STATE;
	if (*retcode == 0x0009)
		return TM_INVALID_STATE;
	return rc;

}

/*
	The hidden function that only used for clean serial number
	and manufacture data
*/
int TM_CleanManufactureData()
{
	int rc = TM_SUCCESS;
	HANDLE hd = g_hd;
	unsigned char cmd[3];
	RESPBUFFER *rbuf = (RESPBUFFER *)malloc(sizeof(RESPBUFFER));

	if (hd == NULL)
		return TM_DEVICE_NO_OPEN;
	if (rbuf == NULL)
		return TM_FAIL;

	cmd[0] = 'C';
	cmd[1] = '9';
	cmd[2] = '9';

	rc = usb_sync_transfer_set(cmd, rbuf->resp, sizeof(cmd), 1);
	if ( rc < 0)
		fprintf(stderr, "%s: rc %d\n", __func__, rc);

	free(rbuf);
	return rc;
}

int TM_SetManufactureData (int iNumBytes, char * cManufactureData)
{
	int rc = TM_SUCCESS;
	HANDLE hd = g_hd;
	unsigned char cmd[3+128];
	RESPBUFFER *rbuf = (RESPBUFFER *)malloc(sizeof(RESPBUFFER));
	uint16_t *retcode;

	if (hd == NULL)
		return TM_DEVICE_NO_OPEN;
	if (rbuf == NULL)
		return TM_FAIL;

	cmd[0] = 'C';
	cmd[1] = '0';
	cmd[2] = '9';

	memcpy(&cmd[3], cManufactureData, sizeof(unsigned char)*128);
	rc = usb_sync_transfer_set(cmd, rbuf->resp, sizeof(cmd), 1);
	if ( rc < 0)
		fprintf(stderr, "%s: rc %d\n", __func__, rc);

	retcode = (uint16_t *)&rbuf->resp[3];
	free(rbuf);

	if (*retcode == 0x0001)
		return TM_INVALID_PARAMETER;
	if (*retcode == 0x0008)
		return TM_FAIL;
	return rc;
}

int TM_GetManufactureData (char * cManufactureData)
{
	int rc = TM_SUCCESS;
	HANDLE hd = g_hd;
	unsigned char cmd[3];
	RESPBUFFER *rbuf = (RESPBUFFER *)malloc(sizeof(RESPBUFFER));
	uint16_t *retcode;
	event_cb * callback = &g_callback;

	if (hd == NULL)
		return TM_DEVICE_NO_OPEN;
	if (rbuf == NULL)
		return TM_FAIL;

	cmd[0] = 'C';
	cmd[1] = '1';
	cmd[2] = '0';

	rc = usb_sync_transfer_set(cmd, rbuf->resp, sizeof(cmd), 1);
	if ( rc < 0)
		fprintf(stderr, "%s: rc %d\n", __func__, rc);

	// Total resp size large than 128 bytes
	memcpy(cManufactureData, &callback->resp2[5], sizeof(char)*64-5);
	memcpy(&cManufactureData[59], callback->resp3, sizeof(char)*64);
	memcpy(&cManufactureData[59+64], callback->resp4, sizeof(char)*5);

	retcode = (uint16_t *)&rbuf->resp[3];
	free(rbuf);

	if (*retcode == 0x0008)
		return TM_FAIL;
	return rc;
}

int TM_GetTouchFirmwareAuthenticationCode (char * cHMACKey, char * cHMAC)
{
	int rc = TM_SUCCESS;
	HANDLE hd = g_hd;
	unsigned char cmd[3+32];
	RESPBUFFER *rbuf = (RESPBUFFER *)malloc(sizeof(RESPBUFFER));
	uint16_t *retcode;

	if (hd == NULL)
		return TM_DEVICE_NO_OPEN;
	if (rbuf == NULL)
		return TM_FAIL;

	cmd[0] = 'C';
	cmd[1] = '0';
	cmd[2] = '6';
	memcpy(&cmd[3], cHMACKey, sizeof(unsigned char)*32);

	rc = usb_sync_transfer_set(cmd, rbuf->resp, sizeof(cmd), 1);
	if ( rc < 0)
		fprintf(stderr, "%s: rc %d\n", __func__, rc);

	memcpy(cHMAC, (char *)&rbuf->resp[5], sizeof(unsigned char)*32);
	retcode = (uint16_t *)&rbuf->resp[3];

	free(rbuf);

	if (*retcode == 0x0001)
		return TM_INVALID_PARAMETER;
	return rc;
}