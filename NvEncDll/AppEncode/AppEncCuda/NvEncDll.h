#pragma once

#ifdef	NvEncDll_EXPORT
#	define NvEncDll_PORT  __declspec(dllexport)   // export DLL information
#else
#	define NvEncDll_PORT  __declspec(dllimport)   // import DLL information
#endif 

#define MY_CONVENTION __stdcall

#include "EncoderConfig.h"
#include "../../../Common/definition.h"

class NvEncDll_PORT NvEncDll
{
public:
	NvEncDll();
	~NvEncDll();
	void* m_encoder;

	void createEncoder(int iGpu, int nWidth, int nHeight, int nFps, char* inputFormat, sEncConfig* _encConfig, void* pParam = nullptr, CallBackEncoded* func = nullptr);
	void encodeFrame(unsigned char * input, int nInputSize, unsigned char* stream, int* nStreamSize, __int64 pts);
	void deleteEncoder();
};