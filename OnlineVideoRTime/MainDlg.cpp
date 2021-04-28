
// MainDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Main.h"
#include "MainDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define GPU_CPU_COPY_MODE	3
#define PADDING_CONSTANT4	4
#define PADDING_CONSTANT32	32
#define PADDING_CONSTANT	PADDING_CONSTANT4

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CMainDlg dialog



CMainDlg::CMainDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_MAIN_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMainDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_URL, m_editUrl);
	DDX_Control(pDX, IDC_BTN_CONNECT, m_btnConnect);
	DDX_Control(pDX, IDC_PICTURE, m_pic);
	DDX_Control(pDX, IDC_BTN_RTSPSENDING, m_btnRtspSending);
	DDX_Control(pDX, IDC_RADIO_EncoderX264, m_radioX264);
	DDX_Control(pDX, IDC_RADIO_ENCODER_NV, m_radioNvEnc);
	DDX_Control(pDX, IDC_EDIT_RTSPURL, m_editRtspURL);
	DDX_Control(pDX, IDC_LIST1, m_list);
}

BEGIN_MESSAGE_MAP(CMainDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_BTN_CONNECT, &CMainDlg::OnBnClickedBtnConnect)
	ON_BN_CLICKED(IDC_BTN_RTSPSENDING, &CMainDlg::OnBnClickedBtnRtspsending)
	ON_MESSAGE(THREAD_TERMINATE_MSG, OnThreadTerminate)
	ON_BN_CLICKED(IDC_RADIO_EncoderX264, &CMainDlg::OnBnClickedRadioEncoderx264)
	ON_BN_CLICKED(IDC_RADIO_ENCODER_NV, &CMainDlg::OnBnClickedRadioEncoderNv)
END_MESSAGE_MAP()


// CMainDlg message handlers

BOOL CMainDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	SetWindowText(L"OnlineVideoRTime");
	
	SetupForDynamicLayout();

	m_isConnect = false;

#ifdef NVIDIA
	m_hNvDec = nullptr;
#endif
	m_hThread = nullptr;
	m_eDecode = nullptr;

	m_editUrl.SetWindowTextW(L"../testVOD/bbb_sunflower_1080p_30fps_normal.mp4");
	//m_editUrl.SetWindowTextW(L"rtsp://192.168.0.xxx:554/streamname");

	m_bRTSPSending = FALSE;
#ifdef NVIDIA
	m_radioX264.SetCheck(FALSE);
	m_radioNvEnc.SetCheck(TRUE);
	//#ifdef ENCODER_EXTERNAL
	//	m_radioX264.SetCheck(TRUE);
	//	m_radioNvEnc.SetCheck(FALSE);
	//	m_radioNvEnc.EnableWindow(FALSE);
	//#endif
#else
	m_radioX264.SetCheck(TRUE);
	m_radioNvEnc.SetCheck(FALSE);
	m_radioNvEnc.EnableWindow(FALSE);
#endif
	ListCntrlInit();

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CMainDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CMainDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CMainDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CMainDlg::OnClose()
{
	// TODO: Add your message handler code here and/or call default
	if (m_isConnect)
		OnBnClickedBtnConnect();

	if (m_bRTSPSending)
		OnBnClickedBtnRtspsending();

	CDialogEx::OnClose();
}


BOOL CMainDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case VK_ESCAPE:
		case VK_RETURN:
			return TRUE;
		default:
			break;
		}
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}


void CMainDlg::SetupForDynamicLayout()
{
	this->EnableDynamicLayout();

	//// move control
	// 1. do not move
	auto move_none = CMFCDynamicLayout::MoveSettings{};

	// 2. X,Y - 100% move
	auto move_both_100 = CMFCDynamicLayout::MoveSettings{};
	move_both_100.m_nXRatio = 100;
	move_both_100.m_nYRatio = 100;

	// 3. X - 100% move
	auto move_x_100 = CMFCDynamicLayout::MoveSettings{};
	move_x_100.m_nXRatio = 100;

	// 4. Y - 100% move
	auto move_y_100 = CMFCDynamicLayout::MoveSettings{};
	move_y_100.m_nYRatio = 100;


	//// resize control
	// 1. do not control resize
	auto size_none = CMFCDynamicLayout::SizeSettings{};

	// 2. X,Y - 100% resize 
	auto size_both_100 = CMFCDynamicLayout::SizeSettings{};
	size_both_100.m_nXRatio = 100;
	size_both_100.m_nYRatio = 100;

	// 3. X - 100% resize
	auto size_x_100 = CMFCDynamicLayout::SizeSettings{};
	size_x_100.m_nXRatio = 100;

	// 4. Y - 100% resize
	auto size_y_100 = CMFCDynamicLayout::SizeSettings{};
	size_y_100.m_nYRatio = 100;


	// get pointer of Dialog's dynamic layout 
	auto manager = this->GetDynamicLayout();

	// create dynamic layout
	manager->Create(this);

	// control add to dynamic layout control
	manager->AddItem(IDC_EDIT_URL, move_none, size_x_100);
	manager->AddItem(IDC_BTN_CONNECT, move_x_100, size_none);
	manager->AddItem(IDC_PICTURE, move_none, size_both_100);
	manager->AddItem(IDC_BTN_RTSPSENDING, move_y_100, size_none);
	manager->AddItem(IDC_RADIO_EncoderX264, move_y_100, size_none);
	manager->AddItem(IDC_RADIO_ENCODER_NV, move_y_100, size_none);
	manager->AddItem(IDC_EDIT_RTSPURL, move_y_100, size_x_100);
	manager->AddItem(IDC_LIST1, move_x_100, size_y_100);
}

