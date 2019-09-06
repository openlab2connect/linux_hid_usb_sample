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
#include <pthread.h>
#include <tm.h>

extern HANDLE g_hd;

int TM_GetSpeakerStatus (int *iStatus, int *iVolume, int *iFreq)
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
	cbuf->cmd[1] = '6';
	cbuf->cmd[2] = '8';

	rc = usb_sync_transfer_get(cbuf->cmd, rbuf->resp, sizeof(CMDBUFFER)-1, 1);
	if ( rc < 0)
		fprintf(stderr, "%s: rc %d\n", __func__, rc);

	*iStatus = (int)rbuf->resp[5];
	*iVolume = (int)rbuf->resp[6];
	*iFreq = (int)((rbuf->resp[8] << 8)|rbuf->resp[7]);

	//libusb_release_interface(hd, 0);
	free(cbuf);
	free(rbuf);
	return rc;
}

int TM_SetSpeakerFreq (int iFreq)
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
	cbuf->cmd[1] = '6';
	cbuf->cmd[2] = '7';
	cbuf->cmd[3] = (unsigned char)(iFreq & 0xFF); // Low nibble
	cbuf->cmd[4] = (unsigned char)((iFreq & 0xFF00) >> 8); // high nibble

	fprintf(stderr, "%s high nibble 0x%x low nibble 0x%x\n", __func__, cbuf->cmd[4], cbuf->cmd[3]);

	rc = usb_sync_transfer_set(cbuf->cmd, rbuf->resp, sizeof(CMDBUFFER)+1, 1);
	if ( rc < 0)
		fprintf(stderr, "%s: rc %d\n", __func__, rc);

	//libusb_release_interface(hd, 0);
	free(cbuf);
	free(rbuf);
	return rc;
}

int TM_SetSpeakerVolume (int iVolume)
{
	int rc = TM_SUCCESS;
	HANDLE hd = g_hd;
	CMDBUFFER *cbuf = (CMDBUFFER *)malloc(sizeof(CMDBUFFER));
	RESPBUFFER *rbuf = (RESPBUFFER *)malloc(sizeof(RESPBUFFER));

	if (iVolume > 100)
		return TM_INVALID_PARAMETER;
	if (hd == NULL)
		return TM_DEVICE_NO_OPEN;
	if (cbuf == NULL || rbuf == NULL)
		return TM_FAIL;

	cbuf->cmd[0] = 'C';
	cbuf->cmd[1] = '6';
	cbuf->cmd[2] = '6';
	cbuf->cmd[3] = (unsigned char)iVolume;

	rc = usb_sync_transfer_set(cbuf->cmd, rbuf->resp, sizeof(CMDBUFFER), 1);
	if ( rc < 0)
		fprintf(stderr, "%s: rc %d\n", __func__, rc);

	//libusb_release_interface(hd, 0);
	free(cbuf);
	free(rbuf);
	return rc;
}

int TM_SetSpeakerOnOff (int iSpeaker)
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
	cbuf->cmd[1] = '6';
	cbuf->cmd[2] = '5';
	cbuf->cmd[3] = (unsigned char)iSpeaker;

	rc = usb_sync_transfer_set(cbuf->cmd, rbuf->resp, sizeof(CMDBUFFER), 1);
	if ( rc < 0)
		fprintf(stderr, "%s: rc %d\n", __func__, rc);

	//libusb_release_interface(hd, 0);
	free(cbuf);
	free(rbuf);
	return rc;
}