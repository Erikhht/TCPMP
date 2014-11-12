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
 * $Id: platform.h 593 2006-01-17 22:25:08Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __PLATFORM_H
#define __PLATFORM_H

#define DEBUG_TEST			0x01
#define DEBUG_FORMAT		0x02
#define DEBUG_PLAYER		0x04
#define DEBUG_WIN			0x08
#define DEBUG_SYS			0x10
#define DEBUG_VIDEO			0x20
#define DEBUG_AUDIO			0x40
#define DEBUG_VCODEC		0x80
#define DEBUG_ACODEC		0x100
#define DEBUG_VCODEC2		0x200

#define PLATFORM_ID			FOURCC('P','L','A','T')

#define PLATFORM_LANG			0x10
#define PLATFORM_MODEL			0x12
#define PLATFORM_CAPS			0x13
#define PLATFORM_ICACHE			0x14
#define PLATFORM_DCACHE			0x15
#define PLATFORM_TYPE			0x17
#define PLATFORM_VER			0x19
#define PLATFORM_TYPENO			0x1A
#define PLATFORM_OEMINFO		0x18
#define PLATFORM_VERSION		0x1B
#define PLATFORM_CPU			0x11
#define PLATFORM_CPUMHZ			0x22
#define PLATFORM_WMPVERSION		0x25
#define PLATFORM_ORIENTATION	0x26

// models
#define	MODEL_UNKNOWN			0
#define	MODEL_IPAQ_H3600		1
#define	MODEL_IPAQ_H3700		2
#define	MODEL_IPAQ_H3800		3
#define	MODEL_IPAQ_H3900		4
#define	MODEL_O2_XDA			5
#define	MODEL_TOSHIBA_E740		6
#define	MODEL_TOSHIBA_E750		7
#define	MODEL_CASIO_E105		8
#define	MODEL_CASIO_E115		9
#define	MODEL_CASIO_E125		10
#define	MODEL_CASIO_EM500		11
#define	MODEL_CASIO_E200		12
#define MODEL_AMIGO_600C		13
#define MODEL_JVC_C33			14
#define MODEL_JORNADA_680		15
#define MODEL_JORNADA_690		16
#define MODEL_JORNADA_710		17
#define MODEL_JORNADA_720		18
#define MODEL_SIGMARION			19
#define MODEL_SIGMARION2		20
#define MODEL_SIGMARION3		21
#define MODEL_MOBILEGEAR2_450	22
#define MODEL_MOBILEGEAR2_430	23
#define MODEL_MOBILEGEAR2_520	24
#define MODEL_MOBILEGEAR2_530	25
#define MODEL_MOBILEGEAR2_550	26
#define MODEL_MOBILEGEAR2_700	27
#define MODEL_MOBILEGEAR2_730	28
#define MODEL_MOBILEPRO_770		29
#define MODEL_MOBILEPRO_780		30
#define MODEL_INTERMEC_6651		31
#define MODEL_JORNADA_540		32
#define	MODEL_TOSHIBA_E800		33
#define	MODEL_CASIO_BE300		34
#define MODEL_SPV_C500_ORIGROM	35
#define MODEL_BSQUARE_ALCHEMY	36
#define MODEL_IPAQ_HX4700		37
#define MODEL_AXIM_X5			38
#define MODEL_AXIM_X50			39
#define MODEL_LOOX_720			40
#define MODEL_COMPAQ_AERO_1500	41
#define MODEL_ZODIAC			42
#define MODEL_TUNGSTEN_T		43
#define MODEL_TUNGSTEN_T2		44
#define MODEL_TUNGSTEN_T3		45
#define MODEL_TUNGSTEN_T5		46
#define MODEL_TREO_600			47
#define MODEL_TREO_650			48
#define MODEL_IQUE_3600			49
#define MODEL_PALM_SIMULATOR	50
#define MODEL_QDA700			51
#define MODEL_TUNGSTEN_E		52
#define MODEL_TUNGSTEN_E2		53
#define MODEL_ZIRE_72			54
#define MODEL_LIFEDRIVE			55
#define MODEL_SONY_TH55			56
#define MODEL_SONY_UX40			57
#define MODEL_SONY_UX50			58
#define MODEL_SONY_VZ90			59
#define MODEL_TUNGSTEN_C		60
#define MODEL_ZIRE_71			61
#define MODEL_NEXIO_XP30		62
#define MODEL_NEXIO_XP40		63
#define MODEL_ZIRE_31			64
#define MODEL_PALM_TX			65
#define MODEL_PALM_Z22			66
#define MODEL_MOBILEPRO_790		67
#define MODEL_NOVOGO			68

