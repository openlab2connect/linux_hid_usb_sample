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

static int xfs_cb(int *iTp, unsigned int *uiX1pos, unsigned int
*uiY1pos, unsigned int *uiX2pos, unsigned int *uiY2pos)
{
	fprintf(stderr, "%s: p%d %d,%d %d,%d\n", __func__, 
		*iTp, *uiX1pos, *uiY1pos, *uiX2pos, *uiY2pos);
	return 0;
}

static int sync_bulk_read(libusb_device_handle *handle) {

	int rc;
	int actual_length;
	int i;
	uint16_t retcode;
	RESPBUFFER *rbuf = (RESPBUFFER *)malloc(sizeof(RESPBUFFER));

	fprintf(stderr, "%s: size %ld\n", __func__, sizeof(RESPBUFFER));

	rc = libusb_bulk_transfer(handle, ENPPOINT_IN, rbuf->resp, sizeof(RESPBUFFER), &actual_length, 0);
    if ( rc == 0 && actual_length == sizeof(RESPBUFFER)) {
    	for (i=0; i<actual_length; i++)
    		fprintf(stderr, "%s: hex 0x%x\n", __func__, rbuf->resp[i]);
    	retcode = (rbuf->resp[3]<<4)|rbuf->resp[4];
    	fprintf(stderr, "%s: data transfer success ret %d\n", __func__, retcode);
    } else
    	fprintf(stderr, "%s: data transfer error %d", __func__, rc);

	if (rbuf)
		free(rbuf);
	return 0;
}

static int sync_bulk_write(libusb_device_handle *handle, unsigned char data) {

	int actual_length;
	int rc;
	CMDBUFFER *cbuf = (CMDBUFFER *)malloc(sizeof(CMDBUFFER));

    cbuf->cmd[0] = 'C';
    cbuf->cmd[1] = '6';
    cbuf->cmd[2] = '1';
    cbuf->cmd[3] = data;

    //buffer_checked(cbuf->cmd, sizeof(CMDBUFFER));
    fprintf(stderr, "%s: size %ld\n", __func__, sizeof(CMDBUFFER));

	rc = libusb_bulk_transfer(handle, ENPPOINT_OUT, cbuf->cmd, sizeof(CMDBUFFER), &actual_length, 0);
	if (rc == 0 && actual_length == sizeof(CMDBUFFER))
		fprintf(stderr, "%s: data transfer success\n", __func__);
	else
		fprintf(stderr, "%s: data transfer error %d\n", __func__, rc);

	if (cbuf)
		free(cbuf);
	return 0;
}

// void *tm_thread(void *arg)
// {
// 	fprintf(stderr, "%s: enter\n", __func__);
// 	CallbackTouchPoint();
// 	fprintf(stderr, "%s: leave\n", __func__);
// }

int main(void)
{
	int rc;
	HANDLE handle;
	int level;
	int error;
	//pthread_t id;

	handle = TM_Open(&error);
	if ( !handle ) {
		fprintf(stderr, "Error finding USB device\n");
		libusb_exit(NULL);
		return 1;
	}

	TM_EnableCallbackTouchPoint(xfs_cb);
	//rc = pthread_create(&id, NULL, &tm_thread, NULL);

	sleep(10);
	TM_DisableCallbackTouchPoint();

	//pthread_join(id, NULL);

exit:
	TM_Close(handle);
	return 0;
}