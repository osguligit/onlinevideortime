#pragma once

#ifdef RTSPClientDll_EXPORTS
#define RTSPClientDll_PORT __declspec( dllexport ) 
#else
#define RTSPClientDll_PORT __declspec( dllimport ) 
#endif

#define DUMMY_SINK_RECEIVE_BUFFER_SIZE 20000000  // 20Mbps
typedef void(CallBackReceived)(void* pParam, int width, int hegith, const char* codec, unsigned char* data, int data_size, bool IdrReceive, __int64 pts, float fps);

class RTSPClientDll_PORT CRTSPClientDll
{
public:
	CRTSPClientDll();
	~CRTSPClientDll();

	void* m_pRtspClientMain;
	int open(int cam_index, char* url, void* pParam, CallBackReceived* func, unsigned char* data);
	int close();
};