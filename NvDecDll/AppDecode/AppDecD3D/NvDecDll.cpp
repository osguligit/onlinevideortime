// UNSSavePicture.cpp : Defines the exported functions for the DLL application.
//

#include <stdio.h>
#include <windows.h>
#include <stdlib.h>

//#pragma warning(error:4706) 

#include "NvDecDll.h"

#include "CNvDec.h"


CNvDecDll::CNvDecDll()
{
	m_pNvDec = nullptr;
}

CNvDecDll::~CNvDecDll()
{
}

int CNvDecDll::start(int _iGpu, int cam_index, char * _url, void * _param, CallBackDecode * func, HANDLE _event)
{
	int ret = -1;

	CNvDec* pNvDec = nullptr;
	pNvDec = new CNvDec(_iGpu, cam_index, _url, _param, func, _event);

	if (pNvDec)
	{
		if (pNvDec->m_inputMode == 0 && !pNvDec->m_bFileExist)
		{
			delete pNvDec;
			pNvDec = nullptr;

			ret = -2;//there's no file
		}
		else
		{
			bool decode_ret = pNvDec->decode();
			//if (!decode_ret)
			//{
			//	pNvDec->stop();
			//	delete pNvDec;
			//	pNvDec = nullptr;
			//}
		
			ret = 0;
			m_pNvDec = reinterpret_cast <void*> (pNvDec);
		}
	}

	return ret;
}

int CNvDecDll::decode()
{
	if (m_pNvDec == nullptr) return -1;

	CNvDec* pNvDec = reinterpret_cast<CNvDec*> (m_pNvDec);
	SetEvent(pNvDec->m_event);

	return 0;
}

int CNvDecDll::stop()
{
	if (m_pNvDec == nullptr) return -1;

	CNvDec* pNvDec = reinterpret_cast<CNvDec*> (m_pNvDec);
	pNvDec->stop();
	delete pNvDec;
	//pNvDec = nullptr;

	return 0;
}

CallBackReceived * CNvDecDll::getReceiveFunc()
{
	if (m_pNvDec == nullptr) return nullptr;

	CNvDec* pNvDec = reinterpret_cast<CNvDec*> (m_pNvDec);
	return pNvDec->callbackfunc_received;
}
