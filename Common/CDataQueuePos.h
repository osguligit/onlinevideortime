#pragma once
//	skkang 161012
//	CDataQueuePos : CEncData�� list�� ����
//	- CData* m_DataQueue[ENCODED_DATA_BUFFER_MAX] : ���ڵ� ������ ���庯���� �̸� ����, �迭�� ����
//	- list<int> m_QueuePos : ������� ���庯���� �迭 index�� list�� ����
//	- int m_writePos : ���� list�� �迭 index
//	- HANDLE m_hMTXQueuePos : ��Ƽ �����忡�� m_EncPosQueue�� �����ϴ� ���� ����ȭ

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