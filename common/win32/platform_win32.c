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
 * $Id: platform_win32.c 622 2006-01-31 19:02:53Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"

#if defined(TARGET_WIN32) || defined(TARGET_WINCE)

#ifndef STRICT
#define STRICT
#endif
#include <windows.h>

// registry backups of original value
#define REG_BATTERYTIMEOUT	 0x2000
#define REG_ACTIMEOUT		 0x2001
#define REG_SCREENSAVER		 0x2002
#define REG_DISPPOWER	 	 0x2003

#define REG_ACSUSPEND		 0x2004
#define REG_BATTSUSPEND		 0x2005
#define REG_ACUSERIDLE		 0x2006
#define REG_BATTUSERIDLE	 0x2007
#define REG_ACSYSTEMIDLE	 0x2008
#define REG_BATTSYSTEMIDLE	 0x2009

#define REG_BATTERYTIMEOUT2  0x200A
#define REG_ACTIMEOUT2		 0x200B

void DMO_Init();
void DMO_Done();
void File_Init();
void File_Done();

#if defined(TARGET_WINCE)
#define HKEY_ROOT HKEY_LOCAL_MACHINE
#else
#define HKEY_ROOT HKEY_CURRENT_USER
#endif

#if defined(TARGET_WINCE)

#define POWER_NAME			1
#define POWER_UNSPEC		-1
#define POWER_D0			0
#define POWER_D4			4

#ifndef DISP_CHANGE_SUCCESSFUL
#define DISP_CHANGE_SUCCESSFUL 0
#endif
#ifndef CDS_TEST
#define CDS_TEST            0x00000002
#endif
#ifndef DM_DISPLAYORIENTATION
#define DM_DISPLAYORIENTATION 0x00800000L
#endif
#ifndef DM_DISPLAYQUERYORIENTATION 
#define DM_DISPLAYQUERYORIENTATION 0x01000000L
#endif
#ifndef DMDO_0
#define DMDO_0      0
#endif
#ifndef DMDO_90
#define DMDO_90     1
#endif
#ifndef DMDO_180
#define DMDO_180    2
#endif
#ifndef DMDO_270
#define DMDO_270    4
#endif

#define SPI_GETOEMINFO				258
#define SPI_GETPLATFORMTYPE			257

typedef struct MSGQUEUEOPTIONS2 
{
	DWORD dwSize;
	DWORD dwFlags;
	DWORD dwMaxMessages;
	DWORD cbMaxMessage;
	BOOL  bReadAccess;

} MSGQUEUEOPTIONS2;

BOOL (WINAPI* FuncCeSetThreadQuantum)(HANDLE, DWORD) = NULL;
static HANDLE (WINAPI* FuncSetPowerRequirement)(PVOID,int,ULONG,PVOID,ULONG) = NULL;
static DWORD (WINAPI* FuncReleasePowerRequirement)(HANDLE) = NULL;
static DWORD (WINAPI* FuncSetDevicePower)(PVOID,DWORD,int) = NULL;
static DWORD (WINAPI* FuncGetDevicePower)(PVOID,DWORD,int*) = NULL;
static BOOL (WINAPI* FuncSHGetDocumentsFolder)(LPCTSTR,LPTSTR) = NULL;
static BOOL (WINAPI* FuncSHGetSpecialFolderPath)(HWND,LPTSTR,int,BOOL) = NULL;
static LONG (WINAPI* FuncChangeDisplaySettingsEx)(LPCTSTR,LPDEVMODE,HWND,DWORD,LPVOID) = NULL;
static BOOL (WINAPI* FuncSetKMode)(BOOL) = NULL;
static BOOL (WINAPI* FuncRedrawWindow)(HWND,CONST RECT*,HRGN,UINT) = NULL;
static void (WINAPI* FuncSHIdleTimerReset)() = NULL;
static BOOL (WINAPI* FuncReadMsgQueue)(HANDLE hMsgQ, LPVOID lpBuffer, DWORD cbBufferSize, LPDWORD lpNumberOfBytesRead, DWORD dwTimeout, DWORD *pdwFlags) = NULL;
static HANDLE (WINAPI* FuncCreateMsgQueue)(LPCWSTR lpName, MSGQUEUEOPTIONS2* lpOptions) = NULL;
static BOOL (WINAPI* FuncCloseMsgQueue)(HANDLE hMsgQ) = NULL;
static HANDLE (WINAPI* FuncRequestPowerNotifications)(HANDLE  hMsgQ,DWORD Flags) = NULL;
static DWORD (WINAPI* FuncStopPowerNotifications)(HANDLE h) = NULL;

static HMODULE CoreDLL = NULL;
static HMODULE CEShellDLL = NULL;
static HANDLE BacklightEvent = NULL;
static HANDLE PowerHandle = NULL;

#endif

static HMODULE AygShellDLL = NULL;

static const tchar_t RegBacklight[] = T("ControlPanel\\Backlight");
static const tchar_t RegBatteryTimeout[] = T("BatteryTimeout");
static const tchar_t RegACTimeout[] = T("ACTimeout");
static const tchar_t RegScreenSaver[] = T("ControlPanel\\ScreenSaver");
static const tchar_t RegMode[] = T("Mode");
static const tchar_t RegPower[] = T("ControlPanel\\Power");
static const tchar_t RegDisplay[] = T("Display");

static const tchar_t RegPowerTimouts[] = T("SYSTEM\\CurrentControlSet\\Control\\Power\\Timeouts");
static const tchar_t RegBattUserIdle[] = T("BattUserIdle");
static const tchar_t RegACUserIdle[] = T("ACUserIdle");
static const tchar_t RegBattSystemIdle[] = T("BattSystemIdle");
static const tchar_t RegACSystemIdle[] = T("ACSystemIdle");
static const tchar_t RegBattSuspend[] = T("BattSuspend");
static const tchar_t RegACSuspend[] = T("ACSuspend");

static const tchar_t RegCASIOBacklight[] = T("Drivers\\CASIO\\BackLight");
static const tchar_t RegTimeoutBattery[] = T("TimeoutBattery");
static const tchar_t RegTimeoutExPower[] = T("TimeoutExPower");

#define GETVFRAMEPHYSICAL			6144
#define GETVFRAMELEN				6145
#define DBGDRIVERSTAT				6146
#define SETPOWERMANAGEMENT			6147
#define GETPOWERMANAGEMENT			6148

typedef struct VIDEO_POWER_MANAGEMENT {
    ULONG Length;
    ULONG DPMSVersion;
    ULONG PowerState;
} VIDEO_POWER_MANAGEMENT;

static rgb Palette[256];
static tchar_t DocumentPath[MAX_PATH]; //important! has to be the MAX_PATH
static tchar_t SystemPath[MAX_PATH]; //important! has to be the MAX_PATH
static bool_t DisplayPower = 1;
static int Orientation = -1;
static bool_t WaitCursor = 0;
static bool_t WaitCursorDisable = 0;
static stream* Debug = NULL;

#if defined(MIPS)
int GetTimeFreq() 
{ 
	return 1000; 
}
int GetTimeTick()
{
	SYSTEMTIME t;
	GetSystemTime(&t);
	return (t.wDay*24+t.wHour)*60*60*1000+(t.wMinute*60+t.wSecond)*1000+t.wMilliseconds;
}
void GetTimeCycle(int* p)
{
	SYSTEMTIME i,j;
	int n=0;
	GetSystemTime(&i);
	do
	{
		++n;
		GetSystemTime(&j);
	}
	while (j.wMilliseconds==i.wMilliseconds);
	p[0] = (j.wDay*24+j.wHour)*60*60*1000+(j.wMinute*60+j.wSecond)*1000+j.wMilliseconds;
	p[1] = n;
}
#elif defined(TARGET_WINCE)
int GetTimeFreq() 
{ 
	return 1000; 
}
int GetTimeTick()
{
	return GetTickCount();
}
void GetTimeCycle(int* p)
{
	int n=1;
	int j;
	int i = GetTickCount();
	while ((j = GetTickCount())==i)
		++n;
	p[0] = j;
	p[1] = n;
}
#else
int GetTimeFreq() 
{ 
	return 1000; 
}
int GetTimeTick()
{
	return timeGetTime();
}
void GetTimeCycle(int* p)
{
	int n=1;
	int j;
	int i = timeGetTime();
	while ((j = timeGetTime())==i)
		++n;
	p[0] = j;
	p[1] = n;
}
#endif

