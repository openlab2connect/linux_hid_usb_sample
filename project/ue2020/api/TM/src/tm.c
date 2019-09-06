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

HANDLE g_hd;
pthread_mutex_t lock;
pthread_t id;
event_cb g_callback;

void buffer_hex_dump(unsigned char* buf, int size) {
	int i;
	for (i = 0; i<size; i++) {
		//fprintf(stderr, "%s: buf[%d] 0x%x\n", __func__, i, buf[i]);
		fprintf(stderr, "%1x ", buf[i]);
		if ( ((i+1) >= 8) && ((i+1)%8 == 0) )
			fprintf(stderr, "\n");
	}
	fprintf(stderr, "\n");
}

static void touch_event_callback(unsigned char * resp)
{
	event_cb *callback = &g_callback;
	uint8_t point;
	uint16_t *posx1, *posy1, *posx2, *posy2;

	int Tip;
	uint32_t uiX1pos, uiY1pos, uiX2pos, uiY2pos;

	//buffer_hex_dump(transfer->buffer, 12);
	point = resp[3];
	posx1 = (uint16_t *)&resp[4];
	posy1 = (uint16_t *)&resp[6];
	posx2 = (uint16_t *)&resp[8];
	posy2 = (uint16_t *)&resp[10];

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

// static void LIBUSB_CALL cb_in(struct libusb_transfer *transfer)
// {
// 	uint8_t point;
// 	uint16_t *posx1, *posy1, *posx2, *posy2;

// 	int Tip;
// 	uint32_t uiX1pos, uiY1pos, uiX2pos, uiY2pos;

// 	event_cb *callback = (event_cb *)transfer->user_data;

// 	//buffer_hex_dump(transfer->buffer, 12);
// 	point = transfer->buffer[3];
// 	posx1 = (uint16_t *)&transfer->buffer[4];
// 	posy1 = (uint16_t *)&transfer->buffer[6];
// 	posx2 = (uint16_t *)&transfer->buffer[8];
// 	posy2 = (uint16_t *)&transfer->buffer[10];

//     if (point == 2)
//     	fprintf(stderr, "%s:p=%d %d,%d %d,%d\n", __func__, point, *posx1, *posy1, *posx2, *posy2);
//     else
//     	fprintf(stderr, "%s:p=%d %d,%d\n", __func__, point, *posx1, *posy1);

//     Tip = (int)point;
//     uiX1pos = (uint32_t)*posx1;
//     uiY1pos = (uint32_t)*posy1;
//     uiX2pos = (uint32_t)*posx2;
//     uiY2pos = (uint32_t)*posy2;

//     if (callback != NULL)
//     	callback->cb(&Tip, &uiX1pos, &uiY1pos, &uiX2pos, &uiY2pos);
// }

// static void LIBUSB_CALL cb_out(struct libusb_transfer *transfer)
// {
// 	int *completed = transfer->user_data;
// 	fprintf(stderr, "%s:%d\n", __func__, __LINE__);
// 	buffer_hex_dump(transfer->buffer, sizeof(RESPBUFFER));
// 	//*completed = 1;
// }

void *EPIN_Sync(void *arg)
{
	int rc;
	//RESPBUFFER * rbuf = (RESPBUFFER *)malloc(sizeof(RESPBUFFER));
	event_cb * callback = &g_callback;

	while ( callback->completed ) {
		rc = usb_sync_resp(callback->resp, 0);
		if (rc < 0) {
			printf("%s: rc %d\n", __func__, rc);
		}
		if ( !callback->completed )
			break;
		if (callback->resp[0] == 'T') {
			// resp are touch points
			touch_event_callback(callback->resp);
		}
		else {
			buffer_hex_dump(callback->resp, RESP_FORMAT_64);
		}
	}

	fprintf(stderr, "%s: leave\n", __func__);
	//free(rbuf);
}

int TM_DisableCallbackTouchPoint(void)
{
	unsigned char cmd[] = "C62";
	event_cb * callback = &g_callback;
	callback->cb = NULL;

	fprintf(stderr, "%s\n", __func__);

	pthread_mutex_lock(&lock);
	callback->completed = 0;
	pthread_mutex_unlock(&lock);
	// wake block thread and go exit
	usb_sync_wake((unsigned char *)&cmd, sizeof(cmd)-1, 1);

	return TM_SUCCESS;
}

int TM_EnableCallbackTouchPoint(_CALLBACKFUNC lpPSFunc)
{
	HANDLE hd = g_hd;

	fprintf(stderr, "%s\n", __func__);
	if (hd == NULL)
		return TM_DEVICE_NO_OPEN;

	event_cb * callback = &g_callback;
	callback->cb = lpPSFunc;
	//callback->completed = 0;

	// callback->completed = 1;
	// pthread_create(&id, NULL, &EPIN_Sync, NULL);

	return TM_SUCCESS;
}

int TM_Close(HANDLE hd)
{
	TM_DisableCallbackTouchPoint();
	usleep(100000);

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

	event_cb * callback = &g_callback;

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

	// star EPIN
	callback->completed = 1;
	pthread_create(&id, NULL, &EPIN_Sync, NULL);

	return hd;
}