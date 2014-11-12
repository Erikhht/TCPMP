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
 * $Id: pace.c 531 2006-01-04 12:56:13Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"

#if defined(TARGET_PALMOS)

#include "pace.h"
#include "peal/arm/pealstub.h"

//tremor uses lot of alloca()
#define STACKSIZE	0x8000
//#define STACKCHECK

#ifdef HAVE_PALMONE_SDK
#define NO_HSEXT_TRAPS
#include <68K/System/HardwareUtils68K.h>
#include <Common/System/HsNavCommon.h>
#include <68K/System/HsExt.h>
#endif

typedef struct launch
{
	UInt32 PealCall; //BE
	UInt32 Module; //BE
	UInt32 LoadModule; //BE
	UInt32 FreeModule; //BE
	UInt32 GetSymbol; //BE

	UInt16 launchParameters[2];
	UInt16 launchCode;
	UInt16 launchFlags;

} launch;

typedef struct gadgethandler
{
	uint8_t Code[68];
	struct gadgethandler* Next;

	FormGadgetTypeInCallback* Gadget; //BE
	uint16_t* Event; //BE
	UInt16 Cmd; //BE
	UInt32 Module; //BE
	UInt32 PealCall; //BE
	UInt32 Wrapper; //BE

	FormType* Form;
	int Index;
	FormGadgetHandlerType* Handler; 

} gadgethandler;

typedef struct eventhandler
{
	uint8_t Code[48];
	struct eventhandler* Next;

	uint16_t* Event; //BE
	UInt32 Module; //BE
	UInt32 PealCall; //BE
	UInt32 Wrapper; //BE

	FormType* Form;
	FormEventHandlerType *Handler;

} eventhandler;

static gadgethandler* GadgetHandler = NULL;
static eventhandler* EventHandler = NULL;
static launch Peal = {0};

static NOINLINE void FreeHandlers()
{
	gadgethandler* j;
	eventhandler* i;
	while ((i=EventHandler)!=NULL)
	{
		EventHandler = i->Next;
		MemPtrFree(i);
	}
	while ((j=GadgetHandler)!=NULL)
	{
		GadgetHandler = j->Next;
		MemPtrFree(j);
	}
}

static NOINLINE void SwapLanchParameters(int launchCode,void* launchParameters,bool_t From)
{
	if (launchCode == sysAppLaunchCmdCustomBase)
	{
		vfspath* p = (vfspath*)launchParameters;
		p->volRefNum = SWAP16(p->volRefNum);
	}

	if (launchCode == sysAppLaunchCmdOpenDB)
	{
		UInt16 tmp;
		SysAppLaunchCmdOpenDBType2* p = (SysAppLaunchCmdOpenDBType2*)launchParameters;
		p->cardNo = SWAP16(p->cardNo);
		tmp = p->dbID[0];
		p->dbID[0] = SWAP16(p->dbID[1]);
		p->dbID[1] = SWAP16(tmp);
	}

	if (launchCode == sysAppLaunchCmdNotify)
	{
		SysNotifyParamType* p = (SysNotifyParamType*)launchParameters;
		UInt32 Type = p->notifyType;
		void* Details = p->notifyDetailsP;

		p->notifyType = SWAP32(p->notifyType);
		p->broadcaster = SWAP32(p->broadcaster);
		p->notifyDetailsP = (void*)SWAP32(p->notifyDetailsP);
		p->userDataP = (void*)SWAP32(p->userDataP);
		
		if (From)
		{
			Type = p->notifyType;
			Details = p->notifyDetailsP;
		}

		if (Type == sysNotifySleepRequestEvent)
		{
			SleepEventParamType* i = (SleepEventParamType*)Details;
			i->reason = SWAP16(i->reason);
			i->deferSleep = SWAP16(i->deferSleep);
		}
	}
}

static INLINE uint32_t ReadSwap32(const void* p)
{
	return 
		(((const uint8_t*)p)[0] << 24)|
		(((const uint8_t*)p)[1] << 16)|
		(((const uint8_t*)p)[2] << 8)|
		(((const uint8_t*)p)[3] << 0);
}

static INLINE void WriteSwap32(void* p,uint32_t i)
{
	((uint8_t*)p)[0] = ((uint8_t*)&i)[3];
	((uint8_t*)p)[1] = ((uint8_t*)&i)[2];
	((uint8_t*)p)[2] = ((uint8_t*)&i)[1];
	((uint8_t*)p)[3] = ((uint8_t*)&i)[0];
}

void Event_M68K_To_ARM(const uint16_t* In,EventType* Out)
{
	Out->eType = SWAP16(In[0]);
	Out->penDown = ((uint8_t*)In)[2];
	Out->tapCount = ((uint8_t*)In)[3];
	Out->screenX = SWAP16(In[2]);
	Out->screenY = SWAP16(In[3]);

	switch (Out->eType)
	{
	// 1*16bit
	case frmLoadEvent:
	case menuEvent:
		Out->data.menu.itemID = SWAP16(In[4]);
		break;

	// 3*16bit
	case frmUpdateEvent:
	case keyUpEvent:
	case keyDownEvent:
	case keyHoldEvent:
		Out->data.keyDown.chr = SWAP16(In[4]);
		Out->data.keyDown.keyCode = SWAP16(In[5]);
		Out->data.keyDown.modifiers = SWAP16(In[6]);
		break;

	// 16bit,32bit
/*	case fldChangedEvent:
		Out->data.fldChanged.fieldID = SWAP16(In[4]);
		Out->data.fldChanged.pField = (FieldType*)ReadSwap32(In+5);
		break;
*/
	// 16bit,32bit,16bit,32bit,16bit,16bit
	case popSelectEvent:
		Out->data.popSelect.controlID = SWAP16(In[4]);
		Out->data.popSelect.controlP = (ControlType*)ReadSwap32(In+5);
		Out->data.popSelect.listID = SWAP16(In[7]);
		Out->data.popSelect.listP = (ListType*)SWAP32(*(uint32_t*)(In+8));
		Out->data.popSelect.selection = SWAP16(In[10]);
		Out->data.popSelect.priorSelection = SWAP16(In[11]);
		break;

	// 16bit,32bit,16bit
	case lstSelectEvent:
		Out->data.lstSelect.listID = SWAP16(In[4]);
		Out->data.lstSelect.pList = (ListType*)ReadSwap32(In+5);
		Out->data.lstSelect.selection = SWAP16(In[7]);
		break;

	//custom
	case sclRepeatEvent:
		Out->data.sclRepeat.scrollBarID = SWAP16(In[4]);
		Out->data.sclRepeat.pScrollBar = (ScrollBarType*)ReadSwap32(In+5);
		Out->data.sclRepeat.value = SWAP16(In[7]);
		Out->data.sclRepeat.newValue = SWAP16(In[8]);
		Out->data.sclRepeat.time = ReadSwap32(In+9);
		break;

	//custom
	case ctlRepeatEvent:
		Out->data.ctlRepeat.controlID = SWAP16(In[4]);
		Out->data.ctlRepeat.pControl = (ControlType*)ReadSwap32(In+5);
		Out->data.ctlRepeat.time = ReadSwap32(In+7);
		Out->data.ctlRepeat.value = SWAP16(In[9]);
		break;

	//custom
	case ctlSelectEvent: 
		Out->data.ctlSelect.controlID = SWAP16(In[4]);
		Out->data.ctlSelect.pControl = (ControlType*)ReadSwap32(In+5);
		Out->data.ctlSelect.on = ((uint8_t*)In)[14];
		Out->data.ctlSelect.reserved1 = ((uint8_t*)In)[15];
		Out->data.ctlSelect.value = SWAP16(In[8]);
		break;

	// 2*32bit
	case winEnterEvent:
	case winExitEvent:
		Out->data.winExit.enterWindow = (WinHandle)SWAP32(((uint32_t*)In)[2]);
		Out->data.winExit.exitWindow = (WinHandle)SWAP32(((uint32_t*)In)[3]);
		break;

	default:
		memcpy(Out->data.generic.datum,In+4,sizeof(uint16_t)*8);
		break;
	}
}

void Event_ARM_To_M68K(const EventType* In,uint16_t* Out)
{
	Out[0] = SWAP16(In->eType);
	((uint8_t*)Out)[2] = In->penDown;
	((uint8_t*)Out)[3] = In->tapCount;
	Out[2] = SWAP16(In->screenX);
	Out[3] = SWAP16(In->screenY);

	switch (In->eType)
	{
	// 1*16bit
	case frmLoadEvent:
	case menuEvent:
		Out[4] = SWAP16(In->data.menu.itemID);
		break;

	// 3*16bit
	case frmUpdateEvent:
	case keyUpEvent:
	case keyDownEvent:
	case keyHoldEvent:
		Out[4] = SWAP16(In->data.keyDown.chr);
		Out[5] = SWAP16(In->data.keyDown.keyCode);
		Out[6] = SWAP16(In->data.keyDown.modifiers);
		break;

	// 16bit,32bit,16bit
	case lstSelectEvent:
		Out[4] = SWAP16(In->data.lstSelect.listID);
		WriteSwap32(Out+5,(uint32_t)In->data.lstSelect.pList);
		Out[7] = SWAP16(In->data.lstSelect.selection);
		break;

	// 16bit,32bit
/*	case fldChangedEvent:
		Out[4] = SWAP16(In->data.fldChanged.fieldID);
		WriteSwap32(Out+5,(uint32_t)In->data.fldChanged.pField);
		break;
*/
	// 16bit,32bit,16bit,32bit,16bit,16bit
	case popSelectEvent:
		Out[4] = SWAP16(In->data.popSelect.controlID);
		WriteSwap32(Out+5,(uint32_t)In->data.popSelect.controlP);
		Out[7] = SWAP16(In->data.popSelect.listID);
		WriteSwap32(Out+8,(uint32_t)In->data.popSelect.listP);
		Out[10] = SWAP16(In->data.popSelect.selection);
		Out[11] = SWAP16(In->data.popSelect.priorSelection);
		break;

	//custom
	case sclRepeatEvent:
		Out[4] = SWAP16(In->data.sclRepeat.scrollBarID);
		WriteSwap32(Out+5,(uint32_t)In->data.sclRepeat.pScrollBar);
		Out[7] = SWAP16(In->data.sclRepeat.value);
		Out[8] = SWAP16(In->data.sclRepeat.newValue);
		WriteSwap32(Out+9,In->data.sclRepeat.time);
		break;

	//custom
	case ctlRepeatEvent:
		Out[4] = SWAP16(In->data.ctlRepeat.controlID);
		WriteSwap32(Out+5,(uint32_t)In->data.ctlRepeat.pControl);
		WriteSwap32(Out+7,In->data.ctlRepeat.time);
		Out[9] = SWAP16(In->data.ctlRepeat.value);
		break;

	//custom
	case ctlSelectEvent: 
		Out[4] = SWAP16(In->data.ctlSelect.controlID);
		WriteSwap32(Out+5,(uint32_t)In->data.ctlSelect.pControl);
		((uint8_t*)Out)[14] = In->data.ctlSelect.on;
		((uint8_t*)Out)[15] = In->data.ctlSelect.reserved1;
		Out[8] = SWAP16(In->data.ctlSelect.value);
		break;

	// 2*32bit
	case winEnterEvent:
	case winExitEvent:
		((uint32_t*)Out)[2] = SWAP32(In->data.winExit.enterWindow);
		((uint32_t*)Out)[3] = SWAP32(In->data.winExit.exitWindow);
		break;

	default:
		memcpy(Out+4,In->data.generic.datum,sizeof(uint16_t)*8);
		break;
	}
}


static void GadgetTypeInCallback_M68K_To_ARM(const uint16_t* In,FormGadgetTypeInCallback* Out)
{
	Out->id = SWAP16(In[0]);
	*(UInt16*)&Out->attr = SWAP16(In[1]);
	Out->rect.topLeft.x = SWAP16(In[2]);
	Out->rect.topLeft.y = SWAP16(In[3]);
	Out->rect.extent.x = SWAP16(In[4]);
	Out->rect.extent.y = SWAP16(In[5]);
	Out->data = (const void*)SWAP32(*(const uint32_t*)(In+6));
	Out->handler = (FormGadgetHandlerType*)SWAP32(*(const uint32_t*)(In+8));
}

static void GadgetTypeInCallback_ARM_To_M68K(const FormGadgetTypeInCallback* In,uint16_t* Out)
{
	Out[0] = SWAP16(In->id);
	Out[1] = SWAP16(*(UInt16*)&In->attr);
	Out[2] = SWAP16(In->rect.topLeft.x);
	Out[3] = SWAP16(In->rect.topLeft.y);
	Out[4] = SWAP16(In->rect.extent.x);
	Out[5] = SWAP16(In->rect.extent.y);
	*(uint32_t*)(Out+6) = SWAP32(In->data);
	*(uint32_t*)(Out+8) = SWAP32(In->handler);
}

