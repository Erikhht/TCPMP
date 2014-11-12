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
 * $Id: platform_palmos.c 615 2006-01-26 16:57:51Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"

#if defined(TARGET_PALMOS)

#include "pace.h"
#include "dia.h"

#ifdef HAVE_PALMONE_SDK
#define NO_HSEXT_TRAPS
#include <68K/System/HardwareUtils68K.h>
#include <Common/System/HsNavCommon.h>
#include <68K/System/HsExt.h>
#include <68K/System/PmPalmOSNVFS.h>
static UInt16 HWU = sysInvalidRefNum;
static bool_t HSExt54 = 0;
static bool_t HWUDisplayPower = 1;
static bool_t HALTreo650 = 0;
#endif

static int KeyboardBacklight = 0;
static bool_t HighDens = 0;
static bool_t SleepDisable = 0;
static bool_t SleepCatch = 0;
static UInt16 SleepSave = 0;
static bool_t DisplayPowerSupport = 0;
static bool_t OEMSleep = 0;
//static uint16_t KeyInitDelay, KeyPeriod, KeyDoubleTapDelay;  
//static Boolean KeyQueueAhead; 

void File_Init();
void File_Done();
void FileDb_Init();
void FileDb_Done();
void VFS_Init();
void VFS_Done();

int DefaultLang()
{
	return LANG_DEFAULT;
}

rgb* Palette = NULL;

void QueryDesktop(video* p)
{
	UInt32 v;
    Coord Width;
    Coord Height;
	UInt16 OldCoord = 0;
	memset(p,0,sizeof(video));

	// we need the display window extent and not the physical screen size 
	// (WinScreenGetAttribute(winScreenWidth|winScreenHeight) would return always 320x320 on Sony devices)

	if (HighDens)
		OldCoord = WinSetCoordinateSystem(kCoordinatesNative);
	WinGetDisplayExtent(&Width, &Height);
	if (HighDens)
		WinSetCoordinateSystem(OldCoord);

	p->Width = Width;
	p->Height = Height;

	if (!WinScreenGetAttribute(winScreenDepth,&v)) p->Pixel.BitCount = (UInt16)v;
	if (!WinScreenGetAttribute(winScreenRowBytes,&v)) p->Pitch = (UInt16)v;

	if (WinScreenGetAttribute(winScreenPixelFormat,&v))
	{
		if (p->Pixel.BitCount == 16)
			v = pixelFormat565LE;
		if (p->Pixel.BitCount <= 8)
			v = pixelFormatIndexedLE;
	}

	if (v==pixelFormat565LE || v==pixelFormat565) 
		DefaultRGB(&p->Pixel,16,5,6,5,0,0,0);

	free(Palette);
	Palette = NULL;

	if (v==pixelFormatIndexed || v==pixelFormatIndexedLE)
	{
		int i,n = 1 << p->Pixel.BitCount;
		p->Pixel.Flags = PF_PALETTE;

		Palette = malloc(sizeof(rgb)*n);
		WinPalette(winPaletteGet,0,(UInt16)n,(RGBColorType*)Palette);

		for (i=0;i<n;++i)
		{
			RGBColorType c = *(RGBColorType*)(Palette+i);
			Palette[i].c.r = c.r;
			Palette[i].c.g = c.g;
			Palette[i].c.b = c.b;
		}

		p->Pixel.Palette = Palette;
	}

	p->Aspect = ASPECT_ONE;
	p->Direction = 0;
}

bool_t HaveDPad()
{
	UInt32 CompanyID;
	FtrGet(sysFtrCreator, sysFtrNumOEMCompanyID, &CompanyID);
	return CompanyID != 'sony';
}

typedef struct companyname
{
	uint32_t CompanyID;
	const char* Name;

} companyname;

typedef struct devicename
{
	uint32_t CompanyID;
	uint32_t DeviceID;
	const char* Name;
	int Model;

} devicename;

static const companyname CompanyName[] = {
	{'PITC',T("PiTech")},
	{'SONY',T("Sony")},
	{'TPWV',T("Tapwave")},
	{'PALM',T("palmOne")},
	{'HSPR',T("Handspring")},
	{'GRMN',T("Gramin")},
	{0},
};

