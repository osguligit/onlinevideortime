#include "stdafx.h"

#include "CRTSPServerMain.h"

#include "../Common/AutoLock.h"

#pragma comment(lib, "winmm.lib")

CRTSPServerMain::CRTSPServerMain(int nMTU, std::string sIP, int nPort, std::string sID, std::string sPW)
	: m_nMTU(nMTU), m_sIP(sIP), m_nPort(nPort), m_sID(sID), m_sPW(sPW)
{
	m_shmaphore = CreateSemaphore(NULL, 1, 1, NULL);

	m_EventLoop = FALSE;
	m_isRun = FALSE;
	m_Thread = nullptr;

	m_pChannellist.clear();

	m_pChannellist.resize(24);//24 : VAS max channel 
}

CRTSPServerMain::~CRTSPServerMain()
{
	if (m_shmaphore)
	{
		CloseHandle(m_shmaphore);
		m_shmaphore = nullptr;
	}

	std::vector<CRTSPStream*>::iterator iter = m_pChannellist.begin();
	for (; iter != m_pChannellist.end(); iter++)
	{
		delete *iter;
	}
	m_pChannellist.clear();
}

eStatus CRTSPServerMain::open(eProtocolMode mode)
{
	m_scheduler = BasicTaskScheduler::createNew();
	m_env = BasicUsageEnvironment::createNew(*m_scheduler);

	UserAuthenticationDatabase* authDB = nullptr;
	if (m_sID.length() >= 0 & m_sPW.length() >= 0)
	{
		authDB = new UserAuthenticationDatabase();
		authDB->addUserRecord(m_sID.c_str(), m_sPW.c_str());//skkang
	}

	ipv4AddressBits localIpAddr = INADDR_ANY;
	if (m_sIP.length() > 0)
	{
		localIpAddr = inet_addr(m_sIP.c_str());
	}

	// Create the RTSP server
	m_rtspServer = RTSPServer::createNew(*m_env, m_nPort, localIpAddr, authDB);
	if (m_rtspServer == NULL)
	{
		//20200428 skkang, check RTSP Client Connection Info

		OutputDebugString("[ERROR] RTSPServer Socket create failed!\r\n");

		m_env->reclaim();
		m_env = NULL;

		delete m_scheduler;
		m_scheduler = NULL;

		return eStatus::fail;
	}

	m_rtspServer->setStreamingMode((StreamingMode)mode);//tcp, udp

	if (!m_Thread)
	{
		m_Thread = (void *)_beginthreadex(NULL, 0, Thread_DoEventLoop, (void*)this, 0, NULL);
	}

	return eStatus::success;
}

void CRTSPServerMain::close()
{
	m_EventLoop = TRUE;

	{//lock range
		CAutoLock lock(m_shmaphore);

		while (m_pChannellist.size() > 0)
		{
			CRTSPStream* channel = m_pChannellist.front();

			SAFE_DELETE(channel);

			m_pChannellist.erase(m_pChannellist.begin());
		}

		m_pChannellist.clear();
	}

	Medium::close(m_rtspServer);

	if (m_env)
	{
		m_env->reclaim();
		m_env = nullptr;
	}

	SAFE_DELETE(m_scheduler);
}

unsigned __stdcall CRTSPServerMain::Thread_DoEventLoop(void * pParam)
{
	CRTSPServerMain* pThis = (CRTSPServerMain*)pParam;

	pThis->m_env->taskScheduler().doEventLoop(&pThis->m_EventLoop);

	return 0;
}

int CRTSPServerMain::add_channel(int nChannelIndex, std::string sStreamName, void * param, CallBackSetConnCnt * func, int nClientMAX, sEncoderParam* pEncParam)
{
	if (m_pChannellist.size() <= nChannelIndex)
	{
		int new_size = m_pChannellist.size() + 10;
		m_pChannellist.resize(new_size);
	}

	CRTSPStream* channel = new CRTSPStream(nChannelIndex, *this, sStreamName, param, func, nClientMAX, pEncParam);
	if (channel != nullptr)
	{
		channel->m_nMTU = m_nMTU;
		CAutoLock lock(m_shmaphore);

		//m_pChannellist.push_back(channel);
		m_pChannellist.at(nChannelIndex) = channel;
		int index = m_pChannellist.size() - 1;
		return index;
	}
	else
		return eStatus::fail;
}

eStatus CRTSPServerMain::remove_channel(int nChannelIndex)
{
	eStatus ret = fail;

	CAutoLock lock(m_shmaphore);

	if (m_pChannellist.size() > nChannelIndex)
	{
		CRTSPStream* pStream = m_pChannellist.at(nChannelIndex);
		if (pStream && pStream->m_nChannelIndex == nChannelIndex)
		{
			delete pStream;
			m_pChannellist.at(nChannelIndex) = nullptr;
			//m_pChannellist.erase(m_pChannellist.begin() + nChannelIndex);
			ret = eStatus::success;
		}
	}

	return ret;
}