void CMainDlg::ListCntrlInit()
{
	m_list.ModifyStyle(LVS_TYPEMASK, LVS_REPORT);

	LV_COLUMN lvcolumn;
	lvcolumn.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
	lvcolumn.fmt = LVCFMT_CENTER;

	// Log List
	lvcolumn.fmt = LVCFMT_CENTER;
	lvcolumn.pszText = TEXT("Time");
	lvcolumn.iSubItem = 0;
	lvcolumn.cx = 80;
	m_list.InsertColumn(0, &lvcolumn);

	lvcolumn.fmt = LVCFMT_LEFT;
	lvcolumn.pszText = TEXT("log");
	lvcolumn.iSubItem = 0;
	lvcolumn.cx = 400;
	m_list.InsertColumn(1, &lvcolumn);

	DWORD dwExStyle = LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP | LVS_EX_TRACKSELECT;
	m_list.SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LPARAM(dwExStyle));

	UpdateData(FALSE);
}

void CMainDlg::Writelog(CString log)
{
	SYSTEMTIME  tCurTime;
	::GetLocalTime(&tCurTime);

	CString time;
	time.Format(L"%02d:%02d:%02d:%03d", (int)tCurTime.wHour, (int)tCurTime.wMinute, (int)tCurTime.wSecond, (int)tCurTime.wMilliseconds);

	int listnum = m_list.GetItemCount();
	m_list.InsertItem(listnum, time);
	m_list.SetItem(listnum, 1, LVFIF_TEXT, log, NULL, NULL, NULL, NULL);
}


void CMainDlg::OnBnClickedBtnConnect()
{
	// TODO: Add your control notification handler code here
	if (!m_isConnect)
	{
		m_editUrl.GetWindowTextW(m_sUrl);

		char url[MAX_PATH];
		WideToMultiByte(m_sUrl, url);
#ifdef NVIDIA
		m_hNvDec = new CNvDecDll();
		if (m_hNvDec)
		{
			if (!m_eDecode)
				m_eDecode = CreateEvent(nullptr, FALSE, FALSE, nullptr);


			if (!m_eRunDecode)
				m_eRunDecode = CreateEvent(nullptr, FALSE, FALSE, nullptr);

			if (!m_semaphore_dec)
				m_semaphore_dec = CreateSemaphore(nullptr, 1, 1, nullptr);


			int ret = m_hNvDec->start(0, 0, url, this, callbackfunc_decoded, m_eRunDecode);//0: gpu id
			if (ret >= 0) //success
			{
				m_btnConnect.SetWindowTextW(L"Disconnect");
				m_isConnect = TRUE;
			}
			else
			{
				SAFE_DELETE(m_hNvDec);
			}
		}
#else
		m_cap = new VideoCapture(url);
		if (m_cap->isOpened())
		{
			m_btnConnect.SetWindowTextW(L"Disconnect");
			m_isConnect = TRUE;

			m_ifps = (int)m_cap->get(CAP_PROP_FPS);
		}
		else
		{
			m_cap->release();
			SAFE_DELETE(m_cap);
		}
		//m_sema = CreateSemaphore(NULL, 1, 1, NULL);
		   //pClient = new CRTSPClientDll();
		   //pClient->open(m_channelIndex, url, (void*)this, callbackfunc_received, nullptr);
		   //m_btnConnect.SetWindowTextW(L"Disconnect");
		   //m_isConnect = TRUE;
#endif	

		if (m_isConnect && !m_hThread)
			m_hThread = (HANDLE)_beginthreadex(NULL, 0, Thread_ShowDecoded, (void*)this, 0, NULL);
	}
	else
	{
		m_isRun = FALSE;

#ifdef NVIDIA
		if (m_hNvDec)
		{
			m_hNvDec->stop();

			delete m_hNvDec;
			m_hNvDec = nullptr;
		}
#else
		if (m_cap->isOpened())
		{
			m_cap->release();
			SAFE_DELETE(m_cap);
		}
		//pClient->close();

		   //delete pClient;
		   //pClient = nullptr;
#endif

		if (m_hThread)// && m_urlValid)
		{
			DWORD exitCode = 0;
			GetExitCodeThread(m_hThread, &exitCode);

			while (exitCode == STILL_ACTIVE)
			{
				//TerminateThread(m_hThread, 0);

				GetExitCodeThread(m_hThread, &exitCode);
				Sleep(300);
			}

			SAFE_CLOSE(m_hThread);
		}

		SAFE_CLOSE(m_eDecode);
		SAFE_CLOSE(m_eRunDecode);
		SAFE_CLOSE(m_semaphore_dec);


		m_isConnect = FALSE;
		m_btnConnect.SetWindowTextW(L"Connect");
	}
}

