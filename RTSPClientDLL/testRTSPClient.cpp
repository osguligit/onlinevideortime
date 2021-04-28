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
// Copyright (c) 1996-2021, Live Networks, Inc.  All rights reserved
// A demo application, showing how to create and run a RTSP client (that can potentially receive multiple streams concurrently).
//
// NOTE: This code - although it builds a running application - is intended only to illustrate how to develop your own RTSP
// client application.  For a full-featured RTSP client application - with much more functionality, and many options - see
// "openRTSP": http://www.live555.com/openRTSP/

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"


#include "func_declare.h"
#include "CRTSPClientMain.h"

#include <time.h>
// Forward function definitions:

// RTSP 'response handlers':
void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString);
void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString);
void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString);

// Other event handler functions:
void subsessionAfterPlaying(void* clientData); // called when a stream's subsession (e.g., audio or video substream) ends
void subsessionByeHandler(void* clientData, char const* reason);
// called when a RTCP "BYE" is received for a subsession
void streamTimerHandler(void* clientData);
// called at the end of a stream's expected duration (if the stream has not already signaled its end using a RTCP "BYE")

//// The main streaming routine (for each "rtsp://" URL):
//void openURL(UsageEnvironment& env, char const* progName, char const* rtspURL);

// Used to iterate through each stream's 'subsessions', setting up each one:
void setupNextSubsession(RTSPClient* rtspClient);

// Used to shut down and close a stream (including its "RTSPClient" object):
void shutdownStream(RTSPClient* rtspClient, int exitCode = 1);

// A function that outputs a string that identifies each stream (for debugging output).  Modify this if you wish:
UsageEnvironment& operator<<(UsageEnvironment& env, const RTSPClient& rtspClient) {
	return env << "[URL:\"" << rtspClient.url() << "\"]: ";
}

// A function that outputs a string that identifies each subsession (for debugging output).  Modify this if you wish:
UsageEnvironment& operator<<(UsageEnvironment& env, const MediaSubsession& subsession) {
	return env << subsession.mediumName() << "/" << subsession.codecName();
}

void usage(UsageEnvironment& env, char const* progName) {
	env << "Usage: " << progName << " <rtsp-url-1> ... <rtsp-url-N>\n";
	env << "\t(where each <rtsp-url-i> is a \"rtsp://\" URL)\n";
}

char eventLoopWatchVariable = 0;

//int main(int argc, char** argv)
int testRTSPClient_main(int argc, char** argv)
{
	// Begin by setting up our usage environment:
	TaskScheduler* scheduler = BasicTaskScheduler::createNew();
	UsageEnvironment* env = BasicUsageEnvironment::createNew(*scheduler);

	// We need at least one "rtsp://" URL argument:
	if (argc < 2) {
		usage(*env, argv[0]);
		return 1;
	}

	// There are argc-1 URLs: argv[1] through argv[argc-1].  Open and start streaming each one:
	for (int i = 1; i <= argc - 1; ++i) {
		openURL(*env, argv[0], argv[i], nullptr, nullptr, nullptr, nullptr);
	}

	// All subsequent activity takes place within the event loop:
	env->taskScheduler().doEventLoop(&eventLoopWatchVariable);
	// This function call does not return, unless, at some point in time, "eventLoopWatchVariable" gets set to something non-zero.

	return 0;

	// If you choose to continue the application past this point (i.e., if you comment out the "return 0;" statement above),
	// and if you don't intend to do anything more with the "TaskScheduler" and "UsageEnvironment" objects,
	// then you can also reclaim the (small) memory used by these objects by uncommenting the following code:
	/*
	  env->reclaim(); env = NULL;
	  delete scheduler; scheduler = NULL;
	*/
}

// Define a class to hold per-stream state that we maintain throughout each stream's lifetime:

class StreamClientState {
public:
	StreamClientState();
	virtual ~StreamClientState();

public:
	MediaSubsessionIterator* iter;
	MediaSession* session;
	MediaSubsession* subsession;
	TaskToken streamTimerTask;
	double duration;
};

// If you're streaming just a single stream (i.e., just from a single URL, once), then you can define and use just a single
// "StreamClientState" structure, as a global variable in your application.  However, because - in this demo application - we're
// showing how to play multiple streams, concurrently, we can't do that.  Instead, we have to have a separate "StreamClientState"
// structure for each "RTSPClient".  To do this, we subclass "RTSPClient", and add a "StreamClientState" field to the subclass:

