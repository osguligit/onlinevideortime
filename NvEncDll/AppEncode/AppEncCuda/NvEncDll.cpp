#include "NvEncDll.h"

#include <stdio.h>
#include <windows.h>
#include <stdlib.h>

#include "CNvEnc.h"

NvEncDll::NvEncDll()
{
	m_encoder = nullptr;
}

NvEncDll::~NvEncDll()
{
}

void NvEncDll::createEncoder(int iGpu, int nWidth, int nHeight, int nFps, char * inputFormat, sEncConfig* _encConfig, void* pParam, CallBackEncoded* func)
{
	CNvEnc* pNvEnc = new CNvEnc(pParam, (CallBackEncoded*)func);
	if (pNvEnc)
	{
		if (m_encoder)
			deleteEncoder();

		//pNvEnc->test();
		pNvEnc->createEncoder(iGpu, nWidth, nHeight, nFps, inputFormat, _encConfig);
		
		m_encoder = (void*)pNvEnc;
	}	
}

void NvEncDll::encodeFrame(unsigned char * input, int nInputSize, unsigned char * stream, int* nStreamSize, __int64 pts)
{
	CNvEnc* pNvEnc = reinterpret_cast<CNvEnc*>(m_encoder);
	if (pNvEnc)
		pNvEnc->encodeFrame(input, nInputSize, stream, nStreamSize, pts);
	else
		*nStreamSize = -1;
}


void NvEncDll::deleteEncoder()
{
	if (m_encoder == nullptr)
		return;

	CNvEnc* pNvEnc = reinterpret_cast<CNvEnc*>(m_encoder);
	
	if (pNvEnc)
	{
		pNvEnc->deleteEncoder();

		delete m_encoder;
		m_encoder = nullptr;
	}
}