unsigned __stdcall CMainDlg::Thread_ShowDecoded(void * pParam)
{
	CMainDlg* pThis = (CMainDlg*)pParam;

	BITMAPINFO* bitInfo = (BITMAPINFO*)malloc(sizeof(BITMAPINFO) + 256 * sizeof(RGBQUAD));
	pThis->m_dc = ::GetDC(pThis->m_pic.m_hWnd);
	CRect m_rect;

	SetEvent(pThis->m_eRunDecode);//NvDecDll_Decode(pThis->m_hNvDec); //

	pThis->m_isFirst = TRUE;
	pThis->m_isRun = TRUE;

	while (pThis->m_isRun)
	{
		DWORD ret = WaitForSingleObject(pThis->m_eDecode, 50);
#ifdef NVIDIA
		if (pThis->m_DecList.size() > 0)
#endif
		{
			cv::Mat frame;
#ifdef NVIDIA
			if (!pThis->m_urlValid)
			{
				pThis->m_isRun = FALSE;
				break;
			}

#if GPU_CPU_COPY_MODE == 1			
			frame = pThis->m_DecList.front();
			pThis->m_DecList.pop_front();

			bitInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bitInfo->bmiHeader.biWidth = frame.cols;
			bitInfo->bmiHeader.biHeight = -frame.rows;
			bitInfo->bmiHeader.biPlanes = 1;
			bitInfo->bmiHeader.biBitCount = 8 * 4;
			bitInfo->bmiHeader.biCompression = BI_RGB;
			bitInfo->bmiHeader.biSizeImage = frame.cols *frame.rows * 4;
			bitInfo->bmiHeader.biXPelsPerMeter = 0;
			bitInfo->bmiHeader.biYPelsPerMeter = 0;
			bitInfo->bmiHeader.biClrUsed = 0;
			bitInfo->bmiHeader.biClrImportant = 0;

			::SetStretchBltMode(pThis->m_dc, MAXSTRETCHBLTMODE);

			pThis->m_pic.GetClientRect(&m_rect);

			StretchDIBits(pThis->m_dc,
				0, 0, m_rect.right, m_rect.bottom,
				0, 0, frame.cols, frame.rows,
				frame.data, //decoded RGBA
				bitInfo, DIB_RGB_COLORS, SRCCOPY);
#else
			cv::cuda::GpuMat gpuframe = pThis->m_DecList.front();
			pThis->m_DecList.pop_front();

			frame.create(cv::Size(gpuframe.cols, gpuframe.rows), CV_8UC4);
			cudaMemcpy(frame.data, gpuframe.data, gpuframe.cols* gpuframe.rows * 4, cudaMemcpyDeviceToHost);

			bitInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bitInfo->bmiHeader.biWidth = frame.cols;
			bitInfo->bmiHeader.biHeight = -frame.rows;
			bitInfo->bmiHeader.biPlanes = 1;
			bitInfo->bmiHeader.biBitCount = 8 * 4;
			bitInfo->bmiHeader.biCompression = BI_RGB;
			bitInfo->bmiHeader.biSizeImage = frame.cols * frame.rows * 4;
			bitInfo->bmiHeader.biXPelsPerMeter = 0;
			bitInfo->bmiHeader.biYPelsPerMeter = 0;
			bitInfo->bmiHeader.biClrUsed = 0;
			bitInfo->bmiHeader.biClrImportant = 0;

			::SetStretchBltMode(pThis->m_dc, MAXSTRETCHBLTMODE);

			pThis->m_pic.GetClientRect(&m_rect);

			StretchDIBits(pThis->m_dc,
				0, 0, m_rect.right, m_rect.bottom,
				0, 0, frame.cols, frame.rows,
				frame.data, //decoded RGBA
				bitInfo, DIB_RGB_COLORS, SRCCOPY);
#endif
#else
			if (pThis->m_cap->isOpened())
			{
				if (pThis->m_cap->read(frame))
				{
					bitInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
					bitInfo->bmiHeader.biWidth = frame.cols;
					bitInfo->bmiHeader.biHeight = -frame.rows;
					bitInfo->bmiHeader.biPlanes = 1;
					bitInfo->bmiHeader.biBitCount = 8 * 3;
					bitInfo->bmiHeader.biCompression = BI_RGB;
					bitInfo->bmiHeader.biSizeImage = frame.cols * frame.rows * 3;
					bitInfo->bmiHeader.biXPelsPerMeter = 0;
					bitInfo->bmiHeader.biYPelsPerMeter = 0;
					bitInfo->bmiHeader.biClrUsed = 0;
					bitInfo->bmiHeader.biClrImportant = 0;

					::SetStretchBltMode(pThis->m_dc, MAXSTRETCHBLTMODE);

					pThis->m_pic.GetClientRect(&m_rect);

					StretchDIBits(pThis->m_dc,
						0, 0, m_rect.right, m_rect.bottom,
						0, 0, frame.cols, frame.rows,
						frame.data, //decoded RGBA
						bitInfo, DIB_RGB_COLORS, SRCCOPY);

					int interval = (int)(1000.0 / (double)pThis->m_ifps);
					cvWaitKey(interval);
					//Sleep(interval);
				}
			}
#endif
			if (pThis->m_bIsConn)
			{
				pThis->m_EncList.push_back(frame);
			}

			//{
			//	FILE* f;
			//	fopen_s(&f, "decoded.rgb", "ab+");
			//	fwrite(frame.data, frame.elemSize(), frame.total(), f);;
			//	fclose(f);
			//}

			frame.release();

			if (pThis->m_eDoSend)
				SetEvent(pThis->m_eDoSend);
		}
#ifdef NVIDIA
		if (pThis->m_hNvDec)
			pThis->m_hNvDec->decode();
#endif
	}

	::ReleaseDC(pThis->m_pic.m_hWnd, pThis->m_dc);

	free(bitInfo);

	if (!pThis->m_urlValid)
	{
		//pThis->OnBnClickedBtnConnect();
		pThis->terminatethread();
	}

	return 0;
}