static NOINLINE bool_t CmdLaunch(int Code)
{
	return Code == sysAppLaunchCmdNormalLaunch ||
		Code == sysAppLaunchCmdOpenDB ||
		Code == sysAppLaunchCmdCustomBase;
}

#if defined(_M_IX86)

#define NONE 0

pcecall PceCall = {NULL};

#include <windows.h>

DLLEXPORT uint32_t PaceMain86(const void *emulStateP, launch* Launch, Call68KFuncType *call68KFuncP)
{
	MemPtr Parameters = (MemPtr)((SWAP16(Launch->launchParameters[0])<<16)|(SWAP16(Launch->launchParameters[1])));
	int Code = SWAP16(Launch->launchCode);
	uint32_t Result; 
	pcecall Old = PceCall;

	if (CmdLaunch(Code))
		memcpy(&Peal,Launch,sizeof(launch));

	PceCall.Func = call68KFuncP;
	PceCall.State = emulStateP;

	SwapLanchParameters(Code,Parameters,1);
	Result = PilotMain((UInt16)Code,Parameters,SWAP16(Launch->launchFlags));
	SwapLanchParameters(Code,Parameters,0);

	if (CmdLaunch(Code))
		FreeHandlers();

	PceCall = Old;
	return Result;
}

void SaveSysRegs(sysregs* p)
{
}

void LoadSysRegs(sysregs* p)
{
}

static NOINLINE unsigned long Call68K(unsigned long trapOrFunction,
									const void *argsOnStackP, unsigned long argsSizeAndwantA0)
{
	pcecall* Call = &PceCall;
	return Call->Func(Call->State,trapOrFunction,argsOnStackP,argsSizeAndwantA0);
}

DLLEXPORT Boolean WrapperEventHandler86(const void *emulStateP, eventhandler* i, Call68KFuncType *call68KFuncP)
{
	Boolean Result;
	EventType EventARM;
	uint16_t* EventM68K = (uint16_t*) SWAP32(i->Event);
	pcecall Old = PceCall;
	PceCall.Func = call68KFuncP;
	PceCall.State = emulStateP;
	Event_M68K_To_ARM(EventM68K,&EventARM);
	Result = i->Handler(&EventARM);
	if (!Result && EventARM.eType == frmCloseEvent)
		i->Form = NULL;
	PceCall = Old;
	return Result;
}

DLLEXPORT Boolean WrapperGadgetHandler86(const void *emulStateP, gadgethandler* i, Call68KFuncType *call68KFuncP)
{
	EventType EventARM;
	Boolean Result;
	FormGadgetTypeInCallback GadgetARM;
	uint16_t* Gadget = (uint16_t*) SWAP32(i->Gadget);
	UInt16 Cmd = SWAP16(i->Cmd);
	uint16_t* EventM68K = (uint16_t*) SWAP32(i->Event);
	pcecall Old = PceCall;
	PceCall.Func = call68KFuncP;
	PceCall.State = emulStateP;
	GadgetTypeInCallback_M68K_To_ARM(Gadget,&GadgetARM);
	if (Cmd == formGadgetHandleEventCmd)
		Event_M68K_To_ARM(EventM68K,&EventARM);
	Result = i->Handler(&GadgetARM,Cmd,&EventARM);
	GadgetTypeInCallback_ARM_To_M68K(&GadgetARM,Gadget);
	if (Cmd == formGadgetDeleteCmd)
		i->Form = NULL;
	PceCall = Old;
	return Result;
}

#elif defined(ARM)

#define NONE

void SaveSysRegs(sysregs* p)
{
	p->SysReg = PceCall.SysReg;
	asm volatile(
		"mov %0, r10\n\t"
		: "=&r"(p->GOT) : : "cc");
}

void LoadSysRegs(sysregs* p)
{
	asm volatile(
		"mov r9,%0\n\t"
		"mov r10,%1\n\t"
		: : "r"(p->SysReg),"r"(p->GOT): "cc");
}

static NOINLINE unsigned long Call68K(unsigned long trapOrFunction,
									  const void *argsOnStackP, unsigned long argsSizeAndwantA0)
{
	pcecall* Call = &PceCall;
	unsigned long Result;
	register void *SysReg asm("r9") = Call->SysReg;
	Result = Call->Func(Call->State,trapOrFunction,argsOnStackP,argsSizeAndwantA0);
	Call->SysReg = SysReg;
	return Result;
}

static NOINLINE void SafeMemCpy(void* d,const void* s,int n)
{
	uint8_t* d8 = (uint8_t*)d;
	const uint8_t* s8= (const uint8_t*)s;
	for (;n>0;--n,++d8,++s8)
		*d8 = *s8;
}

#ifdef STACKCHECK
static void NOINLINE StackCheck(char* p)
{
	int i;
	for (i=0;i<STACKSIZE;++i)
		if (p[i] != 0x55)
			break;
	if (i < 4096)
	{
		char s[64];
		sprintf(s,"Possible stack overflow %d/%d. ",i,STACKSIZE);
		FrmCustomAlert(WarningOKAlert, s, "Please contact developer!", " ");
	}
}
#endif

uint32_t PaceMain(launch* Launch)
{
	MemPtr Parameters = (MemPtr)((SWAP16(Launch->launchParameters[0])<<16)|(SWAP16(Launch->launchParameters[1])));
	int Code = SWAP16(Launch->launchCode);
	void* Old = NULL;
	char* Stack = NULL;
	uint32_t Result; 

	if (CmdLaunch(Code))
	{
		SafeMemCpy(&Peal,Launch,sizeof(launch));
		Stack = MemGluePtrNew(STACKSIZE);
		if (!Stack)
			return errNone;
		memset(Stack,0x55,STACKSIZE);
		Old = SwapSP(Stack+STACKSIZE);
	}

	SwapLanchParameters(Code,Parameters,1);
	Result = PilotMain(Code,Parameters,SWAP16(Launch->launchFlags));
	SwapLanchParameters(Code,Parameters,0);

	if (Stack)
	{
		SwapSP(Old);
#ifdef STACKCHECK
		StackCheck(Stack);
#endif
		MemPtrFree(Stack);
		FreeHandlers();
	}

	return Result;
}

static Boolean WrapperEventHandler(eventhandler* i)
{
	Boolean Result;
	EventType EventARM;
	const uint16_t* EventM68K = (const uint16_t*) SWAP32(i->Event);
	Event_M68K_To_ARM(EventM68K,&EventARM);
	Result = i->Handler(&EventARM);
	if (!Result && EventARM.eType == frmCloseEvent)
		i->Form = NULL;
	return Result;
}

static Boolean WrapperGadgetHandler(gadgethandler* i)
{
	EventType EventARM;
	Boolean Result;
	FormGadgetTypeInCallback GadgetARM;
	uint16_t* Gadget = (uint16_t*) SWAP32(i->Gadget);
	UInt16 Cmd = SWAP16(i->Cmd);
	const uint16_t* EventM68K = (const uint16_t*) SWAP32(i->Event);
	GadgetTypeInCallback_M68K_To_ARM(Gadget,&GadgetARM);
	if (Cmd == formGadgetHandleEventCmd)
		Event_M68K_To_ARM(EventM68K,&EventARM);
	Result = i->Handler(&GadgetARM,Cmd,&EventARM);
	GadgetTypeInCallback_ARM_To_M68K(&GadgetARM,Gadget);
	if (Cmd == formGadgetDeleteCmd)
		i->Form = NULL;
	return Result;
}

#else
#error Not supported platform
#endif

typedef struct emustate
{
	uint32_t instr;
	uint32_t regD[8];
	uint32_t regA[8];
	uint32_t regPC;

} emustate;

#define PACE_PARAM			_v

#define PACE_BEGIN(declare)	NOINLINE declare { uint16_t PACE_PARAM[] = {
#define PACE_BEGINEVENT(declare) NOINLINE declare { uint16_t ev[12]; uint16_t PACE_PARAM[] = {
#define PACE_BEGINEX(declare,name)	NOINLINE declare { uint16_t name[] = {
#define PACE_VAR(name)		}; uint16_t name[] = {

#define PACE_ADD8(name) 	(uint8_t)name,
#define PACE_ADD16(name) 	SWAP16(name),
#define PACE_ADD32(name)	SWAP16(((uint32_t)(name))>>16),SWAP16(name),

#define PACE_DATA16(name)	*name = SWAP16(*name);
#define PACE_DATA32(name)	*name = SWAP32(*name);
#define PACE_CDATA16(name)	if (name) *name = SWAP16(*name);
#define PACE_CDATA32(name)	if (name) *name = SWAP32(*name);
#define PACE_SEL(sel)		((emustate*)PceCall.State)->regD[2] = sel;

#define PACE_END(Trap,PreProcess,PostProcess) }; PreProcess; \
    Call68K(PceNativeTrapNo(Trap),PACE_PARAM,sizeof(PACE_PARAM)); \
	PostProcess; }

#define PACE_ENDVOID(Trap,PreProcess,PostProcess) 0}; PreProcess; \
    Call68K(PceNativeTrapNo(Trap),PACE_PARAM,0); \
	PostProcess; }

#define PACE_END8(Trap,PreProcess,PostProcess) }; uint32_t ret; PreProcess; \
	ret = Call68K(PceNativeTrapNo(Trap),PACE_PARAM,sizeof(PACE_PARAM)); \
	PostProcess; return (uint8_t)ret; }

#define PACE_END8VOID(Trap,PreProcess,PostProcess) 0}; uint32_t ret; PreProcess; \
	ret = Call68K(PceNativeTrapNo(Trap),PACE_PARAM,0); \
	PostProcess; return (uint8_t)ret; }

#define PACE_END16(Trap,PreProcess,PostProcess) }; uint32_t ret; PreProcess; \
	ret = Call68K(PceNativeTrapNo(Trap),PACE_PARAM,sizeof(PACE_PARAM)); \
	PostProcess; return (uint16_t)ret; }

#define PACE_END16VOID(Trap,PreProcess,PostProcess) 0}; uint32_t ret; PreProcess; \
	ret = Call68K(PceNativeTrapNo(Trap),PACE_PARAM,0); \
	PostProcess; return (uint16_t)ret; }

#define PACE_END32(Trap,PreProcess,PostProcess) }; uint32_t ret; PreProcess; \
	ret = Call68K(PceNativeTrapNo(Trap),PACE_PARAM,sizeof(PACE_PARAM)); \
	PostProcess; return ret; }

#define PACE_END32VOID(Trap,PreProcess,PostProcess) 0}; uint32_t ret; PreProcess; \
	ret = Call68K(PceNativeTrapNo(Trap),PACE_PARAM,0); \
	PostProcess; return ret; }

#define PACE_ENDPTR(Trap,PreProcess,PostProcess) }; uint32_t ret; PreProcess; \
	ret = Call68K(PceNativeTrapNo(Trap),PACE_PARAM,sizeof(PACE_PARAM)|kPceNativeWantA0); \
	PostProcess; return (void*)ret; }

#define PACE_ENDPTRVOID(Trap,PreProcess,PostProcess) 0}; uint32_t ret; PreProcess; \
	ret = Call68K(PceNativeTrapNo(Trap),PACE_PARAM,0|kPceNativeWantA0); \
	PostProcess; return (void*)ret; }

static NOINLINE void SwapBlock32(void* Ptr,int n)
{
	if (Ptr)
	{
		int i;
		char* p = (char*)Ptr;
		for (i=0;i<n;++i,p+=4)
			PACE_DATA32((uint32_t*)p);
	}
}

static NOINLINE void SwapStruct(void* Ptr,...)
{
	if (Ptr)
	{
		int Size;
		char* p = (char*)Ptr;
		va_list Args;
		va_start(Args,Ptr);
		while ((Size = va_arg(Args,int))!=0)
		{
			if (Size == 4)
				PACE_DATA32((uint32_t*)p)
			else
			if (Size == 2)
				PACE_DATA16((uint16_t*)p)
			p += Size;
		}
		va_end(Args);
	}
}

//---------------------------------------------------------------------

PACE_BEGIN(Int32 GetOEMSleepMode())
PACE_END32VOID(sysTrapOEMDispatch2,PACE_SEL(263),NONE)

PACE_BEGIN(Err SetOEMSleepMode(Int32 Mode))
PACE_ADD32(Mode)
PACE_END16(sysTrapOEMDispatch2,PACE_SEL(264),NONE)

PACE_BEGIN(UInt16 SysGetOrientation())
PACE_END16VOID(sysTrapPinsDispatch,PACE_SEL(pinSysGetOrientation),NONE)

PACE_BEGIN(Err WinGetPixelRGB(Coord x, Coord y, RGBColorType* rgbP))
PACE_ADD16(x)
PACE_ADD16(y)
PACE_ADD32(rgbP)
PACE_END16(sysTrapWinGetPixelRGB,NONE,NONE)

PACE_BEGIN(void	WinSetForeColorRGB(const RGBColorType* newRgbP, RGBColorType* prevRgbP))
PACE_ADD32(newRgbP)
PACE_ADD32(prevRgbP)
PACE_END(sysTrapWinSetForeColorRGB,NONE,NONE)

