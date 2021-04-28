#pragma once

#include <string>
#include <list>
#include <Windows.h>

#include "../Common/CDataQueue.h"
#include "../Common/definition.h"
#include "../Common/AutoLock.h"

//x264 lib
#include "../x264/x264encoder.h"

#ifdef NVIDIA
//nvenc lib
#pragma  warning(disable:4996)
#include "../NvEncDll/AppEncode/AppEncCuda/NvEncDll.h"
#pragma comment(lib, "../x64/Release/NvEncDll.lib")
#endif



struct sEncodedData
{
	CData* pData[4];
	int index;
};

class CVideoEncoder
{
public:
	CVideoEncoder(CDataQueue* queue, sEncoderParam* pEncParam, __int64* sendingPts);
	~CVideoEncoder();

	CDataQueue* m_queue;
	sEncoderParam* m_pEncoderParam;

	void createEncoder();
	void deleteEncoder();

	void encodeFrame(unsigned char* pData, int nSize, int nWidth, int nHeight, __int64 pts, sEncodedData* pEncoded = nullptr);

	HANDLE m_sema;

	int		    m_nYUVSize;
	long long	m_nFrameCount;
	long long	m_nAUCount;
	long long	m_nYuvCount;
	x264encoder* m_hEncoder;
	CData* m_sps;
	CData* m_pps;
#ifdef NVIDIA
	NvEncDll*    m_hEncoder_nv;//20200914 skkang
#endif
	__int64* m_sendingPts;

	bool m_bEncoderCreated;

	static void Callback_NvEncEncoded(void* pParam, CData* pData);
	
	int index = 0;
};