void ReleaseModule(void** Module)
{
	if (*Module)
	{
		FreeLibrary(*Module);
		*Module = NULL;
	}
}

void GetProc(void** Module,void* Ptr,const tchar_t* ProcName,int Optional)
{
	FARPROC* Func = (FARPROC*)Ptr;
	if (*Module)
	{
#if !defined(TARGET_WINCE) && defined(UNICODE)
		char ProcName8[256];
		TcsToStr(ProcName8,sizeof(ProcName8),ProcName);
		*Func = GetProcAddress(*Module,ProcName8);
#else
		*Func = GetProcAddress(*Module,ProcName);
#endif
		if (!*Func && !Optional)
			ReleaseModule(Module);
	}
	else
		*Func = NULL;
}

int DefaultLang()
{
	tchar_t Lang[16];
	if (GetLocaleInfo(LOCALE_USER_DEFAULT,LOCALE_SABBREVLANGNAME,Lang,TSIZEOF(Lang)) >= 2)
	{
		tcsupr(Lang);
		if (tcscmp(Lang,T("JPN"))==0) tcscpy_s(Lang,TSIZEOF(Lang),T("JA"));
		if (tcscmp(Lang,T("GER"))==0) tcscpy_s(Lang,TSIZEOF(Lang),T("DE"));
		if (tcscmp(Lang,T("SPA"))==0) tcscpy_s(Lang,TSIZEOF(Lang),T("ES"));

		// special three letter cases: Chinese traditional (CHT) and Chinese simplified (CHS)
		if (tcscmp(Lang,T("CHS"))==0 || tcscmp(Lang,T("ZHI"))==0 || tcscmp(Lang,T("ZHM"))==0)
			return FOURCC('C','H','S','_');
		else
		if (tcscmp(Lang,T("CHT"))==0 || tcscmp(Lang,T("ZHH"))==0)
			return FOURCC('C','H','T','_');
		else
		if (tcscmp(Lang,T("PTB"))==0)
			return FOURCC('P','T','B','_');
		else
			return FOURCC(Lang[0],Lang[1],'_','_'); // two letter version
	}
	return LANG_DEFAULT;
}

void PlatformDetect(platform* p)
{
#if defined(TARGET_WINCE)
	tchar_t OemUpper[64];
	HKEY Key;
	RECT WorkArea;
#endif
	OSVERSIONINFO Ver;

	Ver.dwOSVersionInfoSize = sizeof(Ver);
	GetVersionEx(&Ver);
	p->Ver = Ver.dwMajorVersion*100 + Ver.dwMinorVersion;
	stprintf_s(p->Version,TSIZEOF(p->Version),T("%d.%02d"),p->Ver/100,p->Ver%100);

#if defined(TARGET_WINCE)

	p->WMPVersion = 9;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, T("SOFTWARE\\Microsoft\\MediaPlayer\\ASFCodecs"), 0, KEY_READ, &Key) == ERROR_SUCCESS)
	{
		p->WMPVersion = 10;
		RegCloseKey(Key);
	}

	p->Type = TYPE_WINCE;
	if (SystemParametersInfo(SPI_GETPLATFORMTYPE,sizeof(p->PlatformType),p->PlatformType,0)!=0)
	{
		if (tcsicmp(p->PlatformType,T("PocketPC"))==0)
			p->Type = TYPE_POCKETPC;
		else
		if (tcsicmp(p->PlatformType,T("Smartphone"))==0)
			p->Type = TYPE_SMARTPHONE;
	}
	else
	if (GetLastError()==ERROR_ACCESS_DENIED)
	{
		tcscpy_s(p->PlatformType,TSIZEOF(p->PlatformType),T("Smartphone"));
		p->Type = TYPE_SMARTPHONE;
	}
	else
		tcscpy_s(p->PlatformType,TSIZEOF(p->PlatformType),T("Windows CE"));

	SystemParametersInfo(SPI_GETOEMINFO,sizeof(p->OemInfo),p->OemInfo,0);
	tcscpy_s(OemUpper,TSIZEOF(OemUpper),p->OemInfo);
	tcsupr(OemUpper);

#if defined(ARM)
	if (tcsstr(OemUpper,T("NOVOGO")))
		p->Model = MODEL_NOVOGO; // more models...
	else
	if (tcsstr(OemUpper,T("TYPHOON")))
		p->Model = MODEL_SPV_C500_ORIGROM;
	else
	if (tcsstr(OemUpper,T("PW10B")) ||
		tcsstr(OemUpper,T("PW10A")))
	{
		p->Model = MODEL_O2_XDA;
		p->Caps |= CAPS_ONLY12BITRGB;
	}
	else
	if (tcsstr(OemUpper,T("Compal P6C")))
		p->Model = MODEL_AMIGO_600C;
	else
	if (tcsstr(OemUpper,T("Pocket PC J710")))
		p->Model = MODEL_CASIO_E200;
	else
	if (tcsstr(OemUpper,T("JORNADA")))
	{
		p->Caps |= CAPS_LANDSCAPE;
		if (tcsstr(OemUpper,T("680")))
			p->Model = MODEL_JORNADA_680;
		else
		if (tcsstr(OemUpper,T("690")))
			p->Model = MODEL_JORNADA_690;
		else
		if (tcsstr(OemUpper,T("710")))
			p->Model = MODEL_JORNADA_710;
		else
		if (tcsstr(OemUpper,T("720")))
			p->Model = MODEL_JORNADA_720;
	}
	else
	if (tcsstr(OemUpper,T("IPAQ")))
	{
		p->Caps |= CAPS_PORTRAIT;
		if (tcsstr(OemUpper,T("H3600")))
		{
			p->Model = MODEL_IPAQ_H3600;
			p->Caps |= CAPS_ONLY12BITRGB;
		}
		else
		if (tcsstr(OemUpper,T("H3700")))
		{
			p->Model = MODEL_IPAQ_H3700;
			p->Caps |= CAPS_ONLY12BITRGB;
		}
		else
		if (tcsstr(OemUpper,T("H3800")))
			p->Model = MODEL_IPAQ_H3800;
		else
		if (tcsstr(OemUpper,T("H3900")))
			p->Model = MODEL_IPAQ_H3900;
		else
		if (tcsstr(OemUpper,T("HX4700")))
			p->Model = MODEL_IPAQ_HX4700;
	}
	else
	if (tcsstr(OemUpper,T("TOSHIBA")))
	{
		p->Caps |= CAPS_PORTRAIT;
		if (tcsstr(OemUpper,T("E740")))
			p->Model = MODEL_TOSHIBA_E740;
		else
		if (tcsstr(OemUpper,T("E750")))
			p->Model = MODEL_TOSHIBA_E750;
		else
		if (tcsstr(OemUpper,T("E80")))
			p->Model = MODEL_TOSHIBA_E800;
	}
	else
	if (tcsstr(OemUpper,T("AXIM")))
	{
		if (tcscmp(OemUpper+tcslen(OemUpper)-2,T("X5"))==0)
			p->Model = MODEL_AXIM_X5;

		if (tcscmp(OemUpper+tcslen(OemUpper)-3,T("X50"))==0 ||
			tcscmp(OemUpper+tcslen(OemUpper)-4,T("X50V"))==0)
			p->Model = MODEL_AXIM_X50;
	}
	else
	if (tcsstr(OemUpper,T("LOOX")))
	{
		if (tcsstr(OemUpper,T("720")))
			p->Model = MODEL_LOOX_720;
	}
	else
	if (tcsstr(OemUpper,T("NEXIO")))
	{
		if (tcsstr(OemUpper,T("XP40")))
			p->Model = MODEL_NEXIO_XP40;
		else
		if (tcsstr(OemUpper,T("XP30")))
			p->Model = MODEL_NEXIO_XP30;
	}

