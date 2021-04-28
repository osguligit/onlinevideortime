#pragma once

#include <list>
#include "../Common/AutoLock.h"

#define QUEUE_MAXSIZE 100

class CData
{
public:
	CData(int mode, __int64 pts, int bLongStartCode = 0)
	{
		m_mode = mode;
		m_data = new unsigned char[1024 * 1024 * 2];
		if (m_data == nullptr)
		{
			OutputDebugStringA("memory alloc error\n");
		}
		memset(m_data, 0, (1024 * 1024 * 2));
		m_pts = pts;
		m_bLongStartCode = bLongStartCode;
	}
	CData(int mode, int size, unsigned char* data, __int64 pts, int bLongStartCode = 0)
	{
		m_mode = mode;
		m_size = size;
		m_data = new unsigned char[size];// +1];
		if (m_data == nullptr)
		{
			OutputDebugStringA("memory alloc error\n");
		}
		memcpy(m_data, data, size);
		m_pts = pts;
		m_bLongStartCode = bLongStartCode;

		calcLongStartCode();
	}
	~CData()
	{
		if (m_data)
		{
			delete[] m_data;
			m_data = nullptr;
		}
	}

	void setSize(int size) { m_size = size; }
	void setData(unsigned char* data, int size, int pos = 0)
	{
		memcpy(m_data + pos, data, size);
		m_size = size + pos;

		calcLongStartCode();
	}
	void setLongStartCode(int bLongStartCode) { m_bLongStartCode = bLongStartCode; }
	int getLongStartCode() { return m_bLongStartCode; }
	void calcLongStartCode()
	{
		if (m_data[0] == 0x00 && m_data[1] == 0x00 && m_data[2] == 0x01)
			m_bLongStartCode = 3;
		if (m_data[0] == 0x00 && m_data[1] == 0x00 && m_data[2] == 0x00 && m_data[3] == 0x01)
			m_bLongStartCode = 4;
	}

	int m_mode;
	int m_size;
	unsigned char* m_data;
	__int64 m_pts;
	int m_bLongStartCode; // [0]noStartCode, [3]0x00 0x00 0x01 [4]0x00 0x00 0x00 0x01
};

class CDataQueue
{
public:
	CDataQueue(int nMaxSize = QUEUE_MAXSIZE) :m_nMaxSize(nMaxSize)
	{
		m_hSemaphore = CreateSemaphore(nullptr, 1, 1, nullptr);
		m_pts = 0;
		clear();
	}
	~CDataQueue()
	{
		clear();

		if (m_hSemaphore)
		{
			CloseHandle(m_hSemaphore);
			m_hSemaphore = nullptr;
		}
	}

	void clear()
	{
		CAutoLock lock(m_hSemaphore);
		//std::list<CData*>::iterator iter = m_queue.begin();
		//for (; iter != m_queue.end(); iter++)
		//{
		//	delete *iter;
		//}
		while (m_queue.size() > 0)
		{
			CData* data = m_queue.front();
			if(data)
				delete data;

			m_queue.pop_front();
		}
		m_queue.clear();
	}

	int push(CData* data)
	{
		int ret = 0;
		CAutoLock lock(m_hSemaphore);
		if (m_queue.size() < m_nMaxSize)
			m_queue.push_back(data);
		else
			ret = -1;

		if (m_queue.size() > 0)
		{
			CData* data = m_queue.front();
			m_pts = data->m_pts;
		}

		return ret;
	}

	CData* pop()
	{
		CAutoLock lock(m_hSemaphore);
		if (m_queue.size() <= 0)
		{
			return nullptr;
		}

		CData* data = m_queue.front();
		m_queue.pop_front();

		if (data->m_size <= data->m_bLongStartCode)
		{
			delete data;
			data = nullptr;
		}

		return data;
	}

	int size()
	{
		return m_queue.size();
	}

	__int64 getFirstPTS()
	{
		return m_pts;
	}

	HANDLE m_hSemaphore;
	std::list<CData*>m_queue;
	const int m_nMaxSize;
	__int64 m_pts;
};