class ourRTSPClient : public RTSPClient {
public:
	static ourRTSPClient* createNew(CRTSPClientMain* pRtspClientMain, void* pParam, CallBackReceived* func, unsigned char* data,
		UsageEnvironment& env, char const* rtspURL,
		int verbosityLevel = 0,
		char const* applicationName = NULL,
		portNumBits tunnelOverHTTPPortNum = 0);

	//origin
	//static ourRTSPClient* createNew(UsageEnvironment& env, char const* rtspURL,
	//	int verbosityLevel = 0,
	//	char const* applicationName = NULL,
	//	portNumBits tunnelOverHTTPPortNum = 0);

protected:
	ourRTSPClient(CRTSPClientMain* pRtspClientMain, void* pParam, CallBackReceived* func, unsigned char* data,
		UsageEnvironment& env, char const* rtspURL,
		int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum);
	//origin
	//ourRTSPClient(UsageEnvironment& env, char const* rtspURL,
	//	int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum);
	// called only by createNew();
	virtual ~ourRTSPClient();

public:
	StreamClientState scs;

	CRTSPClientMain* m_pRTSPClientMain;
	void* m_pParam;
	CallBackReceived* m_cbFunc;
	unsigned char* m_data;
};

// Define a data sink (a subclass of "MediaSink") to receive the data for each subsession (i.e., each audio or video 'substream').
// In practice, this might be a class (or a chain of classes) that decodes and then renders the incoming audio or video.
// Or it might be a "FileSink", for outputting the received data into a file (as is done by the "openRTSP" application).
// In this example code, however, we define a simple 'dummy' sink that receives incoming data, but does nothing with it.

//#define RECEIVED_FRAME_SAVE_AS_FILE  //skkang
class DummySink : public MediaSink {
public:
	static DummySink* createNew(void* pParam, CallBackReceived* func, unsigned char* data,
		UsageEnvironment& env,
		MediaSubsession& subsession, // identifies the kind of data that's being received
		char const* streamId = NULL); // identifies the stream itself (optional)

private:
	DummySink(void* pParam, CallBackReceived* func, unsigned char* data,
		UsageEnvironment& env, MediaSubsession& subsession, char const* streamId);
	// called only by "createNew()"
	virtual ~DummySink();

	static void afterGettingFrame(void* clientData, unsigned frameSize,
		unsigned numTruncatedBytes,
		struct timeval presentationTime,
		unsigned durationInMicroseconds);
	void afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
		struct timeval presentationTime, unsigned durationInMicroseconds);

private:
	// redefined virtual functions:
	virtual Boolean continuePlaying();

private:
	u_int8_t* fReceiveBuffer;
	MediaSubsession& fSubsession;
	char* fStreamId;
#ifdef RECEIVED_FRAME_SAVE_AS_FILE 
	FILE* pFile = nullptr;
	int	m_writeCnt;
	//u_int8_t*	fFrameBuf;
