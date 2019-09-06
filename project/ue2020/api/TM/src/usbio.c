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
extern event_cb g_callback;

int usb_sync_wake(unsigned char *cmd, int bytes, int dump_resp)
{
	int rc;
	int actual_length;
	//uint16_t *retcode;
	HANDLE hd = g_hd;

    //EP OUT
	rc = libusb_bulk_transfer(hd,
		ENPPOINT_OUT, cmd, bytes, &actual_length, 0);

	if (rc == 0 && actual_length == bytes) {
		fprintf(stderr, "%s:data transfer success\n", __func__);
	}
	else {
		fprintf(stderr, "%s:data transfer error %d\n", __func__, rc);
		return TM_FAIL;
	}

	return TM_SUCCESS;
}

int usb_sync_resp(unsigned char *resp, int dump_resp)
{
	int rc;
	int actual_length;
	HANDLE hd = g_hd;

	//libusb_claim_interface(hd, 0);

	//EP IN
	rc = libusb_bulk_transfer(hd,
		ENPPOINT_IN, resp, sizeof(RESPBUFFER), &actual_length, 0);

	if (rc == 0 && actual_length == sizeof(RESPBUFFER)) {
		// RetCode
		// retcode = (uint16_t *)&resp[3];
		fprintf(stderr, "%s:data response success in bytes %d\n", __func__, actual_length);
		if (dump_resp)
			buffer_hex_dump(resp, RESP_FORMAT_64);
	} else {
		fprintf(stderr, "%s:data response error %d\n", __func__, rc);
		return TM_FAIL;
	}

	libusb_release_interface(hd, 0);

	return TM_SUCCESS;
}

int usb_sync_transfer_get(unsigned char *cmd, unsigned char *resp, int bytes, int dump_resp)
{
	int rc;
	int actual_length;
	uint16_t *retcode;
	HANDLE hd = g_hd;

	event_cb * callback = &g_callback;

	rc = libusb_claim_interface(hd, 0);
	if (rc < 0) {
		fprintf(stderr, "%s:interface claim error %d\n", __func__, rc);
		return TM_ERROR_IO;
	}

    //EP OUT
	rc = libusb_bulk_transfer(hd, 
		ENPPOINT_OUT, cmd, bytes, &actual_length, 0);

	if (rc == 0 && actual_length == bytes) {
		fprintf(stderr, "%s:data transfer success in bytes %d\n", __func__, actual_length);
	}
	else {
		fprintf(stderr, "%s:data transfer error %d\n", __func__, rc);
		return TM_FAIL;
	}

	usleep(200000);
	memcpy(resp, callback->resp, sizeof(RESPBUFFER));

	//EP IN
	// rc = libusb_bulk_transfer(hd,
	// 	ENPPOINT_IN, resp, sizeof(RESPBUFFER), &actual_length, 0);

	// if (rc == 0 && actual_length == sizeof(RESPBUFFER)) {
	// 	// RetCode
	// 	retcode = (uint16_t *)&resp[3];
	// 	fprintf(stderr, "%s:data response success ret %d\n", __func__, *retcode);
	// 	if (dump_resp)
	// 		buffer_hex_dump(resp, RESP_FORMAT_64);
	// } else {
	// 	fprintf(stderr, "%s:data response error %d", __func__, rc);
	// 	return TM_FAIL;
	// }
	return TM_SUCCESS;
}


int usb_sync_transfer_set(unsigned char *cmd, unsigned char *resp, int bytes, int dump_resp)
{
	int rc;
	int actual_length;
	uint16_t *retcode;
	HANDLE hd = g_hd;

	event_cb * callback = &g_callback;

	rc = libusb_claim_interface(hd, 0);
	if (rc < 0) {
		fprintf(stderr, "%s:interface claim error %d\n", __func__, rc);
		return TM_ERROR_IO;
	}

    //EP OUT
	rc = libusb_bulk_transfer(hd,
		ENPPOINT_OUT, cmd, bytes, &actual_length, 0);

	if (rc == 0 && actual_length == bytes) {
		fprintf(stderr, "%s:data transfer success in bytes %d\n", __func__, actual_length);
	}
	else {
		fprintf(stderr, "%s:data transfer error %d\n", __func__, rc);
		return TM_FAIL;
	}

	usleep(200000);
	memcpy(resp, callback->resp, sizeof(RESPBUFFER));

	//EP IN
	// rc = libusb_bulk_transfer(hd,
	// 	ENPPOINT_IN, resp, sizeof(RESPBUFFER), &actual_length, 0);

	// if (rc == 0 && actual_length == sizeof(RESPBUFFER)) {
	// 	retcode = (uint16_t *)&resp[3];
	// 	fprintf(stderr, "%s:data response success ret %d\n", __func__, *retcode);
	// 	if (dump_resp)
	// 		buffer_hex_dump(resp, RESP_FORMAT_5);
	// } else {
	// 	fprintf(stderr, "%s:data response error %d", __func__, rc);
	// 	return TM_FAIL;
	// }
	return TM_SUCCESS;
}

int usb_sync_transfer_set_512(unsigned char * buffer, unsigned char * resp,
	uint32_t address, int dump_resp)
{
	int rc = TM_SUCCESS;
	int actual_length;
	HANDLE hd = g_hd;
	uint16_t * retcode;
	int i = 0;
	unsigned int * fwblk_addr;
	//size_t ii;

	unsigned char cmd[512+7];
	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 'C';
	cmd[1] = '4';
	cmd[2] = '7';
	fwblk_addr = (unsigned int *)&cmd[3];
	*fwblk_addr = address;

	fprintf(stderr, "%s: write address little endian %x %x %x %x\n",
		 __func__, cmd[3], cmd[4], cmd[5], cmd[6]);

	memcpy(&cmd[7], buffer, sizeof(unsigned char)*512);

	// EP OUT
	rc = libusb_bulk_transfer(hd,
	ENPPOINT_OUT, &cmd[i], sizeof(cmd), &actual_length, 0);
	if (rc == 0 && actual_length == sizeof(cmd)) {
		fprintf(stderr, "%s:data transfer success in bytes %d\n", __func__, actual_length);
		i += actual_length;
	}
	else {
		fprintf(stderr, "%s:data transfer error %d\n", __func__, rc);
		return TM_FAIL;
	}

	//EP IN
	rc = libusb_bulk_transfer(hd,
		ENPPOINT_IN, resp, sizeof(RESPBUFFER), &actual_length, 0);

	if (rc == 0 && actual_length == sizeof(RESPBUFFER)) {
			retcode = (uint16_t *)&resp[3];
		fprintf(stderr, "%s:data response success ret %d\n", __func__, *retcode);
		if (dump_resp)
			buffer_hex_dump(resp, RESP_FORMAT_5);
	} else {
		fprintf(stderr, "%s:data response error %d", __func__, rc);
		return TM_FAIL;
	}

	return rc;
}