// UNSSavePicture.cpp : Defines the exported functions for the DLL application.
//

#include <stdio.h>
#include <windows.h>
#include <stdlib.h>

//#pragma warning(error:4706) 

#include "RTSPClientDLL.h"

#include "CRTSPClientMain.h"

CRTSPClientDll::CRTSPClientDll()
{
	m_pRtspClientMain = nullptr;
}

CRTSPClientDll::~CRTSPClientDll()
{
	close();
}

int CRTSPClientDll::open(int cam_index, char * url, void * pParam, CallBackReceived * func, unsigned char * data)
{
	int ret = -1;

	CRTSPClientMain* pRtspClientMain = new CRTSPClientMain(cam_index, url, pParam, func, data);
	if (pRtspClientMain)
	{
		pRtspClientMain->open();

		m_pRtspClientMain = (void*)(pRtspClientMain);
		ret == 0;
	}

	return ret;
}

int CRTSPClientDll::close()
{
	if (m_pRtspClientMain == nullptr) return -1;

	CRTSPClientMain* pRtspClientMain = reinterpret_cast<CRTSPClientMain*> (m_pRtspClientMain);
	if (pRtspClientMain)
	{
		pRtspClientMain->close();

		delete pRtspClientMain;
		pRtspClientMain = nullptr;
		m_pRtspClientMain = nullptr;
	}

	return 0;
}