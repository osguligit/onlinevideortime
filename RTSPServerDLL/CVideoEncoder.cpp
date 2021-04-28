#include "CVideoEncoder.h"
#include "../x264/x264Util.h"
static x264Util util;


CVideoEncoder::CVideoEncoder(CDataQueue * queue, sEncoderParam* pEncParam, __int64* sendingPts)
{
	//memcpy(&m_pEncoderParam, pEncParam, sizeof(sEncoderParam));
	m_pEncoderParam = new sEncoderParam();
	m_pEncoderParam->v_encoder = pEncParam->v_encoder;
	m_pEncoderParam->v_iGpu = pEncParam->v_iGpu;
	m_pEncoderParam->v_srcMode = pEncParam->v_srcMode;
	m_pEncoderParam->v_fps = pEncParam->v_fps;
	m_pEncoderParam->v_frameFormat = pEncParam->v_frameFormat;
	m_pEncoderParam->v_speedPreset = pEncParam->v_speedPreset;
	m_pEncoderParam->v_width = pEncParam->v_width;
	m_pEncoderParam->v_height = pEncParam->v_height;
	m_pEncoderParam->v_profile = pEncParam->v_profile;
	m_pEncoderParam->v_avcLevel = pEncParam->v_avcLevel;
	m_pEncoderParam->v_rateControl = pEncParam->v_rateControl;
	m_pEncoderParam->v_avgBitrate = pEncParam->v_avgBitrate;
	m_pEncoderParam->v_maxBitrate = pEncParam->v_maxBitrate;
	m_pEncoderParam->v_entropyCoding = pEncParam->v_entropyCoding;
	m_pEncoderParam->v_frameFormat = pEncParam->v_frameFormat;
	m_pEncoderParam->v_gopMode = pEncParam->v_gopMode;
	m_pEncoderParam->v_gopSize = pEncParam->v_gopSize;
	m_pEncoderParam->v_bFrame = pEncParam->v_bFrame;

	m_queue = queue;

	m_sendingPts = sendingPts;

	m_sps = nullptr;
	m_pps = nullptr;

	createEncoder();

	m_sema = CreateSemaphore(nullptr, 1, 1, nullptr);
}

CVideoEncoder::~CVideoEncoder()
{
	deleteEncoder();

	SAFE_DELETE(m_pEncoderParam);

	SAFE_CLOSE(m_sema);
}

