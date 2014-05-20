/*
 * eTransCodingDevice.cpp
 *
 *  Created on: 2013. 11. 3.
 *      Author: kos
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "ePreDefine.h"
#include "eTransCodingDevice.h"
//-------------------------------------------------------------------------------

#ifdef DEBUG_LOG
//#undef LOG
//#define LOG(X,...) { do{}while(0); }
#endif

#define IOCTL_OPCODE_SET_VPID   1
#define IOCTL_OPCODE_SET_APID   2
#define IOCTL_OPCODE_SET_PMTPID 3
#define IOCTL_OPCODE_START_TRANSCODING 100
#define IOCTL_OPCODE_STOP_TRANSCODING  200

static int gMaxDeviceCount = 0;
//-------------------------------------------------------------------------------

int eTransCodingDevice::GetMaxDeviceCount()
{
	if(gMaxDeviceCount == 0) {
		char cmdlinepath[256] = {0};
		DIR* d = opendir("/dev");
		if (d != 0) {
			struct dirent* de;
			while ((de = readdir(d)) != 0) {
				if (strncmp("bcm_enc", de->d_name, 7) == 0) {
					gMaxDeviceCount++;
				}
			}
			closedir(d);
		}
	}
	return gMaxDeviceCount;
}
//-------------------------------------------------------------------------------

eTransCodingDevice::eTransCodingDevice()
	: mDeviceFd(0)
{
}
//-------------------------------------------------------------------------------

eTransCodingDevice::~eTransCodingDevice()
{
	Close();
}
//-------------------------------------------------------------------------------

bool eTransCodingDevice::Open()
{
	char path[16] = {0};
	int maxcnt = eTransCodingDevice::GetMaxDeviceCount();
	for (int i = 0; i < maxcnt; ++i) {
		sprintf(path, "/dev/bcm_enc%d", i);
		mDeviceFd = open(path, O_RDWR);
		if(mDeviceFd > 0) break;
	}
	//mDeviceFd = open("/dev/bcm_enc0", O_RDWR);
	if(mDeviceFd <= 0) {
#ifdef DEBUG_LOG
		LOG("transcoding device open failed (%s)(%s : %d).", path, strerror(errno), errno);
#endif
		mDeviceFd = 0;
		return false;
	}
#ifdef DEBUG_LOG
	LOG("transcoding device open ok. [%s]", path);
#endif
	return true;
}
//-------------------------------------------------------------------------------

void eTransCodingDevice::Close()
{
	StopTranscoding();
	if(mDeviceFd > 0) {
		close(mDeviceFd);
#ifdef DEBUG_LOG
		LOG("transcoding device closed. [%d]", mDeviceFd);
#endif
	}
	mDeviceFd = 0;
}
//-------------------------------------------------------------------------------

int eTransCodingDevice::GetDeviceFd()
{
	return mDeviceFd;
}
//-------------------------------------------------------------------------------

bool eTransCodingDevice::SetStreamPid(int aVideoPid, int aAudioPid)
{
	if(aVideoPid > 0) {
		if(ioctl(mDeviceFd, IOCTL_OPCODE_SET_VPID, aVideoPid) < 0) {
#ifdef DEBUG_LOG
			LOG("setting stream video pid failed.");
#endif
			return false;
		}
	}
	if(aAudioPid > 0) {
		if(ioctl(mDeviceFd, IOCTL_OPCODE_SET_APID, aAudioPid) < 0) {
#ifdef DEBUG_LOG
			LOG("setting stream audio pid failed.");
#endif
			return false;
		}
	}
#ifdef DEBUG_LOG
	LOG("setting stream pid ok.");
#endif
	return true;
}
//-------------------------------------------------------------------------------

bool eTransCodingDevice::SetStreamPid(int aVideoPid, int aAudioPid, int aPmtPid)
{
	if(aVideoPid > 0) {
		if(ioctl(mDeviceFd, IOCTL_OPCODE_SET_VPID, aVideoPid) < 0) {
#ifdef DEBUG_LOG
			LOG("setting stream video pid failed.");
#endif
			return false;
		}
	}
	if(aAudioPid > 0) {
		if(ioctl(mDeviceFd, IOCTL_OPCODE_SET_APID, aAudioPid) < 0) {
#ifdef DEBUG_LOG
			LOG("setting stream audio pid failed.");
#endif
			return false;
		}
	}
	if(aPmtPid > 0) {
		if(ioctl(mDeviceFd, IOCTL_OPCODE_SET_PMTPID, aPmtPid) < 0) {
#ifdef DEBUG_LOG
			LOG("setting stream pmt pid failed.");
#endif
			return false;
		}
	}
#ifdef DEBUG_LOG
	LOG("setting stream pid ok.");
#endif
	return true;
}
//-------------------------------------------------------------------------------

bool eTransCodingDevice::StartTranscoding()
{
	if(ioctl(mDeviceFd, IOCTL_OPCODE_START_TRANSCODING, 0) < 0) {
#ifdef DEBUG_LOG
	LOG("start transcoding failed.");
#endif
		return false;
	}
#ifdef DEBUG_LOG
	LOG("start transcoding ok.");
#endif
	return true;
}
//-------------------------------------------------------------------------------

void eTransCodingDevice::StopTranscoding()
{
	ioctl(mDeviceFd, IOCTL_OPCODE_STOP_TRANSCODING, 0);
}
//-------------------------------------------------------------------------------