PACE_BEGIN(void WinPaintPixel(Coord x, Coord y))
PACE_ADD16(x)
PACE_ADD16(y)
PACE_END(sysTrapWinPaintPixel,NONE,NONE)

PACE_BEGIN(void WinDrawPixel(Coord x, Coord y))
PACE_ADD16(x)
PACE_ADD16(y)
PACE_END(sysTrapWinDrawPixel,NONE,NONE)

PACE_BEGIN(void FrmSetFocus(FormType *formP, UInt16 fieldIndex))
PACE_ADD32(formP)
PACE_ADD16(fieldIndex)
PACE_END(sysTrapFrmSetFocus,NONE,NONE)

PACE_BEGIN(void FrmSetTitle (FormType *formP, Char *newTitle))
PACE_ADD32(formP)
PACE_ADD32(newTitle)
PACE_END(sysTrapFrmSetTitle,NONE,NONE)

PACE_BEGIN(FormObjectKind FrmGetObjectType(const FormType *formP, UInt16 objIndex))
PACE_ADD32(formP)
PACE_ADD16(objIndex)
PACE_END8(sysTrapFrmGetObjectType,NONE,NONE)

PACE_BEGIN(FormLabelType *FrmNewLabel (FormType **formPP, UInt16 ID, const Char *textP,
	Coord x, Coord y, FontID font))
PACE_ADD32(formPP)
PACE_ADD16(ID)
PACE_ADD32(textP)
PACE_ADD16(x)
PACE_ADD16(y)
PACE_ADD8(font)
PACE_ENDPTR(sysTrapFrmNewLabel,PACE_CDATA32((UInt32*)formPP),PACE_CDATA32((UInt32*)formPP))

PACE_BEGIN(FormType *FrmNewForm(UInt16 formID, const Char *titleStrP,
	Coord x, Coord y, Coord width, Coord height, Boolean modal,
	UInt16 defaultButton, UInt16 helpRscID, UInt16 menuRscID))
PACE_ADD16(formID)
PACE_ADD32(titleStrP)
PACE_ADD16(x)
PACE_ADD16(y)
PACE_ADD16(width)
PACE_ADD16(height)
PACE_ADD8(modal)
PACE_ADD16(defaultButton)
PACE_ADD16(helpRscID)
PACE_ADD16(menuRscID)
PACE_ENDPTR(sysTrapFrmNewForm,NONE,NONE)

PACE_BEGIN(void FrmCloseAllForms())
PACE_ENDVOID(sysTrapFrmCloseAllForms,NONE,NONE)

PACE_BEGIN(UInt16 FrmGetObjectId (const FormType *formP, UInt16 objIndex))
PACE_ADD32(formP)
PACE_ADD16(objIndex)
PACE_END16(sysTrapFrmGetObjectId,NONE,NONE)

PACE_BEGIN(UInt16 FrmGetNumberOfObjects(const FormType *formP))
PACE_ADD32(formP)
PACE_END16(sysTrapFrmGetNumberOfObjects,NONE,NONE)

PACE_BEGIN(UInt16 FrmGetActiveFormID())
PACE_END16VOID(sysTrapFrmGetActiveFormID,NONE,NONE)

PACE_BEGIN(void FrmGotoForm (UInt16 formId))
PACE_ADD16(formId)
PACE_END(sysTrapFrmGotoForm,NONE,NONE)

PACE_BEGIN(void CtlSetUsable (ControlType *controlP, Boolean usable))
PACE_ADD32(controlP)
PACE_ADD8(usable)
PACE_END(sysTrapCtlSetUsable,NONE,NONE)

PACE_BEGIN(Int16 CtlGetValue (const ControlType *controlP))
PACE_ADD32(controlP)
PACE_END16(sysTrapCtlGetValue,NONE,NONE)

PACE_BEGIN(void CtlSetValue (ControlType *controlP, Int16 newValue))
PACE_ADD32(controlP)
PACE_ADD16(newValue)
PACE_END(sysTrapCtlSetValue,NONE,NONE)

PACE_BEGIN(void CtlSetLabel (ControlType *controlP, const Char *newLabel))
PACE_ADD32(controlP)
PACE_ADD32(newLabel)
PACE_END(sysTrapCtlSetLabel,NONE,NONE)

PACE_BEGIN(void	EvtEnableGraffiti(Boolean enable))
PACE_ADD8(enable)
PACE_END(sysTrapEvtEnableGraffiti,NONE,NONE);

PACE_BEGIN(Err EvtResetAutoOffTimer())
PACE_END16VOID(sysTrapEvtResetAutoOffTimer,NONE,NONE)

PACE_BEGIN(FormType *FrmGetFirstForm())
PACE_ENDPTRVOID(sysTrapFrmGetFirstForm,NONE,NONE)

PACE_BEGIN(void *FrmGetObjectPtr (const FormType *formP, UInt16 objIndex))
PACE_ADD32(formP)
PACE_ADD16(objIndex)
PACE_ENDPTR(sysTrapFrmGetObjectPtr,NONE,NONE)

#if defined(_M_IX86)
// simulator freezes many times. bypassing...
Boolean EvtEventAvail() { return 1; }
#else
PACE_BEGIN(Boolean EvtEventAvail())
PACE_END8VOID(sysTrapEvtEventAvail,NONE,NONE)
#endif

PACE_BEGIN(void EvtGetPen(Int16 *pScreenX, Int16 *pScreenY, Boolean *pPenDown))
PACE_ADD32(pScreenX)
PACE_ADD32(pScreenY)
PACE_ADD32(pPenDown)
PACE_END(sysTrapEvtGetPen,NONE,PACE_DATA16(pScreenX) PACE_DATA16(pScreenY))

PACE_BEGIN(Boolean EvtSysEventAvail(Boolean ignorePenUps))
PACE_ADD8(ignorePenUps)
PACE_END8(sysTrapEvtSysEventAvail,NONE,NONE)

PACE_BEGINEVENT(void EvtAddUniqueEventToQueue(const EventType *eventP, UInt32 id,Boolean inPlace))
PACE_ADD32(&ev)
PACE_ADD32(id)
PACE_ADD8(inPlace)
PACE_END(sysTrapEvtAddUniqueEventToQueue,Event_ARM_To_M68K(eventP,ev),NONE)

PACE_BEGINEVENT(void EvtGetEvent(EventType *event, Int32 timeout))
PACE_ADD32(&ev)
PACE_ADD32(timeout)
PACE_END(sysTrapEvtGetEvent,NONE,Event_M68K_To_ARM(ev,event))

PACE_BEGIN(UInt16 FrmGetObjectIndex (const FormType *formP, UInt16 objID))
PACE_ADD32(formP)
PACE_ADD16(objID)
PACE_END16(sysTrapFrmGetObjectIndex,NONE,NONE)

PACE_BEGIN(void CtlSetSliderValues(ControlType *ctlP, const UInt16 *minValueP, const UInt16 *maxValueP,
								   const UInt16 *pageSizeP, const UInt16 *valueP))
PACE_ADD32(ctlP)
PACE_ADD32(minValueP)
PACE_ADD32(maxValueP)
PACE_ADD32(pageSizeP)
PACE_ADD32(valueP)
PACE_END(sysTrapCtlSetSliderValues,PACE_CDATA16((UInt16*)minValueP) PACE_CDATA16((UInt16*)maxValueP) PACE_CDATA16((UInt16*)pageSizeP) PACE_CDATA16((UInt16*)valueP),
								   PACE_CDATA16((UInt16*)minValueP) PACE_CDATA16((UInt16*)maxValueP) PACE_CDATA16((UInt16*)pageSizeP) PACE_CDATA16((UInt16*)valueP))
								   // possible problem overriding const input

PACE_BEGIN(WinHandle WinSetDrawWindow(WinHandle winHandle))
PACE_ADD32(winHandle)
PACE_ENDPTR(sysTrapWinSetDrawWindow,NONE,NONE)

PACE_BEGIN(WinHandle WinGetDrawWindow())
PACE_ENDPTRVOID(sysTrapWinGetDrawWindow,NONE,NONE)

PACE_BEGIN(void WinDrawLine (Coord x1, Coord y1, Coord x2, Coord y2))
PACE_ADD16(x1)
PACE_ADD16(y1)
PACE_ADD16(x2)
PACE_ADD16(y2)
PACE_END(sysTrapWinDrawLine,NONE,NONE)

PACE_BEGIN(IndexedColorType UIColorGetTableEntryIndex(UIColorTableEntries which))
PACE_ADD8(which)
PACE_END8(sysTrapUIColorGetTableEntryIndex,NONE,NONE)

PACE_BEGIN(IndexedColorType WinSetForeColor (IndexedColorType foreColor))
PACE_ADD8(foreColor)
PACE_END8(sysTrapWinSetForeColor,NONE,NONE)

PACE_BEGIN(IndexedColorType WinSetBackColor (IndexedColorType backColor))
PACE_ADD8(backColor)
PACE_END8(sysTrapWinSetBackColor,NONE,NONE)

PACE_BEGIN(IndexedColorType WinSetTextColor (IndexedColorType textColor))
PACE_ADD8(textColor)
PACE_END8(sysTrapWinSetTextColor,NONE,NONE)

PACE_BEGIN(UInt16 FrmDoDialog (FormType *formP))
PACE_ADD32(formP)
PACE_END16(sysTrapFrmDoDialog,NONE,NONE)

PACE_BEGIN(UInt16 FrmGetFormId (const FormType *formP))
PACE_ADD32(formP)
PACE_END16(sysTrapFrmGetFormId,NONE,NONE)

PACE_BEGIN(void FrmPopupForm (UInt16 formId))
PACE_ADD16(formId)
PACE_END(sysTrapFrmPopupForm,NONE,NONE)

PACE_BEGIN(void FrmDrawForm (FormType *formP))
PACE_ADD32(formP)
PACE_END(sysTrapFrmDrawForm,NONE,NONE)

PACE_BEGIN(void FrmEraseForm (FormType *formP))
PACE_ADD32(formP)
PACE_END(sysTrapFrmEraseForm,NONE,NONE)

PACE_BEGIN(Boolean MenuHideItem(UInt16 id))
PACE_ADD16(id)
PACE_END8(sysTrapMenuHideItem,NONE,NONE)

PACE_BEGINEVENT(Boolean MenuHandleEvent(MenuBarType *menuP, EventType *event, UInt16 *error))
PACE_ADD32(menuP)
PACE_ADD32(&ev)
PACE_ADD32(error)
PACE_END8(sysTrapMenuHandleEvent,Event_ARM_To_M68K(event,ev),PACE_DATA16(error))

PACE_BEGIN(Err SysNotifyRegister(UInt16 cardNo, LocalID dbID, UInt32 notifyType, 
								 SysNotifyProcPtr callbackP, Int8 priority, void *userDataP))
PACE_ADD16(cardNo)
PACE_ADD32(dbID)
PACE_ADD32(notifyType)
PACE_ADD32(callbackP)
PACE_ADD8(priority)
PACE_ADD32(userDataP)
PACE_END16(sysTrapSysNotifyRegister,NONE,NONE)

PACE_BEGIN(Boolean FrmVisible(const FormType *formP))
PACE_ADD32(formP)
PACE_END8(sysTrapFrmVisible,NONE,NONE)

PACE_BEGIN(void FrmSetActiveForm(FormType *formP))
PACE_ADD32(formP)
PACE_END(sysTrapFrmSetActiveForm,NONE,NONE)

PACE_BEGIN(FormType* FrmInitForm(UInt16 rscID))
PACE_ADD16(rscID)
PACE_ENDPTR(sysTrapFrmInitForm,NONE,NONE)

PACE_BEGIN(WinHandle FrmGetWindowHandle(const FormType *formP))
PACE_ADD32(formP)
PACE_ENDPTR(sysTrapFrmGetWindowHandle,NONE,NONE);

PACE_BEGIN(Err SysNotifyUnregister(UInt16 cardNo, LocalID dbID, UInt32 notifyType, Int8 priority))
PACE_ADD16(cardNo)
PACE_ADD32(dbID)
PACE_ADD32(notifyType)
PACE_ADD8(priority)
PACE_END16(sysTrapSysNotifyUnregister,NONE,NONE)

PACE_BEGIN(Err SysCurAppDatabase(UInt16 *cardNoP, LocalID *dbIDP))
PACE_ADD32(cardNoP)
PACE_ADD32(dbIDP)
PACE_END16(sysTrapSysCurAppDatabase,NONE,PACE_DATA16(cardNoP) PACE_DATA32(dbIDP))

PACE_BEGIN(Err WinSetConstraintsSize(WinHandle winH, Coord minH, Coord prefH, Coord maxH,
							Coord minW, Coord prefW, Coord maxW))
PACE_ADD32(winH)
PACE_ADD16(minH)
PACE_ADD16(prefH)
PACE_ADD16(maxH)
PACE_ADD16(minW)
PACE_ADD16(prefW)
PACE_ADD16(maxW)
PACE_END16(sysTrapPinsDispatch,PACE_SEL(pinWinSetConstraintsSize),NONE)