void CVideoEncoder::createEncoder()
{
	int fpsNum = (int)floor(m_pEncoderParam->v_fps);
	int fpsDen = 1;

	if (m_pEncoderParam->v_encoder == x264)
	{
		m_hEncoder = new x264encoder(util.mapSpeedPreset[m_pEncoderParam->v_speedPreset]);

		m_hEncoder->setResolution(m_pEncoderParam->v_width, m_pEncoderParam->v_height);
		m_hEncoder->setFPS(fpsNum, fpsDen);
		m_hEncoder->setGOPSize(m_pEncoderParam->v_gopSize);
		m_hEncoder->setGOPMode(util.mapGopMode[m_pEncoderParam->v_gopMode]);
		m_hEncoder->setEntropyMode(util.mapEntropyMode[m_pEncoderParam->v_entropyCoding]);
		m_hEncoder->setRCMode(util.mapRateControlMode[m_pEncoderParam->v_rateControl]);
		m_hEncoder->setBitrate(m_pEncoderParam->v_avgBitrate);
		if (m_pEncoderParam->v_frameFormat == 0)
			m_hEncoder->setProgressive();
		else if (m_pEncoderParam->v_frameFormat == 1)
			m_hEncoder->setInterlaced();
		m_hEncoder->setBFrameLength(m_pEncoderParam->v_bFrame);
		m_hEncoder->setProfile(util.mapProfile[m_pEncoderParam->v_profile]);

		m_hEncoder->init();

		// NAL structure pointers to get encoded bitstream & structures
		x264_nal_t* p_nals; // nal list (see definition in x264.h)
		int i_nals; // number of nals
		int frame_size; // encoded number of bytes


		frame_size = m_hEncoder->encode_headers(&p_nals, &i_nals);// p_nals: nal data array // i_nals: # of nals

		// output encoded header NALs (SPS,PPS etc) to file
		if (frame_size >= 0) {
			for (int i = 0; i < i_nals; i++) {

				int nal_unit_type = (p_nals[i].p_payload[4] & 0x1F);

				if (nal_unit_type == 7)// SPS
				{
					dsi_t	pSps;
					pSps.len = p_nals[i].i_payload - 4;
					memcpy(pSps.data, &p_nals[i].p_payload[4], pSps.len);
				}
				else if (nal_unit_type == 8)// PPS
				{
					dsi_t	pPps;
					pPps.len = p_nals[i].i_payload - 4;
					memcpy(pPps.data, &p_nals[i].p_payload[4], pPps.len);
				}
			}
		}
	}
#ifdef NVIDIA
	else if (m_pEncoderParam->v_encoder == nvEnc)
	{
		m_hEncoder_nv = new NvEncDll();

		sEncConfig* encConfig = new sEncConfig();
		encConfig->width = m_pEncoderParam->v_width;
		encConfig->height = m_pEncoderParam->v_height;
		encConfig->frameRate = m_pEncoderParam->v_fps;
		encConfig->profile = m_pEncoderParam->v_profile;// util.mapProfile[m_pEncoderParam->v_profile];
		encConfig->RateControlMode = m_pEncoderParam->v_rateControl;// util.mapRateControlMode[m_pEncoderParam->v_rateControl];
		encConfig->bitRate = m_pEncoderParam->v_avgBitrate;
		encConfig->EntropyMode = m_pEncoderParam->v_entropyCoding;// util.mapEntropyMode[m_pEncoderParam->v_entropyCoding];
		encConfig->gopSize = m_pEncoderParam->v_gopSize;

		m_hEncoder_nv->createEncoder(m_pEncoderParam->v_iGpu, m_pEncoderParam->v_width, m_pEncoderParam->v_height, fpsNum, "bgra", encConfig, this, Callback_NvEncEncoded);
	}

#endif
	m_bEncoderCreated = TRUE;
}

void CVideoEncoder::deleteEncoder()
{
	CAutoLock lock(m_sema);

	if (m_pEncoderParam->v_encoder == x264)
	{
		if (m_hEncoder)
		{
			delete m_hEncoder;
			m_hEncoder = nullptr;
		}
	}
#ifdef NVIDIA
	else if (m_pEncoderParam->v_encoder == nvEnc)
	{
		if (m_hEncoder_nv)
		{
			m_hEncoder_nv->deleteEncoder();

			delete m_hEncoder_nv;
			m_hEncoder_nv = nullptr;
		}

		SAFE_DELETE(m_sps);
		SAFE_DELETE(m_pps);
	}
#endif

	m_bEncoderCreated = FALSE;
}

