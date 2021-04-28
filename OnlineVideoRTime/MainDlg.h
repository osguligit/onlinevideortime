
// MainDlg.h : header file
//

#pragma once

#define THREAD_TERMINATE_MSG WM_APP  + 1000

#include <list>

//Encoder Settings
#include "../Common/definition.h"

#include <opencv2/opencv_modules.hpp>
#include <opencv2/core.hpp>
#include <opencv2/core/opengl.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#pragma comment(lib, "D:/Library/opencv-3.4.9/build/install/x64/vc15/lib/opencv_core349.lib")
#pragma comment(lib, "D:/Library/opencv-3.4.9/build/install/x64/vc15/lib/opencv_highgui349.lib")
#pragma comment(lib, "D:/Library/opencv-3.4.9/build/install/x64/vc15/lib/opencv_imgproc349.lib")
#pragma comment(lib, "D:/Library/opencv-3.4.9/build/install/x64/vc15/lib/opencv_imgcodecs349.lib")
#pragma comment(lib, "D:/Library/opencv-3.4.9/build/install/x64/vc15/lib/opencv_videoio349.lib")
using namespace::cv;

//for cuda codecs
#ifdef NVIDIA
#include <opencv2/cudacodec.hpp>
#pragma comment(lib, "D:/Library/opencv-3.4.9/build/install/x64/vc15/lib/opencv_cudev349.lib")
#pragma comment(lib, "D:/Library/opencv-3.4.9/build/install/x64/vc15/lib/opencv_cudaarithm349.lib")
#pragma comment(lib, "D:/Library/opencv-3.4.9/build/install/x64/vc15/lib/opencv_cudabgsegm349.lib")
#pragma comment(lib, "D:/Library/opencv-3.4.9/build/install/x64/vc15/lib/opencv_cudacodec349.lib")
#pragma comment(lib, "D:/Library/opencv-3.4.9/build/install/x64/vc15/lib/opencv_cudafeatures2d349.lib")
#pragma comment(lib, "D:/Library/opencv-3.4.9/build/install/x64/vc15/lib/opencv_cudafilters349.lib")
#pragma comment(lib, "D:/Library/opencv-3.4.9/build/install/x64/vc15/lib/opencv_cudaimgproc349.lib")
#pragma comment(lib, "D:/Library/opencv-3.4.9/build/install/x64/vc15/lib/opencv_cudalegacy349.lib")
#pragma comment(lib, "D:/Library/opencv-3.4.9/build/install/x64/vc15/lib/opencv_cudaobjdetect349.lib")
#pragma comment(lib, "D:/Library/opencv-3.4.9/build/install/x64/vc15/lib/opencv_cudaoptflow349.lib")
#pragma comment(lib, "D:/Library/opencv-3.4.9/build/install/x64/vc15/lib/opencv_cudastereo349.lib")
#pragma comment(lib, "D:/Library/opencv-3.4.9/build/install/x64/vc15/lib/opencv_cudawarping349.lib")
#pragma comment(lib, "D:/Library/opencv-3.4.9/build/install/x64/vc15/lib/opencv_dnn349.lib")

#include <cuda.h>
#include <cuda_runtime_api.h>
#include <opencv2/cudawarping.hpp>
#include <opencv2/cudaarithm.hpp>
#include <opencv2/core/cuda.hpp>
#include <nvtx3/nvToolsExt.h>
#pragma comment(lib, "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v10.0/lib/x64/cublas.lib")
#pragma comment(lib, "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v10.0/lib/x64/curand.lib")
#pragma comment(lib, "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v10.0/lib/x64/cudart.lib")

#include "../NvDecDll/AppDecode/AppDecD3D/NvDecDll.h"
#pragma comment(lib, "../x64/Release/NvDecDll.lib")
#endif

////
//RTSPSending Tutorial 1. 
//RTSPServerDLL.lib import to project
#pragma comment(lib, "../x64/Release/RTSPServerDLL.lib")//../x64/Release/ can be change own project setting
////

////
//RTSPSending Tutorial 2. 
//Module header inlucde
#include "../RTSPServerDLL/RTSPServerDLL.h"
////

