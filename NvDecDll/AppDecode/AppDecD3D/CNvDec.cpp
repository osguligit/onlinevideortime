/*
* Copyright 2017-2018 NVIDIA Corporation.  All rights reserved.
*
* Please refer to the NVIDIA end user license agreement (EULA) associated
* with this source code for terms and conditions that govern your use of
* this software. Any use, reproduction, disclosure, or distribution of
* this software and related documentation outside the terms of the EULA
* is strictly prohibited.
*
*/

//---------------------------------------------------------------------------
//! \file AppDecD3D.cpp
//! \brief Source file for AppDecD3D sample
//!
//! This sample application illustrates the decoding of media file and display of decoded frames in a window.
//! This is done by CUDA interop with D3D(both D3D9 and D3D11).
//! For a detailed list of supported codecs on your NVIDIA GPU, refer : https://developer.nvidia.com/nvidia-video-codec-sdk#NVDECFeatures


#include "CNvDec.h"

#include <iostream>
#include <process.h>

#include <string>


//simplelogger::Logger *logger = simplelogger::LoggerFactory::CreateConsoleLogger();

/**
*   @brief Function template to decode media file pointed to by szInFilePath parameter.
		   The decoded frames are displayed by using the D3D-CUDA interop.
		   In this app FramePresenterType is either FramePresenterD3D9 or FramePresenterD3D11.
		   The presentation rate is based on per frame time stamp.
*   @param  cuContext - Handle to CUDA context
*   @param  szInFilePath - Path to file to be decoded
*   @return 0 on success
*/

CNvDec::CNvDec(int igpu, int cam_index, char* url, void* param, CallBackDecode* func, HANDLE event)
{
	strcpy(m_url, url);
	m_cam_index = cam_index;
	m_iGpu = igpu;
	m_param = param;
	cbFunc = func;

	m_event = event;//CreateEvent(nullptr, FALSE, FALSE, nullptr);// 

	dec = nullptr;

	m_isRun = FALSE;
	m_Thread = nullptr;

	m_ThreadRTSPClient = nullptr;
	m_RTSPClientDLL = nullptr;
	m_EventReceived = nullptr;
	m_fReceiveBuffer = nullptr;
	m_bGetNew = FALSE;

	m_lock = new CAutoLock_NvDec(nullptr);

	m_inputMode = 0;

	m_bFileExist = FALSE;

	init();
}

CNvDec::~CNvDec()
{
	if (dec)
	{
		delete dec;
		dec = nullptr;
	}

	if (m_lock)
	{
		delete m_lock;
		m_lock = nullptr;
	}
}

bool CNvDec::init()
{
	bool bRet = TRUE;

	std::string url(m_url);
	if (url.find("rtsp://") == 0)
	{
		m_inputMode = 1;
	}
	else if (url.find("Hikivision") == 0)
	{
		m_inputMode = 2;
	}
	else
	{
		m_inputMode = 0;

		m_bFileExist = CheckInputFile(m_url);
		bRet = m_bFileExist;
	}

	if (m_inputMode == 0 && !m_bFileExist)
		return bRet;

	ck(cuInit(0));
	int nGpu = 0;
	ck(cuDeviceGetCount(&nGpu));
	if (m_iGpu < 0 || m_iGpu >= nGpu)
	{
		std::ostringstream err;
		err << "GPU ordinal out of range. Should be within [" << 0 << ", " << nGpu - 1 << "]" << std::endl;
		throw std::invalid_argument(err.str());
	}

	return bRet;
}

