#pragma once

//#include "liveMedia.hh"
//#include "BasicUsageEnvironment.hh"
#include "RTSPClientDLL.h"

#include <process.h>

class CRTSPClientMain
{
public:
	//TaskScheduler* m_scheduler;//TaskScheduler* 
	//UsageEnvironment* m_env;//UsageEnvironment*

	void* m_scheduler;//TaskScheduler* 
	void* m_env;//UsageEnvironment*

	int m_index;
	char m_url[512];
	char m_EventLoop; //
	char m_urlOpen; //-1 init, 0 open , 1 close, 2 can't open

	void* m_pParam;
	CallBackReceived* m_cbFunc;
	unsigned char* m_data;

	void* m_hRtspClient;
	bool m_bConnected;

	CRTSPClientMain(int cam_index, char* url, void* pParam, CallBackReceived* func, unsigned char* data);
	~CRTSPClientMain();
	void open();
	void close();

	bool m_isRun;
	void* m_Thread;
	static unsigned __stdcall Thread_DoEventLoop(void * pParam);
};
