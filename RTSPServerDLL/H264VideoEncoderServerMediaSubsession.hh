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
// A 'ServerMediaSubsession' object that creates new, unicast, "RTPSink"s
// on demand, from a H264 Elementary Stream video file.
// C++ header

#ifndef _H264_VIDEO_ENCODER_SERVER_MEDIA_SUBSESSION_HH
#define _H264_VIDEO_ENCODER_SERVER_MEDIA_SUBSESSION_HH

#ifndef _FILE_SERVER_MEDIA_SUBSESSION_HH
#include "FileServerMediaSubsession.hh"
#endif

class H264VideoEncoderServerMediaSubsession : public FileServerMediaSubsession {
public:
  static H264VideoEncoderServerMediaSubsession*
  createNew(UsageEnvironment& env, FramedSource** source, Boolean reuseFirstSource);

  // Used to implement "getAuxSDPLine()":
  void checkForAuxSDPLine1();
  void afterPlayingDummy1();

protected:
	H264VideoEncoderServerMediaSubsession(UsageEnvironment& env,
		FramedSource** source, Boolean reuseFirstSource);
      // called only by createNew();
  virtual ~H264VideoEncoderServerMediaSubsession();

  void setDoneFlag() { fDoneFlag = ~0; }

protected: // redefined virtual functions
  virtual char const* getAuxSDPLine(RTPSink* rtpSink,
				    FramedSource* inputSource);
  virtual FramedSource* createNewStreamSource(unsigned clientSessionId,
					      unsigned& estBitrate);
  virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
                                    unsigned char rtpPayloadTypeIfDynamic,
				    FramedSource* inputSource);

private:
  char* fAuxSDPLine;
  char fDoneFlag; // used when setting up "fAuxSDPLine"
  RTPSink* fDummyRTPSink; // ditto

  FramedSource** mSource; //skkang
  int mBitrate = 1000000; //default 1Mbps
  typedef struct {
	  char data[300];
	  int  len;
  } dsi_t;
  dsi_t		mSps;
  dsi_t		mPps;
  inline dsi_t*	getSps() { return &mSps; }
  inline dsi_t*	getPps() { return &mPps; }
public:
  void setSpsAndPps(dsi_t* pSps, dsi_t* pPps)
  {
	  memcpy(&mSps, pSps, sizeof(mSps));
	  memcpy(&mPps, pPps, sizeof(mPps));
  }
  void setBitrate(int bitrate) { mBitrate = bitrate; }

  int64_t mPTS;
  int64_t mSendingStartPTS;
  inline int64_t setPts() { return mPTS; }
  inline int64_t setSendingStartPTS() { return mSendingStartPTS; }

  int m_RTP_PAYLOAD_MAX_SIZE;
  int m_RTP_PAYLOAD_PREFERRED_SIZE;
  inline void setMTU(int nRTP_PAYLOAD_MAX_SIZE, int nRTP_PAYLOAD_PREFERRED_SIZE) 
  {
	  m_RTP_PAYLOAD_MAX_SIZE = nRTP_PAYLOAD_MAX_SIZE;
	  m_RTP_PAYLOAD_PREFERRED_SIZE = nRTP_PAYLOAD_PREFERRED_SIZE;
	  m_RTP_PAYLOAD_PREFERRED_SIZE = ((m_RTP_PAYLOAD_MAX_SIZE) < 1000 ? (m_RTP_PAYLOAD_MAX_SIZE) : 1000);
  }
};

#endif
