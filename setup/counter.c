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
 * $Id: counter.c 271 2005-08-09 08:31:35Z picard $
 *
 ****************************************************************************/

#include "../common/common.h"
#include <windows.h>
#include "counter.h"

void (STDCALL *GetCounter)(int64_t*) = NULL;

#if defined( MIPS)
void STDCALL GetTimeCounter(int64_t* v)
{
	SYSTEMTIME t;
	GetSystemTime(&t);
	*v = (t.wDay*24+t.wHour)*60*60*1000+(t.wMinute*60+t.wSecond)*1000+t.wMilliseconds;
}
#elif defined(TARGET_WINCE)
void STDCALL GetTimeCounter(int64_t* v)
{
	*v = GetTickCount();
}
#else
void STDCALL GetTimeCounter(int64_t* v)
{
	*v = timeGetTime();
}
#endif

#if defined(TARGET_WINCE)
static HANDLE CoreDLL = NULL;
static BOOL (WINAPI* FuncQueryPerformanceCounter)(LARGE_INTEGER*) = NULL;
static BOOL (WINAPI* FuncQueryPerformanceFrequency)(LARGE_INTEGER*) = NULL;
#else
#define FuncQueryPerformanceFrequency QueryPerformanceFrequency
#define FuncQueryPerformanceCounter QueryPerformanceCounter
#endif

void BeginCounter(int64_t* p)
{
#if defined(TARGET_WINCE)
	CoreDLL = LoadLibrary(T("coredll.dll"));
	if (CoreDLL)
	{
		*(FARPROC*)&FuncQueryPerformanceCounter = GetProcAddress(CoreDLL,T("QueryPerformanceCounter"));
		*(FARPROC*)&FuncQueryPerformanceFrequency = GetProcAddress(CoreDLL,T("QueryPerformanceFrequency"));
	}
#endif

	if (FuncQueryPerformanceFrequency && 
		FuncQueryPerformanceCounter &&
		FuncQueryPerformanceFrequency((LARGE_INTEGER*)p) && *p>5000)
	{
		GetCounter = (void(STDCALL*)(int64_t*)) QueryPerformanceCounter;
	}
	else
	{
		GetCounter = GetTimeCounter;
		*p = 1000;
	}
}

void EndCounter()
{
#if defined(TARGET_WINCE)
	if (CoreDLL) FreeLibrary(CoreDLL);
#endif
}