////
//RTSPSending Tutorial 2-1. 
//When Use This Project's External Encoder, de-comment "//#define ENCODER_EXTERNAL"
//#define ENCODER_EXTERNAL
#ifdef ENCODER_EXTERNAL
#include "../RTSPServerDLL/CVideoEncoder.h"
#ifdef NVIDIA
#pragma comment(lib, "../x64/Release/NvEncDll.lib")
#endif
#endif
////

#include "../RTSPClientDLL/RTSPClientDLL.h"
#pragma comment(lib, "../x64/Release/RTSPClientDLL.lib")

#include "../Common/AutoLock.h"

// CMainDlg dialog
class CMainDlg : public CDialogEx
{
	// Construction
public:
	CMainDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MAIN_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnClose();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	void SetupForDynamicLayout();

	CListCtrl m_list;
	void ListCntrlInit();
	void Writelog(CString log);

	CEdit m_editUrl;
	CButton m_btnConnect;
	CStatic m_pic;
	HDC     m_dc;
	afx_msg void OnBnClickedBtnConnect();

	CString m_sUrl;
	bool m_isConnect;

#ifdef NVIDIA
	CNvDecDll* m_hNvDec;
#else
	VideoCapture* m_cap;
	int m_ifps;
#endif
	HANDLE m_hThread;
	bool m_isRun;
	static unsigned __stdcall Thread_ShowDecoded(void * pParam);

	bool m_urlValid = FALSE;
	int m_decWidth;
	int m_decHeight;
	bool m_isFirst;
	float m_fps = -1.0f;
	HANDLE m_eDecode;
	HANDLE m_eRunDecode;
	clock_t m_time_decode;
	HANDLE m_semaphore_dec;
	int m_nFrameIndex;
	int m_nCallcnt;
#if GPU_CPU_COPY_MODE == 1
	std::list<cv::Mat> m_DecList;
#else
	std::list<cv::cuda::GpuMat> m_DecList;
#endif
	static void callbackfunc_decoded(void * pParam, int width, int height, unsigned char* data, float fps, int nframeIndex);
	void terminatethread();
	afx_msg LRESULT OnThreadTerminate(WPARAM wParam, LPARAM lParam);


	// Sendings ----
	CButton m_radioX264;
	CButton m_radioNvEnc;
	CEdit m_editRtspURL;
	afx_msg void OnBnClickedRadioEncoderx264();
	afx_msg void OnBnClickedRadioEncoderNv();

	////
	//RTSPSending Tutorial 3. 
	//Sending Module Object, Encoder Parametter Object define
	CRTSPServerDLL* m_RTSPSender;
	sEncoderParam* m_pEncParam;
	////
	int	m_channelIndex = 5;//have to unique number
	BOOL m_bRtspConnChanged;
	BOOL m_bIsConn;
	int m_nRtspConnCnt;

	////
	//RTSPSending Tutorial 4. 
	//Callback Function define, it called when Channel Stream URL has set and connection changed
	static void callbackfunc_SetConnCnt(void * pParam, bool isConnect, int nConnCnt, char* log);
	////

	BOOL m_bRTSPSending;
	CButton m_btnRtspSending;
	afx_msg void OnBnClickedBtnRtspsending();

	//HANDLE m_hSemaphore;
	HANDLE m_eDoSend;
#ifdef ENCODER_EXTERNAL
	CVideoEncoder* m_pEncoder;
#endif
	clock_t m_tSending;
	std::list<cv::Mat> m_EncList;
	//cv::Mat m_frameEnc;//input of encoder
	HANDLE m_hThreadSender;
	static unsigned __stdcall Thread_Sending(void * pParam);
	void Sending();

	CRTSPClientDll* pClient;
	HANDLE m_sema;
	static void callbackfunc_received(void* pParam, int width, int hegith, const char* codec, unsigned char* data, int data_size, bool IdrReceive, __int64 pts, float fps);
};


inline int WideToMultiByte(const wchar_t *src, char* dest)
{
	int len = ::WideCharToMultiByte(CP_ACP, 0, src, -1, dest, 0, nullptr, nullptr);
	return ::WideCharToMultiByte(CP_ACP, 0, src, -1, dest, len, nullptr, nullptr);
}

inline int MultiToWide(const char* src, wchar_t *dest)
{
	int len = ::MultiByteToWideChar(CP_ACP, 0, src, -1, dest, 0);
	return ::MultiByteToWideChar(CP_ACP, 0, src, -1, dest, len);
}