PACE_BEGIN(Err SysSetOrientation(UInt16 orientation))
PACE_ADD16(orientation)
PACE_END16(sysTrapPinsDispatch,PACE_SEL(pinSysSetOrientation),NONE)

PACE_BEGIN(UInt16 PINGetInputAreaState())
PACE_END16VOID(sysTrapPinsDispatch,PACE_SEL(pinPINGetInputAreaState),NONE)

PACE_BEGIN(Err PINSetInputTriggerState(UInt16 state))
PACE_ADD16(state)
PACE_END16(sysTrapPinsDispatch,PACE_SEL(pinPINSetInputTriggerState),NONE)

PACE_BEGIN(Err FrmSetDIAPolicyAttr (FormPtr formP, UInt16 diaPolicy))
PACE_ADD32(formP)
PACE_ADD16(diaPolicy)
PACE_END16(sysTrapPinsDispatch,PACE_SEL(pinFrmSetDIAPolicyAttr),NONE)

PACE_BEGIN(Err PINSetInputAreaState(UInt16 state))
PACE_ADD16(state)
PACE_END16(sysTrapPinsDispatch,PACE_SEL(pinPINSetInputAreaState),NONE)

PACE_BEGIN(Err SysSetOrientationTriggerState(UInt16 triggerState))
PACE_ADD16(triggerState)
PACE_END16(sysTrapPinsDispatch,PACE_SEL(pinSysSetOrientationTriggerState),NONE)
			
PACE_BEGIN(Err StatGetAttribute(UInt16 selector, UInt32* dataP))
PACE_ADD16(selector)
PACE_ADD32(dataP)
PACE_END16(sysTrapPinsDispatch,PACE_SEL(pinStatGetAttribute),PACE_DATA32(dataP))
	
PACE_BEGIN(Err StatHide())
PACE_END16VOID(sysTrapPinsDispatch,PACE_SEL(pinStatHide),NONE)

PACE_BEGIN(Err StatShow())
PACE_END16VOID(sysTrapPinsDispatch,PACE_SEL(pinStatShow),NONE)

PACE_BEGIN(UInt16 SysSetAutoOffTime(UInt16 seconds))
PACE_ADD16(seconds)
PACE_END16(sysTrapSysSetAutoOffTime,NONE,NONE);

PACE_BEGIN(Err SysTaskDelay(Int32 delay))
PACE_ADD32(delay)
PACE_END16(sysTrapSysTaskDelay,NONE,NONE);

PACE_BEGIN(UInt16 SysTicksPerSecond())
PACE_END16VOID(sysTrapSysTicksPerSecond,NONE,NONE)

PACE_BEGIN(UInt32 TimGetTicks())
PACE_END32VOID(sysTrapTimGetTicks,NONE,NONE)

PACE_BEGIN(MemPtr MemPtrNew(UInt32 n))
PACE_ADD32(n)
PACE_ENDPTR(sysTrapMemPtrNew,NONE,NONE)

PACE_BEGIN(Err MemChunkFree(MemPtr p))
PACE_ADD32(p)
PACE_END16(sysTrapMemChunkFree,NONE,NONE)

PACE_BEGIN(UInt32 MemPtrSize(MemPtr p))
PACE_ADD32(p)
PACE_END32(sysTrapMemPtrSize,NONE,NONE)

PACE_BEGIN(Err MemPtrResize(MemPtr p,UInt32 n))
PACE_ADD32(p)
PACE_ADD32(n)
PACE_END16(sysTrapMemPtrResize,NONE,NONE)

PACE_BEGIN(Err MemHeapFreeBytes(UInt16 heapID,UInt32* freeP,UInt32* maxP))
PACE_ADD16(heapID)
PACE_ADD32(freeP)
PACE_ADD32(maxP)
PACE_END16(sysTrapMemHeapFreeBytes,NONE,PACE_DATA32(freeP) PACE_DATA32(maxP))

PACE_BEGIN(UInt16 MemHeapID(UInt16 cardNo, UInt16 heapIndex))
PACE_ADD16(cardNo)
PACE_ADD16(heapIndex)
PACE_END16(sysTrapMemHeapID,NONE,NONE)

PACE_BEGIN(Err MemHeapCheck(UInt16 heapID))
PACE_ADD16(heapID)
PACE_END16(sysTrapMemHeapCheck,NONE,NONE)

PACE_BEGIN(UInt32 PrefGetPreference(SystemPreferencesChoice choice))
PACE_ADD8(choice)
PACE_END32(sysTrapPrefGetPreference,NONE,NONE)

PACE_BEGIN(Int16 PrefGetAppPreferences (UInt32 creator, UInt16 id, void *prefs, 
	UInt16 *prefsSize, Boolean saved))
PACE_ADD32(creator)
PACE_ADD16(id)
PACE_ADD32(prefs)
PACE_ADD32(prefsSize)
PACE_ADD8(saved)
PACE_END16(sysTrapPrefGetAppPreferences,PACE_DATA16(prefsSize),PACE_DATA16(prefsSize))

PACE_BEGIN(void PrefSetAppPreferences (UInt32 creator, UInt16 id, Int16 version, const void *prefs, 
	UInt16 prefsSize, Boolean saved))
PACE_ADD32(creator)
PACE_ADD16(id)
PACE_ADD16(version)
PACE_ADD32(prefs)
PACE_ADD16(prefsSize)
PACE_ADD8(saved)
PACE_END(sysTrapPrefSetAppPreferences,NONE,NONE)

PACE_BEGIN(UInt16 FrmCustomAlert(UInt16 alertId,const Char *s1,const Char *s2,const Char *s3))
PACE_ADD16(alertId)
PACE_ADD32(s1)
PACE_ADD32(s2)
PACE_ADD32(s3)
PACE_END16(sysTrapFrmCustomAlert,NONE,NONE)

PACE_BEGIN(void FrmGetObjectBounds (const FormType *formP, UInt16 objIndex,RectangleType *rP))
PACE_ADD32(formP)
PACE_ADD16(objIndex)
PACE_ADD32(rP)
PACE_END(sysTrapFrmGetObjectBounds,NONE,SwapStruct(rP,2,2,2,2,0))

PACE_BEGIN(void WinGetDisplayExtent (Coord *extentX, Coord *extentY))
PACE_ADD32(extentX)
PACE_ADD32(extentY)
PACE_END(sysTrapWinGetDisplayExtent,NONE,PACE_DATA16(extentX) PACE_DATA16(extentY))

PACE_BEGIN(void WinGetBounds(WinHandle winH, RectangleType *rP))
PACE_ADD32(winH)
PACE_ADD32(rP)
PACE_END(sysTrapWinGetBounds,NONE,SwapStruct(rP,2,2,2,2,0))

PACE_BEGINEX(void WinSetBounds (WinHandle winHandle, const RectangleType *rP),r)
PACE_ADD16(rP->topLeft.x)
PACE_ADD16(rP->topLeft.y)
PACE_ADD16(rP->extent.x)
PACE_ADD16(rP->extent.y)
PACE_VAR(PACE_PARAM)
PACE_ADD32(winHandle)
PACE_ADD32(r)
PACE_END(sysTrapWinSetBounds,NONE,NONE)

PACE_BEGINEX(void FrmSetObjectBounds (FormType *formP, UInt16 objIndex,const RectangleType *bounds),r)
PACE_ADD16(bounds->topLeft.x)
PACE_ADD16(bounds->topLeft.y)
PACE_ADD16(bounds->extent.x)
PACE_ADD16(bounds->extent.y)
PACE_VAR(PACE_PARAM)
PACE_ADD32(formP)
PACE_ADD16(objIndex)
PACE_ADD32(r)
PACE_END(sysTrapFrmSetObjectBounds,NONE,NONE)

PACE_BEGINEX(void WinDrawRectangle (const RectangleType *rP, UInt16 cornerDiam),r)
PACE_ADD16(rP->topLeft.x)
PACE_ADD16(rP->topLeft.y)
PACE_ADD16(rP->extent.x)
PACE_ADD16(rP->extent.y)
PACE_VAR(PACE_PARAM)
PACE_ADD32(r)
PACE_ADD16(cornerDiam)
PACE_END(sysTrapWinDrawRectangle,NONE,NONE)

PACE_BEGIN(void WinDrawBitmap (BitmapPtr bitmapP, Coord x, Coord y))
PACE_ADD32(bitmapP)
PACE_ADD16(x)
PACE_ADD16(y)
PACE_END(sysTrapWinDrawBitmap,NONE,NONE)

PACE_BEGINEX(void WinEraseRectangle(const RectangleType *rP, UInt16 cornerDiam),r)
PACE_ADD16(rP->topLeft.x)
PACE_ADD16(rP->topLeft.y)
PACE_ADD16(rP->extent.x)
PACE_ADD16(rP->extent.y)
PACE_VAR(PACE_PARAM)
PACE_ADD32(r)
PACE_ADD16(cornerDiam)
PACE_END(sysTrapWinEraseRectangle,NONE,NONE)

PACE_BEGIN(void WinDrawChars(const Char *chars, Int16 len, Coord x, Coord y))
PACE_ADD32(chars)
PACE_ADD16(len)
PACE_ADD16(x)
PACE_ADD16(y)
PACE_END(sysTrapWinDrawChars,NONE,NONE)

PACE_BEGIN(SysAppInfoPtr SysGetAppInfo(SysAppInfoPtr *uiAppPP, SysAppInfoPtr* actionCodeAppPP))
PACE_ADD32(uiAppPP)
PACE_ADD32(actionCodeAppPP)
PACE_ENDPTR(sysTrapSysGetAppInfo,NONE,NONE) 

PACE_BEGIN(MemPtr MemChunkNew(UInt16 heapID, UInt32 size, UInt16 attr))
PACE_ADD16(heapID)
PACE_ADD32(size)
PACE_ADD16(attr)
PACE_ENDPTR(sysTrapMemChunkNew,NONE,NONE)

PACE_BEGIN(Err VFSVolumeEnumerate(UInt16 *volRefNumP, UInt32 *volIteratorP))
PACE_ADD32(volRefNumP)
PACE_ADD32(volIteratorP)
PACE_END16(sysTrapVFSMgr,PACE_SEL(vfsTrapVolumeEnumerate) PACE_DATA32(volIteratorP),PACE_DATA16(volRefNumP) PACE_DATA32(volIteratorP))

PACE_BEGIN(Err VFSVolumeGetLabel(UInt16 volRefNum, Char *labelP, UInt16 bufLen))
PACE_ADD16(volRefNum)
PACE_ADD32(labelP)
PACE_ADD16(bufLen)
PACE_END16(sysTrapVFSMgr,PACE_SEL(vfsTrapVolumeGetLabel),NONE)

PACE_BEGIN(Err VFSVolumeInfo(UInt16 volRefNum, VolumeInfoType *volInfoP))
PACE_ADD16(volRefNum)
PACE_ADD32(volInfoP)
PACE_END16(sysTrapVFSMgr,PACE_SEL(vfsTrapVolumeInfo),SwapStruct(volInfoP,4,4,4,4,2,2,4,4,0))

PACE_BEGIN(Err FtrPtrFree(UInt32 creator, UInt16 featureNum))
PACE_ADD32(creator)
PACE_ADD16(featureNum)
PACE_END16(sysTrapFtrPtrFree,NONE,NONE)

PACE_BEGIN(Err FtrGet(UInt32 creator, UInt16 featureNum, UInt32 *valueP))
PACE_ADD32(creator)
PACE_ADD16(featureNum)
PACE_ADD32(valueP)
PACE_END16(sysTrapFtrGet,NONE,PACE_DATA32(valueP))

PACE_BEGIN(Err FtrSet(UInt32 creator, UInt16 featureNum, UInt32 newValue))
PACE_ADD32(creator)
PACE_ADD16(featureNum)
PACE_ADD32(newValue)
PACE_END16(sysTrapFtrSet,NONE,NONE)

PACE_BEGIN(Err FtrPtrNew(UInt32 creator, UInt16 featureNum, UInt32 size,void **newPtrP))
PACE_ADD32(creator)
PACE_ADD16(featureNum)
PACE_ADD32(size)
PACE_ADD32(newPtrP)
PACE_END16(sysTrapFtrPtrNew,NONE,PACE_DATA32((UInt32*)newPtrP))

PACE_BEGIN(Err DmWrite(void *recordP, UInt32 offset, const void *srcP, UInt32 bytes))
PACE_ADD32(recordP)
PACE_ADD32(offset)
PACE_ADD32(srcP)
PACE_ADD32(bytes)
PACE_END16(sysTrapDmWrite,NONE,NONE)

PACE_BEGIN(MemHandle DmGetResource(DmResType type, DmResID resID))
PACE_ADD32(type)
PACE_ADD16(resID)
PACE_ENDPTR(sysTrapDmGetResource,NONE,NONE)

PACE_BEGIN(Err DmReleaseResource(MemHandle resourceH))
PACE_ADD32(resourceH)
PACE_END16(sysTrapDmReleaseResource,NONE,NONE)

