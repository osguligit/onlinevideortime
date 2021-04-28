#include "CNvEnc.h"
#include <cuda_runtime_api.h>

#include <process.h>

//#include "../../UNSVideoSenderDll/RTSPLiveConfig.h"

//simplelogger::Logger *logger = nullptr;// simplelogger::LoggerFactory::CreateConsoleLogger();

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

CNvEnc::CNvEnc(void* pParam, CallBackEncoded* func)
{
	m_pParam = pParam;
	m_func = func;

	m_pQueue = nullptr;
	m_eGotData = nullptr;
	m_hThread = nullptr;

	m_sps = nullptr;
	m_pps = nullptr;
}

CNvEnc::~CNvEnc()
{

}

template<class EncoderClass>
inline void CNvEnc::F_InitializeEncoder(EncoderClass & pEnc, NvEncoderInitParam encodeCLIOptions, NV_ENC_BUFFER_FORMAT eFormat, sEncConfig* _encConfig)
{
	NV_ENC_INITIALIZE_PARAMS initializeParams = { NV_ENC_INITIALIZE_PARAMS_VER };
	NV_ENC_CONFIG encodeConfig = { NV_ENC_CONFIG_VER };

	if (_encConfig)
	{
		memcpy(&m_encConig, _encConfig, sizeof(sEncConfig));
		encodeConfig.gopLength = m_encConig.gopSize;
	}
	initializeParams.encodeConfig = &encodeConfig;
	pEnc->CreateDefaultEncoderParams(&initializeParams, encodeCLIOptions.GetEncodeGUID(), encodeCLIOptions.GetPresetGUID(), encodeCLIOptions.GetTuningInfo());

	//set encode params
	if (_encConfig)
	{
		initializeParams.frameRateNum = m_encConig.frameRate;
		initializeParams.maxEncodeWidth = m_encConig.width;
		initializeParams.maxEncodeHeight = m_encConig.height;

		if (m_encConig.profile == 0)
			initializeParams.encodeConfig->profileGUID = NV_ENC_H264_PROFILE_BASELINE_GUID;
		else if (m_encConig.profile == 1)
			initializeParams.encodeConfig->profileGUID = NV_ENC_H264_PROFILE_MAIN_GUID;
		else if (m_encConig.profile == 2)
			initializeParams.encodeConfig->profileGUID = NV_ENC_H264_PROFILE_HIGH_GUID;

		if (m_encConig.RateControlMode == 0)
			initializeParams.encodeConfig->rcParams.rateControlMode = NV_ENC_PARAMS_RC_CBR;
		else if (m_encConig.RateControlMode == 1)
			initializeParams.encodeConfig->rcParams.rateControlMode = NV_ENC_PARAMS_RC_VBR;

		initializeParams.encodeConfig->rcParams.maxBitRate = m_encConig.bitRate;

		if (m_encConig.EntropyMode == 0)
			initializeParams.encodeConfig->encodeCodecConfig.h264Config.entropyCodingMode = NV_ENC_H264_ENTROPY_CODING_MODE_CAVLC;
		else if (m_encConig.EntropyMode == 1)
			initializeParams.encodeConfig->encodeCodecConfig.h264Config.entropyCodingMode = NV_ENC_H264_ENTROPY_CODING_MODE_CABAC;
	}

	encodeCLIOptions.SetInitParams(&initializeParams, eFormat);

	pEnc->CreateEncoder(&initializeParams);
}


int CNvEnc::createEncoder(int iGpu, int nWidth, int nHeight, int nFps, char * inputFormat, sEncConfig* _encConfig)
{
	std::ostringstream oss;

	//set value member variables
	m_iGpu = iGpu;
	m_nWidth = nWidth;
	m_nHeight = nHeight;
	m_nFps = nFps;
	strcpy(m_InputFormat, inputFormat);

	//set GPU device
	cudaSetDevice(m_iGpu);
	ck(cuInit(0));
	int nGpu = 0;
	ck(cuDeviceGetCount(&nGpu));
	if (iGpu < 0 || iGpu >= nGpu)
	{
		std::cout << "GPU ordinal out of range. Should be within [" << 0 << ", " << nGpu - 1 << "]" << std::endl;
		return -1;
	}
	ck(cuDeviceGet(&cuDevice, iGpu));
	//char szDeviceName[80];
	//ck(cuDeviceGetName(szDeviceName, sizeof(szDeviceName), cuDevice));
	//std::cout << "GPU in use: " << szDeviceName << std::endl;	
	ck(cuCtxCreate(&cuContextEnc, 0, cuDevice));

	//set output stream format
	std::vector<std::string> vszFileFormatName =
	{
		"iyuv", "nv12", "yv12", "yuv444", "p010", "yuv444p16", "bgra", "bgra10", "ayuv", "abgr", "abgr10"
	};
	NV_ENC_BUFFER_FORMAT aFormat[] =
	{
		NV_ENC_BUFFER_FORMAT_IYUV,
		NV_ENC_BUFFER_FORMAT_NV12,
		NV_ENC_BUFFER_FORMAT_YV12,
		NV_ENC_BUFFER_FORMAT_YUV444,
		NV_ENC_BUFFER_FORMAT_YUV420_10BIT,
		NV_ENC_BUFFER_FORMAT_YUV444_10BIT,
		NV_ENC_BUFFER_FORMAT_ARGB,
		NV_ENC_BUFFER_FORMAT_ARGB10,
		NV_ENC_BUFFER_FORMAT_AYUV,
		NV_ENC_BUFFER_FORMAT_ABGR,
		NV_ENC_BUFFER_FORMAT_ABGR10,
	};
	auto it = std::find(vszFileFormatName.begin(), vszFileFormatName.end(), m_InputFormat);
	if (it == vszFileFormatName.end())
	{
		return -2;
	}
	eFormat = aFormat[it - vszFileFormatName.begin()];

	oss << " ";
	encodeCLIOptions = NvEncoderInitParam(oss.str().c_str());

	//Encoder create
	//pEnc = new NvEncoderOutputInVidMemCuda(cuContext, m_nWidth, m_nHeight, eFormat);
	pEnc = new NvEncoderCuda(cuContextEnc, nWidth, nHeight, eFormat);
	F_InitializeEncoder(pEnc, encodeCLIOptions, eFormat, _encConfig);

#ifdef USE_ENCODING_THREAD
	m_pQueue = new CDataQueue(30);
	m_bRun = TRUE;
	if (!m_eGotData)
	{
		m_eGotData = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	}
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, Thread_Encoding, (void*)this, 0, NULL);
#endif

	return 0;
}

