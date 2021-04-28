#include "x264encoder.h"
#include <iostream>

using namespace std;

// default constructor
x264encoder::x264encoder(const SPEED_PRESET in_preset) 
{
	//default param set
	default_param_set(in_preset);
}
void x264encoder::setResolution(int width, int height)
{
	m_param.i_width = m_width = width;
	m_param.i_height = m_height = height;
}
void x264encoder::init(void)
{
	m_colorspace = X264_CSP_I420;

	// open encoder
	m_pEncoder = x264_encoder_open(&m_param);

	// allocate in/out picture structures..
	x264_picture_alloc(&m_pic_in, X264_CSP_I420, m_width, m_height);
	//x264_picture_alloc(&m_pic_out, X264_CSP_I420, m_width, m_height);
}

void x264encoder::default_param_set(const SPEED_PRESET in_preset)
{
	// default parameter setting
	// For streaming case, "veryfast", "zerolatency" may be best parameters..
	char*pszPreset = nullptr;
	switch(in_preset) {
	case Preset_Superfast:
		pszPreset = "superfast";
		break;
	case Preset_Ultrafast:
		pszPreset = "ultrafast";
		break;
	default:
		pszPreset = "veryfast";
		break;
	}

	x264_param_default_preset(&m_param, pszPreset, "zerolatency");

	m_param.i_threads = 0;
	m_param.i_width = 176;
	m_param.i_height = 144;
	m_param.i_fps_num = DEFAULT_FPS;
	m_param.i_fps_den = 1;
    m_param.i_csp = X264_CSP_I420;
	// Intra refresh:
	m_param.i_keyint_max = DEFAULT_FPS;
	m_param.b_intra_refresh = 0; // To use GOP size.. disable intra refresh
	//Rate control:
	m_param.rc.i_rc_method = X264_RC_CRF;
	m_param.rc.f_rf_constant = 25;
	m_param.rc.f_rf_constant_max = 35;
    //m_param.rc.b_stat_write = 1; //x264_2pass.log create
	//For streaming:
	m_param.b_repeat_headers = 1;
	m_param.b_annexb = 1;
	//x264_param_apply_profile(&m_param, "baseline");

    m_param.b_sliced_threads = 0;
}

x264encoder::~x264encoder()
{
	x264_picture_clean(&m_pic_in);
	//x264_picture_clean(&m_pic_out);

	memset((char*)&m_pic_in, 0, sizeof(m_pic_in));
	//memset((char*)&m_pic_out, 0, sizeof(m_pic_out)); 

	x264_encoder_close(m_pEncoder);
}


// --- Encoder function warpper of x264 ------
int x264encoder::encode_headers(x264_nal_t **pp_nal, int *pi_nal)
{
	return x264_encoder_headers(m_pEncoder, pp_nal, pi_nal);
}
int x264encoder::encode_oneframe(x264_nal_t **pp_nal, int *pi_nal, int64_t* p_pts)
{
	int ret;
	ret = x264_encoder_encode(m_pEncoder, pp_nal, pi_nal, &m_pic_in, &m_pic_out);
	*p_pts = m_pic_out.i_pts;
	return ret;
}

// only support I420 color space
void x264encoder::prepare_oneframe(uint8_t* in_ptr, int64_t pts)
{
	m_pic_in.i_pts = pts; // set input pts..
	memcpy(m_pic_in.img.plane[0], in_ptr, m_width*m_height*3/2);
}

// Profile
int x264encoder::setProfile(PROFILE profile)
{
	switch (profile) {
	case Profile_Baseline:
		x264_param_apply_profile(&m_param, "baseline");
		break;

	case Profile_Main:
		x264_param_apply_profile(&m_param, "main");
		break;

	case Profile_High:
		x264_param_apply_profile(&m_param, "high");
		break;

	default:
		return FAILURE;
	};

	return SUCCESS;
}

// GOP mode
void x264encoder::setGOPMode(GOPMODE gopmode)
{
	switch(gopmode) {
	case GopMode_Closed:
		m_param.b_open_gop = 0;
		break;
	case GopMode_Open:
		m_param.b_open_gop = 1;
		break;
	}
}

// GOP size
void x264encoder::setGOPSize(int gopsize)
{
	m_param.i_keyint_max = gopsize;
	m_param.i_keyint_min = gopsize;
	m_param.i_scenecut_threshold = 0; // disable scene cut threshold
}

// Entroy mode
void x264encoder::setEntropyMode(ENTROPYMODE mode)
{
	switch(mode) {
	case EntropyMode_CAVLC :
		m_param.b_cabac = 0;
		break;
	case EntropyMode_CABAC:
		m_param.b_cabac = 1;
		break;
	};
}

// Bitrate
void x264encoder::setBitrate(int bitrate) // bitrate in kbps
{
	m_param.rc.i_bitrate = bitrate;
}

// RC mode
void x264encoder::setRCMode(RCMODE mode)
{
	switch(mode){
	case RCMode_VBR:
		m_param.rc.i_rc_method = X264_RC_CRF; // quality based
		break;
	case RCMode_CBR:
		m_param.rc.i_rc_method = X264_RC_ABR; // 
		break;
	};
}

void x264encoder::setBFrameLength(int blen)
{
	m_param.i_bframe = blen;
}

void x264encoder::setFPS(int num)
{
}
void x264encoder::setFPS(FRAMERATEMODE fp_mode)
{
	switch(fp_mode){
	case FrameRate_23_98 :
		setFPS(24000,1001);
		break;
	case FrameRate_29_97:
		setFPS(30000,1001);
		break;
	case FrameRate_59_94:
		setFPS(60000,1001);
		break;
	};
}
void x264encoder::setFPS(int num, int den)
{
	m_param.i_fps_num = num;
	m_param.i_fps_den = den;
}

void x264encoder::intraRefresh(void)
{
	x264_encoder_intra_refresh(m_pEncoder);
	printf("intra refreshed\n");
}

void x264encoder::printEncParameters(void)
{
	// Profile
	// Resolution
	// GOP mode
	// GOP size
	// Entropy mode
	// B-frame size
	// Frame format
	// Rate control method
	// Target bitrate
	// FPS
}