void CMainDlg::callbackfunc_decoded(void * pParam, int width, int height, unsigned char* data, float fps, int nframeIndex)
{
	CMainDlg* pThis = (CMainDlg*)pParam;

#ifdef NVIDIA
	if (width == -1 && height == -1) // url not running /  there's no file
	{
		pThis->m_urlValid = FALSE;
	}
	else if (width == -2 && height == -2) //rtsp disconnected
	{
		pThis->m_urlValid = FALSE;
	}
	else
	{
		pThis->m_urlValid = TRUE;
		pThis->m_decWidth = width;
		pThis->m_decHeight = height;
		pThis->m_fps = fps;
		int dSize = pThis->m_decWidth * pThis->m_decHeight * 4;
		pThis->m_nFrameIndex = nframeIndex;

		if (data != nullptr)
		{
#if GPU_CPU_COPY_MODE == 1 //cpu			
			cv::Mat frame(pThis->m_decHeight, pThis->m_decWidth, CV_8UC4);
			if (data != nullptr && frame.data) cudaMemcpy(frame.data, (uint8_t *)data, dSize, cudaMemcpyDeviceToHost);
			pThis->m_DecList.push_back(frame);
			frame.release();
#elif GPU_CPU_COPY_MODE == 2
			cv::cuda::GpuMat frame(cv::Size(pThis->m_decWidth, pThis->m_decHeight), CV_8UC4);
			if (!frame.empty())
			{
				cudaMemcpy(frame.data, (uint8_t *)data, dSize, cudaMemcpyDeviceToDevice);
				pThis->m_DecList.push_back(frame);
				frame.release();
			}
#elif GPU_CPU_COPY_MODE == 3 
			cv::cuda::GpuMat frame;
			cv::cuda::GpuMat frame_temp;
			if (frame.empty())
			{
				int nPadding = PADDING_CONSTANT - (width % PADDING_CONSTANT);
				if (nPadding == PADDING_CONSTANT)
				{
					frame.create(cv::Size(pThis->m_decWidth, pThis->m_decHeight), CV_8UC4);
					frame.step = pThis->m_decWidth * 4;
				}
				else
				{
					if (frame_temp.empty())
					{
						frame_temp.create(cv::Size(pThis->m_decWidth, pThis->m_decHeight), CV_8UC4);
						frame_temp.step = pThis->m_decWidth * 4;
					}

					int nwidth = width + nPadding;
					frame.create(cv::Size(nwidth, pThis->m_decHeight), CV_8UC4);
					frame.step = nwidth * 4;
				}
			}
			if (data != nullptr && !frame.empty())
			{
				int nPadding = PADDING_CONSTANT - (width % PADDING_CONSTANT);
				if (nPadding == PADDING_CONSTANT)
				{
					cudaMemcpy(frame.data, (uint8_t *)data, dSize, cudaMemcpyDeviceToDevice);
				}
				else
				{
					if (frame_temp.empty())
						cudaMemcpy(frame_temp.data, (uint8_t *)data, dSize, cudaMemcpyDeviceToDevice);
					cv::cuda::copyMakeBorder(frame_temp, frame, 0, 0, 0, nPadding, cv::BORDER_CONSTANT, 0);
				}

				pThis->m_DecList.push_back(frame);

				//check copy 
				//int temp1 = pThis->g_frame.elemSize1();
				//int temp2 = pThis->g_frame.elemSize1();
				//int temp3 = pThis->g_frame.step1();

				//if (!frame_temp.empty())
				//{
				//	Mat test2;
				//	//frame_temp.download(test2);
				//	test2.create(Size(frame_temp.cols, frame_temp.rows), CV_8UC4);
				//	cudaMemcpy(test2.data, (uint8_t *)frame_temp.data, frame_temp.cols * frame_temp.rows * 4, cudaMemcpyDeviceToHost);
				//	//cvResizeWindow("g_frame_temp", frame_temp.cols / 2, frame_temp.rows / 2);
				//	//imshow("frame_temp", test2); cvWaitKey(1);
				//	test2.release();
				//}

				//Mat test3;
				//frame.download(test3);
				//cvResizeWindow("frame download", frame.cols / 2, frame.rows / 2);
				//imshow("frame download", test3); cvWaitKey(1);
				//test3.release();

				//Mat test;
				////frame.download(test);
				//test.create(Size(frame.cols, frame.rows), CV_8UC4);
				//cudaMemcpy(test.data, (uint8_t *)frame.data, frame.cols * frame.rows * 4, cudaMemcpyDeviceToHost);
				////cvResizeWindow("copyMakeBorder", frame.cols/2, frame.rows/2);
				//imshow("copyMakeBorder", test); cvWaitKey(1);
				////imwrite("test.jpg", test);
				//test.release();

				frame.release();
				frame_temp.release();
			}
#endif
		}
	}

	SetEvent(pThis->m_eDecode);
#endif
}