#endif
	bool fIsStartFrame = FALSE;
	void* m_pParam;
	CallBackReceived* m_cbFunc;
	unsigned char* m_data;
	int m_width;
	int m_height;
	bool SPSReceive = FALSE;
	bool IdrReceive = FALSE;


	////////////////
   // Parsing SPS Get Resolution 
	const unsigned char * m_pStart;
	unsigned short m_nLength;
	int m_nCurrentBit;

	inline unsigned int ReadBit()
	{
		if (m_nCurrentBit > m_nLength * 8)
			return 0;
		int nIndex = m_nCurrentBit / 8;
		int nOffset = m_nCurrentBit % 8 + 1;

		m_nCurrentBit++;
		return (m_pStart[nIndex] >> (8 - nOffset)) & 0x01;
	}

	inline unsigned int ReadBits(int n)
	{
		int r = 0;
		int i;
		for (i = 0; i < n; i++)
		{
			r |= (ReadBit() << (n - i - 1));
		}
		return r;
	}

	inline unsigned int ReadExponentialGolombCode()
	{
		int r = 0;
		int i = 0;

		while ((ReadBit() == 0) && (i < 32))
		{
			i++;
		}

		r = ReadBits(i);
		r += (1 << i) - 1;
		return r;
	}

	inline unsigned int ReadSE()
	{
		int r = ReadExponentialGolombCode();
		if (r & 0x01)
		{
			r = (r + 1) / 2;
		}
		else
		{
			r = -(r / 2);
		}
		return r;
	}

	inline bool GetResolution_SPSParse(char* codec, unsigned char* spsBuf, int Length, int *Width, int *Height)
	{
		int nal_type = spsBuf[0] & 0x0f;
		//if (nal_type != NAL_SPS) return false;

		m_pStart = (const unsigned char*)(spsBuf + 1);
		m_nLength = Length;
		m_nCurrentBit = 0;
		const unsigned char* pTmpStart = m_pStart;

		int frame_crop_left_offset = 0;
		int frame_crop_right_offset = 0;
		int frame_crop_top_offset = 0;
		int frame_crop_bottom_offset = 0;

		int profile_idc = ReadBits(8);
		int constraint_set0_flag = ReadBit();
		int constraint_set1_flag = ReadBit();
		int constraint_set2_flag = ReadBit();
		int constraint_set3_flag = ReadBit();
		int constraint_set4_flag = ReadBit();
		int constraint_set5_flag = ReadBit();
		int reserved_zero_2bits = ReadBits(2);
		int level_idc = ReadBits(8);
		int seq_parameter_set_id = ReadExponentialGolombCode();

		if (profile_idc == 100 || profile_idc == 110 ||
			profile_idc == 122 || profile_idc == 244 ||
			profile_idc == 44 || profile_idc == 83 ||
			profile_idc == 86 || profile_idc == 118)
		{
			int chroma_format_idc = ReadExponentialGolombCode();

			if (chroma_format_idc == 3)
			{
				int residual_colour_transform_flag = ReadBit();
			}
			int bit_depth_luma_minus8 = ReadExponentialGolombCode();
			int bit_depth_chroma_minus8 = ReadExponentialGolombCode();
			int qpprime_y_zero_transform_bypass_flag = ReadBit();
			int seq_scaling_matrix_present_flag = ReadBit();

			if (seq_scaling_matrix_present_flag)
			{
				int i = 0;
				for (i = 0; i < 8; i++)
				{
					int seq_scaling_list_present_flag = ReadBit();
					if (seq_scaling_list_present_flag)
					{
						int sizeOfScalingList = (i < 6) ? 16 : 64;
						int lastScale = 8;
						int nextScale = 8;
						int j = 0;
						for (j = 0; j < sizeOfScalingList; j++)
						{
							if (nextScale != 0)
							{
								int delta_scale = ReadSE();
								nextScale = (lastScale + delta_scale + 256) % 256;
							}
							lastScale = (nextScale == 0) ? lastScale : nextScale;
						}
					}
				}
			}
		}

		int log2_max_frame_num_minus4 = ReadExponentialGolombCode();
		int pic_order_cnt_type = ReadExponentialGolombCode();
		if (pic_order_cnt_type == 0)
		{
			int log2_max_pic_order_cnt_lsb_minus4 = ReadExponentialGolombCode();
		}
		else if (pic_order_cnt_type == 1)
		{
			int delta_pic_order_always_zero_flag = ReadBit();
			int offset_for_non_ref_pic = ReadSE();
			int offset_for_top_to_bottom_field = ReadSE();
			int num_ref_frames_in_pic_order_cnt_cycle = ReadExponentialGolombCode();
			int i;
			for (i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++)
			{
				ReadSE();
				//sps->offset_for_ref_frame[ i ] = ReadSE();
			}
		}
		int max_num_ref_frames = ReadExponentialGolombCode();
		int gaps_in_frame_num_value_allowed_flag = ReadBit();
		int pic_width_in_mbs_minus1 = ReadExponentialGolombCode();
		int pic_height_in_map_units_minus1 = ReadExponentialGolombCode();
		int frame_mbs_only_flag = ReadBit();
		if (!frame_mbs_only_flag)
		{
			int mb_adaptive_frame_field_flag = ReadBit();
		}
		int direct_8x8_inference_flag = ReadBit();
		int frame_cropping_flag = ReadBit();
		if (frame_cropping_flag)
		{
			frame_crop_left_offset = ReadExponentialGolombCode();
			frame_crop_right_offset = ReadExponentialGolombCode();
			frame_crop_top_offset = ReadExponentialGolombCode();
			frame_crop_bottom_offset = ReadExponentialGolombCode();
		}
		int vui_parameters_present_flag = ReadBit();
		pTmpStart++;

		if (strcmp(codec, "H264") == 0)//h264
		{
			*Width = ((pic_width_in_mbs_minus1 + 1) * 16) - frame_crop_right_offset * 2 - frame_crop_left_offset * 2;
			*Height = ((2 - frame_mbs_only_flag)* (pic_height_in_map_units_minus1 + 1) * 16) - (frame_crop_top_offset * 2) - (frame_crop_bottom_offset * 2);
		}
		else if (strcmp(codec, "H265") == 0 || strcmp(codec, "HEVC") == 0)//h265
		{
			*Width = frame_crop_right_offset;
			*Height = frame_crop_top_offset;
		}
		return true;
	}

};

#define RTSP_CLIENT_VERBOSITY_LEVEL 1 // by default, print verbose output from each "RTSPClient"

