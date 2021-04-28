#pragma once
//AppDecD3D.cpp Âü°í

#include <cuda.h>
#include <iostream>
#include "NvDecoder/NvDecoder.h"
#include "../Utils/NvCodecUtils.h"
#include "../Utils/FFmpegDemuxer.h"
#include "FramePresenterD3D9.h"
#include "FramePresenterD3D11.h"
#include "../Common/AppDecUtils.h"
#include "../Utils/ColorSpace.h"


class CNvDecH264RGBA
{
	//main
	int m_iGpu;
	CUdevice cuDevice = 0;
	char szDeviceName[80];
	CUcontext cuContext = NULL;

	//NvDecD3D
	//FFmpegDemuxer* demuxer;// (szInFilePath);
	//NvDecoder dec;// (cuContext, true, FFmpeg2NvCodecId(demuxer.GetVideoCodec()));
	//FramePresenterD3D11 presenter;// (cuContext, demuxer.GetWidth(), demuxer.GetHeight());

	int nVideoBytes = 0, nFrameReturned = 0, nFrame = 0;
	uint8_t *pVideo = NULL;
	uint8_t	**ppFrame;

	uint8_t* pFrame_enc;//need recieved encoded data queue
	CUdeviceptr dpFrame;// gpu divice memroy, BGR32 decoded data = 0;
	uint8_t* pFrame_dec;// copy to ram, 

	CNvDecH264RGBA(int _iGpu);
	~CNvDecH264RGBA();


	void CNvDecH264RGBA_Decode();
	void CNvDecH264RGBA_NV12toRGBA();
};