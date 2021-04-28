#pragma once

#ifdef NvDecDll_EXPORTS
#define NvDecDll_PORT __declspec( dllexport ) 
#else
#define NvDecDll_PORT __declspec( dllimport ) 
#endif

typedef void(CallBackDecode)(void* pParam, int width, int hegith, unsigned char* data, float fps, int nframeIndex);
typedef void(CallBackReceived)(void* pParam, int width, int hegith, const char* codec, unsigned char* data, int data_size, bool IdrReceive, __int64 pts, float fps);


class NvDecDll_PORT CNvDecDll
{
public:
	CNvDecDll();
	~CNvDecDll();

	void* m_pNvDec;
	int start(int _iGpu, int cam_index, char* _url, void* _param, CallBackDecode* func, HANDLE _event);
	int decode();
	int stop();
	CallBackReceived*  getReceiveFunc();
};