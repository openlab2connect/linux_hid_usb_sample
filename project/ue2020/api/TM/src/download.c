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

int OpenBootloader()
{
	int rc = TM_SUCCESS;
	HANDLE hd;

	sleep(2);

	rc = libusb_init(NULL);
	if (rc < 0) {
		fprintf(stderr, "Error Initializing libusb: %s\n", libusb_error_name(rc));
		return -11;
    }

    libusb_set_debug(NULL, 3);

	hd = libusb_open_device_with_vid_pid(NULL, VENDOR_ID, BOOTLOADER_ID);
	if ( !hd ) {
		fprintf(stderr, "%s:error finding TM device\n", __func__);
		return -12;
	}

	fprintf(stderr, "%s:TM device open\n", __func__);

	rc = libusb_detach_kernel_driver(hd, 0);
	if (rc < 0 && rc != LIBUSB_ERROR_NOT_FOUND) {
		fprintf(stderr, "%s:detach kernel driver error %d\n", __func__, rc);
		return -13;
	}

	fprintf(stderr, "%s:TM device init done\n", __func__);
	
	if (hd)
		g_hd = hd;
	return rc;
}

unsigned int ccrc32(unsigned char *message, int fwsize) {
   int i, j;
   unsigned int byte, crc, mask;
   i = 0;
   crc = 0xFFFFFFFF;

   while ( i < fwsize ) {
      byte = (unsigned int)message[i]; // Get next byte.
      crc = crc ^ byte;
      for (j = 7; j >= 0; j--) {    // Do eight times.
         mask = -(crc & 1);
         crc = (crc >> 1) ^ (0xEDB88320 & mask);
      }
      i = i + 1;
   }
   return ~crc;
}

unsigned int CalFWCRC32(FILE *ffw)
{
	size_t count;
	FWFILE *ffile = (FWFILE *)malloc(sizeof(FWFILE));
	unsigned int checksum;

	count = fread(ffile->fw, sizeof(unsigned char), sizeof(FWFILE), ffw);

	fprintf(stderr, "file read count %zu %ld\n", count,  sizeof(FWFILE));

	checksum = ccrc32(ffile->fw, sizeof(FWFILE));

	free(ffile);

	return checksum;
}

// int DownloadFinish()
// { 
// 	unsigned char cmd[3+4+256];

// 	memset(cmd, 0x12, sizeof(cmd));
// 	cmd[0] = 'C';
// 	cmd[1] = '4';
// 	cmd[2] = '8';

// 	crc32 = (unsigned int *)&cmd[3];
// 	*crc32 =  checksum;

// 	fprintf(stderr, "%s: crc32 little endian %1x %1x %1x %1x\n", __func__, 
// 		cmd[3], cmd[4], cmd[5], cmd[6]);
// 	// cmd[3] = (unsigned char)(checksum & 0xff);
// 	// cmd[4] = (unsigned char)((checksum & 0xff00) >> 8);
// 	// cmd[5] = (unsigned char)((checksum & 0xff0000 ) >> 16);
// 	// cmd[6] = (unsigned char)((checksum & 0xff000000) >> 24);

// 	rc = usb_sync_transfer_set(cmd, resp, sizeof(cmd), 1);
// 	if (rc < 0)
// 		fprintf(stderr, "%s:rc %d\n", __func__, rc);

// }

int DownloadFinish(unsigned int checksum,  unsigned char * resp) {
	unsigned char cmd[3+4+256];
	int rc = 0;
	uint16_t * retcode;

	// download finish send command with
	// fw crc32 and signature
	// signature can be any number
	memset(cmd, 0x12, sizeof(cmd));
	cmd[0] = 'C';
	cmd[1] = '4';
	cmd[2] = '8';
	cmd[3] = (unsigned char)(checksum & 0xff);
	cmd[4] = (unsigned char)((checksum & 0xff00) >> 8);
	cmd[5] = (unsigned char)((checksum & 0xff0000) >> 16);
	cmd[6] = (unsigned char)((checksum & 0xff000000) >> 24);

	rc = usb_sync_transfer_set_direct(cmd, resp, sizeof(cmd), 1);
	if (rc < 0) {
		retcode = (uint16_t *)&resp[3];
		return *retcode;
	}

	return rc;
}

