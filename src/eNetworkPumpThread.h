/*
 * eDemuxPumpThread.h
 *
 *  Created on: 2013. 9. 12.
 *      Author: kos
 */

#ifndef ENETWORKPUMPTHREAD_H_
#define ENETWORKPUMPTHREAD_H_

#include "uThread.h"
//-------------------------------------------------------------------------------

class eNetworkPumpThread : public uThread
{
private:
	bool mTermFlag, mHeaderEnable;
	int mDeviceFd, mOutputFileFd;

protected:
	void Run();
	void Terminate();
public:
	eNetworkPumpThread(int aDeviceFd, int aOutputFileFd = 0, bool aHeaderEnable=true);
	virtual ~eNetworkPumpThread();
};
//-------------------------------------------------------------------------------

#endif /* ENETWORKPUMPTHREAD_H_ */