// special cpu and framebuffer features
#define CAPS_ARM_5E				0x0001
#define CAPS_ARM_XSCALE			0x1000
#define CAPS_ARM_WMMX			0x0002
#define CAPS_MIPS_VR4110		0x0004
#define CAPS_MIPS_VR4120		0x2000
#define CAPS_MIPS_VR41XX		0x2004
#define CAPS_X86_MMX			0x0008
#define CAPS_X86_MMX2			0x0100
#define CAPS_X86_SSE			0x0010
#define CAPS_X86_SSE2			0x0200
#define CAPS_X86_3DNOW			0x0400
#define CAPS_ONLY12BITRGB		0x0020
#define CAPS_LANDSCAPE			0x0040
#define CAPS_PORTRAIT			0x0080
#define CAPS_OLDSHELL			0x0800
#define CAPS_SONY				0x4000
#define CAPS_ARM_GENERAL		0x8000
#define CAPS_SCREEN_SHIFT		0x10000

// platform types
#define TYPE_WINCE				0
#define TYPE_POCKETPC			1
#define TYPE_SMARTPHONE			2
#define TYPE_WIN32				3
#define TYPE_PALMOS				4
#define TYPE_SYMBIAN			5

void Platform_Init();
void Platform_Done();
void Log_Done();
DLL int QueryPlatform(int Param);

DLL void QueryDesktop(video*);
DLL bool_t IsOrientationChanged();
DLL bool_t GetHandedness();
DLL int GetOrientation();
DLL int SetOrientation(int);
DLL void GlobalInvalidate(const rect*);
DLL void AdjustOrientation(video* p, bool_t Combine);

DLL bool_t HaveDPad();

#ifdef TARGET_WINCE
DLL bool_t KernelMode(bool_t v);
#else
static INLINE bool_t KernelMode(bool_t v) { return 0; }
#endif

DLL bool_t GetDisplayPower();
DLL int SetDisplayPower(bool_t State,bool_t Force);

DLL bool_t GetKeyboardBacklight();
DLL int SetKeyboardBacklight(bool_t State);

DLL void SleepTimeout(bool_t KeepProcess,bool_t KeepDisplay);
DLL void SleepTimerReset();

DLL void GetDebugPath(tchar_t* Path, int PathLen, const tchar_t* FileName);
DLL void GetSystemPath(tchar_t* Path, int PathLen, const tchar_t* FileName);
DLL bool_t SaveDocument(const tchar_t* Name, const tchar_t* Text,tchar_t* URL,int URLLen);

DLL void WaitDisable(bool_t);
DLL bool_t WaitBegin();
DLL void WaitEnd();

DLL void* BrushCreate(rgbval_t);
DLL void BrushDelete(void*);
DLL void WinFill(void* DC,rect* Rect,rect* Exclude,void* Brush);
DLL void WinValidate(const rect*);
DLL void WinInvalidate(const rect*, bool_t Local);
DLL void WinUpdate();

DLL int GetTimeTick();
DLL int GetTimeFreq();
DLL void GetTimeCycle(int*);
DLL int64_t GetTimeDate(); // yyyyhhddhhmmssmmm

DLL bool_t CheckModule(const tchar_t* Name);
DLL void ReleaseModule(void** Module);
DLL void GetProc(void** Module,void* Ptr,const tchar_t* ProcName,int Optional);

DLL void ShowError(int Class, int MsgClass, int MsgNo, ...);
DLL void ShowMessage(const tchar_t* Title,const tchar_t* Msg,...);
DLL void DebugMessage(const tchar_t*,...);
DLL void DebugBinary(const tchar_t*,const void* Data,int Length);
DLL int DebugMask();

DLL int CPUSpeed();

#define DIA_TASKBAR	1
#define DIA_SIP		2
#define DIA_ALL		(DIA_SIP|DIA_TASKBAR)

void DIASet(int State,int Mask);
int DIAGet(int Mask);
bool_t DIASupported();