eStatus CRTSPServerMain::alivecheck_channel(int nChannelIndex)
{
	if (m_pChannellist.size() < nChannelIndex)
		return fail_noChannel;

	eStatus ret = fail;

	CRTSPStream* pStream = m_pChannellist.at(nChannelIndex);
	if (pStream && pStream->m_nChannelIndex == nChannelIndex)
	{
		ret = success;
	}

	return ret;
}

void CRTSPServerMain::getURL_channel(int nChannelIndex, char * url)
{
	if (m_pChannellist.size() < nChannelIndex)
	{
		sprintf(url, "fail, noChannel");
		return;
	}

	CRTSPStream* pStream = m_pChannellist.at(nChannelIndex);
	if (pStream && pStream->m_nChannelIndex == nChannelIndex)
	{
		sprintf(url, "%s", pStream->m_url.c_str());
	}
}

eStatus CRTSPServerMain::pushto(const int nChannelIndex, unsigned char * pData, int nSize, int nWidth, int nHeight, eEncoderType mode)
{
	if (m_pChannellist.size() < nChannelIndex)
		return fail_noChannel;

	eStatus ret = fail;

	CRTSPStream* pStream = m_pChannellist.at(nChannelIndex);
	if (pStream && pStream->m_nChannelIndex == nChannelIndex)
	{
		LARGE_INTEGER frequency;
		LARGE_INTEGER current;

		QueryPerformanceFrequency(&frequency);
		QueryPerformanceCounter(&current);
		__int64 pts = (__int64)((double)current.QuadPart / (double)frequency.QuadPart)* 1000000.0f;
		if (mode == h264)//external encoder
			pStream->pushtoh264(h264, pData, nSize, pts);
		else if (mode == RAW)
			pStream->pushtoRAW(RAW, pData, nSize, nWidth, nHeight, pts);

		ret = eStatus::success;
	}
	else if (pStream == nullptr)
		ret = eStatus::fail_noChannel;

	return ret;
}

//CRTSPServerDLL::add_channel()
CRTSPServerMain::CRTSPStream::CRTSPStream(int nChannelIndex, CRTSPServerMain & server, std::string sStreamName, void * param, CallBackSetConnCnt * func, int nClientMAX, sEncoderParam* pEncParam)
	:m_server(server), m_nChannelIndex(nChannelIndex), m_sStreamName(sStreamName),
	m_param(param), m_func(func), m_nClientMAX(nClientMAX), m_frameRun(1), m_pVidEncoder(nullptr), m_nClientCnt(0)
{
	m_pEncParam = new sEncoderParam();
	memcpy(m_pEncParam, pEncParam, sizeof(sEncoderParam));

	m_ehStart = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	if (!m_hThread)
	{
		m_hThread = (void *)_beginthreadex(NULL, 0, Thread_Main, (void*)this, 0, NULL);
	}

	WaitForSingleObject(m_ehStart, INFINITE);


	ipv4AddressBits localIpAddr = INADDR_ANY;
	if (m_server.m_sIP.length() > 0)
	{
		localIpAddr = inet_addr(m_server.m_sIP.c_str());
	}

	m_queue = new CDataQueue();//encoded data storage
	m_videoSource = EncoderDeviceSource::createNew(*((UsageEnvironment*)m_server.m_env), EncoderDeviceParameters(m_queue, &m_frameRun, m_pEncParam->v_fps));


	m_videoSession = H264VideoEncoderServerMediaSubsession::createNew(*((UsageEnvironment*)m_server.m_env),
		(FramedSource**)&m_videoSource,
		TRUE);
	m_videoSession->setBitrate(pEncParam->v_avgBitrate);
	m_videoSession->setMTU(m_server.m_nMTU, m_server.m_nMTU);
	//m_videoSession->set
	//m_videoSession->setLocalAddr(localIpAddr);
	//m_videoSession->setServerAddressAndPortForSDP(localIpAddr, 8901);


	char const* descriptionString = "Real Time Streaming Protocol";
	m_sms = ServerMediaSession::createNew(*((UsageEnvironment*)m_server.m_env),
		m_sStreamName.c_str(),
		"This is Sending by Live555",
		descriptionString);
	m_sms->setMessage(&m_nThreadId, eMessage::pause, eMessage::resume, eMessage::connected, eMessage::disconnected);//GetThreadId(this)
	m_sms->setClientMAX(m_nClientMAX);
	m_sms->addSubsession(m_videoSession);


	m_rtspServer = (RTSPServer*)m_server.m_rtspServer;
	m_rtspServer->addServerMediaSession(m_sms);

	m_url = m_rtspServer->rtspURL(m_sms);
	char url[512];
	sprintf(url, "%s", m_rtspServer->rtspURL(m_sms));
	m_func(m_param, FALSE, -1, url);
}