void CMainDlg::terminatethread()
{
	CMainDlg* pDlg = (CMainDlg*)(theApp.m_pMainWnd);
	HWND win = pDlg->GetSafeHwnd();

	PostMessage(THREAD_TERMINATE_MSG, -1, 0);
}

LRESULT CMainDlg::OnThreadTerminate(WPARAM wParam, LPARAM lParam)
{
	if (!m_isConnect)
		return -1;

#ifdef NVIDIA
	if (m_hNvDec)
	{
		m_hNvDec->stop();
		delete m_hNvDec;
		m_hNvDec = nullptr;
	}
#endif

	m_isRun = FALSE;
	if (m_hThread)//&& m_urlValid
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

	if (m_eDecode)
	{
		CloseHandle(m_eDecode);
		m_eDecode = nullptr;
	}

	if (m_eRunDecode)
	{
		CloseHandle(m_eRunDecode);
		m_eRunDecode = nullptr;
	}

	//try
	//{
	//	::ReleaseDC(m_pic.m_hWnd, m_dc);
	//}
	//catch (const std::exception& e)
	//{
	//	CString str_tmp;
	//	str_tmp.Format(_T("Problem :%s"), e.what());
	//	OutputDebugString(str_tmp);
	//}

	m_isConnect = FALSE;
	m_btnConnect.SetWindowTextW(L"Connect");

	return 0;
}