PACE_BEGIN(UInt32 MemHandleSize(MemHandle h))
PACE_ADD32(h)
PACE_END32(sysTrapMemHandleSize,NONE,NONE)

PACE_BEGIN(MemPtr MemHandleLock(MemHandle h))
PACE_ADD32(h)
PACE_ENDPTR(sysTrapMemHandleLock,NONE,NONE)

PACE_BEGIN(Err MemHandleUnlock(MemHandle h))
PACE_ADD32(h)
PACE_END16(sysTrapMemHandleUnlock,NONE,NONE)

PACE_BEGIN(Err MemPtrUnlock(MemPtr h))
PACE_ADD32(h)
PACE_END16(sysTrapMemPtrUnlock,NONE,NONE)

PACE_BEGIN(Err DmGetNextDatabaseByTypeCreator(Boolean newSearch, DmSearchStatePtr stateInfoP,
			 		UInt32	type, UInt32 creator, Boolean onlyLatestVers, 
			 		UInt16 *cardNoP, LocalID *dbIDP))
PACE_ADD8(newSearch)
PACE_ADD32(stateInfoP)
PACE_ADD32(type)
PACE_ADD32(creator)
PACE_ADD8(onlyLatestVers)
PACE_ADD32(cardNoP)
PACE_ADD32(dbIDP)
PACE_END16(sysTrapDmGetNextDatabaseByTypeCreator,PACE_DATA32(dbIDP),PACE_DATA16(cardNoP) PACE_DATA32(dbIDP))

PACE_BEGIN(Err DmDatabaseSize(UInt16 cardNo, LocalID dbID, UInt32 *numRecordsP,
					UInt32 *totalBytesP, UInt32 *dataBytesP))
PACE_ADD16(cardNo)
PACE_ADD32(dbID)
PACE_ADD32(numRecordsP)
PACE_ADD32(totalBytesP)
PACE_ADD32(dataBytesP)
PACE_END16(sysTrapDmDatabaseSize,NONE,
		   PACE_CDATA32(numRecordsP)
		   PACE_CDATA32(totalBytesP)
		   PACE_CDATA32(dataBytesP))

PACE_BEGIN(DmOpenRef DmOpenDatabaseByTypeCreator(UInt32 type, UInt32 creator, UInt16 mode))
PACE_ADD32(type)
PACE_ADD32(creator)
PACE_ADD16(mode)
PACE_ENDPTR(sysTrapDmOpenDatabaseByTypeCreator,NONE,NONE)

PACE_BEGIN(Err DmCloseDatabase(DmOpenRef dbP))
PACE_ADD32(dbP)
PACE_END16(sysTrapDmCloseDatabase,NONE,NONE)

PACE_BEGIN(Err DmDeleteDatabase(UInt16 cardNo, LocalID dbID))
PACE_ADD16(cardNo)
PACE_ADD32(dbID)
PACE_END16(sysTrapDmDeleteDatabase,NONE,NONE)

PACE_BEGIN(Err DmSetDatabaseInfo(UInt16 cardNo, LocalID	dbID, const Char *nameP,
					UInt16 *attributesP, UInt16 *versionP, UInt32 *crDateP,
					UInt32 *	modDateP, UInt32 *bckUpDateP,
					UInt32 *	modNumP, LocalID *appInfoIDP,
					LocalID *sortInfoIDP, UInt32 *typeP,
					UInt32 *creatorP))
PACE_ADD16(cardNo)
PACE_ADD32(dbID)
PACE_ADD32(nameP)
PACE_ADD32(attributesP)
PACE_ADD32(versionP)
PACE_ADD32(crDateP)
PACE_ADD32(modDateP)
PACE_ADD32(bckUpDateP)
PACE_ADD32(modNumP)
PACE_ADD32(appInfoIDP)
PACE_ADD32(sortInfoIDP)
PACE_ADD32(typeP)
PACE_ADD32(creatorP)
PACE_END16(sysTrapDmSetDatabaseInfo,
			PACE_CDATA16(attributesP) 
			PACE_CDATA16(versionP) 
			PACE_CDATA32(crDateP) 
			PACE_CDATA32(modDateP) 
			PACE_CDATA32(bckUpDateP) 
			PACE_CDATA32(modNumP) 
			PACE_CDATA32(appInfoIDP) 
			PACE_CDATA32(sortInfoIDP) 
			PACE_CDATA32(typeP) 
			PACE_CDATA32(creatorP),

			PACE_CDATA16(attributesP) 
			PACE_CDATA16(versionP) 
			PACE_CDATA32(crDateP) 
			PACE_CDATA32(modDateP) 
			PACE_CDATA32(bckUpDateP) 
			PACE_CDATA32(modNumP) 
			PACE_CDATA32(appInfoIDP) 
			PACE_CDATA32(sortInfoIDP) 
			PACE_CDATA32(typeP) 
			PACE_CDATA32(creatorP))

PACE_BEGIN(Err DmDatabaseInfo(UInt16 cardNo, LocalID	dbID, Char *nameP,
					UInt16 *attributesP, UInt16 *versionP, UInt32 *crDateP,
					UInt32 *modDateP, UInt32 *bckUpDateP,
					UInt32 *modNumP, LocalID *appInfoIDP,
					LocalID *sortInfoIDP, UInt32 *typeP,
					UInt32 *creatorP))
PACE_ADD16(cardNo)
PACE_ADD32(dbID)
PACE_ADD32(nameP)
PACE_ADD32(attributesP)
PACE_ADD32(versionP)
PACE_ADD32(crDateP)
PACE_ADD32(modDateP)
PACE_ADD32(bckUpDateP)
PACE_ADD32(modNumP)
PACE_ADD32(appInfoIDP)
PACE_ADD32(sortInfoIDP)
PACE_ADD32(typeP)
PACE_ADD32(creatorP)
PACE_END16(sysTrapDmDatabaseInfo,NONE,
			PACE_CDATA16(attributesP) 
			PACE_CDATA16(versionP) 
			PACE_CDATA32(crDateP) 
			PACE_CDATA32(modDateP) 
			PACE_CDATA32(bckUpDateP) 
			PACE_CDATA32(modNumP) 
			PACE_CDATA32(appInfoIDP) 
			PACE_CDATA32(sortInfoIDP) 
			PACE_CDATA32(typeP) 
			PACE_CDATA32(creatorP))

PACE_BEGIN(FileHand FileOpen(UInt16 cardNo, const Char * nameP, UInt32 type, UInt32 creator,
	UInt32 openMode, Err *errP))
PACE_ADD16(cardNo)
PACE_ADD32(nameP)
PACE_ADD32(type)
PACE_ADD32(creator)
PACE_ADD32(openMode)
PACE_ADD32(errP)
PACE_ENDPTR(sysTrapFileOpen,NONE,PACE_CDATA16(errP))

PACE_BEGIN(Err FileClose(FileHand stream))
PACE_ADD32(stream)
PACE_END16(sysTrapFileClose,NONE,NONE)

PACE_BEGIN(Int32 FileReadLow(FileHand stream, void *baseP, Int32 offset, Boolean dataStoreBased, Int32 objSize,
	Int32 numObj, Err *errP))
PACE_ADD32(stream)
PACE_ADD32(baseP)
PACE_ADD32(offset)
PACE_ADD8(dataStoreBased)
PACE_ADD32(objSize)
PACE_ADD32(numObj)
PACE_ADD32(errP)
PACE_END32(sysTrapFileReadLow,NONE,PACE_CDATA16(errP))

PACE_BEGIN(Err FileSeek(FileHand stream, Int32 offset, FileOriginEnum origin))
PACE_ADD32(stream)
PACE_ADD32(offset)
PACE_ADD8(origin)
PACE_END16(sysTrapFileSeek,NONE,NONE)

PACE_BEGIN(Int32 FileTell(FileHand stream, Int32 *fileSizeP, Err *errP))
PACE_ADD32(stream)
PACE_ADD32(fileSizeP)
PACE_ADD32(errP)
PACE_END32(sysTrapFileTell,NONE,PACE_CDATA32(fileSizeP) PACE_CDATA16(errP))

PACE_BEGIN(void WinGetClip(RectangleType *rP))
PACE_ADD32(rP)
PACE_END(sysTrapWinGetClip,NONE,SwapStruct(rP,2,2,2,2,0))

PACE_BEGINEX(void WinSetClip (const RectangleType *rP),r)
PACE_ADD16(rP->topLeft.x)
PACE_ADD16(rP->topLeft.y)
PACE_ADD16(rP->extent.x)
PACE_ADD16(rP->extent.y)
PACE_VAR(PACE_PARAM)
PACE_ADD32(r)
PACE_END(sysTrapWinSetClip,NONE,NONE)

PACE_BEGIN(void WinPushDrawState())
PACE_ENDVOID(sysTrapWinPushDrawState,NONE,NONE)

PACE_BEGIN(void WinPopDrawState())
PACE_ENDVOID(sysTrapWinPopDrawState,NONE,NONE)

PACE_BEGIN(void	SclSetScrollBar (ScrollBarType *bar, Int16 value, Int16 min, Int16 max, Int16 pageSize))
PACE_ADD32(bar)
PACE_ADD16(value)
PACE_ADD16(min)
PACE_ADD16(max)
PACE_ADD16(pageSize)
PACE_END(sysTrapSclSetScrollBar,NONE,NONE)

PACE_BEGIN(UInt16 WinSetCoordinateSystem(UInt16 coordSys))
PACE_ADD16(coordSys)
PACE_END16(sysTrapHighDensityDispatch,PACE_SEL(HDSelectorWinSetCoordinateSystem),NONE)

PACE_BEGIN(Coord WinScaleCoord(Coord coord, Boolean ceiling))
PACE_ADD16(coord)
PACE_ADD8(ceiling)
PACE_END16(sysTrapHighDensityDispatch,PACE_SEL(HDSelectorWinScaleCoord),NONE)

PACE_BEGIN(UInt16 WinGetCoordinateSystem())
PACE_END16VOID(sysTrapHighDensityDispatch,PACE_SEL(HDSelectorWinGetCoordinateSystem),NONE)

PACE_BEGIN(Err WinScreenGetAttribute(WinScreenAttrType selector, UInt32* attrP))
PACE_ADD8(selector)
PACE_ADD32(attrP)
PACE_END16(sysTrapHighDensityDispatch,PACE_SEL(HDSelectorWinScreenGetAttribute),PACE_DATA32(attrP))

PACE_BEGIN(Err WinScreenMode(WinScreenModeOperation operation, 
						UInt32 *widthP,
						UInt32 *heightP, 
						UInt32 *depthP, 
						Boolean *enableColorP))
PACE_ADD8(operation)
PACE_ADD32(widthP)
PACE_ADD32(heightP)
PACE_ADD32(depthP)
PACE_ADD32(enableColorP)
PACE_END16(sysTrapWinScreenMode,PACE_CDATA32(widthP) PACE_CDATA32(heightP) PACE_CDATA32(depthP),
								PACE_CDATA32(widthP) PACE_CDATA32(heightP) PACE_CDATA32(depthP))

PACE_BEGIN(Err WinPalette(UInt8 operation, Int16 startIndex, 
			 	  			 UInt16 paletteEntries, RGBColorType *tableP))
PACE_ADD8(operation)
PACE_ADD16(startIndex)
PACE_ADD16(paletteEntries)
PACE_ADD32(tableP)
PACE_END16(sysTrapWinPalette,NONE,NONE)

PACE_BEGIN(WinHandle WinGetDisplayWindow())
PACE_ENDPTRVOID(sysTrapWinGetDisplayWindow,NONE,NONE)

PACE_BEGIN(WinHandle WinGetActiveWindow())
PACE_ENDPTRVOID(sysTrapWinGetActiveWindow,NONE,NONE)

PACE_BEGIN(BitmapType *WinGetBitmap(WinHandle winHandle))
PACE_ADD32(winHandle)
PACE_ENDPTR(sysTrapWinGetBitmap,NONE,NONE)

PACE_BEGIN(void BmpGetDimensions(const BitmapType * bitmapP,Coord *widthP,Coord *heightP,UInt16 *rowBytesP))
PACE_ADD32(bitmapP)
PACE_ADD32(widthP)
PACE_ADD32(heightP)
PACE_ADD32(rowBytesP)
PACE_END(sysTrapBmpGetDimensions,NONE,PACE_CDATA16(widthP) PACE_CDATA16(heightP) PACE_CDATA16(rowBytesP))

PACE_BEGIN(void* BmpGetBits(BitmapType * bitmapP))
PACE_ADD32(bitmapP)
PACE_ENDPTR(sysTrapBmpGetBits,NONE,NONE)

PACE_BEGIN(void WinSetBackColorRGB(const RGBColorType* newRgbP, RGBColorType* prevRgbP))
PACE_ADD32(newRgbP)
PACE_ADD32(prevRgbP)
PACE_END(sysTrapWinSetBackColorRGB,NONE,NONE)

