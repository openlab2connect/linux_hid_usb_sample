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
#define BOOTLOADER_ID 0x0040
#define ENPPOINT_IN  0x81
#define ENPPOINT_OUT 0x02

#define RESP_FORMAT_5 5
#define RESP_FORMAT_64 64

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
	TM_INVALID_DATE_TIME = -7,
	TM_CALLBACK_NOT_ENABLED = -8,
	TM_INVALID_SIGNATURE = -9,
	TM_CRC_MISMATCH = -10,
};

typedef int (*_CALLBACKFUNC)(int *, unsigned int *, unsigned int *, unsigned int *, unsigned int *);
typedef struct libusb_device_handle * HANDLE;

typedef struct
{
	int iBoardVersion; /* board version; 1 byte; range 0 thru 7, at least.*/
	char cHDMIFwVersion[3]; /* firmware version, 3-byte array */
	char cHDMIFwDate[4]; /* firmware date; 4 bytes. */
	char cTouchFwVersion[3]; /* firmware version, 3-byte array */
	char cTouchFwDate[4]; /* firmware date; 4 bytes. */
	int iuC_ID; /* Identification of the microcontroller.*/
	int iFlashSize; /* Size of the microcontroller’s flash memory in bytes.*/
	int iFlashRes; /* Length of the area inside the flash memory reserved for storage of secure data.*/
	int iVID; /* USB Vendor ID.*/
	int iPID; /* USB Product ID.*/
	int vln; /* lengrh of vendor name */
	char cVendorName[255]; /* Vendor name */
	int pln; /* lengrh of product name */
	char cProductName[255]; /* Product name */
	int SerialNumber; /* Serial number of Touch controller */
	int FirmwareCRC32; /* CRC32 of firmware */
	char FirmwareHash[32]; /* Hash (SHA-256) of firmware */
} TM_DEVICEINFO;

typedef struct {
	unsigned char fw[25088];

} FWFILE;

typedef struct {
	_CALLBACKFUNC cb;
	int completed;
	unsigned char resp[64];
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

void buffer_hex_dump(unsigned char*, int);
int usb_sync_transfer_set(unsigned char *, unsigned char *, int, int);
int usb_sync_transfer_get(unsigned char *, unsigned char *, int, int);
int usb_sync_transfer_set_512(unsigned char *, unsigned char *, uint32_t, int);
// used for callback
void * EPIN_Sync(void *);
int usb_sync_wake(unsigned char *, int, int);
int usb_sync_resp(unsigned char *, int);

HANDLE TM_Open(int *);
int TM_Close(libusb_device_handle *);
int TM_SetLCDBrightnessLevel(int);
int TM_GetLCDBrightnessLevel(int *);
int TM_EnableCallbackTouchPoint(_CALLBACKFUNC);
int TM_DisableCallbackTouchPoint(void);
int TM_SetBioLed (int );
int TM_GetBioLed (int *);
int TM_SetSpeakerOnOff (int);
int TM_SetSpeakerVolume (int);
int TM_SetSpeakerFreq (int);
int TM_GetSpeakerStatus (int *, int *, int *);
int TM_EnterPowerSavingMode (void);
int TM_ExitPowerSavingMode (void);
int TM_SetTpResolution (int, int);
int TM_GetTpResolution (int *, int *);
int TM_FirmwareReset (void);
int TM_Who (TM_DEVICEINFO * );
int TM_FirmwareDownload (char *, char *);