#elif defined(MIPS)
	if (tcsstr(OemUpper,T("BSQUARE")))
	{
		tcscpy_s(OemUpper,TSIZEOF(OemUpper),p->PlatformType);
		tcsupr(OemUpper);

		if (tcsstr(OemUpper,T("ALCHEMY")))
			p->Model = MODEL_BSQUARE_ALCHEMY;

		tcscpy_s(OemUpper,TSIZEOF(OemUpper),p->OemInfo);
		tcsupr(OemUpper);
	}

	if (tcsstr(OemUpper,T("POCKET")))
	{
		if (tcsstr(OemUpper,T(" J74")))
		{
			p->Model = MODEL_CASIO_BE300;
			p->Caps |= CAPS_OLDSHELL;
		}
	}

	if (tcsstr(OemUpper,T("POCKET PC")))
	{
		p->Caps |= CAPS_PORTRAIT;
		if (tcsstr(OemUpper,T(" J67")))
			p->Model = MODEL_CASIO_E125;
		else
		if (tcsstr(OemUpper,T(" J76")))
			p->Model = MODEL_CASIO_EM500;
		else
		if (tcsstr(OemUpper,T(" J58")))
			p->Model = MODEL_CASIO_E115;
		else
		if (tcsstr(OemUpper,T(" J53")) || tcsstr(OemUpper,T(" JX53")))
			p->Model = MODEL_CASIO_E105;
	}

	if (tcsstr(OemUpper,T("HC-VJ")))
		p->Model = MODEL_INTERMEC_6651;
	
	if (tcsstr(OemUpper,T("JVC")))
	{
		if (tcsstr(OemUpper,T("C33")))
			p->Model = MODEL_JVC_C33;
	}

	if (tcsstr(OemUpper,T("COMPAQ AERO 15")))
		p->Model = MODEL_COMPAQ_AERO_1500;

#elif defined(SH3)
	if (tcsstr(OemUpper,T("JORNADA")))
	{
		if (tcsstr(OemUpper,T(" 43")))
		{
			p->Caps |= CAPS_ONLY12BITRGB;
		}
		if (tcsstr(OemUpper,T(" 54")))
		{
			p->Model = MODEL_JORNADA_540;
			p->Caps |= CAPS_ONLY12BITRGB | CAPS_PORTRAIT;
		}
	}
#endif

	if (!AygShellDLL && p->Ver < 300)
		p->Caps |= CAPS_OLDSHELL;

	// detect fake navigator bar and use old style menu
	if (p->Type != TYPE_SMARTPHONE && AygShellDLL && 
		SystemParametersInfo(SPI_GETWORKAREA,0,&WorkArea,0) && WorkArea.top==0)
		p->Caps |= CAPS_OLDSHELL;

#else
	tcscpy_s(p->PlatformType,TSIZEOF(p->PlatformType),T("Windows"));
	p->Type = TYPE_WIN32;
#endif
}

#if defined(TARGET_WINCE)

/*
#define PBT_TRANSITION      1

#define POWER_STATE(i)		((i)&0xFFFF0000)
#define POWER_STATE_ON		0x00010000
#define POWER_STATE_SUSPEND 0x00200000

typedef struct POWER_BROADCAST
{
	DWORD Message;
	DWORD Flags;
	DWORD Length;
	WCHAR SystemPowerState[64];

} POWER_BROADCAST;

static HANDLE Power = NULL;
static HANDLE PowerMsgQueue = NULL;
static HANDLE PowerExitEvent = NULL;
static HANDLE PowerThreadHandle = NULL;

static DWORD WINAPI PowerThread( LPVOID Parameter )
{
	DWORD PowerState = POWER_STATE_ON;
	POWER_BROADCAST Msg;
	DWORD Size;
	DWORD Flags;
	HANDLE Handles[2];
	
	Handles[0] = PowerExitEvent;
	Handles[1] = PowerMsgQueue;

	for (;;)
	{
		DWORD Result = WaitForMultipleObjects(2, Handles, FALSE, INFINITE);
		if (Result == WAIT_OBJECT_0 || Result == WAIT_ABANDONED_0)
			break;

		if (FuncReadMsgQueue(PowerMsgQueue,&Msg,sizeof(Msg),&Size,0,&Flags))
		{
			if (Size >= sizeof(DWORD)*2 && Msg.Message == PBT_TRANSITION && Msg.Flags &&
				PowerState != POWER_STATE(Msg.Flags))
			{
				PowerState = POWER_STATE(Msg.Flags);
				if (PowerState == POWER_STATE_ON || PowerState == POWER_STATE_SUSPEND)
				{
					bool_t b = PowerState == POWER_STATE_SUSPEND;
					node* Player = Context()->Player;
					if (Player)
						Player->Set(Player,PLAYER_POWEROFF,&b,sizeof(b));
				}
			}
		}
	}

	return 0;
}

static void Power_Init()
{
	DWORD Id;
	MSGQUEUEOPTIONS2 MsgOpt;

	if (FuncRequestPowerNotifications && FuncReadMsgQueue && FuncStopPowerNotifications &&
		FuncCreateMsgQueue && FuncCloseMsgQueue)
	{
		memset(&MsgOpt,0,sizeof(MsgOpt));
		MsgOpt.dwSize = sizeof(MsgOpt);
		MsgOpt.dwFlags = 0;
		MsgOpt.dwMaxMessages = 8;
		MsgOpt.cbMaxMessage = sizeof(POWER_BROADCAST);
		MsgOpt.bReadAccess = TRUE;

		PowerExitEvent = CreateEvent(NULL,TRUE,FALSE,NULL);
		if (PowerExitEvent)
		{
			PowerMsgQueue = FuncCreateMsgQueue(NULL,&MsgOpt);
			if (PowerMsgQueue)
			{
				Power = FuncRequestPowerNotifications(PowerMsgQueue,PBT_TRANSITION);
				if (Power)
					PowerThreadHandle = CreateThread(NULL,0,PowerThread,0,0,&Id);
			}
		}
	}
}

static void Power_Done()
{
	if (PowerThreadHandle)
	{
		SetEvent(PowerExitEvent);
		if (WaitForSingleObject(PowerThreadHandle,1000) == WAIT_TIMEOUT)
			TerminateThread(PowerThreadHandle,0);
		PowerThreadHandle = NULL;
	}
	if (Power)
	{
		FuncStopPowerNotifications(Power);
		Power = NULL;
	}
	if (PowerMsgQueue)
	{
		FuncCloseMsgQueue(PowerMsgQueue);
		PowerMsgQueue = NULL;
	}
	if (PowerExitEvent)
	{
		CloseHandle(PowerExitEvent);
		PowerExitEvent = NULL;
	}
}
*/

static void Power_Init() {}
static void Power_Done() {}

#endif

void Platform_Init()
{
#if defined(TARGET_WINCE)
	AygShellDLL = LoadLibrary(T("aygshell.dll"));
	if (AygShellDLL)
		*(FARPROC*)&FuncSHIdleTimerReset = GetProcAddress(AygShellDLL,MAKEINTRESOURCE(2006));

	CoreDLL = LoadLibrary(T("coredll.dll"));
	if (CoreDLL)
	{
		*(FARPROC*)&FuncChangeDisplaySettingsEx = GetProcAddress(CoreDLL,T("ChangeDisplaySettingsEx"));
		*(FARPROC*)&FuncCeSetThreadQuantum = GetProcAddress(CoreDLL,T("CeSetThreadQuantum"));
		*(FARPROC*)&FuncSetKMode = GetProcAddress(CoreDLL,T("SetKMode"));
		*(FARPROC*)&FuncRedrawWindow = GetProcAddress(CoreDLL,T("RedrawWindow"));
		*(FARPROC*)&FuncSHGetSpecialFolderPath = GetProcAddress(CoreDLL,T("SHGetSpecialFolderPath"));
		*(FARPROC*)&FuncSetPowerRequirement = GetProcAddress(CoreDLL,T("SetPowerRequirement"));
		*(FARPROC*)&FuncReleasePowerRequirement = GetProcAddress(CoreDLL,T("ReleasePowerRequirement"));
		*(FARPROC*)&FuncSetDevicePower = GetProcAddress(CoreDLL,T("SetDevicePower"));
		*(FARPROC*)&FuncGetDevicePower = GetProcAddress(CoreDLL,T("GetDevicePower"));
		*(FARPROC*)&FuncReadMsgQueue = GetProcAddress(CoreDLL,T("ReadMsgQueue"));
		*(FARPROC*)&FuncCreateMsgQueue = GetProcAddress(CoreDLL,T("CreateMsgQueue"));
		*(FARPROC*)&FuncCloseMsgQueue = GetProcAddress(CoreDLL,T("CloseMsgQueue"));
		*(FARPROC*)&FuncRequestPowerNotifications = GetProcAddress(CoreDLL,T("RequestPowerNotifications"));
		*(FARPROC*)&FuncStopPowerNotifications = GetProcAddress(CoreDLL,T("StopPowerNotifications"));
	}
	CEShellDLL = LoadLibrary(T("ceshell.dll"));
	if (CEShellDLL)
		*(FARPROC*)&FuncSHGetDocumentsFolder = GetProcAddress(CEShellDLL,MAKEINTRESOURCE(75));

	SystemPath[0] = 0;
	if (FuncSHGetSpecialFolderPath)
		FuncSHGetSpecialFolderPath(NULL,SystemPath,0x24/*CSIDL_WINDOWS*/,FALSE);

	DocumentPath[0] = 0;
	if (FuncSHGetDocumentsFolder)
		FuncSHGetDocumentsFolder(T("\\"),DocumentPath);

	BacklightEvent = CreateEvent(NULL, FALSE, FALSE, T("TIMEOUTDISPLAYOFF"));

	Power_Init();
#endif

	if (!Context()->CodePage)
		Context()->CodePage = CP_ACP; //CP_OEMCP;

	NodeRegisterClass(&Platform);
	Context()->Platform = NodeEnumObject(NULL,PLATFORM_ID);

	SleepTimeout(0,0); // restore backlight settings (if last time crashed)

	File_Init();
	DMO_Init();
}