static unsigned rtspClientCount = 0; // Counts how many streams (i.e., "RTSPClient"s) are currently in use.

void* openURL(UsageEnvironment& env, char const* progName, char const* rtspURL, void* pParam, CallBackReceived* func, unsigned char* data, CRTSPClientMain* pRtspClientMain)
{	// Begin by creating a "RTSPClient" object.  Note that there is a separate "RTSPClient" object for each stream that we wish
	// to receive (even if more than stream uses the same "rtsp://" URL).
	//RTSPClient* rtspClient = ourRTSPClient::createNew(nullptr, env, rtspURL, RTSP_CLIENT_VERBOSITY_LEVEL, progName);
	RTSPClient* rtspClient = ourRTSPClient::createNew(pRtspClientMain, pParam, func, data, env, rtspURL, RTSP_CLIENT_VERBOSITY_LEVEL, progName);
	if (rtspClient == NULL) {
		env << "Failed to create a RTSP client for URL \"" << rtspURL << "\": " << env.getResultMsg() << "\n";
		return nullptr;
	}

	++rtspClientCount;

	// Next, send a RTSP "DESCRIBE" command, to get a SDP description for the stream.
	// Note that this command - like all RTSP commands - is sent asynchronously; we do not block, waiting for a response.
	// Instead, the following function call returns immediately, and we handle the RTSP response later, from within the event loop:
	rtspClient->sendDescribeCommand(continueAfterDESCRIBE);

	return (HANDLE)rtspClient;
}


// Implementation of the RTSP 'response handlers':

void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString) {
	do {
		UsageEnvironment& env = rtspClient->envir(); // alias
		StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

		if (resultCode != 0) {
			env << *rtspClient << "Failed to get a SDP description: " << resultString << "\n";
			delete[] resultString;

			void* pParam = ((ourRTSPClient*)rtspClient)->m_pParam;
			CallBackReceived*  cbFunc = ((ourRTSPClient*)rtspClient)->m_cbFunc;
			cbFunc(pParam, -2, -2, "ConnectFail", nullptr, -2, 0, 0, 0);

			break;
		}

		((ourRTSPClient*)rtspClient)->m_pRTSPClientMain->m_bConnected = TRUE;//20210225 skkang 

		char* const sdpDescription = resultString;
		env << *rtspClient << "Got a SDP description:\n" << sdpDescription << "\n";

		// Create a media session object from this SDP description:
		scs.session = MediaSession::createNew(env, sdpDescription);
		delete[] sdpDescription; // because we don't need it anymore
		if (scs.session == NULL) {
			env << *rtspClient << "Failed to create a MediaSession object from the SDP description: " << env.getResultMsg() << "\n";
			break;
		}
		else if (!scs.session->hasSubsessions()) {
			env << *rtspClient << "This session has no media subsessions (i.e., no \"m=\" lines)\n";
			break;
		}

		// Then, create and set up our data source objects for the session.  We do this by iterating over the session's 'subsessions',
		// calling "MediaSubsession::initiate()", and then sending a RTSP "SETUP" command, on each one.
		// (Each 'subsession' will have its own data source.)
		scs.iter = new MediaSubsessionIterator(*scs.session);
		setupNextSubsession(rtspClient);
		return;
	} while (0);

	// An unrecoverable error occurred with this stream.
	shutdownStream(rtspClient);
}

// By default, we request that the server stream its data using RTP/UDP.
// If, instead, you want to request that the server stream via RTP-over-TCP, change the following to True:
#define REQUEST_STREAMING_OVER_TCP TRUE //skkang

void setupNextSubsession(RTSPClient* rtspClient) {
	UsageEnvironment& env = rtspClient->envir(); // alias
	StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

	scs.subsession = scs.iter->next();
	if (scs.subsession != NULL) {
		if (!scs.subsession->initiate()) {
			env << *rtspClient << "Failed to initiate the \"" << *scs.subsession << "\" subsession: " << env.getResultMsg() << "\n";
			setupNextSubsession(rtspClient); // give up on this subsession; go to the next one
		}
		else {
			env << *rtspClient << "Initiated the \"" << *scs.subsession << "\" subsession (";
			if (scs.subsession->rtcpIsMuxed()) {
				env << "client port " << scs.subsession->clientPortNum();
			}
			else {
				env << "client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum() + 1;
			}
			env << ")\n";

			// Continue setting up this subsession, by sending a RTSP "SETUP" command:
			rtspClient->sendSetupCommand(*scs.subsession, continueAfterSETUP, False, REQUEST_STREAMING_OVER_TCP);
		}
		return;
	}

	// We've finished setting up all of the subsessions.  Now, send a RTSP "PLAY" command to start the streaming:
	if (scs.session->absStartTime() != NULL) {
		// Special case: The stream is indexed by 'absolute' time, so send an appropriate "PLAY" command:
		rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY, scs.session->absStartTime(), scs.session->absEndTime());
	}
	else {
		scs.duration = scs.session->playEndTime() - scs.session->playStartTime();
		rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY);
	}
}