static const devicename DeviceName[] = {
	{'PITC','W300',T(" Qool Labs QDA700"),MODEL_QDA700},
	{'TPWV','Rdog',T(" Zodiac 1/2"),MODEL_ZODIAC},
	{'PSYS',0,T("PalmOS 5 Simulator"),MODEL_PALM_SIMULATOR},
	{'PALM','TunX',T(" LifeDrive"),MODEL_LIFEDRIVE},
	{'PALM','Zi22',T(" Zire 31"),MODEL_ZIRE_31},
	{'PALM','Zpth',T(" Zire 71"),MODEL_ZIRE_71},
	{'PALM','Zi72',T(" Zire 72"),MODEL_ZIRE_72},
	{'PALM','Cct1',T(" Tungsten E"),MODEL_TUNGSTEN_E},
	{'PALM','Zir4',T(" Tungsten E2"),MODEL_TUNGSTEN_E2},
	{'PALM','Frg1',T(" Tungsten T"),MODEL_TUNGSTEN_T},
	{'PALM','Frg2',T(" Tungsten T2"),MODEL_TUNGSTEN_T2},
	{'PALM','Arz1',T(" Tungsten T3"),MODEL_TUNGSTEN_T3},
	{'PALM','TnT5',T(" Tungsten T5"),MODEL_TUNGSTEN_T5},
	{'PALM','MT64',T(" Tungsten C"),MODEL_TUNGSTEN_C},
	{'PALM','D050',T(" TX"),MODEL_PALM_TX},
	{'PALM','D051',T(" Z22"),MODEL_PALM_Z22},
	{'HSPR','H101',T(" Treo 600"),MODEL_TREO_600},
	{'HSPR','H102',T(" Treo 650"),MODEL_TREO_650},
	{'HSPR','H202',T(" Treo 650 Simulator"),MODEL_TREO_650},
	{'GRMN','3600',T(" iQue 3600"),MODEL_IQUE_3600},
	{'SONY','atom',T(" TH55"),MODEL_SONY_TH55},
	{'SONY','amno',T(" UX40"),MODEL_SONY_UX40},
	{'SONY','prmr',T(" UX50"),MODEL_SONY_UX50},
	{'SONY','ancy',T(" VZ90"),MODEL_SONY_VZ90},
	{0},
};

void PlatformDetect(platform* p)
{
	UInt32 DeviceID;
	UInt32 CompanyID;
	UInt32 Version;
	const companyname* CName;
	const devicename* DName;

	FtrGet(sysFtrCreator, sysFtrNumROMVersion, &Version);

	p->Ver = Version;
	stprintf_s(p->Version,TSIZEOF(p->Version),T("%d.%d.%d"),
		sysGetROMVerMajor(Version),
		sysGetROMVerMinor(Version),
		sysGetROMVerFix(Version));

	p->Type = TYPE_PALMOS;
	p->Model = MODEL_UNKNOWN;
	p->OemInfo[0] = 0;

	FtrGet(sysFtrCreator, sysFtrNumOEMCompanyID, &CompanyID);
	FtrGet(sysFtrCreator, sysFtrNumOEMDeviceID, &DeviceID);

	CompanyID = UpperFourCC(CompanyID);
	for (CName=CompanyName;CName->CompanyID;++CName)
		if (CName->CompanyID == CompanyID)
		{
			tcscpy_s(p->OemInfo,TSIZEOF(p->OemInfo),CName->Name);
			break;
		}

	if (CompanyID == 'SONY')
		p->Caps |= CAPS_SONY;

	for (DName=DeviceName;DName->CompanyID;++DName)
		if (DName->CompanyID == CompanyID && DName->DeviceID == DeviceID)
		{
			tcscat_s(p->OemInfo,TSIZEOF(p->OemInfo),DName->Name);
			p->Model = DName->Model;
			break;
		}

	if (p->Model == MODEL_ZIRE_31 || p->Model == MODEL_PALM_Z22)
		p->Caps |= CAPS_ONLY12BITRGB;
}

