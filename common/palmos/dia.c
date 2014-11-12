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
 * $Id: dia.c 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../../common/common.h"

#if defined(TARGET_PALMOS)

#include "pace.h"
#include "dia.h"

#ifdef HAVE_SONY_SDK
#undef CPU_TYPE
#define CPU_TYPE CPU_68K
#include <Libraries/SonySilkLib.h>
#include <SonySystemResources.h>
#include <SonySystemFtr.h>
static UInt16 Sony = sysInvalidRefNum;
#endif

bool_t SwapLandscape = 0;
bool_t SwapPortrait = 0;
static bool_t NativeLandscape = 0;
static UInt32 HasPIN = 0;
static UInt32 Event = 0;

void DIA_Init()
{
	UInt16 card;
	LocalID db;

	FtrGet(pinCreator, pinFtrAPIVersion, &HasPIN);
	if (HasPIN >= pinAPIVersion1_0)
		Event = sysNotifyDisplayResizedEvent;
#ifdef HAVE_SONY_SDK
	else
	{
		UInt32 Ver;

		if (SysLibFind(sonySysLibNameSilk, &Sony) == sysErrLibNotFound)
			SysLibLoad(sysFileTLibrary, sonySysFileCSilkLib, &Sony);

		if (Sony != sysInvalidRefNum && (FtrGet(sonySysFtrCreator,sonySysFtrNumVskVersion,&Ver)!=errNone ||	
			SysLibOpen(Sony)!=errNone))
			Sony = sysInvalidRefNum;
		if (Sony != sysInvalidRefNum)
			Event = sysNotifyDisplayChangeEvent;
	}
#endif

	DIASet(0,DIA_SIP); // before notify, so no event is generated (no form exits yet)

	if (HasPIN < pinAPIVersion1_1)
	{
		video Desktop;
		QueryDesktop(&Desktop);
		NativeLandscape = Desktop.Width > Desktop.Height;
	}

	if (QueryPlatform(PLATFORM_MODEL)==MODEL_ZODIAC)
		SwapPortrait = 1;

	if (Event && SysCurAppDatabase(&card, &db)==errNone)
		SysNotifyRegister(card, db, Event, NULL, sysNotifyNormalPriority, 0);
}

void DIA_Done()
{
	UInt16 card;
	LocalID db;

	if (Event && SysCurAppDatabase(&card, &db)==errNone)
		SysNotifyUnregister(card, db, Event, sysNotifyNormalPriority);

	DIASet(DIA_TASKBAR,DIA_TASKBAR);

#ifdef HAVE_SONY_SDK
	if (Sony != sysInvalidRefNum)
	{
		VskSetState(Sony, vskStateEnable, 0);
		SysLibClose(Sony);
		Sony = sysInvalidRefNum;
	}
#endif
}

#ifdef HAVE_SONY_SDK
static void SonyAllow()
{
	if (VskGetAPIVersion(Sony) >= 3)
		VskSetState(Sony, vskStateEnable, vskResizeHorizontally|vskResizeVertically);
	else
		VskSetState(Sony, vskStateEnable, vskResizeVertically);
}
#endif

bool_t DIAHandleEvent(FormType* Form,EventType* Event)
{
	switch (Event->eType)
	{
	case winEnterEvent:
		if (Event->data.winEnter.enterWindow != (WinHandle)Form || Form != FrmGetActiveForm())
			break;

	case frmOpenEvent:

		if (HasPIN >= pinAPIVersion1_0)
		{
	        WinSetConstraintsSize(FrmGetWindowHandle(Form),160,4096,4096,160,4096,4096);
            PINSetInputTriggerState(pinInputTriggerEnabled);
            SysSetOrientationTriggerState(sysOrientationTriggerEnabled);
		}
#ifdef HAVE_SONY_SDK
		if (Sony != sysInvalidRefNum)
			SonyAllow();
#endif
		break;

	default:
		break;
	}
	return 0;
}

void DIALoad(FormType* Form)
{
	if (HasPIN >= pinAPIVersion1_0 && Form)
	{
		FrmSetDIAPolicyAttr(Form, frmDIAPolicyCustom);
		DIASet(DIAGet(DIA_SIP),DIA_SIP); // don't allow pinAPI 1.1 to restore last known state
	}
#ifdef HAVE_SONY_SDK
	if (Sony != sysInvalidRefNum)
		SonyAllow();
#endif
}

void DIAResizedNotify(SysNotifyParamType* Notify)
{
	if (Notify->notifyType==Event && HasPIN < pinAPIVersion1_1)
	{
		EventType Event;
		memset(&Event,0,sizeof(Event));
		Event.eType = winDisplayChangedEvent;
		EvtAddUniqueEventToQueue(&Event, 0, 1);
	}
}