PACE_BEGIN(Err VFSRegisterDefaultDirectory(const Char *fileTypeStr, UInt32 mediaType, 
			const Char *pathStr))
PACE_ADD32(fileTypeStr)
PACE_ADD32(mediaType)
PACE_ADD32(pathStr)
PACE_END16(sysTrapVFSMgr,PACE_SEL(vfsTrapRegisterDefaultDirectory),NONE)

PACE_BEGIN(Err VFSGetDefaultDirectory(UInt16 volRefNum, const Char *fileTypeStr,Char *pathStr, UInt16 *bufLenP))
PACE_ADD16(volRefNum)
PACE_ADD32(fileTypeStr)
PACE_ADD32(pathStr)
PACE_ADD32(bufLenP)
PACE_END16(sysTrapVFSMgr,PACE_SEL(vfsTrapGetDefaultDirectory) PACE_DATA16(bufLenP),PACE_DATA16(bufLenP));

PACE_BEGIN(Err VFSImportDatabaseFromFile(UInt16 volRefNum, const Char *pathNameP, UInt16 *cardNoP, LocalID *dbIDP))
PACE_ADD16(volRefNum)
PACE_ADD32(pathNameP)
PACE_ADD32(cardNoP)
PACE_ADD32(dbIDP)
PACE_END16(sysTrapVFSMgr,PACE_SEL(vfsTrapImportDatabaseFromFile),PACE_DATA16(cardNoP) PACE_DATA32(dbIDP))

PACE_BEGIN(Err VFSFileOpen(UInt16 volRefNum, const Char *pathNameP, UInt16 openMode, FileRef *fileRefP))
PACE_ADD16(volRefNum)
PACE_ADD32(pathNameP)
PACE_ADD16(openMode)
PACE_ADD32(fileRefP)
PACE_END16(sysTrapVFSMgr,PACE_SEL(vfsTrapFileOpen),PACE_DATA32(fileRefP))

PACE_BEGIN(Err VFSFileClose(FileRef fileRef))
PACE_ADD32(fileRef)
PACE_END16(sysTrapVFSMgr,PACE_SEL(vfsTrapFileClose),NONE)

PACE_BEGIN(Err VFSFileSize(FileRef fileRef,UInt32 *fileSizeP))
PACE_ADD32(fileRef)
PACE_ADD32(fileSizeP)
PACE_END16(sysTrapVFSMgr,PACE_SEL(vfsTrapFileSize),PACE_DATA32(fileSizeP))

PACE_BEGIN(Err VFSFileGetDate(FileRef fileRef, UInt16 whichDate, UInt32 *dateP))
PACE_ADD32(fileRef)
PACE_ADD16(whichDate)
PACE_ADD32(dateP)
PACE_END16(sysTrapVFSMgr,PACE_SEL(vfsTrapFileGetDate),PACE_DATA32(dateP))

PACE_BEGIN(Err VFSDirEntryEnumerate(FileRef dirRef,UInt32 *dirEntryIteratorP, FileInfoType *infoP))
PACE_ADD32(dirRef)
PACE_ADD32(dirEntryIteratorP)
PACE_ADD32(infoP)
PACE_END16(sysTrapVFSMgr,PACE_SEL(vfsTrapDirEntryEnumerate) 
                         PACE_DATA32(dirEntryIteratorP)
						 SwapStruct(infoP,4,4,2,0),
                         PACE_DATA32(dirEntryIteratorP)
						 SwapStruct(infoP,4,4,2,0))

PACE_BEGIN(Err VFSFileGetAttributes(FileRef fileRef, UInt32 *attributesP))
PACE_ADD32(fileRef)
PACE_ADD32(attributesP)
PACE_END16(sysTrapVFSMgr,PACE_SEL(vfsTrapFileGetAttributes),PACE_DATA32(attributesP))

PACE_BEGIN(Err VFSFileReadData(FileRef fileRef, UInt32 numBytes, void *bufBaseP, UInt32 offset, UInt32 *numBytesReadP))
PACE_ADD32(fileRef)
PACE_ADD32(numBytes)
PACE_ADD32(bufBaseP)
PACE_ADD32(offset)
PACE_ADD32(numBytesReadP)
PACE_END16(sysTrapVFSMgr,PACE_SEL(vfsTrapFileReadData),PACE_CDATA32(numBytesReadP))

PACE_BEGIN(Err VFSFileRead(FileRef fileRef, UInt32 numBytes, void *bufP, UInt32 *numBytesReadP))
PACE_ADD32(fileRef)
PACE_ADD32(numBytes)
PACE_ADD32(bufP)
PACE_ADD32(numBytesReadP)
PACE_END16(sysTrapVFSMgr,PACE_SEL(vfsTrapFileRead),PACE_CDATA32(numBytesReadP))

PACE_BEGIN(Err VFSFileWrite(FileRef fileRef, UInt32 numBytes, const void *dataP, UInt32 *numBytesWrittenP))
PACE_ADD32(fileRef)
PACE_ADD32(numBytes)
PACE_ADD32(dataP)
PACE_ADD32(numBytesWrittenP)
PACE_END16(sysTrapVFSMgr,PACE_SEL(vfsTrapFileWrite),PACE_DATA32(numBytesWrittenP))

PACE_BEGIN(Err VFSFileSeek(FileRef fileRef, FileOrigin origin, Int32 offset))
PACE_ADD32(fileRef)
PACE_ADD16(origin)
PACE_ADD32(offset)
PACE_END16(sysTrapVFSMgr,PACE_SEL(vfsTrapFileSeek),NONE)

PACE_BEGIN(Err VFSFileTell(FileRef fileRef,UInt32 *filePosP))
PACE_ADD32(fileRef)
PACE_ADD32(filePosP)
PACE_END16(sysTrapVFSMgr,PACE_SEL(vfsTrapFileTell),PACE_DATA32(filePosP))

PACE_BEGIN(UInt32 PceNativeCall(NativeFuncType *nativeFuncP, void *userDataP))
PACE_ADD32(nativeFuncP)
PACE_ADD32(userDataP)
PACE_END32(sysTrapPceNativeCall,NONE,NONE)

PACE_BEGIN(UInt8 *WinScreenLock(WinLockInitType initMode))
PACE_ADD8(initMode)
PACE_ENDPTR(sysTrapWinScreenLock,NONE,NONE)

PACE_BEGIN(void WinScreenUnlock())
PACE_ENDVOID(sysTrapWinScreenUnlock,NONE,NONE)

PACE_BEGIN(void LstSetHeight (ListType *listP, Int16 visibleItems))
PACE_ADD32(listP)
PACE_ADD16(visibleItems)
PACE_END(sysTrapLstSetHeight,NONE,NONE);

PACE_BEGIN(void LstSetListChoices (ListType *listP, Char **itemsText, Int16 numItems))
PACE_ADD32(listP)
PACE_ADD32(itemsText)
PACE_ADD16(numItems)
PACE_END(sysTrapLstSetListChoices,SwapBlock32(itemsText,numItems),NONE)

PACE_BEGIN(void LstSetSelection(ListType *listP, Int16 itemNum))
PACE_ADD32(listP)
PACE_ADD16(itemNum)
PACE_END(sysTrapLstSetSelection,NONE,NONE)

PACE_BEGIN(Char* LstGetSelectionText(const ListType *listP, Int16 itemNum))
PACE_ADD32(listP)
PACE_ADD16(itemNum)
PACE_ENDPTR(sysTrapLstGetSelectionText,NONE,NONE)

PACE_BEGIN(MemHandle MemHandleNew(UInt32 size))
PACE_ADD32(size)
PACE_ENDPTR(sysTrapMemHandleNew,NONE,NONE)
							
PACE_BEGIN(Err MemHandleFree(MemHandle h))
PACE_ADD32(h)
PACE_END16(sysTrapMemHandleFree,NONE,NONE)

PACE_BEGIN(Char* FldGetTextPtr(const FieldType *fldP))
PACE_ADD32(fldP)
PACE_ENDPTR(sysTrapFldGetTextPtr,NONE,NONE)

PACE_BEGIN(void FldSetTextHandle(FieldType *fldP, MemHandle textHandle))
PACE_ADD32(fldP)
PACE_ADD32(textHandle)
PACE_END(sysTrapFldSetTextHandle,NONE,NONE)

PACE_BEGIN(MemHandle FldGetTextHandle(const FieldType *fldP))
PACE_ADD32(fldP)
PACE_ENDPTR(sysTrapFldGetTextHandle,NONE,NONE)

PACE_BEGIN(void SndPlaySystemSound(SndSysBeepType beepID))
PACE_ADD8(beepID)
PACE_END(sysTrapSndPlaySystemSound,NONE,NONE)

PACE_BEGIN(Err SndStreamCreate(SndStreamRef *channel,SndStreamMode mode,UInt32 samplerate,
						SndSampleType type,SndStreamWidth width,SndStreamBufferCallback func,
						void *userdata,UInt32 buffsize,Boolean armNative))
PACE_ADD32(channel)
PACE_ADD8(mode)
PACE_ADD32(samplerate)
PACE_ADD16(type)
PACE_ADD8(width)
PACE_ADD32(func)
PACE_ADD32(userdata)
PACE_ADD32(buffsize)
PACE_ADD8(armNative)
PACE_END16(sysTrapSndStreamCreate,NONE,PACE_DATA32(channel))

PACE_BEGIN(Err SndStreamDelete(SndStreamRef channel))
PACE_ADD32(channel)
PACE_END16(sysTrapSndStreamDelete,NONE,NONE)

PACE_BEGIN(Err SndStreamStart(SndStreamRef channel))
PACE_ADD32(channel)
PACE_END16(sysTrapSndStreamStart,NONE,NONE)

PACE_BEGIN(Err SndStreamPause(SndStreamRef channel, Boolean pause))
PACE_ADD32(channel)
PACE_ADD8(pause)
PACE_END16(sysTrapSndStreamPause,NONE,NONE)

PACE_BEGIN(Err SndStreamStop(SndStreamRef channel))
PACE_ADD32(channel)
PACE_END16(sysTrapSndStreamStop,NONE,NONE)

PACE_BEGIN(Err SndStreamSetPan(SndStreamRef channel, Int32 panposition))
PACE_ADD32(channel)
PACE_ADD32(panposition)
PACE_END16(sysTrapSndStreamSetPan,NONE,NONE)

PACE_BEGIN(Err SndStreamSetVolume(SndStreamRef channel, Int32 volume))
PACE_ADD32(channel)
PACE_ADD32(volume)
PACE_END16(sysTrapSndStreamSetVolume,NONE,NONE)

PACE_BEGIN(Err SndStreamGetVolume(SndStreamRef channel, Int32 *volume))
PACE_ADD32(channel)
PACE_ADD32(volume)
PACE_END16(sysTrapSndStreamGetVolume,NONE,PACE_DATA32(volume))

PACE_BEGINEVENT(Boolean SysHandleEvent(EventPtr eventP))
PACE_ADD32(&ev)
PACE_END8(sysTrapSysHandleEvent,Event_ARM_To_M68K(eventP,ev),NONE)

PACE_BEGIN(void CtlDrawControl (ControlType *controlP))
PACE_ADD32(controlP)
PACE_END(sysTrapCtlDrawControl,NONE,NONE)

PACE_BEGIN(void FrmCopyLabel (FormType *formP, UInt16 labelID,const Char *newLabel))
PACE_ADD32(formP)
PACE_ADD16(labelID)
PACE_ADD32(newLabel)
PACE_END(sysTrapFrmCopyLabel,NONE,NONE)

PACE_BEGIN(void FldDrawField (FieldType *fldP))
PACE_ADD32(fldP)
PACE_END(sysTrapFldDrawField,NONE,NONE)

PACE_BEGIN(void FldSetTextPtr (FieldType *fldP, Char *textP))
PACE_ADD32(fldP)
PACE_ADD32(textP)
PACE_END(sysTrapFldSetTextPtr,NONE,NONE)

PACE_BEGIN(FormType *FrmGetFormPtr(UInt16 formId))
PACE_ADD16(formId)
PACE_ENDPTR(sysTrapFrmGetFormPtr,NONE,NONE)

PACE_BEGIN(FormType* FrmGetActiveForm())
PACE_ENDPTRVOID(sysTrapFrmGetActiveForm,NONE,NONE)

PACE_BEGINEVENT(Boolean FrmHandleEvent(FormType *formP, EventType *eventP))
PACE_ADD32(formP)
PACE_ADD32(&ev)
PACE_END8(sysTrapFrmHandleEvent,Event_ARM_To_M68K(eventP,ev),NONE)

PACE_BEGIN(void FrmHideObject(FormType *formP, UInt16 objIndex))
PACE_ADD32(formP)
PACE_ADD16(objIndex)
PACE_END(sysTrapFrmHideObject,NONE,NONE)

PACE_BEGIN(void FrmShowObject (FormType *formP, UInt16 objIndex))
PACE_ADD32(formP)
PACE_ADD16(objIndex)
PACE_END(sysTrapFrmShowObject,NONE,NONE)

