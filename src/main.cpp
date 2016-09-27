/*
 * main.cpp
 *
 *  Created on: 2013. 9. 12.
 *      Author: kos
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <poll.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <sys/types.h>

#include <vector>
#include <string>
#include <iterator>

#include "uStringTool.h"

#include "ePreDefine.h"
#include "eParser.h"
#include "eUpstreamSocket.h"
#include "eTransCodingDevice.h"
#include "eHostInfoMgr.h"

#include "eFilePumpThread.h"
#include "eDemuxPumpThread.h"
#include "eNetworkPumpThread.h"

#ifdef DEBUG_LOG
int myPid = 0;
FILE* fpLog = 0;
//#undef LOG
//#define LOG(X,...) { do{}while(0); }
#endif

#ifndef PV
#define PV "v2.2"
#endif /*PV*/

using namespace std;
//-------------------------------------------------------------------------------

eFilePumpThread*    hFilePumpThread    = 0;
eDemuxPumpThread*   hDemuxPumpThread   = 0;
eNetworkPumpThread* hNetworkPumpThread = 0;
eTransCodingDevice* hTranscodingDevice = 0;

void SigHandler(int aSigNo);
//-------------------------------------------------------------------------------

/*
GET /1:0:19:2B66:3F3:1:C00000:0:0:0: HTTP/1.1
Host: 192.168.102.177:8002
User-Agent: VLC/2.0.8 LibVLC/2.0.8
Range: bytes=0-
Connection: close
Icy-MetaData: 1

GET /file?file=/hdd/movie/20131023%201005%20-%20DW%20-%20Germany%20Today.ts HTTP/1.1
Host: 192.168.102.177:8002
User-Agent: VLC/2.0.8 LibVLC/2.0.8
Range: bytes=0-
Connection: close
Icy-MetaData: 1
*/

char gFileType = 'c';
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

