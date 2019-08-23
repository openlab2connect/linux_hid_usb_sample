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

static HANDLE g_hd;
static int g_completed = 0;
pthread_mutex_t lock;

static void buffer_hex_dump(unsigned char* buf, int size) {
	int i;
	for (i = 0; i<size; i++)
		fprintf(stderr, "%s: buf[%d] 0x%x\n", __func__, i, buf[i]);
}

static void LIBUSB_CALL cb_in(struct libusb_transfer *transfer)
{
	uint8_t point;
	uint16_t *posx1, *posy1, *posx2, *posy2;

	int Tip;
	uint32_t uiX1pos, uiY1pos, uiX2pos, uiY2pos;

	event_cb *callback = (event_cb *)transfer->user_data;

	//buffer_hex_dump(transfer->buffer, 12);
	point = transfer->buffer[3];
	posx1 = (uint16_t *)&transfer->buffer[4];
	posy1 = (uint16_t *)&transfer->buffer[6];
	posx2 = (uint16_t *)&transfer->buffer[8];
	posy2 = (uint16_t *)&transfer->buffer[10];

    if (point == 2)
    	fprintf(stderr, "%s:p=%d %d,%d %d,%d\n", __func__, point, *posx1, *posy1, *posx2, *posy2);
    else
    	fprintf(stderr, "%s:p=%d %d,%d\n", __func__, point, *posx1, *posy1);

    Tip = (int)point;
    uiX1pos = (uint32_t)*posx1;
    uiY1pos = (uint32_t)*posy1;
    uiX2pos = (uint32_t)*posx2;
    uiY2pos = (uint32_t)*posy2;

    if (callback != NULL)
    	callback->cb(&Tip, &uiX1pos, &uiY1pos, &uiX2pos, &uiY2pos);
}

// static void LIBUSB_CALL cb_out(struct libusb_transfer *transfer)
// {
// 	int *completed = transfer->user_data;
// 	fprintf(stderr, "%s:%d\n", __func__, __LINE__);
// 	buffer_hex_dump(transfer->buffer, sizeof(RESPBUFFER));
// 	//*completed = 1;
// }

int TM_DisableCallbackTouchPoint(void)
{
	pthread_mutex_lock(&lock);
	g_completed = 1;
	pthread_mutex_unlock(&lock);

	fprintf(stderr, "%s:completed%d\n", __func__, g_completed);
	return TM_SUCCESS;
}

int TM_EnableCallbackTouchPoint(_CALLBACKFUNC lpPSFunc)
{
	int rc;
	HANDLE hd = g_hd;
	struct libusb_transfer *tsf;
	RESPBUFFER *rbuf = (RESPBUFFER *)malloc(sizeof(RESPBUFFER));
	event_cb *callback = malloc(sizeof(event_cb));
	int completed;

	if (hd == NULL)
		return TM_DEVICE_NO_OPEN;
	if (rbuf == NULL)
		return TM_INVALID_PARAMETER;

	// rc = libusb_claim_interface(hd, 0);
	// if (rc < 0) {
	// 	fprintf(stderr, "%s:interface claim error %d\n", __func__, rc);
	// 	return TM_ERROR_IO;
	// }

	callback->cb = lpPSFunc;
	callback->completed = g_completed;
	
	// Async IO call
	tsf = libusb_alloc_transfer(0);
	if (!tsf) {
		fprintf(stderr, "%s: alloc transfer error\n", __func__);
		return TM_FAIL;
	}

	libusb_fill_bulk_transfer(tsf, hd,
		ENPPOINT_IN, rbuf->resp, sizeof(RESPBUFFER), cb_in, callback, 0);
	// rc = libusb_submit_transfer(tsf);
	// if (rc != 0)
	// 	fprintf(stderr, "%s: rc %d\n", __func__, rc);

	while (!g_completed) {
		rc = libusb_submit_transfer(tsf);
	    if (rc != 0) {
	    	fprintf(stderr, "%s: rc %d\n", __func__, rc);
	    	break;
	    }
		rc = libusb_handle_events_completed(NULL, &completed);
		if (rc != 0)
			fprintf(stderr, "%s: rc %d\n", __func__, rc);
		//fprintf(stderr, "%s: handle event completed %d\n", __func__, completed);
	}

	libusb_free_transfer(tsf);
	// libusb_release_interface(hd, 0);

	free(rbuf);
	free(callback);

	if (rc == -6)
		return TM_INVALID_STATE;
	return TM_SUCCESS;
}