void Platform_Done()
{
	File_Done();
	DMO_Done();
	NodeUnRegisterClass(PLATFORM_ID);
	SetDisplayPower(1,0);
	SetKeyboardBacklight(1);

#if defined(TARGET_WINCE)
	Power_Done();
	if (BacklightEvent) CloseHandle(BacklightEvent);
	if (AygShellDLL) FreeLibrary(AygShellDLL);
	if (CoreDLL) FreeLibrary(CoreDLL);
	if (CEShellDLL) FreeLibrary(CEShellDLL);
#endif
}

void Log_Done()
{
	if (Debug)
	{
		StreamClose(Debug);
		Debug = NULL;
	}
}

void WinUpdate()
{
	HWND Wnd = Context()->Wnd;
	if (Wnd)
		UpdateWindow(Wnd);
}

void WinInvalidate(const rect* Rect, bool_t Local)
{
	POINT o;
	RECT r;
	HWND Wnd = Context()->Wnd;

	if (Rect->Height>0 && Rect->Width>0)
	{
		DEBUG_MSG5(DEBUG_VIDEO,T("Invalidate %d,%d,%d,%d (%d)"),Rect->x,Rect->y,Rect->Width,Rect->Height,Local);

		if (Local && Wnd)
		{
			// only local invalidate
			o.x = 0;
			o.y = 0;
			ClientToScreen(Wnd,&o);

			DEBUG_MSG2(DEBUG_VIDEO,T("Invalidate local offset %d,%d"),o.x,o.y);

			r.left = Rect->x - o.x;
			r.top = Rect->y - o.y;
			r.right = r.left + Rect->Width;
			r.bottom = r.top + Rect->Height;
			InvalidateRect(Wnd,&r,0);
		}
		else
			GlobalInvalidate(Rect);
	}
}

void WinValidate(const rect* Rect)
{
	POINT o;
	RECT r;
	HWND Wnd = Context()->Wnd;

	if (Rect->Height>0 && Rect->Width>0 && Wnd)
	{
		DEBUG_MSG4(DEBUG_VIDEO,T("Validate %d,%d,%d,%d"),Rect->x,Rect->y,Rect->Width,Rect->Height);

		o.x = 0;
		o.y = 0;
		ClientToScreen(Wnd,&o);

		DEBUG_MSG2(DEBUG_VIDEO,T("Validate offset %d,%d"),o.x,o.y);

		r.left = Rect->x - o.x;
		r.top = Rect->y - o.y;
		r.right = r.left+Rect->Width;
		r.bottom = r.top+Rect->Height;

		ValidateRect(Wnd,&r);
	}
} 

void AdjustOrientation(video* p, bool_t Combine)
{
	int Width = GetSystemMetrics(SM_CXSCREEN);
	int Height = GetSystemMetrics(SM_CYSCREEN);
	int Orientation;
	rect Virt;

	if ((Width == p->Width*2 && Height == p->Height*2) || 
		(Width == p->Height*2 && Height == p->Width*2))
		p->Pixel.Flags |= PF_PIXELDOUBLE;

	Orientation = GetOrientation();

	if (Combine)
		p->Direction = CombineDir(0,p->Direction,Orientation);

	// still rotated? 
	PhyToVirt(NULL,&Virt,p);
	if (Width != Height && Virt.Width == Height && Virt.Height == Width)
	{
		if (Orientation & DIR_MIRRORLEFTRIGHT)
			p->Direction ^= DIR_SWAPXY | DIR_MIRRORLEFTRIGHT;
		else
			p->Direction ^= DIR_SWAPXY | DIR_MIRRORUPDOWN;
	}
}

bool_t IsOrientationChanged()
{
	int Old = Orientation;
	Orientation = -1;
	return GetOrientation() != Old;
}

int SetOrientation(int Orientation)
{
	return ERR_NOT_SUPPORTED;
}

bool_t GetHandedness()
{
	return 0;
}

int GetOrientation()
{
#if defined(TARGET_WINCE)
	if (Orientation < 0)
	{
		HKEY Key;
		context* p;
		char Buffer[256];
		DEVMODE* Mode = (DEVMODE*)Buffer;

		Mode->dmSize = 192;
		Mode->dmFields = DM_DISPLAYQUERYORIENTATION;

		if (QueryPlatform(PLATFORM_VER) >= 421 && // we don't trust this method on pre wm2003se systems
			FuncChangeDisplaySettingsEx &&
			FuncChangeDisplaySettingsEx(NULL, Mode, NULL, CDS_TEST, NULL) == DISP_CHANGE_SUCCESSFUL)
		{
			Mode->dmFields = DM_DISPLAYORIENTATION;
			FuncChangeDisplaySettingsEx(NULL, Mode, NULL, CDS_TEST, NULL);

			switch ((&Mode->dmDisplayFrequency)[1]) //(Mode->dmDisplayOrientation)
			{
			case DMDO_0: Orientation = 0; break;
			case DMDO_90: Orientation = DIR_SWAPXY | DIR_MIRRORUPDOWN; break;
			case DMDO_270: Orientation = DIR_SWAPXY | DIR_MIRRORLEFTRIGHT; break;
			case DMDO_180: Orientation = DIR_MIRRORUPDOWN | DIR_MIRRORLEFTRIGHT; break;
			}
		}

		p = Context();
		if (Orientation < 0 && p->HwOrientation)
			Orientation = p->HwOrientation(p->HwOrientationContext);

		if (Orientation < 0 && RegOpenKeyEx(HKEY_LOCAL_MACHINE, T("System\\GDI\\ROTATION"), 0, KEY_READ, &Key) == ERROR_SUCCESS)
		{
			DWORD Value;
			DWORD RegSize = sizeof(Value);
			DWORD RegType;

			if (RegQueryValueEx(Key, T("Angle"), 0, &RegType, (LPBYTE) &Value, &RegSize) == ERROR_SUCCESS)
				switch (Value)
				{
				case 0: Orientation = 0; break;
				case 90: Orientation = DIR_SWAPXY | DIR_MIRRORUPDOWN; break;
				case 270: Orientation = DIR_SWAPXY | DIR_MIRRORLEFTRIGHT; break;
				case 180: Orientation = DIR_MIRRORUPDOWN | DIR_MIRRORLEFTRIGHT; break;
				}

			RegCloseKey(Key);
		}

		if (Orientation < 0)
			Orientation = 0;
	}
#else
	Orientation = 0;
#endif
	return Orientation;
}