bool CNvDec::decode()
{
	if (!m_ThreadRTSPClient && m_inputMode == 1)
	{
		m_EventReceived = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (!m_fReceiveBuffer)
		{
			m_fReceiveBuffer = static_cast<uint8_t*>(malloc(DUMMY_SINK_RECEIVE_BUFFER_SIZE * sizeof(uint8_t)));
		}

		m_ThreadRTSPClient = (HANDLE)_beginthreadex(NULL, 0, Thread_RTSPClientOpen, (void*)this, 0, NULL);

		int cnt = 0;
		while (TRUE)
		{
			DWORD ret = WaitForSingleObject(m_EventReceived, 1000);
			if (ret == WAIT_OBJECT_0 && nWidth >= 0 && nHeight >= 0)
				break;
			else if (ret == WAIT_OBJECT_0 && nWidth < 0 && nHeight < 0)
			{
				cbFunc(m_param, -1, -1, nullptr, -1.0f, -1);
				return FALSE;
			}
			//if (++cnt > 2)//must event get in 2 seconds
			//{
			//	cbFunc(m_param, -1, -1, nullptr, -1.0f);
			//	return FALSE;
			//}
		}

		if (strcmp(m_codec, "ConnectFail") == 0)
		{
			cbFunc(m_param, -1, -1, nullptr, -1.0f, -1);
			return FALSE;
		}
	}

	if (m_inputMode == 2)
	{
		m_EventReceived = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (!m_fReceiveBuffer)
		{
			m_fReceiveBuffer = static_cast<uint8_t*>(malloc(DUMMY_SINK_RECEIVE_BUFFER_SIZE * sizeof(uint8_t)));
		}
	}


	if (!m_Thread)
		m_Thread = (HANDLE)_beginthreadex(NULL, 0, Thread_Decoded, (void*)this, 0, NULL);

	return TRUE;
}

bool CNvDec::RTSPModule_Close()
{
	if (m_RTSPClientDLL)
	{
		m_RTSPClientDLL->close();

		delete m_RTSPClientDLL;
		m_RTSPClientDLL = nullptr;
	}

	if (m_ThreadRTSPClient)
	{
		DWORD exitCode = 0;
		GetExitCodeThread(m_ThreadRTSPClient, &exitCode);
		while (exitCode == STILL_ACTIVE)
		{
			TerminateThread(m_ThreadRTSPClient, 0);

			GetExitCodeThread(m_ThreadRTSPClient, &exitCode);
			Sleep(300);
		}

		CloseHandle(m_ThreadRTSPClient);
		m_ThreadRTSPClient = nullptr;
	}

	return TRUE;
}

bool CNvDec::stop()
{
	m_isRun = FALSE;

	//RTSPClient terminate//////////////////////////////////////////////////
	RTSPModule_Close();

	//NvDecoder terminate//////////////////////////////////////////////////
	bool isDoRun = FALSE;
	if (m_Thread)
	{
		DWORD exitCode = 0;
		GetExitCodeThread(m_Thread, &exitCode);

		while (exitCode == STILL_ACTIVE)
		{
			//TerminateThread(m_Thread, 0);
			if (!isDoRun && m_inputMode == 1)
			{
				callbackfunc_received(this, -1, -1, "Invalid", nullptr, 0, 0, 0, 0);
				isDoRun = TRUE;
			}

			GetExitCodeThread(m_Thread, &exitCode);
			Sleep(300);
		}

		CloseHandle(m_Thread);
		m_Thread = nullptr;
	}

	//RTSPClient Data Memory Release
	if (m_EventReceived)
	{
		CloseHandle(m_EventReceived);
		m_EventReceived = nullptr;
	}

	if (m_fReceiveBuffer)
	{
		free(m_fReceiveBuffer);
		m_fReceiveBuffer = nullptr;
	}

	return TRUE;
}

unsigned __stdcall CNvDec::Thread_RTSPClientOpen(void * pParam)
{
	CNvDec* pThis = (CNvDec*)pParam;

	if (pThis->m_RTSPClientDLL == nullptr)
	{
		pThis->m_RTSPClientDLL = new CRTSPClientDll();
		pThis->m_RTSPClientDLL->open(pThis->m_cam_index, pThis->m_url, (void*)pThis, callbackfunc_received, nullptr);// pThis->m_fReceiveBuffer);//
	}

	return 0;
}

void CNvDec::callbackfunc_received(void* pParam, int width, int hegith, const char* codec, unsigned char* data, int data_size, bool IdrReceive, __int64 pts, float fps)
{
	CNvDec* pThis = (CNvDec*)pParam;

	pThis->nWidth = width;
	pThis->nHeight = hegith;
	//strcpy_s(pThis->m_codec, strlen(codec), codec);
	sprintf(pThis->m_codec, "%s", codec);

	pThis->nSize = data_size;
	if (data_size > 0 && pThis->m_fReceiveBuffer)
	{
		if (pThis->m_lock->lock())
		{
			memcpy(pThis->m_fReceiveBuffer, data, data_size);
			pThis->m_fReceiveBuffer[data_size] = '\0';
		}
		pThis->m_bGetNew = TRUE;
		pThis->m_lock->unlock();
	}

	pThis->bIdrReceive = IdrReceive;

	pThis->tPts = pts;

	pThis->m_fps = fps;

	SetEvent(pThis->m_EventReceived);
}

