#include "CRTSPClientMain.h"

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "func_declare.h"

CRTSPClientMain::CRTSPClientMain(int cam_index, char * url, void* pParam, CallBackReceived* func, unsigned char* data)
{
	m_index = cam_index;
	strcpy_s(m_url, url);

	m_scheduler = nullptr;
	m_env = nullptr;

	m_pParam = pParam;
	m_cbFunc = func;
	m_data = data;

	m_EventLoop = 0;
	m_urlOpen = -1;

	m_isRun = FALSE;
	m_Thread = nullptr;

	m_bConnected = FALSE;
}

CRTSPClientMain::~CRTSPClientMain()
{
	close();
}

void CRTSPClientMain::open()
{
	TaskScheduler*  scheduler = BasicTaskScheduler::createNew();
	UsageEnvironment* env = BasicUsageEnvironment::createNew(*scheduler);

	m_scheduler = reinterpret_cast<void*>(scheduler);
	m_env = reinterpret_cast<void*>(env);

	m_hRtspClient = openURL(*env, "RTSPClient", m_url, m_pParam, m_cbFunc, m_data, this);

	if (!m_Thread)
	{
		m_Thread = (void *)_beginthreadex(NULL, 0, Thread_DoEventLoop, (void*)this, 0, NULL);
	}
}

void CRTSPClientMain::close()
{
	TaskScheduler** scheduler = reinterpret_cast<TaskScheduler**>(&m_scheduler);
	UsageEnvironment** env = reinterpret_cast<UsageEnvironment**>(&m_env);

	m_EventLoop = 1;
	if (m_urlOpen == 0)
		m_urlOpen = 1;

	if (m_hRtspClient && m_bConnected)//teardown 
	{
		shutdownStream((RTSPClient*)m_hRtspClient, 0);
		m_hRtspClient = nullptr;
	}

	m_isRun = FALSE;
	if (m_Thread)
	{
		DWORD exitCode = 0;
		GetExitCodeThread((HANDLE)m_Thread, &exitCode);

		while (exitCode == STILL_ACTIVE)
		{
			//TerminateThread(m_Thread, 0);
			GetExitCodeThread((HANDLE)m_Thread, &exitCode);
			Sleep(300);
		}

		CloseHandle((HANDLE)m_Thread);
		m_Thread = nullptr;
	}

	if (*env)
	{
		(*env)->reclaim();
		*env = NULL;
	}

	if (*scheduler)
	{
		delete *scheduler;
		*scheduler = NULL;
	}
}

unsigned __stdcall CRTSPClientMain::Thread_DoEventLoop(void * pParam)
{
	CRTSPClientMain* pThis = (CRTSPClientMain*)pParam;
	UsageEnvironment** env = reinterpret_cast<UsageEnvironment**>(&pThis->m_env);

	(*env)->taskScheduler().doEventLoop(&pThis->m_EventLoop);

	return 0;
}
