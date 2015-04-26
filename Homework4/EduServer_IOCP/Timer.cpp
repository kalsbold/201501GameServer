#include "stdafx.h"
#include "ThreadLocal.h"
#include "Exception.h"
#include "SyncExecutable.h"
#include "Timer.h"



Timer::Timer()
{
	LTickCount = GetTickCount64();
}


void Timer::PushTimerJob(SyncExecutablePtr owner, const TimerTask& task, uint32_t after)
{
	CRASH_ASSERT(LThreadType == THREAD_IO_WORKER);

	//DONE: mTimerJobQueue에 TimerJobElement를 push..
	//집어넣는 시점으로부터 after만큼 뒤에 실행되도록.
	mTimerJobQueue.push(TimerJobElement(owner, task, LTickCount + after));
}


void Timer::DoTimerJob()
{
	/// thread tick update
	LTickCount = GetTickCount64();

	while (!mTimerJobQueue.empty())
	{
		//이 놈이 문제다!
		//이상하게 거는 lock과 나오는 lock이 계속 다름.
		//lock을 걸고 작업 수행중에 다른 스레드들에서 작업을 push하면 뭔가 꼬이면서 crash가 난다.
		//잘 찾아보니 STLAllocator에서 전역 메모리 풀을 이용해서 메모리를 관리함
		//이 놈이 push할 때 vector의 메모리 연속성을 유지하기 위해 메모리 상에 존재하는 원소들 위치를 옮길 수 있다
		//메모리 위치 옮김 -> 참조자는 원래 위치를 가리키고 있음 -> 폭망!
		//따라서 참조자가 아니라 값 복사를 이용하면 아무 문제 없을 듯.
		TimerJobElement timerJobElem = mTimerJobQueue.top();

		if (LTickCount < timerJobElem.mExecutionTick)
			break;

		timerJobElem.mOwner->EnterLock();
		
		timerJobElem.mTask();

		timerJobElem.mOwner->LeaveLock();
		
		mTimerJobQueue.pop();
	}


}

