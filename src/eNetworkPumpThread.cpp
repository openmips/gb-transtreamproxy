/*
 * eDemuxPumpThread.cpp
 *
 *  Created on: 2013. 9. 12.
 *      Author: kos
 */

#include <poll.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "ePreDefine.h"
#include "eNetworkPumpThread.h"
//-------------------------------------------------------------------------------

#ifdef DEBUG_LOG
//#undef LOG
//#define LOG(X,...) { do{}while(0); }
#endif

eNetworkPumpThread::eNetworkPumpThread(int aDeviceFd, int aOutputFileFd, bool aHeaderEnable)
	: mTermFlag(0), mDeviceFd(aDeviceFd), mOutputFileFd(aOutputFileFd), uThread("eNetworkPumpThread"), mHeaderEnable(aHeaderEnable)
{
}
//-------------------------------------------------------------------------------

eNetworkPumpThread::~eNetworkPumpThread()
{
}
//-------------------------------------------------------------------------------

void eNetworkPumpThread::Run()
{
	int rc = 0, wc = 0;
	unsigned char buffer[BUFFER_SIZE];
	struct pollfd pollevt;

	pollevt.fd      = mDeviceFd;
	pollevt.events  = POLLIN | POLLHUP;
	pollevt.revents = 0;

	mTermFlag = true;

	if (mOutputFileFd <= 0) {
		mOutputFileFd = 1;
	}

	if (mHeaderEnable) {
		const char *c = "\
HTTP/1.0 200 OK\r\n\
Connection: close\r\n\
Content-Type: video/mpeg\r\n\
Server: stream_enigma2\r\n\
\r\n";
		wc = write(mOutputFileFd, c, strlen(c));
	}
#ifdef DEBUG_LOG
	LOG("network pump start.", rc);
#endif
	while(mTermFlag) {
		rc = poll((struct pollfd*)&pollevt, 1, 500);
		if (pollevt.revents & POLLIN) {
			rc = read(mDeviceFd, buffer, BUFFER_SIZE);

			if (errno == EINTR || errno == EAGAIN || errno == EBUSY || errno == EOVERFLOW) {
#ifdef DEBUG_LOG
				LOG("(retry... errno : %d)", errno);
#endif
				continue;
			}
			wc = write(mOutputFileFd, buffer, rc);
			if(wc != rc) {
#ifdef DEBUG_LOG
				LOG("need rewrite.. rc[%d], wc[%d]", rc, wc);
#endif
			}
		} else if (pollevt.revents & POLLHUP) {
			ioctl(mDeviceFd, 200, 0);
			break;
		}
	}
#ifdef DEBUG_LOG
	LOG("network pump stoped.", rc);
#endif
	mTermFlag = false;
}
//-------------------------------------------------------------------------------

void eNetworkPumpThread::Terminate()
{
	mTermFlag = false;
}
//-------------------------------------------------------------------------------