void Platform_Init()
{
	UInt32 Version;
	UInt32 Depth;
	UInt16 card;
	LocalID db;
	int Model;
//	UInt16 InitDelay,Period,DoubleTapDelay;
//	Boolean QueueAhead;

	NodeRegisterClass(&Platform);
	Context()->Platform = NodeEnumObject(NULL,PLATFORM_ID);

	Model = QueryPlatform(PLATFORM_MODEL);

	if (SysCurAppDatabase(&card, &db)==errNone)
	{
		SysNotifyRegister(card, db, sysNotifyVolumeMountedEvent, NULL, sysNotifyNormalPriority, 0);
		SysNotifyRegister(card, db, sysNotifySleepRequestEvent, NULL, sysNotifyNormalPriority, 0);
		SysNotifyRegister(card, db, sysNotifySleepNotifyEvent, NULL, sysNotifyNormalPriority, 0);
		SysNotifyRegister(card, db, sysNotifyLateWakeupEvent, NULL, sysNotifyNormalPriority, 0);
	}

#ifdef HAVE_PALMONE_SDK
	if (SysLibFind(kHWUtilsName, &HWU) == sysErrLibNotFound)
		SysLibLoad(kHWUtilsType, kHWUtilsCreator, &HWU);
	if (HWU != sysInvalidRefNum && SysLibOpen(HWU)!=errNone)
		HWU = sysInvalidRefNum;

	if (HWU != sysInvalidRefNum)
		DisplayPowerSupport = 1;
	HSExt54 = FtrGet( hsFtrCreator, hsFtrIDVersion, &Version)==errNone && Version>=0x05400000;
#if 0
	// for some reason the player crashes during program exit on Treo650
	if (HSExt54 && Model==MODEL_TREO_650 && !DisplayPowerSupport)
	{
		HALTreo650 = 1;
		DisplayPowerSupport = 1;
	}
#endif
#endif

	if (Model==MODEL_ZODIAC)
	{
		OEMSleep = 1;
		DisplayPowerSupport = 1;
	}

	DIA_Init();

	Depth = 16; // set screen mode to 16bit if possible
	WinScreenMode(winScreenModeSet, NULL, NULL, &Depth, NULL);

	HighDens = FtrGet(sysFtrCreator, sysFtrNumWinVersion, &Version)==errNone && Version>=4;

//	KeyRates(0,&KeyInitDelay,&KeyPeriod,&KeyDoubleTapDelay,&KeyQueueAhead);
//	InitDelay = 100; 
//	Period = 50;
//	DoubleTapDelay = 100;
//	QueueAhead = 0;
//	KeyRates(1,&InitDelay,&Period,&DoubleTapDelay,&QueueAhead);

	File_Init();
	FileDb_Init();
	VFS_Init();
}

void Log_Done()
{
}

void Platform_Done()
{
	UInt16 card;
	LocalID db;

//	KeyRates(1,&KeyInitDelay,&KeyPeriod,&KeyDoubleTapDelay,&KeyQueueAhead);

	File_Done();
	FileDb_Done();
	VFS_Done();
	NodeUnRegisterClass(PLATFORM_ID);

	SetDisplayPower(1,0);
	SetKeyboardBacklight(1);
	SleepTimeout(0,0);

   	WinScreenMode(winScreenModeSetToDefaults, NULL, NULL, NULL, NULL);

	DIA_Done();

#ifdef HAVE_PALMONE_SDK
	if (HWU != sysInvalidRefNum)
	{
		SysLibClose(HWU);
		HWU = sysInvalidRefNum;
	}
#endif

	if (SysCurAppDatabase(&card, &db)==errNone)
	{
		SysNotifyUnregister(card, db, sysNotifyVolumeMountedEvent, sysNotifyNormalPriority);
		SysNotifyUnregister(card, db, sysNotifySleepRequestEvent, sysNotifyNormalPriority);
		SysNotifyUnregister(card, db, sysNotifySleepNotifyEvent, sysNotifyNormalPriority);
		SysNotifyUnregister(card, db, sysNotifyLateWakeupEvent, sysNotifyNormalPriority);
	}

	free(Palette);
	Palette = NULL;
}

int ThreadPriority(void* Thread,int Priority) { return 0; }
void* ThreadCreate(int(*Start)(void*),void* Parameter,int Quantum) { return NULL; }

void WaitDisable(bool_t v) {}
bool_t WaitBegin() { return 0; }
void WaitEnd() {}

