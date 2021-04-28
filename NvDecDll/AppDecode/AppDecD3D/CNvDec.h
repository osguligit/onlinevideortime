#pragma once

#include <cuda.h>
#include "NvDecoder/NvDecoder.h"
#include "../Utils/NvCodecUtils.h"
#include "../Utils/FFmpegDemuxer.h"
#include "FramePresenterD3D9.h"
#include "FramePresenterD3D11.h"
#include "../Common/AppDecUtils.h"
#include "../Utils/ColorSpace.h"

#include "NvDecDll.h" //calback define

#include "AutoLock_NvDec.h"

#include "../../RTSPClientDLL/RTSPClientDLL.h"

class CNvDec
{
public:
	//CUcontext cuContext;
	//FFmpegDemuxer* demuxer;
	NvDecoder* dec;
	int m_iGpu;
	int m_cam_index;
	char m_url[_MAX_PATH];
	void* m_param;
	HANDLE m_event;

	int m_inputMode; //[0]file, [1]rtsp, [2]Hikivision
	bool m_bFileExist;
	float m_fps;

	clock_t m_prevTime;

	CNvDec(int igpu, int cam_index, char* url, void* param, CallBackDecode* func, HANDLE event);
	~CNvDec();
	bool init();
	bool decode();
	bool RTSPModule_Close();
	bool stop();
	CallBackDecode* cbFunc;

	HANDLE m_Thread;
	bool m_isRun;
	static unsigned __stdcall Thread_Decoded(void * pParam);

		
	CRTSPClientDll* m_RTSPClientDLL;
	HANDLE m_EventReceived;
	HANDLE m_ThreadRTSPClient;
	static unsigned __stdcall Thread_RTSPClientOpen(void * pParam);

	char m_codec[16];
	int nWidth;
	int nHeight;
	uint8_t* m_fReceiveBuffer;
	bool m_bGetNew;
	CAutoLock_NvDec* m_lock;
	int nSize;
	bool bIdrReceive;
	__int64 tPts;
	static void callbackfunc_received(void* pParam, int width, int hegith, const char* codec, unsigned char* data, int data_size, bool IdrReceive, __int64 pts, float fps);
};