PACE_BEGINEVENT(static Boolean _FrmDispatchEvent(EventType *eventP))
PACE_ADD32(&ev)
PACE_END8(sysTrapFrmDispatchEvent,Event_ARM_To_M68K(eventP,ev),NONE)

PACE_BEGIN(static void _FrmSetGadgetHandler(FormType *formP, UInt16 objIndex,FormGadgetHandlerType *attrP))
PACE_ADD32(formP)
PACE_ADD16(objIndex)
PACE_ADD32(attrP)
PACE_END(sysTrapFrmSetGadgetHandler,NONE,NONE)

PACE_BEGIN(static void _FrmSetEventHandler(FormType *formP,FormEventHandlerType *handler))
PACE_ADD32(formP)
PACE_ADD32(handler)
PACE_END(sysTrapFrmSetEventHandler,NONE,NONE)

PACE_BEGIN(static void _FrmDeleteForm(FormType *formP))
PACE_ADD32(formP)
PACE_END(sysTrapFrmDeleteForm,NONE,NONE)

PACE_BEGIN(static void _FrmReturnToForm(UInt16 formId))
PACE_ADD16(formId)
PACE_END(sysTrapFrmReturnToForm,NONE,NONE)

PACE_BEGIN(Err EvtEnqueueKey(WChar ascii, UInt16 keycode, UInt16 modifiers))
PACE_ADD16(ascii)
PACE_ADD16(keycode)
PACE_ADD16(modifiers)
PACE_END16(sysTrapEvtEnqueueKey,NONE,NONE)

PACE_BEGIN(FontID FntSetFont(FontID font))
PACE_ADD8(font)
PACE_END8(sysTrapFntSetFont,NONE,NONE)

PACE_BEGIN(UInt32 KeyCurrentState())
PACE_END32VOID(sysTrapKeyCurrentState,NONE,NONE)

PACE_BEGIN(UInt32 TimGetSeconds(void))
PACE_END32VOID(sysTrapTimGetSeconds,NONE,NONE)

PACE_BEGIN(void TimSecondsToDateTime(UInt32 seconds, DateTimeType *dateTimeP))
PACE_ADD32(seconds)
PACE_ADD32(dateTimeP)
PACE_END(sysTrapTimSecondsToDateTime,NONE,SwapStruct(dateTimeP,2,2,2,2,2,2,2,0))

/*PACE_BEGIN(Err KeyRates(Boolean set, UInt16 *initDelayP, UInt16 *periodP, 
						UInt16 *doubleTapDelayP, Boolean *queueAheadP))
PACE_ADD8(set)
PACE_ADD32(initDelayP)
PACE_ADD32(periodP)
PACE_ADD32(doubleTapDelayP)
PACE_ADD32(queueAheadP)
PACE_END16(sysTrapKeyRates,PACE_DATA16(initDelayP) PACE_DATA16(periodP) PACE_DATA16(doubleTapDelayP),
						   PACE_DATA16(initDelayP) PACE_DATA16(periodP) PACE_DATA16(doubleTapDelayP))
*/

PACE_BEGIN(Err SysLibOpen(UInt16 refNum))
PACE_ADD16(refNum)
PACE_END16(sysLibTrapOpen,NONE,NONE)

PACE_BEGIN(Err SysLibClose(UInt16 refNum))
PACE_ADD16(refNum)
PACE_END16(sysLibTrapClose,NONE,NONE)

PACE_BEGIN(UInt32 RotationMgrGetLibAPIVersion(UInt16 refNum))
PACE_ADD16(refNum)
PACE_END32((sysLibTrapCustom+0),NONE,NONE)

PACE_BEGIN(Err RotationMgrAttributeGet(UInt16 refNum, UInt32 attribute, UInt32 *valueP))
PACE_ADD16(refNum)
PACE_ADD32(attribute)
PACE_ADD32(valueP)
PACE_END16((sysLibTrapCustom+5),NONE,PACE_DATA32(valueP));

PACE_BEGIN(UInt16 SysBatteryInfo(Boolean set, UInt16 *warnThresholdP, UInt16 *criticalThresholdP,
						Int16 *maxTicksP, SysBatteryKind* kindP, Boolean *pluggedIn, UInt8 *percentP))
PACE_ADD8(set)
PACE_ADD32(warnThresholdP)
PACE_ADD32(criticalThresholdP)
PACE_ADD32(maxTicksP)
PACE_ADD32(kindP)
PACE_ADD32(pluggedIn)
PACE_ADD32(percentP)
PACE_END16(sysTrapSysBatteryInfo,NONE,PACE_CDATA16(warnThresholdP) PACE_CDATA16(criticalThresholdP) PACE_CDATA16(maxTicksP))

#ifdef HAVE_PALMONE_SDK

PACE_BEGIN(Err HsNavGetFocusRingInfo(const FormType* formP, UInt16* objectIDP, 
					   Int16* extraInfoP, RectangleType* boundsInsideRingP,
					   HsNavFocusRingStyleEnum* ringStyleP))
PACE_ADD16(hsSelNavGetFocusRingInfo)
PACE_ADD32(formP)
PACE_ADD32(objectIDP)
PACE_ADD32(extraInfoP)
PACE_ADD32(boundsInsideRingP)
PACE_ADD32(ringStyleP)
PACE_END16(sysTrapOEMDispatch, NONE, PACE_CDATA16(objectIDP) PACE_CDATA16(extraInfoP) PACE_CDATA16(ringStyleP) SwapStruct(boundsInsideRingP,2,2,2,2,0))

PACE_BEGINEX(Err HsNavDrawFocusRing(FormType* formP, UInt16 objectID, Int16 extraInfo,
				    RectangleType* boundsInsideRingP,
					HsNavFocusRingStyleEnum ringStyle, Boolean forceRestore),r)
PACE_ADD16(boundsInsideRingP->topLeft.x)
PACE_ADD16(boundsInsideRingP->topLeft.y)
PACE_ADD16(boundsInsideRingP->extent.x)
PACE_ADD16(boundsInsideRingP->extent.y)
PACE_VAR(PACE_PARAM)
PACE_ADD16(hsSelNavDrawFocusRing)
PACE_ADD16(objectID)
PACE_ADD16(extraInfo)
PACE_ADD32(r)
PACE_ADD16(ringStyle)
PACE_ADD8(forceRestore)
PACE_END16(sysTrapOEMDispatch,NONE,NONE)

PACE_BEGIN(Err HsNavRemoveFocusRing (FormType* formP))
PACE_ADD16(hsSelNavRemoveFocusRing)
PACE_ADD32(formP)
PACE_END16(sysTrapOEMDispatch,NONE,NONE)

PACE_BEGIN(void HsNavObjectTakeFocus (const FormType* formP, UInt16 objID))
PACE_ADD16(hsSelNavObjectTakeFocus)
PACE_ADD32(formP)
PACE_ADD16(objID)
PACE_END(sysTrapOEMDispatch,NONE,NONE)

PACE_BEGIN(Err HsLightCircumstance(Boolean add, UInt16 circumstance))
PACE_ADD16(hsSelLightCircumstance)
PACE_ADD8(add)
PACE_ADD16(circumstance)
PACE_END16(sysTrapOEMDispatch, NONE, NONE)

PACE_BEGIN(UInt16 HsCurrentLightCircumstance())
PACE_ADD16(hsGetCurrentLightCircumstance)
PACE_END16(sysTrapOEMDispatch, NONE, NONE)

PACE_BEGIN(Err HsAttrGet(UInt16 attr, UInt32 param, UInt32* valueP))
PACE_ADD16(hsSelAttrGet)
PACE_ADD16(attr)
PACE_ADD32(param)
PACE_ADD32(valueP)
PACE_END16(sysTrapOEMDispatch, NONE, PACE_DATA32(valueP))

PACE_BEGIN(Err HsAttrSet(UInt16 attr, UInt32 param, UInt32* valueP))
PACE_ADD16(hsSelAttrSet)
PACE_ADD16(attr)
PACE_ADD32(param)
PACE_ADD32(valueP)
PACE_END16(sysTrapOEMDispatch, PACE_DATA32(valueP), PACE_DATA32(valueP))

PACE_BEGIN(Err HWUBlinkLED(UInt16 refnum, Boolean b))
PACE_ADD16(refnum)
PACE_ADD8(b)
PACE_END16((sysLibTrapCustom+0),NONE,NONE)

PACE_BEGIN(Err HWUSetBlinkRate(UInt16 refnum, UInt16 rate))
PACE_ADD16(refnum)
PACE_ADD16(rate)
PACE_END16((sysLibTrapCustom+1),NONE,NONE)

PACE_BEGIN(Err HWUEnableDisplay(UInt16 refnum, Boolean on))
PACE_ADD16(refnum)
PACE_ADD8(on)
PACE_END16((sysLibTrapCustom+2),NONE,NONE)

PACE_BEGIN(Boolean HWUGetDisplayState(UInt16 refnum))
PACE_ADD16(refnum)
PACE_END8((sysLibTrapCustom+3),NONE,NONE)

PACE_BEGIN(void *DexGetDisplayAddress(UInt16 refnum))
PACE_ADD16(refnum)
PACE_ENDPTR((sysLibTrapCustom+4),NONE,NONE)

PACE_BEGIN(void DexGetDisplayDimensions(UInt16 refnum, Coord *width, Coord *height, UInt16 *rowBytes))
PACE_ADD16(refnum)
PACE_ADD32(width)
PACE_ADD32(height)
PACE_ADD32(rowBytes)
PACE_END((sysLibTrapCustom+5),NONE,PACE_CDATA16(width) PACE_CDATA16(height) PACE_CDATA16(rowBytes))

#endif

#ifdef HAVE_SONY_SDK

PACE_BEGIN(UInt32 VskGetAPIVersion(UInt16 refNum))
PACE_ADD16(refNum)
PACE_END32((sysLibTrapCustom+3),NONE,NONE)

PACE_BEGIN(Err VskSetState(UInt16 refNum, UInt16 stateType, UInt16 state))
PACE_ADD16(refNum)
PACE_ADD16(stateType)
PACE_ADD16(state)
PACE_END16((sysLibTrapCustom+6),NONE,NONE)

PACE_BEGIN(Err VskGetState(UInt16 refNum, UInt16 stateType, UInt16 *stateP))
PACE_ADD16(refNum)
PACE_ADD16(stateType)
PACE_ADD32(stateP)
PACE_END16((sysLibTrapCustom+7),NONE,PACE_DATA16(stateP))

#endif

PACE_BEGIN(Err SysLibLoad(UInt32 libType, UInt32 libCreator, UInt16 *refNumP))
PACE_ADD32(libType)
PACE_ADD32(libCreator)
PACE_ADD32(refNumP)
PACE_END16(sysTrapSysLibLoad,NONE,PACE_DATA16(refNumP))

PACE_BEGIN(Err SysLibFind(const Char *nameP, UInt16 *refNumP))
PACE_ADD32(nameP)
PACE_ADD32(refNumP)
PACE_END16(sysTrapSysLibFind,NONE,PACE_DATA16(refNumP))

NOINLINE MemPtr MemGluePtrNew(UInt32 Size)
{
	MemPtr p = MemPtrNew(Size);
	if (!p)
	{ 
		SysAppInfoPtr appInfoP;
		UInt16 OwnerID = SysGetAppInfo(&appInfoP,&appInfoP)->memOwnerID;
		p = MemChunkNew(0, Size,(UInt16)(OwnerID | memNewChunkFlagNonMovable | memNewChunkFlagAllowLarge));
	}
	return p;
}

NOINLINE FormEventHandlerType* FrmGetEventHandler(FormType *Form)
{
	eventhandler *i,*j;
	for (i=EventHandler,j=NULL;i;j=i,i=i->Next)
		if (i->Form == Form)
		{
			if (j)
			{
				// move ahead for faster search next time
				j->Next = i->Next;
				i->Next = EventHandler;
				EventHandler = i;
			}
			return i->Handler;
		}

	return NULL;
}

Boolean FrmDispatchEvent(EventType *eventP)
{
	// avoid using m68k wrapper for some common events (which are sent to the active form)

	if (eventP->eType == keyHoldEvent || eventP->eType == keyDownEvent || 
		eventP->eType == keyUpEvent || eventP->eType == nilEvent ||
		eventP->eType == penDownEvent || eventP->eType == penMoveEvent ||
		eventP->eType == penUpEvent)
	{
		FormType *Form = FrmGetActiveForm();
		FormEventHandlerType* Handler = FrmGetEventHandler(Form);
		if (Handler)
		{
			// call ARM event handler
			if (Handler(eventP))
				return 1;
			// call system event handler
			return FrmHandleEvent(Form,eventP);
		}
	}

	return _FrmDispatchEvent(eventP);
}

static void SetCodePtr(uint8_t* p,void* Ptr)
{
	p[0] = (uint8_t)(((uint32_t)Ptr) >> 24);
	p[1] = (uint8_t)(((uint32_t)Ptr) >> 16);
	p[2] = (uint8_t)(((uint32_t)Ptr) >> 8);
	p[3] = (uint8_t)(((uint32_t)Ptr) >> 0);
}