void CVideoEncoder::encodeFrame(unsigned char* pData, int nSize, int nWidth, int nHeight, __int64 pts, sEncodedData* pEncoded)
{
	CAutoLock lock(m_sema);

	if (m_bEncoderCreated)
	{
		if (m_pEncoderParam->v_encoder == x264)
		{
			if (!m_hEncoder)
				return;

			x264_nal_t* p_nals = NULL;
			int		    i_nals = 0;
			long long   i_pts = pts;

			m_hEncoder->prepare_oneframe(pData, i_pts);
			int i_frame_size = m_hEncoder->encode_oneframe(&p_nals, &i_nals, &i_pts);//  p_nals : NAL data (array), i_nals : # of NALs
			if (i_frame_size <= 0)
				return;

			int i_payload_total = 0;
			int i_payload_pos = 0;
			CData* pdata = nullptr;

			for (int i = 0; i < i_nals; i++)
			{
				int i_payload = p_nals[i].i_payload;
				int i_type = p_nals[i].i_type;
				unsigned char* p_payload = p_nals[i].p_payload;

				if (i_type == NAL_SEI)
				{
					continue;
				}
				else if ((i_type == NAL_SPS) || (i_type == NAL_PPS))
				{
					CData* pdata = new CData(0, pts);// , p_nals[i].b_long_startcode);
					if (p_nals[i].b_long_startcode == 0)
					{
						pdata->setData(p_payload, i_payload, 1);
					}
					else
					{
						pdata->setData(p_payload, i_payload);
					}
					//pdata->setLongStartCode(1);

					if (m_queue)
					{
#ifdef ENCODED_DATA_SAVE_AS_FILE
						try {
							FILE* fh264;
							fopen_s(&fh264, "encoded_x264.h264", "ab+");
							fwrite(pdata->m_data, sizeof(unsigned char), pdata->m_size, fh264);
							fclose(fh264);
						}
						catch (const std::exception& e)
						{
							char log[128];
							sprintf_s(log, "encoed data save as file fail, error : %s\n", e.what());
							OutputDebugStringA(log);
						}
#endif

						m_queue->push(pdata);
						*m_sendingPts = m_queue->getFirstPTS();

						pData = nullptr;
					}

					if (pEncoded)//run as External Encoder
					{
						pEncoded->pData[pEncoded->index++] = pdata;
					}
				}
				else if (i_payload)
				{
					if (i_payload_total == 0)
					{
						pdata = new CData(0, pts);
					}

					if (pdata)
					{
						if (p_nals[i].b_long_startcode == 0)
						{
							pdata->setData(p_payload, i_payload, i_payload_pos + 1);

							i_payload_pos += 1;
							i_payload_total += 1;
						}
						else
						{
							pdata->setData(p_payload, i_payload, i_payload_pos);
						}

						i_payload_pos += i_payload;
						i_payload_total += i_payload;
					}
				}
			}
			//pdata->setLongStartCode(1);
			if (m_queue && i_payload_total > 0)
			{
				if (pdata->m_data != nullptr)
				{
#ifdef ENCODED_DATA_SAVE_AS_FILE
					try {
						FILE* fh264;
						fopen_s(&fh264, "encoded_x264.h264", "ab+");
						fwrite(pdata->m_data, sizeof(unsigned char), pdata->m_size, fh264);
						fclose(fh264);
					}
					catch (const std::exception& e)
					{
						char log[128];
						sprintf_s(log, "encoed data save as file fail, error : %s\n", e.what());
						OutputDebugStringA(log);
					}
#endif
				}

				m_queue->push(pdata);
				*m_sendingPts = m_queue->getFirstPTS();

				pData = nullptr;
			}

			if (pEncoded && i_payload_total > 0)//run as External Encoder
			{
				pEncoded->pData[pEncoded->index++] = pdata;
			}

			if (i_payload_total == 0)
			{
				if (pData)
				{
					delete pData;
					pData = nullptr;
				}
			}
		}
#ifdef NVIDIA
		else if (m_pEncoderParam->v_encoder == nvEnc)
		{
#ifdef USE_ENCODING_THREAD
			m_hEncoder_nv->encodeFrame(pData, nSize, nullptr, nullptr, pts);
#else
			CData* streamData = new CData(nvEnc, pts);
			m_hEncoder_nv->encodeFrame(pData, nSize, streamData->m_data, &streamData->m_size, pts);

			if (streamData->m_size > 0)
			{
#ifdef ENCODED_DATA_SAVE_AS_FILE
				FILE* fh264;
				fopen_s(&fh264, "encoded_nv.h264", "ab+");
				fwrite(streamData->m_data, sizeof(unsigned char), streamData->m_size, fh264);
				fclose(fh264);
#endif

				//char filename[64];
				//sprintf(filename, "nvEnc_%05d.dat", ++index);
				//FILE* f;
				//fopen_s(&f, filename, "wb");
				//fwrite(streamData->m_data, sizeof(uint8_t), streamData->m_size, f);
				//fclose(f);


				if ((streamData->m_data[0] == 0x00 && streamData->m_data[1] == 0x00 && streamData->m_data[2] == 0x00 && streamData->m_data[3] == 0x01 && streamData->m_data[4] == 0x67))
				{
					int pps_pos = -1, idr_pos = -1, pps_len = -1;
					for (int i = 0; i < 100; i++)
					{
						if ((streamData->m_data[i] == 0x00 && streamData->m_data[i + 1] == 0x00 && streamData->m_data[i + 2] == 0x00 && streamData->m_data[i + 3] == 0x01 && streamData->m_data[i + 4] == 0x68))
							pps_pos = i;
						if ((streamData->m_data[i] == 0x00 && streamData->m_data[i + 1] == 0x00 && streamData->m_data[i + 2] == 0x00 && streamData->m_data[i + 3] == 0x01 && streamData->m_data[i + 4] == 0x65))
						{
							idr_pos = i;
							pps_len = idr_pos - pps_pos;
							break;
						}
					}

					if (m_sps == nullptr && pps_pos > 0)
					{
						m_sps = new CData(nvEnc, pps_pos, streamData->m_data, pts);
					}
					if (m_sps && m_queue && m_sps->m_size > 0)
					{
						CData* temp_sps = new CData(nvEnc, m_sps->m_size, m_sps->m_data, pts);
						m_queue->push(temp_sps);
						*m_sendingPts = m_queue->getFirstPTS();

						if (pEncoded)//run as External Encoder
						{
							pEncoded->pData[pEncoded->index++] = m_sps;
						}
					}

					if (m_pps == nullptr && pps_pos > 0 && pps_len > 0)
					{
						m_pps = new CData(nvEnc, pps_len, &streamData->m_data[pps_pos], pts);
					}
					if (m_pps && m_queue)
					{
						CData* temp_pps = new CData(nvEnc, m_pps->m_size, m_pps->m_data, pts);
						m_queue->push(temp_pps);
						*m_sendingPts = m_queue->getFirstPTS();
						if (pEncoded)//run as External Encoder
						{
							pEncoded->pData[pEncoded->index++] = m_pps;
						}
					}

					CData* idr = new CData(nvEnc, streamData->m_size - (idr_pos), &streamData->m_data[idr_pos], pts);
					if (m_queue)
					{
						m_queue->push(idr);
						*m_sendingPts = m_queue->getFirstPTS();
					}
					SAFE_DELETE(streamData);
				}
				else if ((streamData->m_data[0] == 0x00 && streamData->m_data[1] == 0x00 && streamData->m_data[2] == 0x00 && streamData->m_data[3] == 0x01 && streamData->m_data[4] == 0x65))
				{
					if (m_sps && m_queue)
					{
						CData* temp_sps = new CData(nvEnc, m_sps->m_size, m_sps->m_data, pts);
						m_queue->push(temp_sps);
						*m_sendingPts = m_queue->getFirstPTS();

						if (pEncoded)//run as External Encoder
						{
							pEncoded->pData[pEncoded->index++] = m_sps;
						}
					}

					if (m_pps&& m_queue)
					{
						CData* temp_pps = new CData(nvEnc, m_pps->m_size, m_pps->m_data, pts);
						m_queue->push(temp_pps);
						*m_sendingPts = m_queue->getFirstPTS();
						if (pEncoded)//run as External Encoder
						{
							pEncoded->pData[pEncoded->index++] = m_pps;
						}
					}

					//CData* idr = new CData(nvEnc, pts);
					//idr->setData(streamData->m_data, streamData->m_size, 1);
					if (m_queue)
					{
						m_queue->push(streamData);
						*m_sendingPts = m_queue->getFirstPTS();
					}
					//SAFE_DELETE(streamData);
				}
				else
				{
					if (m_queue)
					{
						m_queue->push(streamData);
						*m_sendingPts = m_queue->getFirstPTS();
					}

					if (pEncoded)//run as External Encoder
					{
						pEncoded->pData[pEncoded->index++] = streamData;
					}
				}
			}
			else
			{
				SAFE_DELETE(streamData);
			}
#endif
		}
#endif
	}
}

void CVideoEncoder::Callback_NvEncEncoded(void * pParam, CData* pData)
{
	CVideoEncoder* pThis = (CVideoEncoder*)pParam;

	if (pThis->m_queue)
	{
		CData* data = new CData(pData->m_mode, pData->m_size, pData->m_data, pData->m_pts);
		pThis->m_queue->push(data);
		*pThis->m_sendingPts = pThis->m_queue->getFirstPTS();
	}

	//external Encoder
	//if (mode == 1 && pEncoded)
	//{
	//	pEncoded->pData[pEncoded->index++] = streamData;
	//}
}