uint16_t DownloadFirmware(FILE * ffw, unsigned char * resp)
{
	int rc = 0;
	size_t i = 0;
	size_t count = 0;
	uint32_t address = 0;
	unsigned int checksum;
	uint16_t * retcode;
	int fw_size;

	fseek(ffw, 0L, SEEK_END);
	fw_size = ftell(ffw);
	rewind(ffw);
	unsigned char * ffile = malloc(sizeof(unsigned char)*fw_size);

	// fw read length
	count = fread(ffile, sizeof(unsigned char), sizeof(unsigned char)*fw_size, ffw);
	fprintf(stderr, "%s: fw_size %d file count %lu\n", __func__, fw_size, count);

	// fw CRC32
	checksum = ccrc32(ffile, sizeof(unsigned char)*fw_size);
	fprintf(stderr, "%s:checksum 0x%x\n", __func__, checksum);

	while (address < count) {
		rc = usb_sync_transfer_set_512(&ffile[address], resp, address, 1);
		
		if (rc < 0) {
			fprintf(stderr, "%s:rc %d\n", __func__, rc);
			break;
		}

		// dump file block 512 bytes with address
		for (i=0; i<512; i++) {
			fprintf(stderr, "%x ", ffile[address + i]);
			if ( ((i+1) >= 16) && ((i+1)%16 == 0) )
				fprintf(stderr, "\n");
		}
		
		address += 512;
		usleep(250000);
	}

	//fprintf(stderr, "%s:write all blocks done address %d\n", __func__, address);
	if (rc < 0) {
		free(ffile);
		retcode = (uint16_t *)&resp[3];
		return *retcode;
	}

	free(ffile);

	// download finish
	DownloadFinish(checksum, resp);

	return 0;
}

uint16_t CheckDownloadConfigFile(FILE * fp, unsigned char * resp)
{
	int rc;
	char * line = NULL;
	char * findstring = NULL;
	char signdate[8];
	size_t len = 0;
	ssize_t read, i;
	uint16_t * retcode;
	char cmd[] = "C4620191231";

	while ((read = getline(&line, &len, fp)) != -1) {
		printf("file return line of length %zu:\n", read);
		printf("%s\n", line);
		findstring = strstr(line, "signdate =");
		if (findstring)
			break;
	}

	for (i=0; (i+10)<(read-1); i++)
		signdate[i] = findstring[i+10];

    memcpy(&cmd[3], &signdate, sizeof(signdate));
    // declear [] add '/' in the end
    fprintf(stderr, "cmd [%s] bytes %ld\n", cmd, sizeof(cmd));

	rc = usb_sync_transfer_set((unsigned char *)&cmd, resp, sizeof(cmd)-1, 1);
	if ( rc < 0)
		fprintf(stderr, "%s: rc %d\n", __func__, rc);
	retcode = (uint16_t *)&resp[3];

	return *retcode;
}

int TM_FirmwareDownload (char *lpszConfigFWFile, char *lpszFWFile)
{
	int rc = TM_SUCCESS;
	HANDLE hd = g_hd;
	CMDBUFFER *cbuf = (CMDBUFFER *)malloc(sizeof(CMDBUFFER));
	RESPBUFFER *rbuf = (RESPBUFFER *)malloc(sizeof(RESPBUFFER));

	FILE * fp = (FILE *)lpszConfigFWFile;
	FILE * ffw = (FILE *)lpszFWFile;
	uint16_t retcode;

	if (hd == NULL)
		return TM_DEVICE_NO_OPEN;
	if (cbuf == NULL || rbuf == NULL || fp == NULL || ffw == NULL)
		return TM_FAIL;

	retcode = CheckDownloadConfigFile(fp, rbuf->resp);
	if (retcode != 0x0000)
		goto done;

	TM_DisableCallbackTouchPoint();
	sleep(1);

	// now fw jump to bootloader
	// product id is 0x0040
	libusb_release_interface(hd, 0);
	if (hd)
		libusb_close(hd);
	libusb_exit(NULL);

	rc = OpenBootloader();
	if (rc < 0)
		return rc;

	retcode  = DownloadFirmware(ffw, rbuf->resp);
	if (retcode != 0x0000)
		goto done;

	//libusb_release_interface(hd, 0);
	sleep(1);

done:
	free(cbuf);
	free(rbuf);

	if (retcode != 0x0000) {
		if (retcode == 0x0004)
			rc = TM_INVALID_DATE_TIME;
		if (retcode == 0x0001)
			rc = TM_INVALID_PARAMETER;
		if (retcode == 0x0006)
			rc = TM_INVALID_SIGNATURE;
		if (retcode == 0x000a)
			rc = TM_CRC_MISMATCH;
	}
	return rc;
}