static void RemoveHandler(FormType *Form)
{
	eventhandler* i;
	for (i=EventHandler;i;i=i->Next)
		if (i->Form == Form)
			i->Form = NULL;
}

void FrmDeleteForm( FormType *formP )
{
	_FrmDeleteForm(formP);
	RemoveHandler(formP);
}

void FrmReturnToForm( UInt16 formId )
{
	FormType* Form = FrmGetActiveForm();
	_FrmReturnToForm(formId);
	RemoveHandler(Form);
}

void* m68kCallBack(m68kcallback p,NativeFuncType* Func)
{
	static const uint8_t Code[24] = 
	{ 0x4E,0x56,0x00,0x00,						// link a6,#0
	  0x48,0x6E,0x00,0x08,					    // pea     8(a6)
	  0x2F,0x3C,0x00,0x00,0x00,0x00,			// move.l  #$ARM,-(sp)
	  0x4E,0x4F,								// trap    #$F
	  0xA4,0x5A,								// dc.w    $A45A
	  0x70,0x00,								// moveq   #0,d0
	  0x4E,0x5E,								// unlk    a6
	  0x4E,0x75 };								// rts

	memcpy(p,Code,sizeof(m68kcallback));
	SetCodePtr((uint8_t*)p+10,(void*)(uint32_t)Func);
	return p;
}

void FrmSetGadgetHandler(FormType *formP, UInt16 objIndex,FormGadgetHandlerType *attrP)
{
	gadgethandler* i;
	gadgethandler* Free = NULL;
	for (i=GadgetHandler;i;i=i->Next)
	{
		if (i->Form == formP && i->Index == objIndex)
			break;
		if (!i->Form)
			Free = i;
	}
	if (!i)
	{
		if (Free)
			i = Free;
		else
		{
			i = MemGluePtrNew(sizeof(gadgethandler));
			i->Next = GadgetHandler;
			GadgetHandler = i;
		}
	}

	if (i)
	{
		static const uint8_t Code[68] = 
		{ 0x4E,0x56,0x00,0x00,						// link a6,#0
		  0x30,0x2E,0x00,0x0C,						// move.w  $C(a6),d0
		  0x22,0x2E,0x00,0x0E,						// move.l  $E(a6),d1
		  0x23,0xEE,0x00,0x08,0x00,0x00,0x00,0x00,  // move.l  8(a6),($Gadget).l
		  0x33,0xC0,0x00,0x00,0x00,0x00,			// move.w  d0,($Cmd).l
		  0x23,0xC1,0x00,0x00,0x00,0x00,			// move.l  d1,($Event).l
		  0x2F,0x3C,0x00,0x00,0x00,0x00,			// move.l  #This,-(sp)
		  0x2F,0x39,0x00,0x00,0x00,0x00,			// move.l  (Wrapper).l,-(sp)
		  0x2F,0x39,0x00,0x00,0x00,0x00,			// move.l  (Module).l,-(sp)
		  0x20,0x79,0x00,0x00,0x00,0x00,			// movea.l (PealCall).l,a0
		  0x4E,0x90,								// jsr     (a0)
		  0x02,0x80,0x00,0x00,0x00,0xFF,			// andi.l  #$FF,d0
		  0x4E,0x5E,								// unlk    a6
		  0x4E,0x75 };								// rts

#if defined(_M_IX86)
		const tchar_t* Name = T("tcpmp.dll\0WrapperGadgetHandler86");
		i->Wrapper = SWAP32(Name);
#else
		i->Wrapper = SWAP32(WrapperGadgetHandler);
#endif
		i->Form = formP;
		i->Index = objIndex;
		i->Handler = attrP;
		i->Module = Peal.Module;
		i->PealCall = Peal.PealCall;

		memcpy(i->Code,Code,sizeof(Code));
		SetCodePtr(i->Code+16,&i->Gadget);
		SetCodePtr(i->Code+22,&i->Cmd);
		SetCodePtr(i->Code+28,&i->Event);
		SetCodePtr(i->Code+34,i);
		SetCodePtr(i->Code+40,&i->Wrapper);
		SetCodePtr(i->Code+46,&i->Module);
		SetCodePtr(i->Code+52,&i->PealCall);

		_FrmSetGadgetHandler(formP,objIndex,(FormGadgetHandlerType*)i->Code);
	}
}

void FrmSetEventHandler(FormType *formP,FormEventHandlerType *handler)
{
	eventhandler* i;
	eventhandler* Free = NULL;
	for (i=EventHandler;i;i=i->Next)
	{
		if (i->Form == formP)
			break;
		if (!i->Form)
			Free = i;
	}
	if (!i)
	{
		if (Free)
			i = Free;
		else
		{
			i = MemGluePtrNew(sizeof(eventhandler));
			i->Next = EventHandler;
			EventHandler = i;
		}
	}

	if (i)
	{
		static const uint8_t Code[48] = 
		{ 0x4E,0x56,0x00,0x00,						// link a6,#0
		  0x23,0xEE,0x00,0x08,0x00,0x00,0x00,0x00,	// move.l  arg_0(a6),(Event).l
		  0x2F,0x3C,0x00,0x00,0x00,0x00,			// move.l  #This,-(sp)
		  0x2F,0x39,0x00,0x00,0x00,0x00,			// move.l  (Wrapper).l,-(sp)
		  0x2F,0x39,0x00,0x00,0x00,0x00,			// move.l  (Module).l,-(sp)
		  0x20,0x79,0x00,0x00,0x00,0x00,			// movea.l (PealCall).l,a0
		  0x4E,0x90,								// jsr     (a0)
		  0x4E,0x5E,								// unlk    a6
		  0x02,0x80,0x00,0x00,0x00,0xFF,			// andi.l  #$FF,d0
		  0x4E,0x75 };								// rts

#if defined(_M_IX86)
		const tchar_t* Name = T("tcpmp.dll\0WrapperEventHandler86");
		i->Wrapper = SWAP32(Name);
#else
		i->Wrapper = SWAP32(WrapperEventHandler);
#endif
		i->Form = formP;
		i->Handler = handler;
		i->Module = Peal.Module;
		i->PealCall = Peal.PealCall;
		i->Event = NULL;

		memcpy(i->Code,Code,sizeof(Code));
		SetCodePtr(i->Code+8,&i->Event);
		SetCodePtr(i->Code+14,i);
		SetCodePtr(i->Code+20,&i->Wrapper);
		SetCodePtr(i->Code+26,&i->Module);
		SetCodePtr(i->Code+32,&i->PealCall);

		_FrmSetEventHandler(formP,(FormEventHandlerType*)i->Code);
	}
}

void* PealLoadModule(uint16_t FtrId,Boolean Mem,Boolean OnlyFtr,Boolean MemSema)
{
	uint16_t Param[4] = {SWAP16(FtrId),(uint8_t)Mem,(uint8_t)OnlyFtr,(uint8_t)MemSema};
	return (void*)Call68K(SWAP32(Peal.LoadModule),&Param,sizeof(Param)|kPceNativeWantA0);
}

PACE_BEGIN(void* PealGetSymbol(void* Module,const tchar_t* Name))
PACE_ADD32(Module)
PACE_ADD32(Name)
}; return (void*)Call68K(SWAP32(Peal.GetSymbol),PACE_PARAM,sizeof(PACE_PARAM)|kPceNativeWantA0);
}

PACE_BEGIN(void PealFreeModule(void* Module))
PACE_ADD32(Module)
}; Call68K(SWAP32(Peal.FreeModule),PACE_PARAM,sizeof(PACE_PARAM));
}

Err FrmGlueNavGetFocusRingInfo(const FormType* formP, UInt16* objectIDP, 
    Int16* extraInfoP, RectangleType* boundsInsideRingP,
    FrmNavFocusRingStyleEnum* ringStyleP)
{
	UInt32 ver;
	if (FtrGet(sysFileCSystem, sysFtrNumFiveWayNavVersion, &ver)==errNone)
		return FrmNavGetFocusRingInfo(formP,objectIDP,extraInfoP,boundsInsideRingP,ringStyleP);
#ifdef HAVE_PALMONE_SDK
	else if (FtrGet(hsFtrCreator,hsFtrIDNavigationSupported,&ver)==errNone)
		return HsNavGetFocusRingInfo(formP,objectIDP,extraInfoP,boundsInsideRingP,ringStyleP);
#endif
	return uilibErrObjectFocusModeOff;
}

Err FrmGlueNavDrawFocusRing(FormType* formP, UInt16 objectID, Int16 extraInfo,
    RectangleType* boundsInsideRingP,
    FrmNavFocusRingStyleEnum ringStyle, Boolean forceRestore)
{
	UInt32 ver;
	if (FtrGet(sysFileCSystem, sysFtrNumFiveWayNavVersion, &ver)==errNone)
		return FrmNavDrawFocusRing(formP,objectID,extraInfo,boundsInsideRingP,ringStyle,forceRestore);
#ifdef HAVE_PALMONE_SDK
	else if (FtrGet(hsFtrCreator,hsFtrIDNavigationSupported,&ver)==errNone)
		return HsNavDrawFocusRing(formP,objectID,extraInfo,boundsInsideRingP,ringStyle,forceRestore);
#endif
	return uilibErrObjectFocusModeOff;
}

Err FrmGlueNavRemoveFocusRing (FormType* formP)
{
	UInt32 ver;
	if (FtrGet(sysFileCSystem, sysFtrNumFiveWayNavVersion, &ver)==errNone)
		return FrmNavRemoveFocusRing(formP);
#ifdef HAVE_PALMONE_SDK
	else if (FtrGet(hsFtrCreator,hsFtrIDNavigationSupported,&ver)==errNone)
		return HsNavRemoveFocusRing(formP);
#endif
	return uilibErrObjectFocusModeOff;
}

void FrmGlueNavObjectTakeFocus(FormType* formP, UInt16 objID)
{
	UInt32 ver;
	if (FtrGet(sysFileCSystem, sysFtrNumFiveWayNavVersion, &ver)==errNone)
		FrmNavObjectTakeFocus(formP,objID);
#ifdef HAVE_PALMONE_SDK
	else if (FtrGet(hsFtrCreator,hsFtrIDNavigationSupported,&ver)==errNone)
		HsNavObjectTakeFocus(formP,objID);
#endif
}

Boolean FrmGlueNavIsSupported(void)
{
	UInt32 ver;
	if (FtrGet(sysFileCSystem, sysFtrNumFiveWayNavVersion, &ver)==errNone)
		return 1;
#ifdef HAVE_PALMONE_SDK
	if (FtrGet(hsFtrCreator,hsFtrIDNavigationSupported,&ver)==errNone)
		return 1;
#endif
	return 0;
}

PACE_BEGIN(Err FrmNavGetFocusRingInfo(const FormType* formP, UInt16* objectIDP, 
					   Int16* extraInfoP, RectangleType* boundsInsideRingP,
					   FrmNavFocusRingStyleEnum* ringStyleP))
PACE_ADD16(NavSelectorFrmNavGetFocusRingInfo)
PACE_ADD32(formP)
PACE_ADD32(objectIDP)
PACE_ADD32(extraInfoP)
PACE_ADD32(boundsInsideRingP)
PACE_ADD32(ringStyleP)
PACE_END16(sysTrapNavSelector, NONE, PACE_CDATA16(objectIDP) PACE_CDATA16(extraInfoP) PACE_CDATA16(ringStyleP) SwapStruct(boundsInsideRingP,2,2,2,2,0))

PACE_BEGINEX(Err FrmNavDrawFocusRing(FormType* formP, UInt16 objectID, Int16 extraInfo,
				    RectangleType* boundsInsideRingP,
					FrmNavFocusRingStyleEnum ringStyle, Boolean forceRestore),r)
PACE_ADD16(boundsInsideRingP->topLeft.x)
PACE_ADD16(boundsInsideRingP->topLeft.y)
PACE_ADD16(boundsInsideRingP->extent.x)
PACE_ADD16(boundsInsideRingP->extent.y)
PACE_VAR(PACE_PARAM)
PACE_ADD16(NavSelectorFrmNavDrawFocusRing)
PACE_ADD16(objectID)
PACE_ADD16(extraInfo)
PACE_ADD32(r)
PACE_ADD16(ringStyle)
PACE_ADD8(forceRestore)
PACE_END16(sysTrapNavSelector,NONE,NONE)

PACE_BEGIN(Err FrmNavRemoveFocusRing (FormType* formP))
PACE_ADD16(NavSelectorFrmNavRemoveFocusRing)
PACE_ADD32(formP)
PACE_END16(sysTrapNavSelector,NONE,NONE)

PACE_BEGIN(void FrmNavObjectTakeFocus (const FormType* formP, UInt16 objID))
PACE_ADD16(NavSelectorFrmNavObjectTakeFocus)
PACE_ADD32(formP)
PACE_ADD16(objID)
PACE_END(sysTrapNavSelector,NONE,NONE)

#endif