void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString) {
	do {
		UsageEnvironment& env = rtspClient->envir(); // alias
		StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias
		void* pParam = ((ourRTSPClient*)rtspClient)->m_pParam;
		CallBackReceived*  cbFunc = ((ourRTSPClient*)rtspClient)->m_cbFunc;
		unsigned char* data = ((ourRTSPClient*)rtspClient)->m_data;
		ourRTSPClient* pOurClient = dynamic_cast<ourRTSPClient*>(rtspClient);


		if (resultCode != 0) {
			env << *rtspClient << "Failed to set up the \"" << *scs.subsession << "\" subsession: " << resultString << "\n";

			cbFunc(pParam, -2, -2, "ConnectFail", nullptr, -2, 0, 0, 0);

			break;
		}

		env << *rtspClient << "Set up the \"" << *scs.subsession << "\" subsession (";
		if (scs.subsession->rtcpIsMuxed()) {
			env << "client port " << scs.subsession->clientPortNum();
		}
		else {
			env << "client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum() + 1;
		}
		env << ")\n";

		// Having successfully setup the subsession, create a data sink for it, and call "startPlaying()" on it.
		// (This will prepare the data sink to receive data; the actual flow of data from the client won't start happening until later,
		// after we've sent a RTSP "PLAY" command.)

		scs.subsession->sink = DummySink::createNew(pParam, cbFunc, data, env, *scs.subsession, rtspClient->url());
		// perhaps use your own custom "MediaSink" subclass instead
		if (scs.subsession->sink == NULL) {
			env << *rtspClient << "Failed to create a data sink for the \"" << *scs.subsession
				<< "\" subsession: " << env.getResultMsg() << "\n";
			break;
		}

		env << *rtspClient << "Created a data sink for the \"" << *scs.subsession << "\" subsession\n";
		scs.subsession->miscPtr = rtspClient; // a hack to let subsession handler functions get the "RTSPClient" from the subsession 
		scs.subsession->sink->startPlaying(*(scs.subsession->readSource()),
			subsessionAfterPlaying, scs.subsession);
		// Also set a handler to be called if a RTCP "BYE" arrives for this subsession:
		if (scs.subsession->rtcpInstance() != NULL) {
			scs.subsession->rtcpInstance()->setByeWithReasonHandler(subsessionByeHandler, scs.subsession);
		}

		pOurClient->m_pRTSPClientMain->m_urlOpen = 0; //skkang
	} while (0);
	delete[] resultString;

	// Set up the next subsession, if any:
	setupNextSubsession(rtspClient);
}

void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString) {
	Boolean success = False;

	do {
		UsageEnvironment& env = rtspClient->envir(); // alias
		StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

		if (resultCode != 0) {
			env << *rtspClient << "Failed to start playing session: " << resultString << "\n";
			break;
		}

		// Set a timer to be handled at the end of the stream's expected duration (if the stream does not already signal its end
		// using a RTCP "BYE").  This is optional.  If, instead, you want to keep the stream active - e.g., so you can later
		// 'seek' back within it and do another RTSP "PLAY" - then you can omit this code.
		// (Alternatively, if you don't want to receive the entire stream, you could set this timer for some shorter value.)
		if (scs.duration > 0) {
			unsigned const delaySlop = 2; // number of seconds extra to delay, after the stream's expected duration.  (This is optional.)
			scs.duration += delaySlop;
			unsigned uSecsToDelay = (unsigned)(scs.duration * 1000000);
			scs.streamTimerTask = env.taskScheduler().scheduleDelayedTask(uSecsToDelay, (TaskFunc*)streamTimerHandler, rtspClient);
		}

		env << *rtspClient << "Started playing session";
		if (scs.duration > 0) {
			env << " (for up to " << scs.duration << " seconds)";
		}
		env << "...\n";

		success = True;
	} while (0);
	delete[] resultString;

	if (!success) {
		// An unrecoverable error occurred with this stream.
		shutdownStream(rtspClient);
	}
}


// Implementation of the other event handlers:

void subsessionAfterPlaying(void* clientData) {
	MediaSubsession* subsession = (MediaSubsession*)clientData;
	RTSPClient* rtspClient = (RTSPClient*)(subsession->miscPtr);

	// Begin by closing this subsession's stream:
	Medium::close(subsession->sink);
	subsession->sink = NULL;

	// Next, check whether *all* subsessions' streams have now been closed:
	MediaSession& session = subsession->parentSession();
	MediaSubsessionIterator iter(session);
	while ((subsession = iter.next()) != NULL) {
		if (subsession->sink != NULL) return; // this subsession is still active
	}

	// All subsessions' streams have now been closed, so shutdown the client:
	shutdownStream(rtspClient);
}

void subsessionByeHandler(void* clientData, char const* reason) {
	MediaSubsession* subsession = (MediaSubsession*)clientData;
	RTSPClient* rtspClient = (RTSPClient*)subsession->miscPtr;
	UsageEnvironment& env = rtspClient->envir(); // alias

	env << *rtspClient << "Received RTCP \"BYE\"";
	if (reason != NULL) {
		env << " (reason:\"" << reason << "\")";
		delete[](char*)reason;
	}
	env << " on \"" << *subsession << "\" subsession\n";

	// Now act as if the subsession had closed:
	subsessionAfterPlaying(subsession);
}

void streamTimerHandler(void* clientData) {
	ourRTSPClient* rtspClient = (ourRTSPClient*)clientData;
	StreamClientState& scs = rtspClient->scs; // alias

	scs.streamTimerTask = NULL;

	// Shut down the stream:
	shutdownStream(rtspClient);
}

void shutdownStream(RTSPClient* rtspClient, int exitCode) {
	UsageEnvironment& env = rtspClient->envir(); // alias
	StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias
	ourRTSPClient* pOurClient = dynamic_cast<ourRTSPClient*>(rtspClient);

	// First, check whether any subsessions have still to be closed:
	if (scs.session != NULL) {
		Boolean someSubsessionsWereActive = False;
		MediaSubsessionIterator iter(*scs.session);
		MediaSubsession* subsession;

		while ((subsession = iter.next()) != NULL) {
			if (subsession->sink != NULL) {
				Medium::close(subsession->sink);
				subsession->sink = NULL;

				if (subsession->rtcpInstance() != NULL) {
					subsession->rtcpInstance()->setByeHandler(NULL, NULL); // in case the server sends a RTCP "BYE" while handling "TEARDOWN"
				}

				someSubsessionsWereActive = True;
			}
		}

		if (someSubsessionsWereActive) {
			// Send a RTSP "TEARDOWN" command, to tell the server to shutdown the stream.
			// Don't bother handling the response to the "TEARDOWN".
			rtspClient->sendTeardownCommand(*scs.session, NULL);
		}
	}

	env << *rtspClient << "Closing the stream.\n";
	Medium::close(rtspClient);
	// Note that this will also cause this stream's "StreamClientState" structure to get reclaimed.

	if (--rtspClientCount == 0) {
		// The final stream has ended, so exit the application now.
		// (Of course, if you're embedding this code into your own application, you might want to comment this out,
		// and replace it with "eventLoopWatchVariable = 1;", so that we leave the LIVE555 event loop, and continue running "main()".)
		//exit(exitCode);

		void* pParam = ((ourRTSPClient*)rtspClient)->m_pParam;
		CallBackReceived*  cbFunc = ((ourRTSPClient*)rtspClient)->m_cbFunc;
		cbFunc(pParam, -2, -2, "ConnectFail", nullptr, -1, -1, -1, -1.0f);
		//pOurClient->m_pRTSPClientMain->m_urlOpen = 2;
		if (pOurClient &&pOurClient->m_pRTSPClientMain)
			pOurClient->m_pRTSPClientMain->m_EventLoop = 1;
	}
}


// Implementation of "ourRTSPClient":
//ourRTSPClient* ourRTSPClient::createNew(CRTSPClientMain& RTSPClientMain, UsageEnvironment& env, char const* rtspURL,
//	int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum) {
//	return new ourRTSPClient(RTSPClientMain, env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum);
//}
//
//ourRTSPClient::ourRTSPClient(CRTSPClientMain& RTSPClientMain, UsageEnvironment& env, char const* rtspURL,
//	int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum)
//	: RTSPClient(env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum, -1), m_RTSPClientMain(RTSPClientMain) {
//}
ourRTSPClient* ourRTSPClient::createNew(CRTSPClientMain* pRtspClientMain, void* pParam, CallBackReceived* func, unsigned char* data,
	UsageEnvironment& env, char const* rtspURL,
	int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum) {
	return new ourRTSPClient(pRtspClientMain, pParam, func, data, env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum);
}