unsigned __stdcall CNvDec::Thread_Decoded(void * pParam)
{
	CNvDec* pThis = (CNvDec*)pParam;

	cudaSetDevice(pThis->m_iGpu);

	CUdevice cuDevice = 0;
	ck(cuDeviceGet(&cuDevice, pThis->m_iGpu));

	//char szDeviceName[80];
	//ck(cuDeviceGetName(szDeviceName, sizeof(szDeviceName), cuDevice));
	//std::cout << "GPU in use: " << szDeviceName << std::endl;

	FFmpegDemuxer* demuxer = nullptr;
	CUdeviceptr dpFrame = 0;//nvdec decoded data
	int nVideoBytes = 0, nFrameReturned = 0, nFrame = 0;
	uint8_t *pVideo = NULL, **ppFrame;//ppFrame : yuv data

	FramePresenterD3D11* presenter = nullptr;
	int64_t pts, *pTimestamp;
	bool m_bFirstFrame = true;
	int64_t firstPts = 0, startTime = 0;
	LARGE_INTEGER m_Freq;
	QueryPerformanceFrequency(&m_Freq);

	bool isfirst = true;
	uint8_t* frame_data = nullptr;

	CUcontext cuContextDec = NULL;
	ck(cuCtxCreate(&cuContextDec, 0, cuDevice));//CU_CTX_SCHED_BLOCKING_SYNC

	if (pThis->m_inputMode == 0)
	{
		demuxer = new FFmpegDemuxer(pThis->m_url);
		pThis->dec = new NvDecoder(cuContextDec, true, FFmpeg2NvCodecId(demuxer->GetVideoCodec()));
		ck(cuMemAlloc(&dpFrame, demuxer->GetWidth() * demuxer->GetHeight() * 4));
		//frame_data = new uint8_t[demuxer->GetWidth() * demuxer->GetHeight() * 4];//skkang
		//presenter = new FramePresenterD3D11(cuContext, pThis->dec->GetWidth(), pThis->dec->GetHeight());
	}
	else if (pThis->m_inputMode == 1)//rtsp
	{
		if (strcmp(pThis->m_codec, "H264") == 0)
		{
			pThis->dec = new NvDecoder(cuContextDec, true, FFmpeg2NvCodecId(AV_CODEC_ID_H264));
		}
		else if (strcmp(pThis->m_codec, "H265") == 0 || strcmp(pThis->m_codec, "HEVC") == 0)
		{
			pThis->dec = new NvDecoder(cuContextDec, true, FFmpeg2NvCodecId(AV_CODEC_ID_HEVC));
		}

		//char log[128];
		//sprintf_s(log, "%s - Codec(%s)\n", pThis->m_url, pThis->m_codec);
		//OutputDebugString(log);

		ck(cuMemAlloc(&dpFrame, pThis->nWidth * pThis->nHeight * 4));
		//frame_data = new uint8_t[pThis->nWidth * pThis->nHeight * 4];
		//presenter = new FramePresenterD3D11(cuContext, pThis->nWidth, pThis->nHeight);
	}
	else if (pThis->m_inputMode == 2)//Hikivision
	{
		while (TRUE)
		{
			DWORD ret = WaitForSingleObject(pThis->m_EventReceived, 100);
			if (ret == WAIT_OBJECT_0)
				break;
		}

		if (strcmp(pThis->m_codec, "H264") == 0)
		{
			pThis->dec = new NvDecoder(cuContextDec, true, FFmpeg2NvCodecId(AV_CODEC_ID_H264));
		}
		else if (strcmp(pThis->m_codec, "H265") == 0 || strcmp(pThis->m_codec, "HEVC") == 0)
		{
			pThis->dec = new NvDecoder(cuContextDec, true, FFmpeg2NvCodecId(AV_CODEC_ID_HEVC));
		}

		//char log[128];
		//sprintf_s(log, "%s - Codec(%s)\n", pThis->m_url, pThis->m_codec);
		//OutputDebugString(log);

		ck(cuMemAlloc(&dpFrame, pThis->nWidth * pThis->nHeight * 4));
	}

	pThis->m_prevTime = 0;
	pThis->tPts = 0;

	FILE *fH264 = nullptr;
	//fopen_s(&fH264, "nvDEc_in.h264", "wb");// "out.yuv", "wb");
	int filewritecnt = 0;

	char log[128];
	int interval = 66;

	int timeout_cnt = 0;
	bool bGetNew = false;

	pThis->m_isRun = TRUE;
	while (pThis->m_isRun)
	{
		if (pThis->m_inputMode == 0)
		{
			DWORD ret = WaitForSingleObject(pThis->m_event, 30);
			//if (ret == WAIT_TIMEOUT)
			//{
			//	if (timeout_cnt++ > 10)
			//		break;
			//}

			if (ret != WAIT_OBJECT_0)
				continue;

			demuxer->Demux(&pVideo, &nVideoBytes, &pts);
			pThis->dec->Decode(pVideo, nVideoBytes, &ppFrame, &nFrameReturned, 0, &pTimestamp, pts);
			bGetNew = true;

			pThis->m_fps = pThis->dec->fps;
			if (pThis->m_fps == 0)
				pThis->m_fps = 15.0f;

			interval = floor(1000.0 / pThis->m_fps);
			interval /= 10;
			interval *= 10;
			Sleep(interval);
		}
		else if (pThis->m_inputMode == 1 || pThis->m_inputMode == 2)//rtsp(1) hikivision(2)
		{
			DWORD ret = WaitForSingleObject(pThis->m_EventReceived, 100);
			if (ret != WAIT_OBJECT_0)
			{
				if (!pThis->m_bGetNew)
				{
					if (++timeout_cnt > 10)// 100ms x 10 = 1000ms
					{
						pThis->cbFunc(pThis->m_param, -2, -2, nullptr, -1.0f, -1);
						break;
					}
				}
				continue;
			}

			timeout_cnt = 0;

			if (fH264)
			{
				fwrite(pThis->m_fReceiveBuffer, pThis->nSize, sizeof(uint8_t), fH264);
				fflush(fH264);
				filewritecnt++;

				//sprintf(log, "Decoder Input fwritecnt: %d\n ", filewritecnt);
				//OutputDebugString(log);
			}

			//ret = WaitForSingleObject(pThis->m_event, 30); //do Decoding 
			//if (ret != WAIT_OBJECT_0)
			//	continue;

			pThis->m_prevTime = clock();
			if (pThis->m_lock->lock() && pThis->m_bGetNew)
			{
				pThis->dec->Decode(pThis->m_fReceiveBuffer, pThis->nSize, &ppFrame, &nFrameReturned, 0, &pTimestamp, pThis->tPts);//
				pThis->m_bGetNew = FALSE;
				bGetNew = TRUE;
			}
			pThis->m_lock->unlock();
		}

		if (pThis->m_inputMode != 0 && !bGetNew)
		{
			continue;
		}
		bGetNew = false;

		if (!nFrame && nFrameReturned)
		{
			//LOG(INFO) << pThis->dec->GetVideoInfo();
			char log[1024];
			sprintf(log, "GetVideoInfo: %s\n ", pThis->dec->GetVideoInfo().c_str());
			OutputDebugString(log);
		}

		for (int i = 0; i < nFrameReturned; i++)
		{
			if (!pThis->m_isRun)
				break;

			if (pThis->dec->GetBitDepth() == 8)
			{
				if (pThis->dec->GetOutputFormat() == cudaVideoSurfaceFormat_YUV444)
					YUV444ToColor32<BGRA32>((uint8_t *)ppFrame[i], pThis->dec->GetWidth(), (uint8_t *)dpFrame, 4 * pThis->dec->GetWidth(), pThis->dec->GetWidth(), pThis->dec->GetHeight());
				else    // default assumed as NV12
					Nv12ToColor32<BGRA32>((uint8_t *)ppFrame[i], pThis->dec->GetWidth(), (uint8_t *)dpFrame, 4 * pThis->dec->GetWidth(), pThis->dec->GetWidth(), pThis->dec->GetHeight());
			}
			else
			{
				if (pThis->dec->GetOutputFormat() == cudaVideoSurfaceFormat_YUV444_16Bit)
					YUV444P16ToColor32<BGRA32>((uint8_t *)ppFrame[i], 2 * pThis->dec->GetWidth(), (uint8_t *)dpFrame, 4 * pThis->dec->GetWidth(), pThis->dec->GetWidth(), pThis->dec->GetHeight());
				else // default assumed as P016
					P016ToColor32<BGRA32>((uint8_t *)ppFrame[i], 2 * pThis->dec->GetWidth(), (uint8_t *)dpFrame, 4 * pThis->dec->GetWidth(), pThis->dec->GetWidth(), pThis->dec->GetHeight());
			}

			//if (isfirst)
			//{
			//	cudaMemcpy(frame_data, (uint8_t *)dpFrame, 4 * pThis->dec->GetWidth()*pThis->dec->GetHeight(), cudaMemcpyDeviceToHost);
			//	FILE *fout;
			//	fopen_s(&fout, "out.raw", "ab");// "out.yuv", "wb");
			//	fwrite(frame_data, 4 * pThis->dec->GetWidth() * pThis->dec->GetHeight(), sizeof(uint8_t), fout);
			//	fclose(fout);
			//	//isfirst = false;
			//}

			if (presenter)
			{
				LARGE_INTEGER counter;
				if (m_bFirstFrame)
				{
					firstPts = pTimestamp[i];
					QueryPerformanceCounter(&counter);
					startTime = 1000 * counter.QuadPart / m_Freq.QuadPart;
					m_bFirstFrame = false;
				}
				QueryPerformanceCounter(&counter);
				int64_t curTime = 1000 * counter.QuadPart / m_Freq.QuadPart;
				int64_t expectedRenderTime = pTimestamp[i] - firstPts + startTime;
				int64_t delay = 30;// expectedRenderTime - curTime;
				if (pTimestamp[i] == 0)
					delay = 0;
				if (delay < 0)
					continue;

				if (pThis->m_inputMode == 0)
				{
					presenter->PresentDeviceFrame((uint8_t *)dpFrame, pThis->dec->GetWidth() * 4, delay);
				}
				else if (pThis->m_inputMode == 1)//rtsp
				{
					presenter->PresentDeviceFrame((uint8_t *)dpFrame, pThis->nWidth * 4, delay);
				}
			}

			////20200506 skkang
			//cudaMemcpy(frame_data, (uint8_t *)dpFrame, 4 * pThis->dec->GetWidth()* pThis->dec->GetHeight(), cudaMemcpyDeviceToHost);
			//pThis->cbFunc(pThis->m_param, pThis->dec->GetWidth(), pThis->dec->GetHeight(), frame_data, pThis->m_fps);
			//pThis->m_fps = 15.0f;
			pThis->cbFunc(pThis->m_param, pThis->dec->GetWidth(), pThis->dec->GetHeight(), (uint8_t *)dpFrame, pThis->m_fps, nFrame + i);//fps;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
		}
		nFrame += nFrameReturned;

		if (!pThis->m_isRun)
			break;

		if (pThis->m_inputMode == 0 && nVideoBytes == 0 || pThis->m_inputMode != 0 && pThis->nSize == 0)
		{
			pThis->cbFunc(pThis->m_param, -2, -2, nullptr, 0.0f, -1);
			break;
		}

		//std::cout << "Total frame decoded: " << nFrame << std::endl;
		//sprintf(log, "Total frame decoded: %d\n ", nFrame);
		//OutputDebugString(log);
	}

	if (fH264)
	{
		fclose(fH264);

		sprintf(log, "Decoder Input fwritecnt: %d\n ", filewritecnt);
		OutputDebugString(log);
	}


	if (presenter)
		delete presenter;

	if (frame_data)
		delete (frame_data);

	if (pThis->dec)
	{
		delete pThis->dec;
		pThis->dec = nullptr;
	}

	if (demuxer)
		delete(demuxer);

	if (dpFrame)
		ck(cuMemFree(dpFrame));

	if (cuContextDec)
		ck(cuCtxDestroy(cuContextDec));

	return 0;
}