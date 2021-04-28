#pragma once

#ifdef RTSPServerDll_EXPORTS
#define RTSPServerDll_PORT __declspec(dllexport) 
#else
#define RTSPServerDll_PORT __declspec(dllimport) 
#endif 

#include <string>
#include <windows.h>

#include "../Common/definition.h"

class RTSPServerDll_PORT CRTSPServerDLL
{
public:
	CRTSPServerDLL(std::string sIP = "127.0.0.1", int nPort = 8554,
		std::string sID = "skkang", std::string sPW = "skkang", eProtocolMode mode = eProtocolMode::eRTP_UDP, int nMTU = 1452);
	~CRTSPServerDLL();


	HANDLE m_Server;
	eStatus open(eProtocolMode mode); //rtspserver create
	void close();
	int add_channel(int nChannelIndex, std::string sStreamName, void* param, CallBackSetConnCnt* func, sEncoderParam* pEncParam, int nClientMAX = 3);
	eStatus remove_channel(int nChannelIndex);
	eStatus alivecheck_channel(int nChannelIndex);
	void getURL_channel(int nChannelIndex, char* url);
	eStatus pushto(const int nChannelIndex, unsigned char* pData, int nSize, int nWidth, int nHeight, eEncoderType mode = RAW);
};
