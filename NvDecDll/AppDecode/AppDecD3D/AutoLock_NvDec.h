#pragma once
class CAutoLock_NvDec
{
public:

	inline CAutoLock_NvDec(HANDLE lock)
	{
		if (lock == nullptr)
			mLock = CreateSemaphore(NULL, 1, 1, NULL);// lock;//
		else
			mLock = lock;

		//DWORD dwResult = -1;
		//dwResult = WaitForSingleObject(mLock, INFINITE);
		//switch (dwResult)
		//{
		//case WAIT_OBJECT_0:
		//	break;
		//case WAIT_ABANDONED:
		//	break;
		//case WAIT_TIMEOUT:
		//	break;
		//}
	}

	inline CAutoLock_NvDec(HANDLE lock,  int time)
	{
		if (lock == nullptr)
			mLock = CreateSemaphore(NULL, 1, 1, NULL);// lock;//
		else
			mLock = lock;

		mTime = time;

		//DWORD dwResult = -1;
		//dwResult = WaitForSingleObject(mLock, mTime);
		//switch (dwResult)
		//{
		//case WAIT_OBJECT_0:
		//	break;
		//case WAIT_ABANDONED:
		//	break;
		//case WAIT_TIMEOUT:
		//	break;
		//}
	}

	inline virtual ~CAutoLock_NvDec(void)
	{
		LONG	lPreviousCount = 0;
		::ReleaseSemaphore(mLock, 1, &lPreviousCount);

		CloseHandle(mLock);
		mLock = nullptr;
	}

	inline bool virtual lock()
	{
		DWORD dwResult = -1;
		dwResult = WaitForSingleObject(mLock, INFINITE);

		bool ret = FALSE;

		switch (dwResult)
		{
		case WAIT_OBJECT_0:
			ret = TRUE;
			break;
		case WAIT_ABANDONED:
			break;
		case WAIT_TIMEOUT:
			break;
		}

		return ret;
	}

	inline bool virtual lock_time()
	{
		DWORD dwResult = -1;
		dwResult = WaitForSingleObject(mLock, mTime);

		bool ret = FALSE;
		
		switch (dwResult)
		{
		case WAIT_OBJECT_0:
			ret = TRUE;
			break;
		case WAIT_ABANDONED:
			break;
		case WAIT_TIMEOUT:
			break;
		}

		return ret;
	}

	inline void virtual unlock()
	{
		LONG	lPreviousCount = 0;
		::ReleaseSemaphore(mLock, 1, &lPreviousCount);
	}

private:

	HANDLE	mLock;
	int mTime;
};