int main(int argc, char** argv)
{
	signal(SIGINT, SigHandler);

	char saveFileName[256] = {0};
	if (argc > 1) {
		if (strcmp(argv[1], "-v") == 0) {
			printf("%s %s\n", argv[0], PV);
			return 0;
		}
		else if (strcmp(argv[1], "-f") == 0) {
			strcpy(saveFileName, argv[2]);
			gFileType = 'f';
		}
		else {
			printf("%s [-v | -f [FILENAME]]\n", argv[0]);
			return 0;
		}
	}

	char request[MAX_LINE_LENGTH] = {0};
	int videopid = 0, audiopid = 0, pmtid = 0;

#ifdef DEBUG_LOG
	myPid = getpid();
	fpLog = fopen("/tmp/transtreamproxy.log", "a+");
#endif

	std::string ipaddr = eHostInfoMgr::GetHostAddr();
#ifdef DEBUG_LOG
	LOG("client info : %s, device count : %d", ipaddr.c_str(), eTransCodingDevice::GetMaxDeviceCount());
#endif
	eHostInfoMgr hostmgr("tsp", eTransCodingDevice::GetMaxDeviceCount());
	if (hostmgr.Init() == false) {
		return 1;
	}
	if (hostmgr.IsExist(ipaddr) > 0) {
		hostmgr.Update(ipaddr, getpid());
	} else {
		hostmgr.Register(ipaddr, getpid());
	}

	if (!ReadRequest(request)) {
		RETURN_ERR_400();
	}
#ifdef DEBUG_LOG
	LOG("%s", request);
#endif

	if (strncmp(request, "GET /", 5)) {
		RETURN_ERR_400();
	}

	char* http = strchr(request + 5, ' ');
	if (!http || strncmp(http, " HTTP/1.", 7)) {
#ifdef DEBUG_LOG
		LOG("Not support request (%s).", http);
#endif
		RETURN_ERR_400("Not support request (%s).", http);
	}

#ifdef DEBUG_LOG
	LOG("%s", 	request + 5);
#endif

	bool isfilestream = true;
	std::string responsedata = "";
	if(strncmp(request + 4, "/file?", 6) != 0) {
		char authorization[MAX_LINE_LENGTH] = {0};
		if(eParser::Authorization(authorization)) {
			RETURN_ERR_401();
		}

		eUpstreamSocket upstreamsocket;
		if(!upstreamsocket.Connect()) {
#ifdef DEBUG_LOG
			LOG("Upstream connect failed.");
#endif
			RETURN_ERR_502("Upstream connect failed.");
		}

		if(upstreamsocket.Request(eParser::ServiceRef(request + 5, authorization), responsedata) < 0) {
#ifdef DEBUG_LOG
			LOG("Upstream request failed.");
#endif
			RETURN_ERR_502();
		}
		isfilestream = false;
	}

	eTransCodingDevice transcoding;
	if(transcoding.Open() == false) {
#ifdef DEBUG_LOG
		LOG("Open device failed.");
#endif
		return 1;//RETURN_ERR_502("Open device failed.");
	}
	hTranscodingDevice = &transcoding;

	int outputFileFd = 0;
	if (gFileType == 'f') {
		outputFileFd = open(saveFileName, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (outputFileFd <= 0) {
			outputFileFd = 0;
		}
#ifdef DEBUG_LOG
		LOG("Save File Name detected!! [%s][%d]", saveFileName, outputFileFd);
#endif
	}
	bool ispidseted = false;
	eNetworkPumpThread networkpump(transcoding.GetDeviceFd(), outputFileFd);
	hNetworkPumpThread = &networkpump;

	if(!isfilestream) {
		int demuxno = 0;
		std::string wwwauthenticate = "";
		std::vector<unsigned long> pidlist;
		ispidseted = eParser::LiveStreamPid(responsedata, pidlist, demuxno, videopid, audiopid, pmtid, wwwauthenticate);
		if(ispidseted) {
			if(transcoding.SetStreamPid(videopid, audiopid, pmtid) == false) {
#ifdef DEBUG_LOG
				LOG("Pid setting failed.");
#endif
				return 1;//RETURN_ERR_502("Pid setting failed.");
			}
		} else {
#ifdef DEBUG_LOG
			LOG("Invalid upstream response.");
#endif
			RETURN_ERR_502("Invalid upstream response.");
		}

#ifdef DEBUG_LOG
		LOG("stream pids parsing result : %d, video : %d, audio : %d, pmt : %d, pids size : [%d]", ispidseted, videopid, audiopid, pmtid, pidlist.size());
		for(int j = 0; j < pidlist.size(); ++j) {
			LOG("saved pid : [%x]", pidlist[j]);
		}
#endif

		eDemuxPumpThread demuxpump;
		if(pidlist.size() > 0) {
			if(!demuxpump.Open(demuxno)) {
#ifdef DEBUG_LOG
				LOG("Demux open failed.");
#endif
				return 1;//RETURN_ERR_502("%s", demuxpump.GetMessage().c_str());
			}
			demuxpump.SetDeviceFd(transcoding.GetDeviceFd());
			demuxpump.Start();
			hDemuxPumpThread = &demuxpump;

			if(demuxpump.GetState() < eDemuxState::stSetedFilter) {
				if(!demuxpump.SetFilter(pidlist)) {
#ifdef DEBUG_LOG
					LOG("Demux setting filter failed.");
#endif
					return 1;//RETURN_ERR_502("Demux setting filter failed.");
				}
			}
			if(!demuxpump.SetPidList(pidlist)) {
#ifdef DEBUG_LOG
				LOG("PID setting failed.");
#endif
				RETURN_ERR_502("PID setting failed.");
			}
		} else {
#ifdef DEBUG_LOG
			LOG("No found PID for selected stream.");
#endif
			return 1;//RETURN_ERR_502("No found PID for selected stream.");
		}

#ifdef NORMAL_STREAMPROXY
		demuxpump.Join();
#else
		if(transcoding.StartTranscoding()  == false) {
			return 1;//RETURN_ERR_502("Transcoding start failed.");
		}
		networkpump.Start();
		networkpump.Join();
		demuxpump.Stop();
		demuxpump.Join();
#endif
	} else {
		std::string srcfilename = "";
		eParser::FileName(request, http, srcfilename);

		ispidseted = eParser::MetaData(srcfilename, videopid, audiopid);
		if(ispidseted) {
			if(transcoding.SetStreamPid(videopid, audiopid) == false) {
#ifdef DEBUG_LOG
				LOG("No found PID for selected stream.");
#endif
				return 1;//RETURN_ERR_502("Pid setting failed.");
			}
		}
#ifdef DEBUG_LOG
		LOG("meta parsing result : %d, video : %d, audio : %d", ispidseted, videopid, audiopid);
#endif

		eFilePumpThread filepump(transcoding.GetDeviceFd());
		if(filepump.Open(srcfilename) == false) {
#ifdef DEBUG_LOG
			LOG("TS file open failed.");
#endif
			RETURN_ERR_503("TS file open failed.");
		}
		filepump.Start();
		hFilePumpThread = &filepump;

		sleep(1);
		filepump.SeekOffset(0);
		if(transcoding.StartTranscoding() == false) {
#ifdef DEBUG_LOG
			LOG("Transcoding start failed.");
#endif
			return 1;//RETURN_ERR_502("Transcoding start failed.");
		}

		networkpump.Start();
		networkpump.Join();
		filepump.Stop();
		filepump.Join();
	}
#ifdef DEBUG_LOG
	fclose(fpLog);
#endif
	return 0;
}
//-------------------------------------------------------------------------------

FILE* fp = fopen("/home/root/input.txt", "r");
char* ReadRequest(char* aRequest)
{
	if (fp <= 0) {
		fp = stdin;
	}
	return fgets(aRequest, MAX_LINE_LENGTH-1, fp);
}
//-------------------------------------------------------------------------------

void SigHandler(int aSigNo)
{
#ifdef DEBUG_LOG
	LOG("%d", aSigNo);
#endif
	switch(aSigNo) {
	case SIGINT:
#ifdef DEBUG_LOG
		LOG("SIGINT detected.");
#endif
		exit(0);
	}
}
//-------------------------------------------------------------------------------