void QueryDesktop(video* p)
{
	HDC DC = GetDC(0);

	memset(p,0,sizeof(video));

	p->Width = GetDeviceCaps(DC,HORZRES);
	p->Height = GetDeviceCaps(DC,VERTRES);
	p->Aspect = ASPECT_ONE;
	p->Direction = 0;

	if (GetDeviceCaps(DC,BITSPIXEL)*GetDeviceCaps(DC,PLANES) <= 8)
	{
		p->Pixel.Flags = PF_PALETTE;
		p->Pixel.BitCount = GetDeviceCaps(DC,BITSPIXEL);

		if (p->Pixel.BitCount>4) //4,2,1 bits are grayscale (default palette required)
		{
			GetSystemPaletteEntries(DC,0,256,(PALETTEENTRY*)Palette);
			p->Pixel.Palette = Palette;
		}
	}
	else
	if (QueryPlatform(PLATFORM_CAPS) & CAPS_ONLY12BITRGB)
		DefaultRGB(&p->Pixel,GetDeviceCaps(DC,BITSPIXEL),4,4,4,1,2,1);
	else
	{
		int i;
		int RBits = 4;
		int GBits = 4;
		int BBits = 4;
		int BitCount = GetDeviceCaps(DC,BITSPIXEL);

		COLORREF Old = GetPixel(DC,0,0);
		for (i=3;i>=0;--i)
		{
			COLORREF c = SetPixel(DC,0,0,0x010101<<i);
			if (c == (COLORREF)-1)
			{
				if (BitCount > 16)
					RBits = GBits = BBits = 8;
				else
			{
					RBits = BBits = 5; GBits = 6;
				}
				break;
			}
			if (c & 0xFF)	  RBits++;
			if (c & 0xFF00)	  GBits++;
			if (c & 0xFF0000) BBits++;
		}
		SetPixel(DC,0,0,Old);

		DefaultRGB(&p->Pixel,BitCount,RBits,GBits,BBits,0,0,0);
	}

	ReleaseDC(0,DC);
}

#if defined(TARGET_WINCE)
bool_t KernelMode(bool_t v)
{
	if (FuncSetKMode)
		return FuncSetKMode(v);
	return 0;
}
#endif

bool_t GetKeyboardBacklight()
{
#if defined(TARGET_WINCE)
	int DeviceState;
	// Treo700w
	if (FuncGetDevicePower && FuncGetDevicePower(T("kyl0:"),POWER_NAME,&DeviceState)==ERROR_SUCCESS)
		return DeviceState == POWER_D4 ? 0:1;
#endif
	return 0;
}

int SetKeyboardBacklight(bool_t State)
{
#if defined(TARGET_WINCE)
	if (FuncSetDevicePower)
	{
		// Treo700w
		if (FuncSetDevicePower(T("kyl0:"),POWER_NAME,State?POWER_UNSPEC:POWER_D4)==ERROR_SUCCESS)
			return ERR_NONE;
	}
#endif
	return ERR_NOT_SUPPORTED;
}

bool_t GetDisplayPower()
{
	return DisplayPower;
}

int SetDisplayPower(bool_t State,bool_t Force)
{
	VIDEO_POWER_MANAGEMENT VPM;
	HDC DC;

	if (DisplayPower != State || Force)
	{
		DisplayPower = State;

		DC = GetDC(NULL);
		VPM.Length = sizeof(VPM);
		VPM.DPMSVersion = 1;
		VPM.PowerState = State ? 1:4;

		ExtEscape(DC, SETPOWERMANAGEMENT, sizeof(VPM), (LPCSTR) &VPM, 0, NULL);
		ReleaseDC(NULL, DC);
	}
	return ERR_NONE;
}

static bool_t ChangeRegEntry(bool_t CurrentUser, const tchar_t* Reg, const tchar_t* Name, bool_t State, int NewValue, int BackupId) 
{
	HKEY Key = 0;
	DWORD Size;
	DWORD Value;

	if (RegOpenKeyEx(CurrentUser ? HKEY_CURRENT_USER:HKEY_LOCAL_MACHINE,Reg, 0, KEY_READ|KEY_WRITE, &Key) == ERROR_SUCCESS) 
	{
		if (State)
		{
			// save old value 
			Size = sizeof(Value);
			if (RegQueryValueEx(Key, Name,NULL,NULL,(LPBYTE)&Value,&Size) == ERROR_SUCCESS)
				NodeRegSaveValue(0,BackupId,&Value,sizeof(Value),TYPE_INT);

			// set no screensaver
			Value = NewValue;
			Size = sizeof(Value);
			RegSetValueEx(Key, Name,0,REG_DWORD,(LPBYTE)&Value,Size);
		}
		else
		if (NodeRegLoadValue(0,BackupId,&Value,sizeof(Value),TYPE_INT))
		{
			// restore value (and delete old value)
			Size = sizeof(Value);
			RegSetValueEx(Key, Name,0,REG_DWORD,(LPBYTE)&Value,Size);

			NodeRegSaveValue(0,BackupId,NULL,0,0);
		}

		RegCloseKey(Key);
	}

	return Key != 0;
}

static BOOL ScreenSaveActive = -1;
static BOOL PowerOffActive = -1;
static BOOL LowPowerActive = -1;
static bool_t BacklightTimeoutDisabled = -1;

void SleepTimerReset()
{
#if defined(TARGET_WINCE)
	SystemIdleTimerReset();
	if (FuncSHIdleTimerReset)
		if (BacklightTimeoutDisabled || !QueryAdvanced(ADVANCED_HOMESCREEN))
			FuncSHIdleTimerReset();
	if (BacklightTimeoutDisabled)
		SetEvent(BacklightEvent);
#endif
}

void SleepTimeout(bool_t KeepProcess,bool_t KeepDisplay)
{
	if (!KeepProcess)
		KeepDisplay = 0;

	if (KeepDisplay && QueryAdvanced(ADVANCED_NOBACKLIGHT))
		KeepDisplay = 0;

#if defined(TARGET_WIN32)
	if (KeepDisplay)
	{
		if (ScreenSaveActive == -1)
		{
			SystemParametersInfo(SPI_GETSCREENSAVEACTIVE,0,&ScreenSaveActive,0);
			SystemParametersInfo(SPI_SETSCREENSAVEACTIVE,FALSE,NULL,0);
		}
		if (PowerOffActive == -1)
		{
			SystemParametersInfo(SPI_GETPOWEROFFACTIVE,0,&PowerOffActive,0);
			SystemParametersInfo(SPI_SETPOWEROFFACTIVE,FALSE,NULL,0);
		}
		if (LowPowerActive == -1)
		{
			SystemParametersInfo(SPI_GETLOWPOWERACTIVE,0,&LowPowerActive,0);
			SystemParametersInfo(SPI_SETLOWPOWERACTIVE,FALSE,NULL,0);
		}
	}
	else
	{
		if (ScreenSaveActive != -1)
		{
			SystemParametersInfo(SPI_SETSCREENSAVEACTIVE,ScreenSaveActive,NULL,0);
			ScreenSaveActive = -1;
		}
		if (PowerOffActive != -1)
		{
			SystemParametersInfo(SPI_SETPOWEROFFACTIVE,PowerOffActive,NULL,0);
			PowerOffActive = -1;
		}
		if (LowPowerActive != -1)
		{
			SystemParametersInfo(SPI_SETLOWPOWERACTIVE,LowPowerActive,NULL,0);
			LowPowerActive = -1;
		}
	}
#endif

	if (KeepDisplay != BacklightTimeoutDisabled)
	{
		HANDLE Handle;
		int Value;
		int Model = QueryPlatform(PLATFORM_MODEL);

		BacklightTimeoutDisabled = KeepDisplay;

#ifdef MIPS
		if (Model == MODEL_CASIO_E125 ||
			Model == MODEL_CASIO_EM500 ||
			Model == MODEL_CASIO_E115 ||
			Model == MODEL_CASIO_BE300 ||
			Model == MODEL_CASIO_E105)
		{
			Value = 0;
			ChangeRegEntry(0,RegCASIOBacklight,RegTimeoutBattery,KeepDisplay,Value,REG_BATTERYTIMEOUT);
			ChangeRegEntry(0,RegCASIOBacklight,RegTimeoutExPower,KeepDisplay,Value,REG_ACTIMEOUT);
		}
		else
#endif
		{
			Value = (QueryPlatform(PLATFORM_TYPENO) == TYPE_SMARTPHONE) ? 7199999 /*10*3600*1000*/ : 0x7FFFFFFF;
			if (Model == MODEL_AXIM_X5) 
				Value = 0;
			ChangeRegEntry(1,RegScreenSaver,RegMode,KeepDisplay,1,REG_SCREENSAVER);
			ChangeRegEntry(1,RegBacklight,RegBatteryTimeout,KeepDisplay,Value,REG_BATTERYTIMEOUT);
			ChangeRegEntry(1,RegBacklight,RegACTimeout,KeepDisplay,Value,REG_ACTIMEOUT);
			ChangeRegEntry(1,RegPower,RegDisplay,KeepDisplay,-1,REG_DISPPOWER);

			// just in case try HKEY_LOCAL_MACHINE as well 
			ChangeRegEntry(0,RegBacklight,RegBatteryTimeout,KeepDisplay,Value,REG_BATTERYTIMEOUT2);
			ChangeRegEntry(0,RegBacklight,RegACTimeout,KeepDisplay,Value,REG_ACTIMEOUT2);
		}
	
		Handle = CreateEvent(NULL, FALSE, FALSE, T("BackLightChangeEvent"));
		if (Handle) 
		{
			SetEvent(Handle);
			CloseHandle(Handle);
		}

		// support for sample Power Manager implementation 
		// maybe if they use the sample code, there might be bugs with SystemIdleTimerReset()...

		if (ChangeRegEntry(0,RegPowerTimouts,RegACUserIdle,KeepDisplay,0,REG_ACUSERIDLE))
		{
			ChangeRegEntry(0,RegPowerTimouts,RegBattUserIdle,KeepDisplay,0,REG_BATTUSERIDLE);
			ChangeRegEntry(0,RegPowerTimouts,RegACSystemIdle,KeepDisplay,0,REG_ACSYSTEMIDLE);
			ChangeRegEntry(0,RegPowerTimouts,RegBattSystemIdle,KeepDisplay,0,REG_BATTSYSTEMIDLE);
			ChangeRegEntry(0,RegPowerTimouts,RegACSuspend,KeepDisplay,0,REG_ACSUSPEND);
			ChangeRegEntry(0,RegPowerTimouts,RegBattSuspend,KeepDisplay,0,REG_BATTSUSPEND);

			Handle = CreateEvent(NULL, FALSE, FALSE, T("PowerManager/ReloadActivityTimeouts"));
			if (Handle) 
			{
				SetEvent(Handle);
				CloseHandle(Handle);
			}
		}

#if defined(TARGET_WINCE)

		if (FuncSetPowerRequirement && FuncReleasePowerRequirement)
		{
			if (KeepDisplay && !PowerHandle)
			{
				PowerHandle = FuncSetPowerRequirement(T("BKL1:"),POWER_D0,POWER_NAME,NULL,0);
			}
			else if (!KeepDisplay && PowerHandle)
			{
				FuncReleasePowerRequirement(PowerHandle);
				PowerHandle = NULL;
			}
		}
#endif
	}
}