bool_t GetKeyboardBacklight()
{
#ifdef HAVE_PALMONE_SDK
	if (HSExt54)
		return (HsCurrentLightCircumstance() & hsLightCircumstanceKeylightOffFlagBit)==0;
#endif
	return 0;
}

static int _SetKeyboardBacklight(bool_t State)
{
#ifdef HAVE_PALMONE_SDK
	if (HSExt54 && HsLightCircumstance((Boolean)(State==0),hsLightCircumstanceKeylightOff)==errNone)
		return ERR_NONE;
#endif
	return ERR_NOT_SUPPORTED;
}

int SetKeyboardBacklight(bool_t State)
{
	int Result = ERR_NONE;
	if (!State)
	{
		if (GetKeyboardBacklight())
		{
			Result = _SetKeyboardBacklight(0);
			++KeyboardBacklight;
		}
	}
	else
	{
		for (;KeyboardBacklight>0;--KeyboardBacklight)
			Result = _SetKeyboardBacklight(1);
	}
	return Result;
}

bool_t GetDisplayPower()
{
#ifdef HAVE_PALMONE_SDK
	if (HWU != sysInvalidRefNum)
	{
		static UInt16 HWU2 = sysInvalidRefNum;
		Boolean b = HWUGetDisplayState(HWU);

		// some graffiti utlitize can unload HWUtils 
		// (example Graffiti Anywhere on T|T: power off, action,action to turn on)
		if (b!=1 && SysLibFind(kHWUtilsName, &HWU2) == sysErrLibNotFound)
		{
			SysLibLoad(kHWUtilsType, kHWUtilsCreator, &HWU2);
			b = HWUGetDisplayState(HWU);
		}

		if (b && !HWUDisplayPower)
		{
			EvtEnableGraffiti(true);
			HWUBlinkLED(HWU,0);
		}

		HWUDisplayPower = b!=0;
		return HWUDisplayPower;
	}
	if (HALTreo650)
	{
		UInt32 n = 1;
		HsAttrGet(hsAttrDisplayOn,0,&n);
		return n != 0;
	}
#endif

	if (OEMSleep)
		return (GetOEMSleepMode() & 2) == 0;
	return 1;
}

int SetDisplayPower(bool_t State,bool_t Force)
{
	if (OEMSleep)
	{
		int OldMode = GetOEMSleepMode();
		int NewMode = State?(SleepCatch?1:0):2;
		if ((OldMode & 2) != (NewMode & 2))
		{
			if ((OldMode & 3) && !NewMode)
				EvtResetAutoOffTimer();
			SetOEMSleepMode(NewMode);
		}
		return ERR_NONE;
	}

	if (!Force && State == GetDisplayPower())
		return ERR_NONE;

#ifdef HAVE_PALMONE_SDK
	if (HWU != sysInvalidRefNum && HWUEnableDisplay(HWU,(Boolean)State)==errNone)
	{
		if (!State)
			HWUSetBlinkRate(HWU,1000);
		else
			EvtResetAutoOffTimer();
		EvtEnableGraffiti((Boolean)State);
		if (State || QueryAdvanced(ADVANCED_BLINKLED))
			HWUBlinkLED(HWU,(Boolean)(!State));
		HWUDisplayPower = State;
		return ERR_NONE;
	}
	if (HALTreo650)
	{
		if (!State)
			HALDisplayOff_TREO650();
		else
			HALDisplayWake();
		return ERR_NONE;
	}
#endif

	return ERR_NOT_SUPPORTED;
}

