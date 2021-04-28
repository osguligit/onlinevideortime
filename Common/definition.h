#pragma once

#ifndef __INCLUDE_DEFINES_H_
#define __INCLUDE_DEFINES_H_

#ifndef SAFE_FREE
#define SAFE_FREE(p)	{if(p) {free(p); (p)=nullptr;}}
#endif

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)	{if(p) {delete p; (p)=nullptr;}}
#endif

#ifndef SAFE_DELETE_BUF
#define SAFE_DELETE_BUF(p)	{if(p) {delete[] p; (p)=nullptr;}}
#endif

#ifndef SAFE_CLOSE
#define SAFE_CLOSE(p)	{if(p) {CloseHandle(p); (p)=nullptr;}}
#endif

#define NVIDIA
//#define ENCODED_DATA_SAVE_AS_FILE

#include "./CDataQueue.h"
typedef void(CallBackSetConnCnt)(void* pParam, bool isConnect, int nConnCnt, char* log);
typedef void(CallBackEncoded)(void* pParam, CData* pData); //pParam : CVideoEncoder object

enum eEncoderType
{
	h264 = 0,
	h265,
	mjpeg,
	RAW
};

enum eStatus {
	fail = -1,
	success,
	fail_noChannel
};

enum eProtocolMode {
	eRTP_UDP,
	eRTP_TCP,
	eRAW_UDP
};

enum eMessage {
	pause = 0,
	resume,
	connected,
	disconnected
};

enum eEncoder {
	nvEnc = 0,
	x264,
	external
};

enum eSourceFormat {
	NV12 = 0,
	BGRA
};

typedef enum { FPS_23_98, FPS_29_97, FPS_59_94 }FPS_MODE;

typedef struct {
	char data[256];
	int len;
} dsi_t;


typedef struct
{
	int v_encoder;          //eEncoderType
	int v_iGpu;				//gpu index
	int v_srcMode;			//0:Nv12, 1:BGRA
	float v_fps;
	int v_frameFormat;      //frameFormat (0:progressive, 1:interlaced)
	int v_speedPreset;      //Preset (0:veryfast, 1:superfast, 2:ultrafast)        
	int v_width;
	int v_height;
	int v_profile;          //video profile (0: baseline, 1: main, 2: high)
	float v_avcLevel;       //4.0;
	int v_rateControl;      //0 //;rate control (0:CBR, 1:VBR, 2:CappedVBR, 3:ConstantQP)
	int v_avgBitrate;       //(1: automatical caculation)
	int v_maxBitrate;       //(1: automatical caculation)
	int v_entropyCoding;    //entroy (0:CAVLC, 1:CABAC)
	int v_gopMode;          //gop mode(0:Fixed, 1:Flexible)
	int v_gopSize;          //I-Frame sending period
	int v_bFrame;
}sEncoderParam;
#endif