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
 * $Id: benchmark.c 332 2005-11-06 14:31:57Z picard $
 *
 ****************************************************************************/

#include "../common/common.h"
#include "counter.h"
#include <windows.h>

#undef malloc
#undef free

#ifdef UNICODE
#define tcsicmp _wcsicmp
#else
#define tcsicmp stricmp;
#endif

void Swap( int* a, int* b)
{
	int t = *a;
	*a = *b;
	*b = t;
}

void SlowVideo(char* Video,int BPP,int Width,int Height,int StrideX,int StrideY,int32_t* Result)
{
	if (BPP == 16)
	{
		int64_t t0,t1,t;
		int nv,ns;
		int Mul,Len;

		int SystemLength = 512*1024;
		char* System = (char*) malloc( SystemLength ); //hopefully cpu cache will be smaller

		if (System)
		{
			TRY_BEGIN

			if (StrideX < 0)
			{
				StrideX = -StrideX;
				Video -= StrideX * (Width - 1);
			}

			if (StrideY < 0)
			{
				StrideY = -StrideY;
				Video -= StrideY * (Height - 1);
			}

			if (StrideX > StrideY)
			{
				Swap(&Width,&Height);
				Swap(&StrideX,&StrideY);
			}		

			Len = (Width*BPP) >> 3;

			if (Len>32 && !IsBadWritePtr(Video,Len) && !IsBadReadPtr(Video,Len))
			{
				int i;
				int Rows = SystemLength / Len;

				memset(System,0,Rows*Len);
				memcpy(System,Video,Len);

				BeginCounter(&t);

				t >>= 1; // 0.5 sec

				SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_HIGHEST);

				nv=0;
				GetCounter(&t0);
				t0 += t;				
				do
				{
					for (i=0;i<Rows;++i)
						memcpy(Video,System,Len);
					nv++;
					GetCounter(&t1);
				} while (t1 < t0);

				ns=0;
				GetCounter(&t0);
				t0 += t;				
				do
				{
					for (i=0;i<Rows;i++)
						memcpy(System+i*Len,System,Len);
					ns++;
					GetCounter(&t1);
				} while (t1 < t0);

				Mul = 6;

				if (nv*Mul >= ns)
					*Result = 0; // fast
				else
					*Result = 1; // slow (video memory or in general)

				SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_NORMAL);
				EndCounter();

#ifdef BENCH
				{
					tchar_t Msg[256];
					stprintf_s(Msg,TSIZEOF(Msg),T("Video %d\nSystem %d\nResult %d"),nv,ns,*Result);
					MessageBox(NULL,Msg,T(""),MB_OK|MB_SETFOREGROUND);
				}
#endif
			}

			TRY_END

			free(System);
		}
	}

}

//----------------------------------------------------------------------

void SlowVideoRAW(int32_t* Result)
{
	RawFrameBufferInfo Info;
	HDC DC = GetDC(NULL);

	memset(&Info,0,sizeof(Info));

	if (ExtEscape(DC, GETRAWFRAMEBUFFER, 0, NULL, sizeof(RawFrameBufferInfo), (char*)&Info) >= 0 && Info.pFramePointer)
		SlowVideo(Info.pFramePointer,Info.wBPP,Info.cxPixels,Info.cyPixels,Info.cxStride,Info.cyStride,Result);
	else
	{
		tchar_t PlatformType[256];
		int SmartPhone = 0;
		OSVERSIONINFO Ver;
		Ver.dwOSVersionInfoSize = sizeof(Ver);
		GetVersionEx(&Ver);

		if (SystemParametersInfo(SPI_GETPLATFORMTYPE,sizeof(PlatformType),PlatformType,0)!=0)
		{
			if (tcsicmp(PlatformType,T("Smartphone"))==0)
				SmartPhone = 1;
		}
		else
		if (GetLastError()==ERROR_ACCESS_DENIED)
			SmartPhone = 1;

		if (Ver.dwMajorVersion*100 + Ver.dwMinorVersion >= 421 && !SmartPhone)
		{
			//try gxinfo
			DWORD Code = GETGXINFO;
			if (ExtEscape(DC, ESC_QUERYESCSUPPORT, sizeof(DWORD), (char*)&Code, 0, NULL) > 0)
			{
				DWORD DCWidth = GetDeviceCaps(DC,HORZRES);
				DWORD DCHeight = GetDeviceCaps(DC,VERTRES);
				GXDeviceInfo Info;
				memset(&Info,0,sizeof(Info));
				Info.Version = 100;
				ExtEscape(DC, GETGXINFO, 0, NULL, sizeof(Info), (char*)&Info);

				if (Info.cbStride>0 && !(Info.ffFormat & kfLandscape) &&
					((DCWidth == Info.cxWidth && DCHeight == Info.cyHeight) ||
					 (DCWidth == Info.cyHeight && DCHeight == Info.cxWidth)))
					SlowVideo(Info.pvFrameBuffer,Info.cBPP,Info.cxWidth,Info.cyHeight,Info.cBPP >> 3,Info.cbStride,Result);
			}
		}
	}

	ReleaseDC(NULL,DC);
}