////
//RTSPSending Tutorial 4-1. 
void CMainDlg::callbackfunc_SetConnCnt(void * pParam, bool isConnect, int nConnCnt, char * log)
{
	CMainDlg* pThis = (CMainDlg*)pParam;

	pThis->m_bRtspConnChanged = TRUE;

	CString info(log);
	if (nConnCnt == -1)
		pThis->m_editRtspURL.SetWindowText(info); //created channel URL
	else if (nConnCnt == -2)
	{
		pThis->Writelog(info);
#ifdef ENCODER_EXTERNAL
		//if (pThis->m_bIsConn)
		//{
		//	if (!pThis->m_pEncoder)
		//		pThis->m_pEncoder = new CVideoEncoder(nullptr, pThis->m_pEncParam, nullptr);
		//}
		//else
		//{
		//	if (pThis->m_pEncoder)
		//	{
		//		pThis->m_pEncoder->deleteEncoder();
		//		SAFE_DELETE(pThis->m_pEncoder);
		//	}
		//}
#endif
	}
	else
	{
		pThis->m_nRtspConnCnt = nConnCnt;

		if (isConnect)
		{
			info.Format(L"[Connected] current clientcnt : %d", (int)pThis->m_nRtspConnCnt);
			//if (!pThis->m_pEncoder)
			//	pThis->m_pEncoder = new CVideoEncoder(nullptr, pThis->m_pEncParam, nullptr);
		}
		else
		{
			info.Format(L"[DisConnected] current clientcnt : %d", (int)pThis->m_nRtspConnCnt);
			//if (pThis->m_pEncoder && pThis->m_nRtspConnCnt == 0)
			//{
			//	pThis->m_pEncoder->deleteEncoder();
			//	SAFE_DELETE(pThis->m_pEncoder);
			//}
		}

		if (pThis->m_nRtspConnCnt > 0)
			pThis->m_bIsConn = TRUE;
		else
			pThis->m_bIsConn = FALSE;

		pThis->Writelog(info); // client count chaged, when write on common file do not write here, and use m_bRtspConnChanged on thread
	}
}
////

void CMainDlg::OnBnClickedRadioEncoderx264()
{
	// TODO: Add your control notification handler code here	
	m_radioNvEnc.SetCheck(FALSE);
}


void CMainDlg::OnBnClickedRadioEncoderNv()
{
	// TODO: Add your control notification handler code here
#ifdef NVIDIA
	m_radioX264.SetCheck(FALSE);
#else
	m_radioNvEnc.SetCheck(FALSE);
#endif
}