void _Assert(const char* Exp,const char* File,int Line)
{
	TCHAR TExp[MAXPATH];
	TCHAR TFile[MAXPATH];
	AsciiToTcs(TExp,TSIZEOF(TExp),File);
	stprintf_s(TFile,TSIZEOF(TFile),T("%s:%d"),TExp,Line);
	AsciiToTcs(TExp,TSIZEOF(TExp),Exp);
#ifndef NDEBUG
	DebugBreak();
#endif
	MessageBox(NULL,TExp,TFile,MB_OK);
}

void* BrushCreate(rgbval_t Color) { return CreateSolidBrush(Color); }
void BrushDelete(void* Handle) { if (Handle) DeleteObject((HGDIOBJ)Handle); }
void WinFill(void* DC,rect* Rect,rect* Exclude,void* Brush)
{
	RECT r;
	if (Exclude)
		ExcludeClipRect(DC,Exclude->x,Exclude->y,Exclude->x + Exclude->Width,Exclude->y + Exclude->Height);

	r.left = Rect->x;
	r.top = Rect->y;
	r.right = Rect->x + Rect->Width;
	r.bottom = Rect->y + Rect->Height;

	FillRect(DC,&r,(HBRUSH)Brush);
}

void ShowMessage(const tchar_t* Title,const tchar_t* Msg,...)
{
	bool_t Wait = WaitCursor;
	tchar_t s[1024];
	va_list Args;
	va_start(Args,Msg);
	vstprintf_s(s,TSIZEOF(s),Msg,Args);
	va_end(Args);
	if (Wait) WaitEnd();
	MessageBox(NULL,s,Title,MB_OK|MB_SETFOREGROUND|MB_TOPMOST);
	if (Wait) WaitBegin();
}

void DebugMessage(const tchar_t* Msg,...)
{
	va_list Args;
	tchar_t s[1024];

#ifndef NDEBUG
	if (!Debug)	
		Debug = StreamOpen(T("\\debug.txt"),1);
#else
	if (!Debug)
	{
		GetDebugPath(s,TSIZEOF(s),T("log.txt"));
		Debug = StreamOpen(s,1);
	}
#endif

	s[0]=0;

#if 0
//#ifdef NDEBUG
	{
		//SYSTEMTIME t;
		//GetSystemTime(&t);
		//stprintf(s+tcslen(s), T("%02d:%02d:%02d "),t.wHour,t.wMinute,t.wSecond);
		int min,sec;
		int t = GetTickCount();
		min = t / 60000;
		t -= min*60000;
		sec = t / 1000;
		t -= sec*1000;
		stcatprintf_s(s,TSIZEOF(s),T("%02d:%02d.%03d "),min,sec,t);
	}
#endif

#ifndef NDEBUG
//	stprintf_s(s,TSIZEOF(s),T("%04x "),(int)GetCurrentThreadId());
#endif
	va_start(Args,Msg);
	vstprintf_s(s+tcslen(s),TSIZEOF(s)-tcslen(s), Msg, Args);
	va_end(Args);
	tcscat_s(s,TSIZEOF(s),T("\n"));

#ifndef NDEBUG
	OutputDebugString(s);
#endif
	StreamPrintf(Debug,T("%s"),s);
}

typedef struct contextreg
{
	int Ofs;
	const tchar_t* Name;
} contextreg;

#if defined(_M_IX86)
static contextreg Reg[] = 
{
	OFS(CONTEXT,Eax), T("eax"),
	OFS(CONTEXT,Ebx), T("ebx"),
	OFS(CONTEXT,Ecx), T("ecx"),
	OFS(CONTEXT,Edx), T("edx"),
	OFS(CONTEXT,Esi), T("esi"),
	OFS(CONTEXT,Edi), T("edi"),
	OFS(CONTEXT,Ebp), T("ebp"),
	OFS(CONTEXT,Esp), T("esp"),
	OFS(CONTEXT,Eip), T("eip"),
	OFS(CONTEXT,EFlags), T("flags"),

	OFS(CONTEXT,Esp), NULL, 
};
#elif defined(MIPS)
static contextreg Reg[] = 
{
	OFS(CONTEXT,IntZero), T("Zero"),
	OFS(CONTEXT,IntAt), T("At"),
	OFS(CONTEXT,IntV0), T("V0"),
	OFS(CONTEXT,IntV1), T("V1"),
	OFS(CONTEXT,IntA0), T("A0"),
	OFS(CONTEXT,IntA1), T("A1"),
	OFS(CONTEXT,IntA2), T("A2"),
	OFS(CONTEXT,IntA3), T("A3"),
	OFS(CONTEXT,IntT0), T("T0"),
	OFS(CONTEXT,IntT1), T("T1"),
	OFS(CONTEXT,IntT2), T("T2"),
	OFS(CONTEXT,IntT3), T("T3"),
	OFS(CONTEXT,IntT4), T("T4"),
	OFS(CONTEXT,IntT5), T("T5"),
	OFS(CONTEXT,IntT6), T("T6"),
	OFS(CONTEXT,IntT7), T("T7"),
	OFS(CONTEXT,IntS0), T("S0"),
	OFS(CONTEXT,IntS1), T("S1"),
	OFS(CONTEXT,IntS2), T("S2"),
	OFS(CONTEXT,IntS3), T("S3"),
	OFS(CONTEXT,IntS4), T("S4"),
	OFS(CONTEXT,IntS5), T("S5"),
	OFS(CONTEXT,IntS6), T("S6"),
	OFS(CONTEXT,IntS7), T("S7"),
	OFS(CONTEXT,IntT8), T("T8"),
	OFS(CONTEXT,IntT9), T("T9"),
	OFS(CONTEXT,IntK0), T("K0"),
	OFS(CONTEXT,IntK1), T("K1"),
	OFS(CONTEXT,IntGp), T("Gp"),
	OFS(CONTEXT,IntSp), T("Sp"),
	OFS(CONTEXT,IntS8), T("S8"),
	OFS(CONTEXT,IntRa), T("Ra"),
	OFS(CONTEXT,IntLo), T("Lo"),
	OFS(CONTEXT,IntHi), T("Hi"),
	OFS(CONTEXT,IntSp), NULL,
};
#elif defined(ARM)
static contextreg Reg[] = 
{
	OFS(CONTEXT,R0), T("R0"),
	OFS(CONTEXT,R1), T("R1"),
	OFS(CONTEXT,R2), T("R2"),
	OFS(CONTEXT,R3), T("R3"),
	OFS(CONTEXT,R4), T("R4"),
	OFS(CONTEXT,R5), T("R5"),
	OFS(CONTEXT,R6), T("R6"),
	OFS(CONTEXT,R7), T("R7"),
	OFS(CONTEXT,R8), T("R8"),
	OFS(CONTEXT,R9), T("R9"),
	OFS(CONTEXT,R10),T("R10"),
	OFS(CONTEXT,R11),T("R11"),
	OFS(CONTEXT,R12),T("R12"),
	OFS(CONTEXT,Sp), T("Sp"),
	OFS(CONTEXT,Lr), T("Lr"),
	OFS(CONTEXT,Pc), T("Pc"),
	OFS(CONTEXT,Psr),T("Psr"),
	OFS(CONTEXT,Sp), NULL,
};
#elif defined(SH3)
static contextreg Reg[] = 
{
	OFS(CONTEXT,PR), T("PR"),
	OFS(CONTEXT,MACH), T("MACH"),
	OFS(CONTEXT,MACL), T("MACL"),
	OFS(CONTEXT,GBR), T("GBR"),
	OFS(CONTEXT,R0), T("R0"),
	OFS(CONTEXT,R1), T("R1"),
	OFS(CONTEXT,R2), T("R2"),
	OFS(CONTEXT,R3), T("R3"),
	OFS(CONTEXT,R4), T("R4"),
	OFS(CONTEXT,R5), T("R5"),
	OFS(CONTEXT,R6), T("R6"),
	OFS(CONTEXT,R7), T("R7"),
	OFS(CONTEXT,R8), T("R8"),
	OFS(CONTEXT,R9), T("R9"),
	OFS(CONTEXT,R10), T("R10"),
	OFS(CONTEXT,R11), T("R11"),
	OFS(CONTEXT,R12), T("R12"),
	OFS(CONTEXT,R13), T("R13"),
	OFS(CONTEXT,R14), T("R14"),
	OFS(CONTEXT,R15), T("R15"),
	OFS(CONTEXT,R15), NULL,
};
#else
static contextreg Reg[] = { -1, NULL };
#endif