void DIASet(int State,int Mask)
{
	if (HasPIN >= pinAPIVersion1_0)
	{
		if (Mask & DIA_SIP)
		{
			if (State & DIA_SIP)
				PINSetInputAreaState(pinInputAreaOpen);
			else
				PINSetInputAreaState(pinInputAreaClosed);
		}
		if (Mask & DIA_TASKBAR)
		{
			if (State & DIA_TASKBAR)
				StatShow();
			else
				StatHide();
		}
	}
#ifdef HAVE_SONY_SDK
	else
	if (Sony != sysInvalidRefNum)
	{
        UInt16 Allow;
        if (VskGetState(Sony, vskStateEnable, &Allow)!=errNone || Allow==0)
			SonyAllow();

		if ((Mask & DIA_ALL)!=DIA_ALL)
			State |= DIAGet(DIA_ALL) & ~Mask;

		if (State & DIA_SIP)
			VskSetState(Sony, vskStateResize, vskResizeMax);
		else
		if (State & DIA_TASKBAR)
			VskSetState(Sony, vskStateResize, vskResizeMin);
		else
			VskSetState(Sony, vskStateResize, vskResizeNone);
	}
#endif
}

bool_t DIASupported()
{
#ifdef HAVE_SONY_SDK
	if (Sony != sysInvalidRefNum) return 1;
#endif
	return HasPIN >= pinAPIVersion1_0;
}

int DIAGet(int Mask)
{
	int State=0;

	if (HasPIN >= pinAPIVersion1_0)
	{
		UInt32 Value;
		if ((Mask & DIA_SIP) && PINGetInputAreaState() == pinInputAreaOpen)
			State |= DIA_SIP;
		if (Mask & DIA_TASKBAR && (StatGetAttribute(statAttrBarVisible,&Value)!=errNone || Value!=0))
			State |= DIA_TASKBAR;
	}
#ifdef HAVE_SONY_SDK
	else
	if (Sony != sysInvalidRefNum)
	{
		UInt16 Value;
		if (VskGetState(Sony, vskStateResize, &Value)==errNone)
			switch (Value) 
			{
			case vskResizeMax:
				State = DIA_SIP|DIA_TASKBAR;
				break;
			case vskResizeNone:
				State = 0;
				break;
			case vskResizeMin:
			default:
				State = DIA_TASKBAR;
				break;
			}

		State &= Mask;
	}
#endif
	return State;
}

static NOINLINE int DirSwap(int Dir)
{
	if ((SwapLandscape && (Dir & DIR_SWAPXY)) ||
		(SwapPortrait && !(Dir & DIR_SWAPXY)))
		Dir ^= DIR_MIRRORUPDOWN|DIR_MIRRORLEFTRIGHT;
	return Dir;
}

int SetOrientation(int Dir)
{
	if (HasPIN >= pinAPIVersion1_1)
	{
		UInt16 i;

		Dir = DirSwap(Dir);
		if (Dir & DIR_SWAPXY)
			i = (UInt16)((Dir & DIR_MIRRORUPDOWN) ? sysOrientationReverseLandscape : sysOrientationLandscape);
		else
			i = (UInt16)((Dir & DIR_MIRRORUPDOWN) ? sysOrientationReversePortrait : sysOrientationPortrait);

		if (SysSetOrientation(i)==errNone)
			return ERR_NONE;
	}
	return ERR_NOT_SUPPORTED;
}

int GetOrientation()
{
	int Dir = 0;
	if (HasPIN >= pinAPIVersion1_1)
	{
		switch (SysGetOrientation())
		{
		case sysOrientationPortrait: Dir = 0; break;
		case sysOrientationLandscape: Dir = DIR_SWAPXY | DIR_MIRRORLEFTRIGHT; break;
		case sysOrientationReversePortrait: Dir = DIR_MIRRORUPDOWN | DIR_MIRRORLEFTRIGHT; break;
		case sysOrientationReverseLandscape: Dir = DIR_SWAPXY | DIR_MIRRORUPDOWN; break;
		}

		Dir = DirSwap(Dir);
	}
	else
	if (NativeLandscape)
		Dir = DIR_SWAPXY | DIR_MIRRORLEFTRIGHT;
	return Dir;
}

void AdjustOrientation(video* p, bool_t Combine)
{
}

void DIAGetState(diastate* State)
{
    WinGetDisplayExtent(&State->Width, &State->Height);
	State->Direction = 0;
	if (HasPIN >= pinAPIVersion1_1)
		State->Direction =SysGetOrientation();
}

bool_t GetHandedness()
{
	return PrefGetPreference(prefVersion) >= 12 && PrefGetPreference(prefHandednessChoice)==1;
}

#endif