//----------------------------------------------------------------------

typedef struct GXDisplayProperties 
{
	int32_t cxWidth;
	int32_t cyHeight;
	int32_t cbxPitch;
	int32_t cbyPitch;
	int32_t cBPP;
	int32_t ffFormat;
} GXDisplayProperties;

#define GX_FULLSCREEN		0x01

void SlowVideoGAPI(HMODULE GAPI,HWND Wnd,int32_t* Result)
{
	int (*GXOpenDisplay)(HWND hWnd, DWORD dwFlags);
	int (*GXCloseDisplay)();
	char* (*GXBeginDraw)();
	int (*GXEndDraw)();
	GXDisplayProperties (*GXGetDisplayProperties)();
	BOOL (*GXIsDisplayDRAMBuffer)();

	*(FARPROC*)&GXOpenDisplay = GetProcAddress(GAPI,T("?GXOpenDisplay@@YAHPAUHWND__@@K@Z"));
	*(FARPROC*)&GXCloseDisplay = GetProcAddress(GAPI,T("?GXCloseDisplay@@YAHXZ"));
	*(FARPROC*)&GXBeginDraw = GetProcAddress(GAPI,T("?GXBeginDraw@@YAPAXXZ"));
	*(FARPROC*)&GXEndDraw = GetProcAddress(GAPI,T("?GXEndDraw@@YAHXZ"));
	*(FARPROC*)&GXGetDisplayProperties = GetProcAddress(GAPI,T("?GXGetDisplayProperties@@YA?AUGXDisplayProperties@@XZ"));
	*(FARPROC*)&GXIsDisplayDRAMBuffer = GetProcAddress(GAPI,T("?GXIsDisplayDRAMBuffer@@YAHXZ"));

	if (GXOpenDisplay && GXCloseDisplay &&
		GXBeginDraw && GXEndDraw && GXGetDisplayProperties &&
		GXOpenDisplay(Wnd,GX_FULLSCREEN) != 0)
	{
		if (!GXIsDisplayDRAMBuffer || !GXIsDisplayDRAMBuffer())
		{
			char* Ptr;
			GXDisplayProperties Info = GXGetDisplayProperties();

			Ptr = GXBeginDraw();
			if (Ptr)
			{
				SlowVideo(Ptr,Info.cBPP,Info.cxWidth,Info.cyHeight,Info.cbxPitch,Info.cbyPitch,Result);
				GXEndDraw();
			}
		}
		GXCloseDisplay();
	}
}

#ifdef BENCH

#define SRAM_BASE	0x5c000000

#define TBL_CACHE	0x08
#define TBL_BUFFER	0x04

extern BOOL WINAPI VirtualSetAttributes( LPVOID lpvAddress, DWORD cbSize, DWORD dwNewFlags, DWORD dwMask, LPDWORD lpdwOldFlags );
extern BOOL WINAPI VirtualCopy(LPVOID, LPVOID, DWORD, DWORD);

void SlowSDRAM(int32_t* Result)
{
	char* Buffer = malloc(320*240*2);
	SlowVideo(Buffer,16,240,320,2,480,Result);
	free(Buffer);
}

void SlowSRAM(int32_t* Result)
{
	DWORD Size = (320*240*2 + 4095) & ~4095;
	int BufferPhy = SRAM_BASE;
	void* Buffer = VirtualAlloc(0, Size, MEM_RESERVE, PAGE_NOACCESS);
	if (!Buffer)
		return;

	if (!VirtualCopy(Buffer,(LPVOID)(BufferPhy >> 8), Size, PAGE_READWRITE | PAGE_NOCACHE | PAGE_PHYSICAL))
		return;

	if (!VirtualSetAttributes(Buffer,320*240*2,TBL_BUFFER,TBL_CACHE|TBL_BUFFER,NULL))
		return;

	SlowVideo(Buffer,16,240,320,2,480,Result);

	VirtualFree(Buffer,0,MEM_RELEASE);
}

int WinMain(HINSTANCE Instance,HINSTANCE Parent,TCHAR* Cmd,int CmdShow)
{
	int32_t Result;
	//SAFE_BEGIN
/*	HMODULE GAPI = LoadLibrary(T("gx.dll"));
	if (GAPI)
	{
		SlowVideoGAPI(GAPI,NULL,&Result);
		FreeLibrary(GAPI);
	}
*/
	//SlowSDRAM(&Result);
	//SlowSRAM(&Result);
	SlowVideoRAW(&Result);
	//SAFE_END
	return 0;
}
#endif