void CMainDlg::OnBnClickedBtnRtspsending()
{
	// TODO: Add your control notification handler code here
	if (!m_bRTSPSending)
	{
		if (!m_isConnect)
			OnBnClickedBtnConnect();

		m_bRTSPSending = TRUE;

		////
		//RTSPSending Tutorial 5. 
		//Sending Module Object create with RTSPServer settings
		m_RTSPSender = new CRTSPServerDLL("127.0.0.1", 554, "", "", eProtocolMode::eRTP_UDP, 1452); //set Server Info
		if (m_RTSPSender)
		{
			////
			//RTSPSending Tutorial 6. 
			//Setting Encoder parameter / even use "External Encoder", have to set "v_avgBitrate"
			m_pEncParam = new sEncoderParam();
			if (m_radioX264.GetCheck())
				m_pEncParam->v_encoder = eEncoder::x264;
			else
				m_pEncParam->v_encoder = eEncoder::nvEnc;
			m_pEncParam->v_iGpu = 0;
			m_pEncParam->v_srcMode = BGRA;
			m_pEncParam->v_fps = 15;//default			
			if(m_fps > 0.0f)
				m_pEncParam->v_fps = (int)m_fps;
			m_pEncParam->v_frameFormat = 0;
			m_pEncParam->v_speedPreset = 1;
			m_pEncParam->v_width = 1920;//1280;//
			m_pEncParam->v_height = 1080;//720;//
			m_pEncParam->v_profile = 0;
			m_pEncParam->v_avcLevel = 4.0f;
			m_pEncParam->v_rateControl = 1;
			m_pEncParam->v_avgBitrate = 6000000;//4Mbps //related by resolution have to calc adjust 
			m_pEncParam->v_maxBitrate = 6000000;
			m_pEncParam->v_entropyCoding = 0;
			m_pEncParam->v_gopMode = 0;
			m_pEncParam->v_gopSize = 15;
			m_pEncParam->v_bFrame = 0;
			//// RTSPSending Tutorial 6. end

			////
			//RTSPSending Tutorial 7. 
			//Add channel with setting stream info
			int maxClient = 2;
			int ret = m_RTSPSender->add_channel(m_channelIndex, "tester", this, callbackfunc_SetConnCnt, m_pEncParam, maxClient);
			if (ret == eStatus::fail)
			{
				CString info = L"add_channel failed";
				m_editRtspURL.SetWindowText(info);

				SAFE_DELETE(m_pEncParam);
				SAFE_DELETE(m_RTSPSender);
				return;
			}
			////

			//if (!m_hSemaphore) m_hSemaphore = CreateSemaphore(nullptr, 1, 1, nullptr);

			if (!m_eDoSend)
				m_eDoSend = CreateEvent(nullptr, FALSE, FALSE, nullptr);

#ifdef ENCODER_EXTERNAL
			if (!m_pEncoder)
				m_pEncoder = new CVideoEncoder(nullptr, m_pEncParam, nullptr);
#endif
			m_EncList.clear();

			if (!m_hThreadSender)
				m_hThreadSender = (HANDLE)_beginthreadex(NULL, 0, Thread_Sending, (void*)this, 0, NULL);

		}
		//// RTSPSending Tutorial 5. end

		m_btnRtspSending.SetWindowText(L"RTSP Stop");
		m_radioX264.EnableWindow(FALSE);
		m_radioNvEnc.EnableWindow(FALSE);
	}
	else
	{
		m_bIsConn = FALSE;
		m_bRTSPSending = FALSE;

		////
		//RTSPSending Tutorial 9. 
		//remove channel
		if (m_RTSPSender)
		{
			eStatus ret = m_RTSPSender->remove_channel(m_channelIndex);

			////
			//RTSPSending Tutorial 10. 
			//Sending Module Object delete
			SAFE_DELETE(m_RTSPSender);
			////
		}
		////



		if (m_hThreadSender)
		{
			DWORD exitCode = 0;
			GetExitCodeThread(m_hThreadSender, &exitCode);

			while (exitCode == STILL_ACTIVE)
			{
				//TerminateThread(m_hThreadSender, 0);

				GetExitCodeThread(m_hThreadSender, &exitCode);
				Sleep(300);
			}

			SAFE_CLOSE(m_hThreadSender);
		}

#ifdef ENCODER_EXTERNAL
		if (m_pEncoder)
		{
			m_pEncoder->deleteEncoder();
			SAFE_DELETE(m_pEncoder);
		}
#endif
		////
		//RTSPSending Tutorial 11.
		//delete encoder parameter
		SAFE_DELETE(m_pEncParam);
		////

		//SAFE_CLOSE(m_hSemaphore);
		SAFE_CLOSE(m_eDoSend);

		m_EncList.clear();

		m_radioX264.EnableWindow(TRUE);
#ifdef NVIDIA
		m_radioNvEnc.EnableWindow(TRUE);
		//#ifdef ENCODER_EXTERNAL
		//		m_radioX264.SetCheck(TRUE);
		//		m_radioNvEnc.SetCheck(FALSE);
		//		m_radioNvEnc.EnableWindow(FALSE);
		//#endif
#else
		m_radioNvEnc.EnableWindow(FALSE);
#endif
		m_btnRtspSending.SetWindowText(L"RTSP Sending");
	}
}

unsigned __stdcall CMainDlg::Thread_Sending(void * pParam)
{
	CMainDlg* pThis = (CMainDlg*)pParam;

	while (pThis->m_bRTSPSending)
	{
		if (pThis->m_eDoSend)
		{
			while (WaitForSingleObject(pThis->m_eDoSend, 100) == WAIT_OBJECT_0)
			{
				pThis->Sending();
			}
#ifdef ENCODER_EXTERNAL
			if (clock() - pThis->m_tSending > 5000)
			{
				if (pThis->m_pEncoder && pThis->m_pEncoder->m_bEncoderCreated)
				{
					pThis->m_pEncoder->deleteEncoder();
					pThis->m_pEncoder->m_bEncoderCreated = FALSE;
				}
			}
#endif

			if (pThis->m_bRtspConnChanged)
			{
				//write log or display connection changed info
				pThis->m_bRtspConnChanged = FALSE;
			}
		}
	}
	return 0;
}

