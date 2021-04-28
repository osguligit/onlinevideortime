#pragma once

#include <liveMedia.hh>
#include <BasicUsageEnvironment.hh>
#include <GroupsockHelper.hh>
#include "EncoderDeviceSource.hh"
#include "H264VideoEncoderServerMediaSubsession.hh"

#include <string>
#include <vector>
#include <process.h>
#include <time.h>

#include "../Common/definition.h"
#include "../Common/CDataQueue.h"
#include "CVideoEncoder.h"

class CRTSPServerMain
{
public:
	TaskScheduler* m_scheduler;
	UsageEnvironment* m_env;
	RTSPServer* m_rtspServer;
	

	void* m_hEClose;
	bool m_closeDone;

	void* m_pParam;
	CallBackSetConnCnt* m_cbFunc;
	unsigned char* m_fReceiveBuffer;
	unsigned char* m_fReceiveBufferCopy;

	CRTSPServerMain(int nMTU, std::string sIP, int nPort, std::string sID, std::string sPW);
	~CRTSPServerMain();

	const std::string m_sIP;
	const int m_nPort;
	const int m_nMTU;
	const std::string m_sID;
	const std::string m_sPW;

	eStatus open(eProtocolMode mode); //rtspserver create
	void close();

	char m_EventLoop;
	bool m_isRun;
	void* m_Thread;
	static unsigned __stdcall Thread_DoEventLoop(void * pParam);

	HANDLE m_shmaphore;
	class CRTSPStream;
	std::vector<CRTSPStream*> m_pChannellist;
	int add_channel(int nChannelIndex, std::string sStreamName, void* param, CallBackSetConnCnt* func, int nClientMAX, sEncoderParam* pEncParam);
	eStatus remove_channel(int nChannelIndex);
	eStatus alivecheck_channel(int nChannelIndex);
	void getURL_channel(int nChannelIndex, char* url);
	eStatus pushto(const int nChannelIndex, unsigned char* pData, int nSize, int nWidth, int nHeight, eEncoderType mode);

	class CRTSPStream
	{
	public:
		CRTSPStream(int nCamIndex, CRTSPServerMain& server, std::string sStreamName, void* param, CallBackSetConnCnt* func, int nClientMAX = 3, sEncoderParam* pEncParam = nullptr);
		~CRTSPStream();

		const int m_nChannelIndex;

		CRTSPServerMain& m_server;

		RTSPServer* m_rtspServer;
		ServerMediaSession* m_sms;
		EncoderDeviceSource* m_videoSource;
		H264VideoEncoderServerMediaSubsession* m_videoSession;


		int m_nMTU;
		const std::string m_sStreamName;
		void* m_param; //CallBackSetConnCnt object handle
		CallBackSetConnCnt* m_func;
		const int m_nClientMAX;
		sEncoderParam* m_pEncParam;
		CDataQueue* m_queue;

		int m_frameRun;

		int m_nClientCnt;//connected client of each stream
		clock_t m_tDisconnected;
		std::string m_url;

		void pushtoh264(eEncoderType mode, unsigned char* data, int size, __int64 pts);
		CVideoEncoder* m_pVidEncoder;
		void pushtoRAW(eEncoderType mode, unsigned char* data, int size, int nWidth, int nHeight, __int64 pts);

		int m_nThreadId;
		HANDLE m_hThread;
		bool m_bRun;
		static unsigned int Thread_Main(HANDLE param);
		HANDLE m_ehStart;
		int ProcessMessage();
		DWORD OnMessage(MSG* pMsg);
	};

};
