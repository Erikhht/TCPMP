/*****************************************************************************
 *
 * This program is free software ; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * $Id: multithread_win32.c 343 2005-11-16 20:11:07Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"

#if defined(TARGET_WIN32) || defined(TARGET_WINCE)

//#define LOCK_TIMEOUT

#ifndef STRICT
#define STRICT
#endif
#include <windows.h>

#if defined(TARGET_WINCE)
extern BOOL (WINAPI* FuncCeSetThreadQuantum)(HANDLE, DWORD);
#endif

void* LockCreate()
{
#ifndef LOCK_TIMEOUT
	void* p = malloc(sizeof(CRITICAL_SECTION));
	InitializeCriticalSection((LPCRITICAL_SECTION)p);
	return p;
#else
	return CreateMutex(NULL,FALSE,NULL);
#endif
}

void LockDelete(void* p)
{
	if (p)
	{
#ifndef LOCK_TIMEOUT
		DeleteCriticalSection((LPCRITICAL_SECTION)p);
		free(p);
#else
		CloseHandle(p);
#endif
	}
}

void LockEnter(void* p)
{
	if (p)
#ifndef LOCK_TIMEOUT
		EnterCriticalSection((LPCRITICAL_SECTION)p);
#else
		while (WaitForSingleObject(p,3000)==WAIT_TIMEOUT)
			DebugBreak();
#endif
}

void LockLeave(void* p)
{
	if (p)
#ifndef LOCK_TIMEOUT
		LeaveCriticalSection((LPCRITICAL_SECTION)p);
#else
		ReleaseMutex(p);
#endif
}

int ThreadPriority(void* Thread,int Priority)
{
	int Old;
	if (!Thread) Thread = GetCurrentThread();
	Old = GetThreadPriority(Thread);

#if defined(TARGET_WINCE)
	Priority = THREAD_PRIORITY_NORMAL + Priority;
	if (Priority > THREAD_PRIORITY_IDLE)
		Priority = THREAD_PRIORITY_IDLE;
	if (Priority < THREAD_PRIORITY_TIME_CRITICAL)
		Priority = THREAD_PRIORITY_TIME_CRITICAL;
	SetThreadPriority(Thread,Priority);
	if (FuncCeSetThreadQuantum)
	{
		if (Old == THREAD_PRIORITY_TIME_CRITICAL && Priority != THREAD_PRIORITY_TIME_CRITICAL)
			FuncCeSetThreadQuantum(Thread,25);
		if (Old != THREAD_PRIORITY_TIME_CRITICAL && Priority == THREAD_PRIORITY_TIME_CRITICAL)
			FuncCeSetThreadQuantum(Thread,250);
	}
	return Old - THREAD_PRIORITY_NORMAL;
#else
	Priority = THREAD_PRIORITY_NORMAL - Priority;
	if (Priority < THREAD_PRIORITY_IDLE)
		Priority = THREAD_PRIORITY_IDLE;
	if (Priority > THREAD_PRIORITY_TIME_CRITICAL)
		Priority = THREAD_PRIORITY_TIME_CRITICAL;
	SetThreadPriority(Thread,Priority);
	return -(Old - THREAD_PRIORITY_NORMAL);
#endif
}

bool_t EventWait(void* Handle,int Time) { return WaitForSingleObject(Handle,Time) == WAIT_OBJECT_0; } // -1 = INFINITE
void* EventCreate(bool_t ManualReset,bool_t InitState) { return CreateEvent(NULL,ManualReset,InitState,NULL); }
void EventSet(void* Handle) { SetEvent(Handle); }
void EventReset(void* Handle) { ResetEvent(Handle); }
void EventClose(void* Handle) { CloseHandle(Handle); }

int ThreadId() { return GetCurrentThreadId(); }
void ThreadSleep(int Time) { Sleep(Time); }
void ThreadTerminate(void* Handle)
{
	if (Handle && WaitForSingleObject(Handle,5000) == WAIT_TIMEOUT)
		TerminateThread(Handle,0);
}

void* ThreadCreate(int(*Start)(void*),void* Parameter,int Quantum)
{
	DWORD Id;
	HANDLE Handle = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)Start,Parameter,0,&Id);

#if defined(TARGET_WINCE)
	if (FuncCeSetThreadQuantum)
		FuncCeSetThreadQuantum(Handle,Quantum);
#endif
	return Handle;
}

#endif
