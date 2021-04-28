#pragma once
//	skkang 161012
//	CDataQueuePos : CEncData를 list로 관리
//	- CData* m_DataQueue[ENCODED_DATA_BUFFER_MAX] : 인코딩 데이터 저장변수를 미리 생성, 배열로 관리
//	- list<int> m_QueuePos : 사용중인 저장변수의 배열 index를 list로 관리
//	- int m_writePos : 현재 list에 배열 index
//	- HANDLE m_hMTXQueuePos : 멀티 쓰레드에서 m_EncPosQueue에 접근하는 것을 동기화

#include <stdio.h>
#include <Windows.h>
#include <list>
using namespace std;

#define ENCODED_DATA_BUFFER_MAX 16

class CDataQueuePos
{
public:
	CDataQueuePos() : m_writePos(0)
	{
		for (int i = 0; i < ENCODED_DATA_BUFFER_MAX; i++)
		{	
            m_Data[i] = new unsigned char[1024 * 1024 * 2];
		}
		m_hMTXQueuePos = CreateMutex(nullptr, FALSE, nullptr);

		clearQueuePos();
	}

	~CDataQueuePos()
	{
		for (int i = 0; i < ENCODED_DATA_BUFFER_MAX; i++)
		{
			if (m_Data[i])
			{
				delete[] m_Data[i];
			}
		}

		clearQueuePos();

		CloseHandle(m_hMTXQueuePos);
		m_hMTXQueuePos = nullptr;
	}

public:
	inline void clearQueuePos()
	{
		WaitForSingleObject(m_hMTXQueuePos, INFINITE);
		m_writePos = 0;
        m_QueuePos.clear();
		ReleaseMutex(m_hMTXQueuePos);
	}

	inline void popFrontPos()
	{
		WaitForSingleObject(m_hMTXQueuePos, INFINITE);

		if (m_QueuePos.size() > 0)
		{
            m_QueuePos.pop_front();
		}

		ReleaseMutex(m_hMTXQueuePos);
	}

	inline void popBackPos()
	{
		WaitForSingleObject(m_hMTXQueuePos, INFINITE);

		if (m_QueuePos.size() > 0)
		{	
            m_QueuePos.pop_back();
			if(m_writePos)
				m_writePos--;
		}

		ReleaseMutex(m_hMTXQueuePos);
	}

	inline unsigned char* getPushAddr()
	{
		WaitForSingleObject(m_hMTXQueuePos, INFINITE);
		if (m_writePos >= ENCODED_DATA_BUFFER_MAX)
		{
			m_writePos = 0;
		}
		int pos = m_writePos;
		if (m_QueuePos.size() >= ENCODED_DATA_BUFFER_MAX)
		{
			OutputDebugStringA("[CDataQueuePos] m_EncData bufferful! \r\n");
			ReleaseMutex(m_hMTXQueuePos);
			return nullptr;
		}

        m_QueuePos.push_back(pos);
		m_writePos++;

		ReleaseMutex(m_hMTXQueuePos);

		return m_Data[pos];
	}

	inline unsigned char* getPopAddr()
	{
		int pos = -1;

		WaitForSingleObject(m_hMTXQueuePos, INFINITE);
		if (m_QueuePos.size() > 0)
		{
			pos = m_QueuePos.front();
		}
		ReleaseMutex(m_hMTXQueuePos);

		if (pos >= 0)
		{
			return m_Data[pos];
		}

		return nullptr;
	}

private:
	unsigned char* m_Data[ENCODED_DATA_BUFFER_MAX];

	list<int> m_QueuePos;
	int m_writePos;
	HANDLE m_hMTXQueuePos;
};