int CNvEnc::encodeFrame(unsigned char * pInput, int nInputSize, unsigned char * pStream, int* nStreamSize, __int64 pts)
{
#ifdef USE_ENCODING_THREAD
	CData* pData = new CData(0, nInputSize, pInput, pts);
	if (m_pQueue)
		m_pQueue->push(pData);
#else
	cudaSetDevice(m_iGpu);
	std::vector<std::vector<uint8_t>> vPacket;
	if (pInput)
	{
		const NvEncInputFrame* encoderInputFrame = pEnc->GetNextInputFrame();
		NvEncoderCuda::CopyToDeviceFrame(cuContextEnc, pInput, 0, (CUdeviceptr)encoderInputFrame->inputPtr,
			(int)encoderInputFrame->pitch,
			pEnc->GetEncodeWidth(),
			pEnc->GetEncodeHeight(),
			CU_MEMORYTYPE_HOST,
			encoderInputFrame->bufferFormat,
			encoderInputFrame->chromaOffsets,
			encoderInputFrame->numChromaPlanes);

		pEnc->EncodeFrame(vPacket);

		for (std::vector<uint8_t> &stream : vPacket)
		{
			*nStreamSize = (int)stream.size();
			memcpy(pStream, reinterpret_cast<unsigned char*>(stream.data()), *nStreamSize);
		}
	}

	vPacket.clear();
#endif

	return 0;
}

int CNvEnc::deleteEncoder()
{
	m_bRun = FALSE;
	if (m_hThread)
	{
		DWORD exitCode = 0;
		GetExitCodeThread(m_hThread, &exitCode);
		while (exitCode == STILL_ACTIVE)
		{
			TerminateThread(m_hThread, 0);

			GetExitCodeThread(m_hThread, &exitCode);
			Sleep(300);
		}

		CloseHandle(m_hThread);
		m_hThread = nullptr;
	}

	if (m_eGotData)
	{
		CloseHandle(m_eGotData);
		m_eGotData = nullptr;
	}

	if (m_pQueue)
	{
		delete m_pQueue;
		m_pQueue = nullptr;
	}

	if (pEnc)
	{
		pEnc->DestroyEncoder();
		delete pEnc;
		pEnc = nullptr;
	}

	SAFE_DELETE(m_sps);
	SAFE_DELETE(m_pps);

	if (cuContextEnc)
		ck(cuCtxDestroy(cuContextEnc));

	return 0;
}

unsigned __stdcall CNvEnc::Thread_Encoding(void * pParam)
{
	CNvEnc *pThis = (CNvEnc*)pParam;
	pThis->Thread_Encoding_Main();
	return 0;
}

void CNvEnc::Thread_Encoding_Main()
{
	cudaSetDevice(m_iGpu);
	while (m_bRun)
	{
		DWORD ret = WaitForSingleObject(m_eGotData, 100);
		if ((ret == WAIT_OBJECT_0 | ret == WAIT_TIMEOUT) && m_pQueue && m_pQueue->size() > 0)
		{
			CData* pData = m_pQueue->pop();
			std::vector<std::vector<uint8_t>> vPacket;

			if (pData)
			{
				const NvEncInputFrame* encoderInputFrame = pEnc->GetNextInputFrame();
				NvEncoderCuda::CopyToDeviceFrame(cuContextEnc, pData->m_data, 0, (CUdeviceptr)encoderInputFrame->inputPtr,
					(int)encoderInputFrame->pitch,
					pEnc->GetEncodeWidth(),
					pEnc->GetEncodeHeight(),
					CU_MEMORYTYPE_HOST,
					encoderInputFrame->bufferFormat,
					encoderInputFrame->chromaOffsets,
					encoderInputFrame->numChromaPlanes);

				pEnc->EncodeFrame(vPacket);

				for (int i = 0; i < vPacket.size(); i++)
				{
					std::vector<uint8_t> &stream = vPacket.at(i);
					CData* pEncoded = new CData(0, (int)stream.size(), stream.data(), pData->m_pts);
					if (m_pParam && m_func && pEncoded->m_size > 0)
					{
						{
							FILE* fh264 = nullptr;
							fopen_s(&fh264, "encoded_nv.h264", "ab+");
							if (fh264)
							{
								fwrite(pEncoded->m_data, sizeof(unsigned char), pEncoded->m_size, fh264);
								fclose(fh264);
							}
						}

						m_func(m_pParam, pEncoded);
					}

					delete pEncoded;
				}

				delete pData;
			}

			vPacket.clear();
		}
	}
}
