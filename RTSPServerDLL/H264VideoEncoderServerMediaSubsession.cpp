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
// on demand, from a H264 video file.
// Implementation

////
//   "H264VideoFileServerMediaSubsession.hh" modified by skkang 2021, Jan, 
////

#include "H264VideoEncoderServerMediaSubsession.hh"
#include "H264VideoRTPSink.hh"
#include "ByteStreamFileSource.hh"
#include "H264VideoStreamFramer.hh"

//skkang
#include "H264VideoRTPSource.hh"
#include "EncoderDeviceSource.hh"
#include "H264VideoStreamDiscreteFramer.hh"
#include "Base64.hh"

extern int RTP_PAYLOAD_MAX_SIZE;
extern int RTP_PAYLOAD_PREFERRED_SIZE;

H264VideoEncoderServerMediaSubsession*
H264VideoEncoderServerMediaSubsession::createNew(UsageEnvironment& env,
	                      FramedSource** source,
					      Boolean reuseFirstSource) {
  return new H264VideoEncoderServerMediaSubsession(env, source, reuseFirstSource);
}

H264VideoEncoderServerMediaSubsession::H264VideoEncoderServerMediaSubsession(UsageEnvironment& env,
	FramedSource** source, Boolean reuseFirstSource)
  : FileServerMediaSubsession(env, "TEST", reuseFirstSource),
    fAuxSDPLine(NULL), fDoneFlag(0), fDummyRTPSink(NULL), mSource(source){
}

H264VideoEncoderServerMediaSubsession::~H264VideoEncoderServerMediaSubsession() {
  delete[] fAuxSDPLine;
}

static void afterPlayingDummy(void* clientData) {
  H264VideoEncoderServerMediaSubsession* subsess = (H264VideoEncoderServerMediaSubsession*)clientData;
  subsess->afterPlayingDummy1();
}

void H264VideoEncoderServerMediaSubsession::afterPlayingDummy1() {
  // Unschedule any pending 'checking' task:
  envir().taskScheduler().unscheduleDelayedTask(nextTask());
  // Signal the event loop that we're done:
  setDoneFlag();
}

static void checkForAuxSDPLine(void* clientData) {
  H264VideoEncoderServerMediaSubsession* subsess = (H264VideoEncoderServerMediaSubsession*)clientData;
  subsess->checkForAuxSDPLine1();
}

void H264VideoEncoderServerMediaSubsession::checkForAuxSDPLine1() {
  nextTask() = NULL;

  char const* dasl;
  if (fAuxSDPLine != NULL) {
    // Signal the event loop that we're done:
    setDoneFlag();
  } else if (fDummyRTPSink != NULL && (dasl = fDummyRTPSink->auxSDPLine()) != NULL) {
    fAuxSDPLine = strDup(dasl);
    fDummyRTPSink = NULL;

    // Signal the event loop that we're done:
    setDoneFlag();
  } else if (!fDoneFlag) {
    // try again after a brief delay:
    int uSecsToDelay = 100000; // 100 ms
    nextTask() = envir().taskScheduler().scheduleDelayedTask(uSecsToDelay,
			      (TaskFunc*)checkForAuxSDPLine, this);
  }
}