int TM_GetLCDBrightnessLevel(int *level)
{
	int actual_length = 0;
	int rc;
	HANDLE hd = g_hd;
	CMDBUFFER *cbuf = (CMDBUFFER *)malloc(sizeof(CMDBUFFER));
	RESPBUFFER *rbuf = (RESPBUFFER *)malloc(sizeof(RESPBUFFER));
	uint16_t retcode;
	// int i;

	if (hd == NULL)
		return TM_DEVICE_NO_OPEN;
	if (cbuf == NULL || rbuf == NULL)
		return TM_INVALID_PARAMETER;
	// brightness maximum value
    if (*level > 100)
    	return TM_INVALID_PARAMETER;

	rc = libusb_claim_interface(hd, 0);
	if (rc < 0) {
		fprintf(stderr, "%s:interface claim error %d\n", __func__, rc);
		return TM_ERROR_IO;
	}

	cbuf->cmd[0] = 'C';
    cbuf->cmd[1] = '6';
    cbuf->cmd[2] = '4';

    //EP OUT
	rc = libusb_bulk_transfer(hd, 
		ENPPOINT_OUT, cbuf->cmd, sizeof(CMDBUFFER)-1, &actual_length, 0);

	if (rc == 0 && actual_length == sizeof(CMDBUFFER)-1) {
		fprintf(stderr, "%s:data transfer success\n", __func__);
	}
	else {
		fprintf(stderr, "%s:data transfer error %d\n", __func__, rc);
		free(cbuf);
		return TM_FAIL;
	}

	//EP IN
	rc = libusb_bulk_transfer(hd, 
		ENPPOINT_IN, rbuf->resp, sizeof(RESPBUFFER), &actual_length, 0);

    if (rc == 0 && actual_length == sizeof(RESPBUFFER)) {
    	buffer_hex_dump(rbuf->resp, RESP_FORMAT_6);
    	// for (i=0; i<RESP_FORMAT_6; i++)
    	// 	fprintf(stderr, "%s:HEX 0x%x\n", __func__, rbuf->resp[i]);
    	// little endian
    	retcode = (rbuf->resp[4]<<4)|rbuf->resp[3];
    	*level = rbuf->resp[5];
    	fprintf(stderr, "%s:data transfer success ret %d level %d\n", __func__, retcode, *level);
    } else {
    	fprintf(stderr, "%s:data transfer error %d", __func__, rc);
    	free(rbuf);
    	return TM_FAIL;
    }

    free(cbuf);
	free(rbuf);

	libusb_release_interface(hd, 0);

	return TM_SUCCESS;
}

int TM_SetLCDBrightnessLevel(int level)
{
	int actual_length = 0;
	int rc;
	HANDLE hd = g_hd;
	CMDBUFFER *cbuf = (CMDBUFFER *)malloc(sizeof(CMDBUFFER));
	RESPBUFFER *rbuf = (RESPBUFFER *)malloc(sizeof(RESPBUFFER));
	uint16_t retcode;
	// int i;

	if (hd == NULL)
		return TM_DEVICE_NO_OPEN;
	if (cbuf == NULL || rbuf == NULL)
		return TM_INVALID_PARAMETER;
	// brightness maximum value
    if (level > 100)
    	return TM_INVALID_PARAMETER;

	rc = libusb_claim_interface(hd, 0);
	if (rc < 0) {
		fprintf(stderr, "%s:interface claim error %d\n", __func__, rc);
		return TM_ERROR_IO;
	}

	cbuf->cmd[0] = 'C';
    cbuf->cmd[1] = '6';
    cbuf->cmd[2] = '3';
    cbuf->cmd[3] = (unsigned char)level;

    //EP OUT
	rc = libusb_bulk_transfer(hd, 
		ENPPOINT_OUT, cbuf->cmd, sizeof(CMDBUFFER), &actual_length, 0);

	if (rc == 0 && actual_length == sizeof(CMDBUFFER)) {
		fprintf(stderr, "%s:data transfer success\n", __func__);
	}
	else {
		fprintf(stderr, "%s:data transfer error %d\n", __func__, rc);
		free(cbuf);
		return TM_FAIL;
	}

	//EP IN
	rc = libusb_bulk_transfer(hd, 
		ENPPOINT_IN, rbuf->resp, sizeof(RESPBUFFER), &actual_length, 0);

    if (rc == 0 && actual_length == sizeof(RESPBUFFER)) {
    	buffer_hex_dump(rbuf->resp, RESP_FORMAT_5);
    	// for (i=0; i<RESP_FORMAT_5; i++)
    	// 	fprintf(stderr, "%s:HEX 0x%x\n", __func__, rbuf->resp[i]);
    	// little endian
    	retcode = (rbuf->resp[4]<<4)|rbuf->resp[3];
    	fprintf(stderr, "%s:data transfer success ret %d\n", __func__, retcode);
    } else {
    	fprintf(stderr, "%s:data transfer error %d", __func__, rc);
    	free(rbuf);
    	return TM_FAIL;
    }

    free(cbuf);
	free(rbuf);
	libusb_release_interface(hd, 0);

	return TM_SUCCESS;
}

int TM_Close(HANDLE hd)
{
	libusb_close(hd);
	libusb_exit(NULL);
	pthread_mutex_destroy(&lock);
	return 0;
}

HANDLE TM_Open(int *erron)
{
	int rc;
	HANDLE hd;
	const struct libusb_version *version;

	version = libusb_get_version();
	fprintf(stderr, "libusb version: %d.%d.%d.%d\n"
		,version->major, version->minor, version->micro, version->nano);

	rc = libusb_init(NULL);
	if (rc < 0) {
		fprintf(stderr, "Error Initializing libusb: %s\n", libusb_error_name(rc));
		return NULL;
    }

    libusb_set_debug(NULL, 3);

	hd = libusb_open_device_with_vid_pid(NULL, VENDOR_ID, PRODUCT_ID);
	if ( !hd ) {
		fprintf(stderr, "%s:error finding TM device\n", __func__);
		*erron = -1;
		return NULL;
	}

	fprintf(stderr, "%s:TM device open\n", __func__);

	rc = libusb_detach_kernel_driver(hd, 0);
	if (rc < 0 && rc != LIBUSB_ERROR_NOT_FOUND) {
		fprintf(stderr, "%s:detach kernel driver error %d\n", __func__, rc);
		*erron = -1;
		return NULL;
	}

	fprintf(stderr, "%s:TM device init done\n", __func__);
	
	*erron = TM_SUCCESS;
	if (hd)
		g_hd = hd;

	pthread_mutex_init(&lock, NULL);
	return hd;
}