ourRTSPClient::ourRTSPClient(CRTSPClientMain* pRtspClientMain, void* pParam, CallBackReceived* func, unsigned char* data,
	UsageEnvironment& env, char const* rtspURL,
	int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum)
	: RTSPClient(env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum, -1), m_pRTSPClientMain(pRtspClientMain), m_pParam(pParam), m_cbFunc(func), m_data(data) {
}

ourRTSPClient::~ourRTSPClient() {
}


// Implementation of "StreamClientState":

StreamClientState::StreamClientState()
	: iter(NULL), session(NULL), subsession(NULL), streamTimerTask(NULL), duration(0.0) {
}

StreamClientState::~StreamClientState() {
	delete iter;
	if (session != NULL) {
		// We also need to delete "session", and unschedule "streamTimerTask" (if set)
		UsageEnvironment& env = session->envir(); // alias

		env.taskScheduler().unscheduleDelayedTask(streamTimerTask);
		Medium::close(session);
	}
}


// Implementation of "DummySink":

// Even though we're not going to be doing anything with the incoming data, we still need to receive it.
// Define the size of the buffer that we'll use:
//#define DUMMY_SINK_RECEIVE_BUFFER_SIZE 200000

DummySink* DummySink::createNew(void* pParam, CallBackReceived* func, unsigned char* data,
	UsageEnvironment& env, MediaSubsession& subsession, char const* streamId)
{
	return new DummySink(pParam, func, data, env, subsession, streamId);
}

DummySink::DummySink(void* pParam, CallBackReceived* func, unsigned char* data,
	UsageEnvironment& env, MediaSubsession& subsession, char const* streamId)
	: MediaSink(env),
	fSubsession(subsession),
	m_pParam(pParam), m_cbFunc(func), m_data(data)
{
	fStreamId = strDup(streamId);
	fReceiveBuffer = new u_int8_t[DUMMY_SINK_RECEIVE_BUFFER_SIZE];

	if (!m_data)
		m_data = new u_int8_t[DUMMY_SINK_RECEIVE_BUFFER_SIZE];

#ifdef RECEIVED_FRAME_SAVE_AS_FILE s
	fopen_s(&pFile, "live555receive1.h264", "wb");
	//fFrameBuf = new u_int8_t[DUMMY_SINK_RECEIVE_BUFFER_SIZE + 4];
	m_writeCnt = 0;
#endif

}

DummySink::~DummySink() {
	delete[] fReceiveBuffer;
	delete[] fStreamId;

	if (m_data)
		delete[] m_data;
	//delete[] fFrameBuf;
#ifdef RECEIVED_FRAME_SAVE_AS_FILE 
	if (pFile)
	{
		fclose(pFile); pFile = nullptr;

		char log[64];
		sprintf_s(log, "[DummySink] fwritecnt: %d\n ", m_writeCnt);
		OutputDebugString(log);
}
#endif
}

void DummySink::afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes,
	struct timeval presentationTime, unsigned durationInMicroseconds) {
	DummySink* sink = (DummySink*)clientData;
	sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

// If you don't want to see debugging output for each received frame, then comment out the following line:
//#define DEBUG_PRINT_EACH_RECEIVED_FRAME 1
void DummySink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
	struct timeval presentationTime, unsigned /*durationInMicroseconds*/)
{
	// We've just received a frame of data.  (Optionally) print out information about it:
#ifdef DEBUG_PRINT_EACH_RECEIVED_FRAME
	if (fStreamId != NULL) envir() << "Stream \"" << fStreamId << "\"; ";
	envir() << fSubsession.mediumName() << "/" << fSubsession.codecName() << ":\tReceived " << frameSize << " bytes";
	if (numTruncatedBytes > 0) envir() << " (with " << numTruncatedBytes << " bytes truncated)";
	char uSecsStr[6 + 1]; // used to output the 'microseconds' part of the presentation time
	sprintf_s(uSecsStr, "%06u", (unsigned)presentationTime.tv_usec);
	envir() << ".\tPresentation time: " << (int)presentationTime.tv_sec << "." << uSecsStr;
	if (fSubsession.rtpSource() != NULL && !fSubsession.rtpSource()->hasBeenSynchronizedUsingRTCP()) {
		envir() << "!"; // mark the debugging output to indicate that this presentation time is not RTCP-synchronized
	}
#ifdef DEBUG_PRINT_NPT
	envir() << "\tNPT: " << fSubsession.getNormalPlayTime(presentationTime);
#endif
	envir() << "\n";
#endif

#if 0//def RECEIVED_FRAME_SAVE_AS_FILE 
	if (strcmp(fSubsession.codecName(), "H264") == 0)
	{
		unsigned char nalu_header[4] = { 0, 0, 0, 1 };
		unsigned int num = 0;
		SPropRecord *pSPropRecord = parseSPropParameterSets(fSubsession.fmtp_spropparametersets(), num);
		unsigned int extraLen = 0;
		for (unsigned int i = 0; i < num; i++)
		{
			memcpy(&fFrameBuf[extraLen], &nalu_header[0], 4);
			extraLen += 4;
			memcpy(&fFrameBuf[extraLen], pSPropRecord[i].sPropBytes, pSPropRecord[i].sPropLength);
			extraLen += pSPropRecord[i].sPropLength;
		}
		memcpy(&fFrameBuf[extraLen], &nalu_header[0], 4);
		extraLen += 4;
		delete[] pSPropRecord;

		memcpy(fFrameBuf + extraLen, fReceiveBuffer, frameSize);
		int totalSize;
		totalSize = extraLen + frameSize;
		fwrite(fFrameBuf, 1, totalSize, pFile);
		fflush(pFile);
		printf("\tsaved %d bytes\n", totalSize);
}
#endif


