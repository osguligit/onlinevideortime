#ifndef	__X264_WRAPPER_H__
#define __X264_WRAPPER_H__

#include <stdint.h>

extern "C" {
#include "x264.h"
}

class x264encoder {

public:

	enum { SUCCESS = 1, FAILURE = 0 };
	enum SPEED_PRESET { Preset_Veryfast, Preset_Superfast, Preset_Ultrafast };
	enum PROFILE { Profile_Baseline , Profile_Main, Profile_High, Profile_Unsupported } ;
	enum GOPMODE{ GopMode_Closed , GopMode_Open } ;
	enum ENTROPYMODE{ EntropyMode_CAVLC, EntropyMode_CABAC } ;
	enum FRAMEFORMAT{ FrameFormat_Progressive, FrameFormat_Interlaced };
	enum FRAMERATEMODE { FrameRate_23_98, FrameRate_29_97, FrameRate_59_94 };
	enum RCMODE { RCMode_VBR , RCMode_CBR };

	x264_param_t m_param; // codec parameter
private:

	// Private parameters for internal codec structure
	x264_t* m_pEncoder; // encoder handle
	x264_picture_t m_pic_in, m_pic_out; // picture structure (i/o)

	int m_width, m_height; // resolution
	int m_colorspace; // color space

	static const int DEFAULT_FPS = 30;

	void default_param_set(const SPEED_PRESET in_preset);
public:
	// constructor
	x264encoder(const SPEED_PRESET in_preset = Preset_Veryfast);

	void init(void);

	int encode_headers(x264_nal_t **pp_nal, int *pi_nal);
	int encode_oneframe(x264_nal_t **pp_nal, int *pi_nal, int64_t* p_pts);

	void prepare_oneframe(uint8_t* in_ptr, int64_t pts);

	~x264encoder();

	// ----- new things ---
	void setDefault(void); // default setup

	void setResolution(int width, int height); // resolution

	// Profile
	int setProfile(PROFILE profile);

	// GOP mode
	void setGOPMode(GOPMODE gopmode);

	// GOP size
	void setGOPSize(int gopsize);

	// Entroy mode
	void setEntropyMode(ENTROPYMODE mode);

	// Bitrate
	void setBitrate(int bitrate); // suceess/fail

	// Frame rate setting
	// 
	void setFPS(int num); // integer fps
	void setFPS(FRAMERATEMODE fp_mode); // float fps 23.98 / 29.97 / 59.94
	void setFPS(int num, int den); // general case

	// RC mode
	void setRCMode(RCMODE mode);
	//RCMODE getRCMode(void) const;

	void setBFrameLength(int blen);

	// Interlaced/Progressive (Progressive is Default)
	void setInterlaced(void) { m_param.b_interlaced = 1; };
	void setProgressive(void) { m_param.b_interlaced = 0; };

	void intraRefresh(void);

	void printEncParameters(void);

};

#endif