CRTSPServerMain::CRTSPStream::~CRTSPStream()
{
	m_frameRun = 0;
	m_bRun = FALSE;

	//m_rtspServer->closeAllClientSessionsForServerMediaSession(m_sms);
	//m_rtspServer->removeServerMediaSession(m_sms);
    m_rtspServer->deleteServerMediaSession(m_sms);

	SAFE_DELETE(m_pVidEncoder);

	SAFE_DELETE(m_queue);

	SAFE_DELETE(m_pEncParam);

	SAFE_CLOSE(m_pEncParam);

	if (m_hThread)
	{
		DWORD exitCode = 0;
		GetExitCodeThread(m_hThread, &exitCode);

		int cnt = 0;
		while (exitCode == STILL_ACTIVE)
		{
			if (cnt++ == 10)
				TerminateThread(m_hThread, 0);

			GetExitCodeThread(m_hThread, &exitCode);
			Sleep(300);
		}

		CloseHandle(m_hThread);
		m_hThread = nullptr;
	}
}

void CRTSPServerMain::CRTSPStream::pushtoh264(eEncoderType mode, unsigned char* data, int size, __int64 pts)//without encoder, just push to encoded queue
{
	if (m_bRun && m_nClientCnt != 0 && size > 0)
	{
		CData* pdata = new CData(mode, size, data, pts);
		//data
		m_queue->push(pdata);
		m_videoSession->mSendingStartPTS = m_queue->getFirstPTS();
	}
}

void CRTSPServerMain::CRTSPStream::pushtoRAW(eEncoderType eEncoderType, unsigned char* data, int size, int nWidth, int nHeight, __int64 pts)
{
	if (m_bRun && m_nClientCnt != 0 && size > 0)
	{
		//to encode RAW to h264
		if (m_pVidEncoder == nullptr)
		{
			m_pVidEncoder = new CVideoEncoder(m_queue, m_pEncParam, &m_videoSession->mSendingStartPTS);
			m_func(m_param, TRUE, -2, "Client connected, createEncoder");
		}

		if (m_pVidEncoder)
		{
			m_pVidEncoder->encodeFrame(data, size, nWidth, nHeight, pts);
		}
	}
}

unsigned int CRTSPServerMain::CRTSPStream::Thread_Main(HANDLE param)
{
	CRTSPStream* pThis = (CRTSPStream*)param;

	pThis->m_bRun = TRUE;
	pThis->m_nThreadId = GetCurrentThreadId();
	if (pThis->m_ehStart)
		SetEvent(pThis->m_ehStart);

	pThis->ProcessMessage();

	return 0;
}

int CRTSPServerMain::CRTSPStream::ProcessMessage()
{
	MSG msg;
	while (m_bRun)
	{
		BOOL bGot = PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
		while (msg.message != WM_QUIT)
		{
			bGot = PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);

			if (bGot)
			{
				OnMessage(&msg);
			}
			else
				Sleep(1000);


			if (m_bRun && m_tDisconnected != 0)// && clock() - m_tDisconnected > 5000)//delete encoder and stop //)// 
			{
				SAFE_DELETE(m_pVidEncoder);
				m_tDisconnected = 0;

				m_queue->clear();

				m_func(m_param, FALSE, -2, "Client None, deleteEncoder");// during 5sec
			}
		}
	}

	return 0;
}

DWORD CRTSPServerMain::CRTSPStream::OnMessage(MSG * pMsg)
{
	DWORD	dwResult = -1;

	WPARAM	wParam = pMsg->wParam;
	LPARAM	lParam = pMsg->lParam;
	switch (pMsg->message)
	{
	case eMessage::pause:
		break;//stop encoding
	case eMessage::resume:
		break;//run encoding
	case eMessage::connected:
	{
		char log[128];		
		sprintf(log, "%s", m_sStreamName.c_str());
		m_func(m_param, TRUE, (int)wParam, log);
		m_nClientCnt = (int)wParam;
		m_tDisconnected = 0;

		sprintf(log, "[Connected] %s current clientcnt : %d\n", m_sStreamName.c_str(), (int)wParam);
		OutputDebugString(log);

		break;
	}
	case eMessage::disconnected:
	{
		char log[128];		
		sprintf(log, "%s", m_sStreamName.c_str());
		m_func(m_param, FALSE, (int)wParam, log);
		m_nClientCnt = (int)wParam;
		if (m_nClientCnt == 0)
		{
			m_tDisconnected = clock();
		}

		sprintf(log, "[Disconnected] %s current clientcnt : %d\n", m_sStreamName.c_str(), (int)wParam);
		OutputDebugString(log);

		break;
	}
	}

	return dwResult;
}