char const* H264VideoEncoderServerMediaSubsession::getAuxSDPLine(RTPSink* rtpSink, FramedSource* inputSource) {
  if (fAuxSDPLine != NULL)
	  return fAuxSDPLine; // it's already been set up (for a previous client)
#if 1  //skkang
  else
  {
	  dsi_t*	pSps = getSps();
	  dsi_t*	pPps = getPps();

	  u_int8_t* sps = (u_int8_t*)(pSps->data); unsigned spsSize = pSps->len;
	  u_int8_t* pps = (u_int8_t*)(pPps->data); unsigned ppsSize = pPps->len;

	  u_int32_t profile_level_id;
	  if (spsSize < 4) { // sanity check
		  profile_level_id = 0;
	  }
	  else {
		  profile_level_id = (sps[1] << 16) | (sps[2] << 8) | sps[3]; // profile_idc|constraint_setN_flag|level_idc
	  }

	  // Set up the "a=fmtp:" SDP line for this stream:
	  char* sps_base64 = base64Encode((char*)sps, spsSize);
	  char* pps_base64 = base64Encode((char*)pps, ppsSize);
	  char const* fmtpFmt =
		  "a=fmtp:%d packetization-mode=1"
		  ";profile-level-id=%06X"
		  ";sprop-parameter-sets=%s,%s\r\n";
	  unsigned fmtpFmtSize = strlen(fmtpFmt)
		  + 3 /* max char len */
		  + 6 /* 3 bytes in hex */
		  + strlen(sps_base64) + strlen(pps_base64);
	  char* fmtp = new char[fmtpFmtSize];
	  sprintf(fmtp, fmtpFmt,
		  96, //rtpPayloadType(),
		  profile_level_id,
		  sps_base64, pps_base64);
	  delete[] sps_base64;
	  delete[] pps_base64;

	  fAuxSDPLine = fmtp;
  }
#else 
  //original code
  if (fDummyRTPSink == NULL) { // we're not already setting it up for another, concurrent stream
    // Note: For H264 video files, the 'config' information ("profile-level-id" and "sprop-parameter-sets") isn't known
    // until we start reading the file.  This means that "rtpSink"s "auxSDPLine()" will be NULL initially,
    // and we need to start reading data from our file until this changes.
    fDummyRTPSink = rtpSink;

    // Start reading the file:
    fDummyRTPSink->startPlaying(*inputSource, afterPlayingDummy, this);

    // Check whether the sink's 'auxSDPLine()' is ready:
    checkForAuxSDPLine(this);
  }

  envir().taskScheduler().doEventLoop(&fDoneFlag);
#endif

  return fAuxSDPLine;
}

FramedSource* H264VideoEncoderServerMediaSubsession::createNewStreamSource(unsigned /*clientSessionId*/, unsigned& estBitrate)
{
#if 1
	estBitrate = mBitrate / 1000;

	EncoderDeviceSource*		pSource = (EncoderDeviceSource *)*mSource;
	EncoderDeviceParameters*	pParams = pSource->getParams();
	
	CDataQueue*	m_pQueue = pParams->m_pQueue;
	int* pRunning = pParams->m_bRunning;
	float fps = pParams->m_fps;
	EncoderDeviceParameters	videoSourceParams(m_pQueue, pRunning, fps);
	EncoderDeviceSource *pInputSource = EncoderDeviceSource::createNew(envir(), videoSourceParams);
	
	if (*mSource == NULL) return NULL;
	fFileSize = 0;

	if (TRUE)
	{
		return H264VideoStreamDiscreteFramer::createNew(envir(), (FramedSource*)pInputSource);
	}
	return H264VideoStreamFramer::createNew(envir(), (FramedSource*)pInputSource);
#else
	//original code
  estBitrate = 500; // kbps, estimate

  // Create the video source:
  ByteStreamFileSource* fileSource = ByteStreamFileSource::createNew(envir(), fFileName);
  if (fileSource == NULL) return NULL;
  fFileSize = fileSource->fileSize();

  // Create a framer for the Video Elementary Stream:
  return H264VideoStreamFramer::createNew(envir(), fileSource);
#endif
}

RTPSink* H264VideoEncoderServerMediaSubsession
::createNewRTPSink(Groupsock* rtpGroupsock,
		   unsigned char rtpPayloadTypeIfDynamic,
		   FramedSource* /*inputSource*/) {
#if 1
	dsi_t*	pSps = getSps();
	dsi_t*	pPps = getPps();


	RTP_PAYLOAD_MAX_SIZE = m_RTP_PAYLOAD_MAX_SIZE;
	RTP_PAYLOAD_PREFERRED_SIZE = m_RTP_PAYLOAD_PREFERRED_SIZE;

	H264VideoRTPSink* pRtpSink = H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic, (u_int8_t const*)pSps->data, pSps->len, (u_int8_t const*)pPps->data, pPps->len);

	//pRtpSink->setPtsPtr(&mPTS);
	//pRtpSink->setSendingStartPtsPtr(&mSendingStartPTS);

	return pRtpSink;
#else
	//origianl code
  return H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
#endif
}