void FindFiles(const tchar_t* Path, const tchar_t* Mask,void(*Process)(const tchar_t*,void*),void* Param)
{
	WIN32_FIND_DATA FindData;
	tchar_t FindPath[MAXPATH];
	HANDLE Find;

	tcscpy_s(FindPath,TSIZEOF(FindPath),Path);
	tcscat_s(FindPath,TSIZEOF(FindPath),Mask);
	Find = FindFirstFile(FindPath,&FindData);

	if (Find != INVALID_HANDLE_VALUE)
	{
		do
		{
			tcscpy_s(FindPath,TSIZEOF(FindPath),Path);
			tcscat_s(FindPath,TSIZEOF(FindPath),FindData.cFileName);
			Process(FindPath,Param);
		}
		while (FindNextFile(Find,&FindData));

		FindClose(Find);
	}
}

void GetModulePath(tchar_t* Path,const tchar_t* Module)
{
	tchar_t* s;
	HANDLE Handle = NULL;
	if (Module)
		Handle = GetModuleHandle(Module);
	GetModuleFileName(Handle,Path,MAXPATH);
	s = tcsrchr(Path,'\\');
	if (s) s[1]=0;
}

void GetDebugPath(tchar_t* Path, int PathLen, const tchar_t* FileName)
{
	stprintf_s(Path,PathLen,T("%s\\%s"),DocumentPath,FileName);
}

void GetSystemPath(tchar_t* Path, int PathLen, const tchar_t* FileName)
{
	stprintf_s(Path,PathLen,T("%s\\%s"),SystemPath,FileName);
}

int64_t GetTimeDate()
{
	SYSTEMTIME t;
	GetSystemTime(&t);
	return (((t.wHour*100)+t.wMinute)*100+t.wSecond)*1000 +
		(int64_t)(((t.wYear*100)+t.wMonth)*100+t.wDay) * 1000000000;
}

bool_t SaveDocument(const tchar_t* Name, const tchar_t* Text,tchar_t* URL,int URLLen)
{
	stream* f;
	stprintf_s(URL,URLLen,T("%s\\%s.txt"),DocumentPath,Name);

	f = StreamOpen(URL,1);
	if (!f)
		return 0;

	StreamText(f,Text,0);
	StreamClose(f);
	return 1;
}

int SafeException(void* p)
{
	EXCEPTION_POINTERS* Data = (EXCEPTION_POINTERS*)p;
	stream* File;
	tchar_t Path[MAXPATH];

	if (Context())
	{
		GetDebugPath(Path,TSIZEOF(Path),T("crash.txt"));
		File = StreamOpen(Path,1);
		if (File)
		{
			void** Stack;
			contextreg* r;
			int No;
			const uint8_t* ContextRecord = (const uint8_t*) Data->ContextRecord;
			EXCEPTION_RECORD* Record = Data->ExceptionRecord;
			tchar_t* Name;
			int DllBase;
			tchar_t DllName[MAXPATH];

			StreamPrintf(File,T("%s %s crash report\n----------------------------\n"),
					Context()->ProgramName,Context()->ProgramVersion);

			switch (Record->ExceptionCode)
			{
			case STATUS_ACCESS_VIOLATION:		Name = T("Access violation"); break;
			case STATUS_BREAKPOINT:				Name = T("Breakpoint"); break;
			case STATUS_DATATYPE_MISALIGNMENT:	Name = T("Datatype misalignment"); break;
			case STATUS_ILLEGAL_INSTRUCTION:	Name = T("Illegal instruction"); break;
			case STATUS_INTEGER_DIVIDE_BY_ZERO: Name = T("Int divide by zero"); break;
			case STATUS_INTEGER_OVERFLOW:		Name = T("Int overflow"); break;
			case STATUS_PRIVILEGED_INSTRUCTION: Name = T("Priv instruction"); break;
			case STATUS_STACK_OVERFLOW:			Name = T("Stack overflow"); break;
			default:							Name = T("Unknown"); break;
			}

			if (!NodeLocatePtr(Record->ExceptionAddress,DllName,TSIZEOF(DllName),&DllBase))
			{
				DllName[0] = 0;
				DllBase = (int)Record->ExceptionAddress;
			}
			StreamPrintf(File,T("%s(%08x) at %08x (%s:%08x)"),Name,Record->ExceptionCode,Record->ExceptionAddress,DllName,DllBase);

			if (Record->ExceptionCode == STATUS_ACCESS_VIOLATION)
			{
				if (Record->ExceptionInformation[0])
					Name = T("Write to");
				else
					Name = T("Read from");
				StreamPrintf(File,T("\n%s %08x"),Name,Record->ExceptionInformation[1]);
				if (NodeLocatePtr((void*)Record->ExceptionInformation[1],DllName,TSIZEOF(DllName),&DllBase))
					StreamPrintf(File,T(" (%s:%08x)"),DllName,DllBase);
			}
			
			// context

			StreamPrintf(File,T("\n\ncpu dump:"));

			for (r=Reg;r->Name;++r)
			{
				void* Ptr = *(void**)(ContextRecord+r->Ofs);

				StreamPrintf(File,T("\n%-5s = %08x"),r->Name,Ptr);

				if (NodeLocatePtr(Ptr,DllName,TSIZEOF(DllName),&DllBase))
					StreamPrintf(File,T(" (%s:%08x)"),DllName,DllBase);
			}

			if (r->Ofs >= 0)
			{
				StreamPrintf(File,T("\n\nstack dump:"));

				Stack = *(void***)(ContextRecord+r->Ofs);
				for (No=0;No<256;++No,++Stack)
				{
					if (!IsBadReadPtr(Stack,sizeof(void*)))
					{
						void* Ptr = *Stack;

						StreamPrintf(File,T("\n%08x %08x"),Stack,Ptr);

						if (NodeLocatePtr(Ptr,DllName,TSIZEOF(DllName),&DllBase))
							StreamPrintf(File,T(" (%s:%08x)"),DllName,DllBase);
					}
					else
						StreamPrintf(File,T("\n%08x ????????"),Stack);
				}
			}

			StreamPrintf(File,T("\n\n"));
			TRY_BEGIN 
			{
				NodeBroadcast(NODE_CRASH,NULL,0);
			}
			TRY_END
			NodeDump(File);
			StreamClose(File);
		}

		MessageBox(NULL,LangStr(PLATFORM_ID,PLATFORM_CRASH_MESSAGE),
			LangStr(PLATFORM_ID,PLATFORM_CRASH_TITLE),MB_OK|MB_SETFOREGROUND|MB_TOPMOST|MB_ICONSTOP);
	}

	Mem_Done(); // use safevirtualfree (if done by OS, it could be buggy on O2 Atom)
	TerminateProcess(GetCurrentProcess(),1);
	return 1;
}