#ifndef NDEBUG
  #define DEBUG_BIN(m,x,p,n) if (DebugMask() & (m)) DebugBinary(x,p,n)
  #define DEBUG_MSG(m,x) if (DebugMask() & (m)) DebugMessage(x)
  #define DEBUG_MSG1(m,x,a) if (DebugMask() & (m)) DebugMessage(x,a)
  #define DEBUG_MSG2(m,x,a,b) if (DebugMask() & (m)) DebugMessage(x,a,b)
  #define DEBUG_MSG3(m,x,a,b,c) if (DebugMask() & (m)) DebugMessage(x,a,b,c)
  #define DEBUG_MSG4(m,x,a,b,c,d) if (DebugMask() & (m)) DebugMessage(x,a,b,c,d)
  #define DEBUG_MSG5(m,x,a,b,c,d,e) if (DebugMask() & (m)) DebugMessage(x,a,b,c,d,e)
  #define DEBUG_MSG6(m,x,a,b,c,d,e,f) if (DebugMask() & (m)) DebugMessage(x,a,b,c,d,e,f)
  #define DEBUG_MSG7(m,x,a,b,c,d,e,f,g) if (DebugMask() & (m)) DebugMessage(x,a,b,c,d,e,f,g)
#else
  #define DEBUG_BIN(m,x,p,n)
  #define DEBUG_MSG(m,x)
  #define DEBUG_MSG1(m,x,a)
  #define DEBUG_MSG2(m,x,a,b)
  #define DEBUG_MSG3(m,x,a,b,c)
  #define DEBUG_MSG4(m,x,a,b,c,d)
  #define DEBUG_MSG5(m,x,a,b,c,d,e)
  #define DEBUG_MSG6(m,x,a,b,c,d,e,f)
  #define DEBUG_MSG7(m,x,a,b,c,d,e,f,g)
#endif

DLL int SafeException(void*);

#if defined(_MSC_VER) && (defined(TARGET_WIN32) || defined(TARGET_WINCE))
void* __cdecl _exception_info();
#define SAFE_BEGIN __try {
#define SAFE_END ;} __except (SafeException(_exception_info())) {}
#define TRY_BEGIN __try {
#define TRY_END ;} __except (1) {}
#else
#define SAFE_BEGIN 
#define SAFE_END
#define TRY_BEGIN 
#define TRY_END 
#endif

#define PLATFORM_CRASH_MESSAGE		100
#define PLATFORM_CRASH_TITLE		101
#define PLATFORM_YES				102
#define PLATFORM_NO					103
#define PLATFORM_DUMP_MESSAGE		104
#define PLATFORM_DUMP_TITLE			105
#define PLATFORM_OK					106
#define PLATFORM_CANCEL				107
#define PLATFORM_ERROR				108
#define PLATFORM_MEMORY_LOW			109
#define PLATFORM_KEY_ENTER			110
#define PLATFORM_KEY_UP				111
#define PLATFORM_KEY_DOWN			112
#define PLATFORM_KEY_LEFT			113
#define PLATFORM_KEY_RIGHT			114
#define PLATFORM_KEY_SPACE			115
#define PLATFORM_KEY_APP			116
#define PLATFORM_KEY_CTRL			117
#define PLATFORM_KEY_SHIFT			118
#define PLATFORM_KEY_ALT			119
#define PLATFORM_KEY_WIN			120
#define PLATFORM_KEY_ACTION			123
#define PLATFORM_ASSIGN				121
#define PLATFORM_CLEAR				122
#define PLATFORM_RESET				130
#define PLATFORM_KEY_ESCAPE			131
#define PLATFORM_KEY_PREV			132
#define PLATFORM_KEY_NEXT			133
#define PLATFORM_KEY_PLAY			134
#define PLATFORM_KEY_STOP			135
#define PLATFORM_DONE				136

#define HOTKEY_MASK					0x0FFFF
#define HOTKEY_SHIFT				0x10000
#define HOTKEY_CTRL					0x20000
#define	HOTKEY_ALT					0x40000
#define	HOTKEY_WIN					0x80000
#define	HOTKEY_KEEP					0x100000

//---------
// private

typedef struct platform
{
	node Node;
	int Model;
	int Caps;
	int ICache;
	int DCache;
	int Type;
	int Ver;
	int Orientation;
	int WMPVersion;
	int64_t Tick0;
	bool_t DisplayPower;
	tchar_t OemInfo[64];
	tchar_t PlatformType[20];
	tchar_t CPU[32];
	tchar_t Version[16];

} platform;

extern const nodedef Platform;

#endif
