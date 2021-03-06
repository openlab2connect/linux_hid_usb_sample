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

int main(void)
{
	int rc;
	HANDLE handle;
	int level;
	int error;
	int data;

	handle = TM_Open(&error);
	if ( !handle ) {
		fprintf(stderr, "Error finding USB device\n");
		libusb_exit(NULL);
		return 1;
	}

	TM_EnableCallbackTouchPoint(xfs_cb);
	while(1) {
		TM_SetBioLed(1);
		TM_GetBioLed(&data);
		fprintf(stderr, "get data %d\n", data);
		sleep(2);
		TM_SetBioLed(0);
		TM_GetBioLed(&data);
		fprintf(stderr, "get data %d\n", data);
		sleep(2);
		TM_SetBioLed(2);
		TM_GetBioLed(&data);
		fprintf(stderr, "get data %d\n", data);
		sleep(2);
		TM_SetBioLed(0);
		TM_GetBioLed(&data);
		fprintf(stderr, "get data %d\n", data);
		//TM_DisableCallbackTouchPoint();
	}
exit:
	TM_Close(handle);
	return 0;
}