void TryInvalidate(HWND Wnd, RECT* r0)
{
	RECT r;

	if (IsWindowVisible(Wnd))
	{
		GetWindowRect(Wnd,&r);
		if (r0->left < r.right && r0->right > r.left &&
			r0->top < r.bottom && r0->bottom > r.top)
		{
			// window is visible and intersects with r0

			HWND Child;
			POINT o;
			o.x = o.y = 0;
			ClientToScreen(Wnd,&o);

			r.left = r0->left - o.x;
			r.right = r0->right - o.x;
			r.top = r0->top - o.y;
			r.bottom = r0->bottom - o.y;
			InvalidateRect(Wnd,&r,0);

			Child = GetWindow(Wnd,GW_CHILD);
			while (Child && IsWindow(Child))
			{
				TryInvalidate(Child,r0);
				Child = GetWindow(Child,GW_HWNDNEXT);
			}
		}
	}
}

BOOL CALLBACK EnumInvalidate(HWND Wnd, LPARAM lParam)
{
	TryInvalidate(Wnd,(RECT*)lParam);
	return 1;
}

void GlobalInvalidate(const rect* Rect)
{
	RECT r;
	r.left = Rect->x;
	r.top = Rect->y;
	r.right = r.left + Rect->Width;
	r.bottom = r.top + Rect->Height;

#if defined(TARGET_WINCE)
	if (FuncRedrawWindow) // doesn't work for mio558???
		FuncRedrawWindow(NULL,&r,NULL,0x0001|0x0080); //RDW_INVALIDATE|RDW_ALLCHILDREN
	else
		EnumWindows(EnumInvalidate,(LPARAM)&r);
#else
	RedrawWindow(NULL,&r,NULL,RDW_INVALIDATE|RDW_ALLCHILDREN);
#endif
}

void HotKeyToString(tchar_t* Out, size_t OutLen, int HotKey)
{
	int Id = 0;
	
	Out[0] = 0;
	if (HotKey)
	{
		bool_t Keep = (HotKey & HOTKEY_KEEP) != 0;
		if ((HotKey & HOTKEY_MASK) < 0xC1 || (HotKey & HOTKEY_MASK) > 0xCF)
		{
			if (HotKey & HOTKEY_WIN) { tcscat_s(Out,OutLen,LangStr(PLATFORM_ID,PLATFORM_KEY_WIN)); tcscat_s(Out,OutLen,T("+")); }
			if (HotKey & HOTKEY_SHIFT) { tcscat_s(Out,OutLen,LangStr(PLATFORM_ID,PLATFORM_KEY_SHIFT)); tcscat_s(Out,OutLen,T("+")); }
			if (HotKey & HOTKEY_CTRL) { tcscat_s(Out,OutLen,LangStr(PLATFORM_ID,PLATFORM_KEY_CTRL)); tcscat_s(Out,OutLen,T("+")); }
			if (HotKey & HOTKEY_ALT) { tcscat_s(Out,OutLen,LangStr(PLATFORM_ID,PLATFORM_KEY_ALT)); tcscat_s(Out,OutLen,T("+")); }
		}
		HotKey &= HOTKEY_MASK;

		if ((HotKey >= '0' && HotKey <= '9') ||
			(HotKey >= 'A' && HotKey <= 'Z'))
			stcatprintf_s(Out,OutLen,T("%c"),HotKey);
		else
		if (HotKey >= 0x70 && HotKey < 0x80)
			stcatprintf_s(Out,OutLen,T("F%d"),HotKey-0x70+1);
		else
		switch (HotKey)
		{
		case 0x86:
			Id = PLATFORM_KEY_ACTION;
			break;
		case VK_RETURN:
			Id = PLATFORM_KEY_ENTER;
			break;
		case VK_ESCAPE:
			Id = PLATFORM_KEY_ESCAPE;
			break;
		case VK_LEFT:
			Id = PLATFORM_KEY_LEFT;
			break;
		case VK_RIGHT:
			Id = PLATFORM_KEY_RIGHT;
			break;
		case VK_UP:
			Id = PLATFORM_KEY_UP;
			break;
		case VK_DOWN:
			Id = PLATFORM_KEY_DOWN;
			break;
		case VK_SPACE:
			Id = PLATFORM_KEY_SPACE;
			break;
		case 0xC1:
		case 0xC2:
		case 0xC3:
		case 0xC4:
		case 0xC5:
		case 0xC6:
		case 0xC7:
		case 0xC8:
		case 0xC9:
		case 0xCA:
		case 0xCB:
		case 0xCC:
		case 0xCD:
		case 0xCE:
		case 0xCF:
			stcatprintf_s(Out,OutLen,LangStr(PLATFORM_ID,PLATFORM_KEY_APP),HotKey-0xC0);
			break;
		case 0xB0:
			Id = PLATFORM_KEY_NEXT;
			break;
		case 0xB1:
			Id = PLATFORM_KEY_PREV;
			break;
		case 0xB2:
			Id = PLATFORM_KEY_STOP;
			break;
		case 0xB3:
			Id = PLATFORM_KEY_PLAY;
			break;
		default:
			stcatprintf_s(Out,OutLen,T("#%02X"),HotKey);
			break;
		}
		if (Id)
			tcscat_s(Out,OutLen,LangStr(PLATFORM_ID,Id));
		if (Keep) { tcscat_s(Out,OutLen,T("*")); }
	}
}

void WaitDisable(bool_t v)
{
	WaitCursorDisable = v;	
	if (v && WaitCursor)
		WaitEnd();
}

bool_t WaitBegin()
{
	//DEBUG_MSG(DEBUG_SYS,T("WaitBegin"));
	if (WaitCursorDisable)
		return 0;
	SetCursor(LoadCursor (NULL, IDC_WAIT));
	WaitCursor = 1;
	return 1;
}

void WaitEnd()
{
	//DEBUG_MSG(DEBUG_SYS,T("WaitEnd"));
	SetCursor(LoadCursor (NULL, IDC_ARROW));
	WaitCursor = 0;
}

bool_t CheckModule(const tchar_t* Name)
{
	DWORD RegType;
	DWORD RegSize;
	DWORD Disp;
	void* Module;
	tchar_t Path[MAXPATH];
	HKEY Key = NULL;

	if (!Name[0])
		return 0;

	RegSize = sizeof(Path);
	stprintf_s(Path,TSIZEOF(Path),T("SOFTWARE\\%s\\DLLPath"),Context()->ProgramName);
	if (RegCreateKeyEx(HKEY_ROOT, Path, 0, NULL, 0, KEY_READ|KEY_WRITE, NULL, &Key, &Disp) == ERROR_SUCCESS)
	{
		if (RegQueryValueEx(Key, Name, 0, &RegType, (PBYTE)Path, &RegSize) == ERROR_SUCCESS)
		{
			if (RegType==REG_SZ && FileExits(Path))
			{
				RegCloseKey(Key);
				return 1;
			}
			RegDeleteValue(Key,Name);
		}
	}

	Module = LoadLibrary(Name);
	if (Module)
	{
		if (Key)
		{
			GetModuleFileName(Module,Path,MAXPATH);
			RegSetValueEx(Key, Name, 0, REG_SZ, (PBYTE)Path, sizeof(tchar_t)*(tcslen(Path)+1));
			RegCloseKey(Key);
		}
		FreeLibrary(Module);
		return 1;
	}

	if (Key)	
		RegCloseKey(Key);
	return 0;
}

bool_t HaveDPad()
{
	return 1;
}

#endif
