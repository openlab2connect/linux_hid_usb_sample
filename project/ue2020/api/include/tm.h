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

#include <libusb-1.0/libusb.h>

// #define VENDOR_ID    0x0416
// #define PRODUCT_ID   0x0041

#define VENDOR_ID    0x1662
#define PRODUCT_ID   0x7001
#define BOOTLOADER_PRODUCT_ID 0x0040
#define ENPPOINT_IN  0x81
#define ENPPOINT_OUT 0x02

#define RESP_FORMAT_5 5
#define RESP_FORMAT_6 6

#define TIMEOUT 15*1000 // milliseconds

//TODO: 
enum tm_error {
	TM_SUCCESS = 0,
	TM_ERROR_IO = -1,
	TM_FAIL = -2,
	TM_ERROR_INVALID_HANDLE = -3,
	TM_DEVICE_NO_OPEN = -4,
	TM_INVALID_PARAMETER = -5,
	TM_INVALID_STATE = -6,
	TM_CALLBACK_NOT_ENABLED = -8,
};

typedef int (*_CALLBACKFUNC)(int *, unsigned int *, unsigned int *, unsigned int *, unsigned int *);
typedef struct libusb_device_handle * HANDLE;

typedef struct {
	_CALLBACKFUNC cb;
	int completed;
} event_cb;

typedef struct {
	unsigned char data[10];

} TOUCHBUFFER;

typedef struct {
	unsigned char cmd[4];

} CMDBUFFER;

typedef struct {
	unsigned char resp[64];

} RESPBUFFER;

HANDLE TM_Open(int *);
int TM_Close(libusb_device_handle *);
int TM_SetLCDBrightnessLevel(int);
int TM_GetLCDBrightnessLevel(int *);
int TM_EnableCallbackTouchPoint(_CALLBACKFUNC);
int TM_DisableCallbackTouchPoint(void);