	BYTE nal_type = (unsigned)(fReceiveBuffer[0]) & 0x0F;
	if (!SPSReceive)
	{
		if ((strcmp(fSubsession.codecName(), "H264") == 0 && nal_type == NAL_SPS) || ((strcmp(fSubsession.codecName(), "H265") == 0 || strcmp(fSubsession.codecName(), "HEVC") == 0) && nal_type == h265_NAL_SPS))
		{
			SPSReceive = TRUE;

			GetResolution_SPSParse((char*)fSubsession.codecName(), fReceiveBuffer, frameSize, &m_width, &m_height);
		}
	}

	if (!IdrReceive)
	{
		if ((strcmp(fSubsession.codecName(), "H264") == 0 && nal_type == NAL_SLICE_IDR) || ((strcmp(fSubsession.codecName(), "H265") == 0 || strcmp(fSubsession.codecName(), "HEVC") == 0) && nal_type == h265_NAL_IDR))
			IdrReceive = TRUE;
	}

	if ((strcmp(fSubsession.codecName(), "H264") == 0 && IdrReceive) || (strcmp(fSubsession.codecName(), "H265") == 0 || strcmp(fSubsession.codecName(), "HEVC") == 0))
	{
		unsigned char nalu_header[4] = { 0, 0, 0, 1 };
		unsigned int extraLen = 0;
		//if (nal_type == NAL_SPS || nal_type == NAL_PPS)
		{
			unsigned int num = 0;
			SPropRecord *pSPropRecord = parseSPropParameterSets(fSubsession.fmtp_spropparametersets(), num);

			for (unsigned int i = 0; i < num; i++)
			{
				memcpy(&m_data[extraLen], &nalu_header[0], 4);
				extraLen += 4;

				memcpy(&m_data[extraLen], pSPropRecord[i].sPropBytes, pSPropRecord[i].sPropLength);
				extraLen += pSPropRecord[i].sPropLength;
			}
			delete[] pSPropRecord;
		}

		memcpy(&m_data[extraLen], &nalu_header[0], 4);
		extraLen += 4;

		memcpy(m_data + extraLen, fReceiveBuffer, frameSize);

		int totalSize = extraLen + frameSize;

		//m_cbFunc(m_pParam, fSubsession.videoWidth(), fSubsession.videoHeight(), fSubsession.codecName(), m_data, totalSize, IdrReceive, (unsigned)presentationTime.tv_usec);
		float fps = fSubsession.bandwidth() / 100.0f;
		m_cbFunc(m_pParam, m_width, m_height, fSubsession.codecName(), m_data, totalSize, IdrReceive, GET_7FFE008CLOCK_MILLISEC, fps);

#ifdef RECEIVED_FRAME_SAVE_AS_FILE 
		if (pFile)
		{
			fwrite(m_data, 1, totalSize, pFile);
			fflush(pFile);
			m_writeCnt++;

			//char log[64];
			//sprintf_s(log, "[DummySink] fwritecnt: %d\n ", m_writeCnt);
			//OutputDebugString(log);
	}
#endif
		//printf("\tsaved %d bytes\n", totalSize);
	}

	// Then continue, to request the next frame of data:
	continuePlaying();
}

Boolean DummySink::continuePlaying() {
	if (fSource == NULL) return False; // sanity check (should not happen)

	// Request the next frame of data from our input source.  "afterGettingFrame()" will get called later, when it arrives:
	fSource->getNextFrame(fReceiveBuffer, DUMMY_SINK_RECEIVE_BUFFER_SIZE,
		afterGettingFrame, this,
		onSourceClosure, this);
	return True;
}