#pragma once

#include "BasicUsageEnvironment.hh"

#include "RTSPClientDLL.h"
#include "CRTSPClientMain.h"
//#include "../NvCodec/AppDecode/AppDecD3D/CNvDec.h"

#ifndef GET_7FFE008CLOCK_MICROSEC
#define GET_7FFE008CLOCK_MICROSEC ((*(__int64 *)0x7FFE0008)/10)             //*(__int64 *)0x7FFE0008 //unit : 100nanosecond
#define GET_7FFE008CLOCK_MILLISEC ((*(__int64 *)0x7FFE0008)/(10*1000))      //*(__int64 *)0x7FFE0008 //unit : 100nanosecond
#define GET_7FFE008CLOCK_SEC ((*(__int64 *)0x7FFE0008)/(10*1000*1000))      //*(__int64 *)0x7FFE0008 //unit : 100nanosecond
#endif

enum nal_unit_type_e
{
	NAL_UNKNOWN = 0,
	NAL_SLICE = 1,
	NAL_SLICE_DPA = 2,
	NAL_SLICE_DPB = 3,
	NAL_SLICE_DPC = 4,
	NAL_SLICE_IDR = 5,    /* ref_idc != 0 */
	NAL_SEI = 6,    /* ref_idc == 0 */
	NAL_SPS = 7,
	NAL_PPS = 8
	/* ref_idc == 0 for 6,9,10,11,12 */
};


enum h265_nal_unit_type_e
{       
	h265_NAL_SPS = 2,
	h265_NAL_PPS = 4,
	h265_NAL_IDR = 14,
	/* ref_idc == 0 for 6,9,10,11,12 */
};

void* openURL(UsageEnvironment& env, char const* progName, char const* rtspURL, void* pParam, CallBackReceived* func, unsigned char* data, CRTSPClientMain* pRtspClientMain);
void shutdownStream(RTSPClient* rtspClient, int exitCode);