void CMainDlg::Sending()
{
	if (m_RTSPSender && m_EncList.size() > 0 && m_bIsConn)
	{
		//CAutoLock lock(m_hSemaphore);
		cv::Mat m_frameEnc = m_EncList.front();
		m_EncList.pop_front();
		if (m_frameEnc.empty())
			return;

		////
		//RTSPSending Tutorial 8. 
		//Pushto Encoded/Raw data to RTSPSending Model
#ifdef ENCODER_EXTERNAL
		if (m_pEncoder)
		{
			if (!m_pEncoder->m_bEncoderCreated)
				m_pEncoder->createEncoder();

			sEncodedData* pEncoded = new sEncodedData();
			pEncoded->index = 0;
			if (m_pEncParam->v_encoder == eEncoder::x264)
			{
				cv::Mat NV12_frame;
				cv::cvtColor(m_frameEnc, NV12_frame, CV_RGB2YUV_YV12);
				//m_RTSPSender->pushto(m_channelIndex, NV12_frame.data, NV12_frame.total()*NV12_frame.elemSize(), NV12_frame.cols, NV12_frame.rows, RAW);
				m_pEncoder->encodeFrame(NV12_frame.data, NV12_frame.total()*NV12_frame.elemSize(), NV12_frame.cols, NV12_frame.rows, 0, pEncoded);
			}
			else
				m_pEncoder->encodeFrame(m_frameEnc.data, m_frameEnc.total()*m_frameEnc.elemSize(), m_frameEnc.cols, m_frameEnc.rows, 0, pEncoded);

			for (int i = 0; i < pEncoded->index; i++)
			{
				FILE* ftest;
				fopen_s(&ftest, "external.h264", "a+");
				fwrite(pEncoded->pData[i]->m_data, sizeof(unsigned char), pEncoded->pData[i]->m_size, ftest);
				fclose(ftest);

				m_RTSPSender->pushto(m_channelIndex, pEncoded->pData[i]->m_data, pEncoded->pData[i]->m_size, m_frameEnc.cols, m_frameEnc.rows, h264);
				SAFE_DELETE(pEncoded->pData[i]);
			}
			m_tSending = clock();
			SAFE_DELETE(pEncoded);
		}
#else
		cv::Mat rtspInput;
		if (m_frameEnc.cols != m_pEncParam->v_width || m_frameEnc.rows != m_pEncParam->v_height)
			cv::resize(m_frameEnc, rtspInput, Size(m_pEncParam->v_width, m_pEncParam->v_height));
		else
			rtspInput = m_frameEnc;

		if (m_pEncParam->v_encoder == eEncoder::x264)
		{
			cv::Mat NV12_frame;
			cv::cvtColor(rtspInput, NV12_frame, CV_RGB2YUV_YV12);
			m_RTSPSender->pushto(m_channelIndex, NV12_frame.data, NV12_frame.total()*NV12_frame.elemSize(), NV12_frame.cols, NV12_frame.rows, RAW);
		}
		else
			m_RTSPSender->pushto(m_channelIndex, rtspInput.data, rtspInput.total()*rtspInput.elemSize(), rtspInput.cols, rtspInput.rows, RAW);
#endif
		//// RTSPSending Tutorial 8. end

		m_frameEnc.release();
	}
}

void CMainDlg::callbackfunc_received(void* pParam, int width, int hegith, const char* codec, unsigned char* data, int data_size, bool IdrReceive, __int64 pts, float fps)
{
	CMainDlg* pThis = (CMainDlg*)pParam;

	//pThis->nWidth = width;
	//pThis->nHeight = hegith;
	//strcpy_s(pThis->m_codec, strlen(codec), codec);
	//sprintf(pThis->m_codec, "%s", codec);

	//pThis->nSize = data_size;
	//char* m_fReceiveBuffer = new char[data_size];
	CAutoLock(pThis->m_sema);
	if (data_size > 0 && data)
	{
		FILE* f;
		fopen_s(&f, "receive.h264", "ab+");
		fwrite(data, sizeof(unsigned char), data_size, f);
		fclose(f);

		//CRect m_rect;
		//BITMAPINFO* bitInfo = (BITMAPINFO*)malloc(sizeof(BITMAPINFO) + 256 * sizeof(RGBQUAD));
		//    
		//        bitInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		//        bitInfo->bmiHeader.biWidth = width;
		//        bitInfo->bmiHeader.biHeight = -hegith;
		//        bitInfo->bmiHeader.biPlanes = 1;
		//        bitInfo->bmiHeader.biBitCount = 8 * 4;
		//        bitInfo->bmiHeader.biCompression = BI_RGB;
		//        bitInfo->bmiHeader.biSizeImage = width * hegith * 4;
		//        bitInfo->bmiHeader.biXPelsPerMeter = 0;
		//        bitInfo->bmiHeader.biYPelsPerMeter = 0;
		//        bitInfo->bmiHeader.biClrUsed = 0;
		//        bitInfo->bmiHeader.biClrImportant = 0;

		//        ::SetStretchBltMode(pThis->m_dc, MAXSTRETCHBLTMODE);

		//        pThis->m_pic.GetClientRect(&m_rect);

		//        StretchDIBits(pThis->m_dc,
		//            0, 0, m_rect.right, m_rect.bottom,
		//            0, 0, frame.cols, frame.rows,
		//            frame.data, //decoded RGBA
		//            bitInfo, DIB_RGB_COLORS, SRCCOPY);



	}
}