// RTSPServerDLL.cpp : Defines the exported functions for the DLL application.
//
#include "stdafx.h"

#include "RTSPServerDLL.h"

#include "CRTSPServerMain.h"

int getLocal_IPAddr(char* ipAddr);

CRTSPServerDLL::CRTSPServerDLL(std::string sIP, int nPort, std::string sID, std::string sPW, eProtocolMode mode, int nMTU)
{
	std::string ip;
	ip = sIP;
	struct sockaddr_in sa;
	int ret = inet_pton(AF_INET, sIP.c_str(), &sa.sin_addr);//skkang 20200303 check for string is ipaddress, is 0 success
	if (!ret)
	{
		ip = "127.0.0.1";		
	}

	if (ip.compare("127.0.0.1") == 0)
	{	
		char tmp[64];
		getLocal_IPAddr(tmp);
		ip = tmp;
	}

	CRTSPServerMain* server = new CRTSPServerMain(nMTU, ip, nPort, sID, sPW);
	if (server)
	{
		server->open(mode); //server create
		m_Server = (HANDLE)server;
	}
}

CRTSPServerDLL::~CRTSPServerDLL()
{
	if (m_Server) 
	{
		CRTSPServerMain* server = (CRTSPServerMain*)m_Server;
		server->close(); //server destroy
		delete server;
		m_Server = nullptr;
	}
}

eStatus CRTSPServerDLL::open(eProtocolMode mode)
{
	eStatus ret = fail;
	if (m_Server)
	{
		CRTSPServerMain* server = (CRTSPServerMain*)m_Server;
		ret = server->open(mode);
	}

	return ret;
}

void CRTSPServerDLL::close()
{
	if (m_Server)
	{
		CRTSPServerMain* server = (CRTSPServerMain*)m_Server;
		server->close();
	}
}

int CRTSPServerDLL::add_channel(int nChannelIndex, std::string streamName, void* param, CallBackSetConnCnt* func, sEncoderParam* pEncParam, int nClientMAX)
{
	int ret = -1;
	if (m_Server)
	{
		CRTSPServerMain* server = (CRTSPServerMain*)m_Server;
		ret = server->add_channel(nChannelIndex, streamName, param, func, nClientMAX, pEncParam);
	}

	return ret;
}

eStatus CRTSPServerDLL::remove_channel(int nChannelIndex)
{
	eStatus ret = eStatus::fail;
	if (m_Server)
	{
		CRTSPServerMain* server = (CRTSPServerMain*)m_Server;
		ret = server->remove_channel(nChannelIndex);
	}

	return ret;
}

eStatus CRTSPServerDLL::alivecheck_channel(int nChannelIndex)
{
	eStatus ret = eStatus::fail;
	if (m_Server)
	{
		CRTSPServerMain* server = (CRTSPServerMain*)m_Server;
		ret = server->alivecheck_channel(nChannelIndex);
	}

	return ret;
}

void CRTSPServerDLL::getURL_channel(int nChannelIndex, char* url)
{
	if (m_Server)
	{
		CRTSPServerMain* server = (CRTSPServerMain*)m_Server;
		server->getURL_channel(nChannelIndex, url);
	}
}

eStatus CRTSPServerDLL::pushto(const int nChannelIndex, unsigned char* pData, int nSize, int nWidth, int nHeight, eEncoderType mode)
{
	eStatus ret = eStatus::fail;
	if (m_Server)
	{
		CRTSPServerMain* server = (CRTSPServerMain*)m_Server;
		ret = server->pushto(nChannelIndex, pData, nSize, nWidth, nHeight, mode);
	}

	return ret;
}



int getLocal_IPAddr(char* ipAddr)
{
	WSADATA WSAData;

	// Initialize winsock dll
	if (::WSAStartup(MAKEWORD(1, 0), &WSAData))
	{
		return -1;// Error handling
	}

	// Get local host name
	char szHostName[128] = "";

	if (::gethostname(szHostName, sizeof(szHostName)))
	{
		return -1;// Error handling -> call 'WSAGetLastError()'
	}

	// Get local IP addresses
	struct sockaddr_in SocketAddress;
	struct hostent     *pHost = 0;

	pHost = ::gethostbyname(szHostName);
	if (!pHost)
	{
		// Error handling -> call 'WSAGetLastError()'
	}

	memcpy(&SocketAddress.sin_addr, pHost->h_addr_list[0], pHost->h_length);
	strcpy(ipAddr, inet_ntoa(SocketAddress.sin_addr));

	// Cleanup
	WSACleanup();

	return 0;
}