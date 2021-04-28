/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// "liveMedia"
// Copyright (c) 1996-2021 Live Networks, Inc.  All rights reserved.
// A template for a MediaSource encapsulating an audio/video input device
// 
// NOTE: Sections of this code labeled "%%% TO BE WRITTEN %%%" are incomplete, and needto be written by the programmer
// (depending on the features of the particulardevice).
// C++ header

#ifndef _ENCODER_DEVICE_SOURCE_HH
#define _ENCODER_DEVICE_SOURCE_HH

#ifndef _FRAMED_SOURCE_HH
#include "FramedSource.hh"
#endif

#include "../Common/CDataQueue.h"

// The following class can be used to define specific encoder parameters
class EncoderDeviceParameters {
  //%%% TO BE WRITTEN %%%
  //skkang
public:
	CDataQueue*	m_pQueue;
	int* m_bRunning;
	float m_fps;	
	EncoderDeviceParameters(CDataQueue* pQueue, int* bRunning, float fps)
		: m_pQueue(pQueue), m_bRunning(bRunning), m_fps(fps)
	{
	}
	~EncoderDeviceParameters() {}
};

class EncoderDeviceSource: public FramedSource {
public:
  static EncoderDeviceSource* createNew(UsageEnvironment& env,
	  EncoderDeviceParameters params);

public:
  static EventTriggerId eventTriggerId;
  // Note that this is defined here to be a static class variable, because this code is intended to illustrate how to
  // encapsulate a *single* device - not a set of devices.
  // You can, however, redefine this to be a non-static member variable.

protected:
  EncoderDeviceSource(UsageEnvironment& env, EncoderDeviceParameters params);
  // called only by createNew(), or by subclass constructors
  virtual ~EncoderDeviceSource();

private:
  // redefined virtual functions:
  virtual void doGetNextFrame();
  //virtual void doStopGettingFrames(); // optional

private:
  static void deliverFrame0(void* clientData);
  void deliverFrame();

private:
  static unsigned referenceCount; // used to count how many instances of this class currently exist
  EncoderDeviceParameters fParams;

  //skkang
protected:
  unsigned fLastPlayTime;
  unsigned frameCount;

  unsigned int convertToTimestamp(int64_t pts, struct timeval* pOutTS);
  virtual int compareTime(int64_t pts);
  virtual char* getMediaType() { return (char*)"Video";}
public:
  static u_int64_t	sBaseTime;
  inline EncoderDeviceParameters* getParams() {return &fParams;}
  int index = 0;
};

#endif