void LaunchNotify(SysNotifyParamType* Params,bool_t HasFocus)
{
	DIAResizedNotify(Params);

	if (Params->notifyType == sysNotifyLateWakeupEvent)
	{
		bool_t b = 0;
		node* Player = Context()->Player;
		if (Player)
			Player->Set(Player,PLAYER_POWEROFF,&b,sizeof(b));
	}

	if (Params->notifyType == sysNotifySleepNotifyEvent && !(SleepCatch && OEMSleep))
	{
		bool_t b = 1;
		node* Player = Context()->Player;
		if (Player)
			Player->Set(Player,PLAYER_POWEROFF,&b,sizeof(b));
	}

	/*
	if (Params->notifyType == sysNotifySleepRequestEvent && !OEMSleep && HasFocus &&
		((SleepEventParamType*)Params->notifyDetailsP)->reason == sysSleepPowerButton && !GetDisplayPower())
	{
		((SleepEventParamType*)Params->notifyDetailsP)->deferSleep++;
		Params->handled = 1;
		SetDisplayPower(1,0);
		SetKeyboardBacklight(1);
	}
	*/

	if (Params->notifyType == sysNotifySleepRequestEvent && SleepCatch && !OEMSleep && HasFocus &&
		((SleepEventParamType*)Params->notifyDetailsP)->reason == sysSleepAutoOff)
	{
		((SleepEventParamType*)Params->notifyDetailsP)->deferSleep++;
		Params->handled = 1;
		SetDisplayPower(0,0);
		SetKeyboardBacklight(0);
	}

	if (Params->notifyType == sysNotifyVolumeMountedEvent)
	{
		Params->handled |= vfsHandledStartPrc|vfsHandledUIAppSwitch;
		//todo: switch to this volume in open dialog...
	}
}

void SleepTimerReset()
{
}

void SleepTimeout(bool_t KeepProcess,bool_t KeepDisplay)
{
	bool_t Disable;
	
	if (!DisplayPowerSupport || !KeepProcess)
		KeepDisplay = KeepProcess;

	Disable = KeepProcess && KeepDisplay;
	SleepCatch = KeepProcess && !KeepDisplay;

	if (SleepDisable != Disable)
	{
		SleepDisable = Disable;
		if (Disable)
		{
			SetDisplayPower(1,0);
			SleepSave = SysSetAutoOffTime(0);
		}
		else
		{
			if (!SleepCatch) 
			{
				EvtResetAutoOffTimer();
				GetDisplayPower(); // update
			}
			SysSetAutoOffTime(SleepSave);
		}
	}

	if (OEMSleep)
	{
		int OldMode = GetOEMSleepMode();
		if (((OldMode & 3)!=0) != SleepCatch)
		{
			if ((OldMode & 3) && !SleepCatch)
				EvtResetAutoOffTimer();
			SetOEMSleepMode(SleepCatch?1:0);
		}
	}
}

void _Assert(const char* Exp,const char* File,int Line)
{
	tchar_t TExp[MAXPATH];
	tchar_t TFile[MAXPATH];
	AsciiToTcs(TExp,TSIZEOF(TExp),File);
	stprintf_s(TFile,TSIZEOF(TFile),T("\n%s:%d"),TExp,Line);
	AsciiToTcs(TExp,TSIZEOF(TExp),Exp);
	FrmCustomAlert(WarningOKAlert,TExp,TFile," ");
}

void ShowMessage(const tchar_t* Title,const tchar_t* Msg,...)
{
	tchar_t s[512];
	va_list Args;
	va_start(Args,Msg);
	if (Title)
	{
		tcscpy_s(s,TSIZEOF(s),Title);
		tcscat_s(s,TSIZEOF(s),T(":\n"));
	}
	else
		s[0] = 0;
	vstprintf_s(s+tcslen(s),TSIZEOF(s)-tcslen(s), Msg, Args);
	va_end(Args);

	DIASet(DIA_TASKBAR,DIA_TASKBAR);
	FrmCustomAlert(WarningOKAlert, s, " ", " ");
}

void HotKeyToString(tchar_t* Out, size_t OutLen, int HotKey)
{
	Out[0] = 0;
	if (HotKey)
	{
		bool_t Keep = (HotKey & HOTKEY_KEEP) != 0;
		HotKey &= HOTKEY_MASK;
		stprintf_s(Out,OutLen,T("#%02X"),HotKey);
		if (Keep) tcscat_s(Out,OutLen,T("*")); 
	}
}

void ThreadSleep(int Time) 
{ 
	SysTaskDelay(Time);
}

int GetTimeFreq() 
{ 
	return SysTicksPerSecond();
}

int GetTimeTick()
{
	return TimGetTicks();
}

void GetTimeCycle(int* p)
{
	int n=1;
	int j;
	int i = TimGetTicks();
	while ((j = TimGetTicks())==i)
		++n;
	p[0] = j;
	p[1] = n;
}

#ifdef _WIN32
#include <windows.h>
static FILE* Debug = NULL;
#endif

