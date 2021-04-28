#pragma once
#include "x264encoder.h"

#include <memory>
#include <map>
#include <unordered_map>
#include <iostream>


class x264Util
{
public:
	x264Util()
	{
		mapSpeedPreset[1] = x264encoder::Preset_Superfast;
		mapSpeedPreset[2] = x264encoder::Preset_Ultrafast;
		mapSpeedPreset[0] = x264encoder::Preset_Veryfast;

		mapGopMode[1] = x264encoder::GOPMODE::GopMode_Open;
		mapGopMode[0] = x264encoder::GOPMODE::GopMode_Closed;

		mapRateControlMode[1] = x264encoder::RCMODE::RCMode_VBR;
		mapRateControlMode[0] = x264encoder::RCMODE::RCMode_CBR;

		mapEntropyMode[1] = x264encoder::ENTROPYMODE::EntropyMode_CABAC;
		mapEntropyMode[0] = x264encoder::ENTROPYMODE::EntropyMode_CAVLC;

		mapProfile[0] = x264encoder::PROFILE::Profile_Baseline;
		mapProfile[1] = x264encoder::PROFILE::Profile_Main;
		mapProfile[2] = x264encoder::PROFILE::Profile_High;
		mapProfile[3] = x264encoder::PROFILE::Profile_Unsupported;
	}

	~x264Util() {    }

	std::map<int, enum x264encoder::SPEED_PRESET> mapSpeedPreset;
	std::map<int, enum x264encoder::GOPMODE> mapGopMode;
	std::map<int, enum x264encoder::RCMODE> mapRateControlMode;
	std::map<int, enum x264encoder::ENTROPYMODE> mapEntropyMode;
	std::map<int, enum x264encoder::PROFILE> mapProfile;
};