void DebugMessage(const tchar_t* Msg, ...)
{
	va_list Args;
    RectangleType r;
	tchar_t s[512];

	va_start(Args,Msg);
	vstprintf_s(s,TSIZEOF(s), Msg, Args);
	va_end(Args);

    r.topLeft.x = 0;
    r.topLeft.y = 0;
    r.extent.x = 160;
    r.extent.y = 15; 

    WinEraseRectangle(&r, 0);
    WinDrawChars(s, (Int16)strlen(s), 1, 1);

#ifdef _WIN32
	if (!Debug) Debug = fopen("\\debug.txt","w+");
	tcscat_s(s,TSIZEOF(s),"\n");
	OutputDebugString(s);
	fprintf(Debug,"%s",s);
	fflush(Debug);
#endif
}

void* BrushCreate(rgbval_t Color) { return (void*)Color; }
void BrushDelete(void* Handle) {}

void WinFill(void* DC,rect* Rect,rect* Exclude,void* Brush)
{
	uint32_t Color = (uint32_t)Brush;
	RGBColorType c,c0;
	RectangleType r;
	UInt16 OldCoord = 0;
	WinHandle Old = NULL;
	if (DC)
		Old = WinSetDrawWindow(FrmGetWindowHandle((FormType*)DC));
	if (HighDens)
		OldCoord = WinSetCoordinateSystem(kCoordinatesNative);

	c.index = 0;
	c.r = ((rgb*)&Color)->c.r;
	c.g = ((rgb*)&Color)->c.g;
	c.b = ((rgb*)&Color)->c.b;
	WinSetBackColorRGB(&c,&c0);

	r.topLeft.x = (Coord)Rect->x;
	r.topLeft.y = (Coord)Rect->y;
	r.extent.x = (Coord)Rect->Width;
	r.extent.y = (Coord)Rect->Height;

	if (Exclude)
	{
		r.extent.y = (Coord)(Exclude->y - Rect->y);
		if (r.extent.y > 0 && r.extent.x > 0)
			WinEraseRectangle(&r,0);

		r.extent.y = (Coord)(Rect->y + Rect->Height - Exclude->y - Exclude->Height);
		r.topLeft.y = (Coord)(Exclude->y + Exclude->Height);
		if (r.extent.y > 0 && r.extent.x > 0)
			WinEraseRectangle(&r,0);

		r.extent.y = (Coord)Rect->Height;
		r.topLeft.y = (Coord)Rect->y; 

		r.extent.x = (Coord)(Exclude->x - Rect->x);
		if (r.extent.x > 0 && r.extent.y > 0)
			WinEraseRectangle(&r,0);

		r.extent.x = (Coord)(Rect->x + Rect->Width - Exclude->x - Exclude->Width);
		r.topLeft.x = (Coord)(Exclude->x + Exclude->Width);
		if (r.extent.x > 0 && r.extent.y > 0)
			WinEraseRectangle(&r,0);
	}
	else
	if (r.extent.x > 0 && r.extent.y > 0)
		WinEraseRectangle(&r,0);

	WinSetBackColorRGB(&c0,&c);

	if (HighDens)
		WinSetCoordinateSystem(OldCoord);

	if (Old)
		WinSetDrawWindow(Old);
}

void WinUpdate()
{
}

void WinValidate(const rect* Rect)
{
}

void WinInvalidate(const rect* Rect, bool_t Local)
{
}

void ReleaseModule(void** Module)
{
	if (*Module)
		*Module = NULL;
}

int64_t GetTimeDate()
{
	DateTimeType Date;
	TimSecondsToDateTime(TimGetSeconds(),&Date);

	return (((Date.hour*100)+Date.minute)*100+Date.second)*1000 +
		(int64_t)(((Date.year*100)+Date.month)*100+Date.day) * 1000000000;
}

bool_t SaveDocument(const tchar_t* Name, const tchar_t* Text,tchar_t* URL,int URLLen)
{
	//todo...
	return 0;
}

void GetDebugPath(tchar_t* Path, int PathLen, const tchar_t* FileName)
{
	stprintf_s(Path,PathLen,T("slot1:\\%s"),FileName);
}

void GetSystemPath(tchar_t* Path, int PathLen, const tchar_t* FileName)
{
	if (PathLen>0)
		*Path = 0;
}

double compability_with_old_aac(int i) { return i; } // needed for defining __floatsidf()

#endif
