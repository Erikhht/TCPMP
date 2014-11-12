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
 * $Id: pace.h 332 2005-11-06 14:31:57Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __PACE_H
#define __PACE_H

#if defined(TARGET_PALMOS)

#if _MSC_VER > 1000
#pragma warning( disable:4068 4204 )
#endif

#undef BIG_ENDIAN
#define USE_TRAPS 0
#include <PalmOS.h>
#include <CoreTraps.h>
#include <UIResources.h>
#include <Rect.h>
#include <SystemMgr.h>
#include <Form.h>
#include <ExpansionMgr.h>
#include <VFSMgr.h>
#include <MemGlue.h>
#include <PceNativeCall.h>
#include <SysGlue.h>
#include <FrmGlue.h>

#if PALMOS_SDK_VERSION < 0x0541
#error Please update to Palm OS 5 SDK R4
#endif

typedef struct sysregs
{
	void* GOT;
	void* SysReg;

} sysregs;

void SaveSysRegs(sysregs* p);
void LoadSysRegs(sysregs* p);
void LaunchNotify(SysNotifyParamType*,bool_t HasFocus);

extern Int32 GetOEMSleepMode();
extern Err SetOEMSleepMode(Int32 Mode);

extern void* PealLoadModule(uint16_t FtrId,Boolean Mem,Boolean OnlyFtr,Boolean MemSema);
extern void PealFreeModule(void*);
extern void* PealGetSymbol(void*,const tchar_t* Name);

extern Err SysGetEntryAddresses(UInt32 refNum, UInt32 entryNumStart,
		UInt32 entryNumEnd, void **addressP);
extern Err SysFindModule(UInt32 dbType, UInt32 dbCreator, UInt16 rsrcID,
		UInt32 flags, UInt32 *refNumP);
extern Err SysLoadModule(UInt32 dbType, UInt32 dbCreator, UInt16 rsrcID,
		UInt32 flags, UInt32 *refNumP);
extern Err SysUnloadModule(UInt32 refNum);
extern void HALDelay(UInt32 microSec);
extern void HALDisplayWake();
extern void HALDisplayOff_TREO650();

extern void SonyCleanDCache(void*, UInt32);
extern void SonyInvalidateDCache(void*, UInt32);

extern int PalmCall(void* Func,...); //0..4
extern int PalmCall2(void* Func,...); //5..8

#if defined(_MSC_VER)
#define PALMCALL0(Func) Func()
#define PALMCALL1(Func,a) Func(a)
#define PALMCALL2(Func,a,b) Func(a,b)
#define PALMCALL3(Func,a,b,c) Func(a,b,c)
#define PALMCALL4(Func,a,b,c,d) Func(a,b,c,d)
#define PALMCALL5(Func,a,b,c,d,e) Func(a,b,c,d,e)
#define PALMCALL6(Func,a,b,c,d,e,f) Func(a,b,c,d,e,f)
#define PALMCALL7(Func,a,b,c,d,e,f,g) Func(a,b,c,d,e,f,g)
#define PALMCALL8(Func,a,b,c,d,e,f,g,h) Func(a,b,c,d,e,f,g,h)
#else
#define PALMCALL0(Func) PalmCall(Func)
#define PALMCALL1(Func,a) PalmCall(Func,a)
#define PALMCALL2(Func,a,b) PalmCall(Func,a,b)
#define PALMCALL3(Func,a,b,c) PalmCall(Func,a,b,c)
#define PALMCALL4(Func,a,b,c,d) PalmCall(Func,a,b,c,d)
#define PALMCALL5(Func,a,b,c,d,e) PalmCall2(Func,a,b,c,d,e)
#define PALMCALL6(Func,a,b,c,d,e,f) PalmCall2(Func,a,b,c,d,e,f)
#define PALMCALL7(Func,a,b,c,d,e,f,g) PalmCall2(Func,a,b,c,d,e,f,g)
#define PALMCALL8(Func,a,b,c,d,e,f,g,h) PalmCall2(Func,a,b,c,d,e,f,g,h)
#endif

typedef uint16_t m68kcallback[12];
extern void* m68kCallBack(m68kcallback,NativeFuncType*);

extern const tchar_t* VFSToVol(const tchar_t* URL,uint16_t* Vol);
extern bool_t VFSFromVol(uint16_t Vol,const tchar_t* Path,tchar_t* URL,int URLLen);
extern bool_t DBFrom(uint16_t Card,uint32_t DB,tchar_t* URL,int URLLen);

extern FormEventHandlerType* FrmGetEventHandler(FormType *Form);

#define memNewChunkFlagAllowLarge 0x1000

typedef struct SysAppLaunchCmdOpenDBType2
{
	UInt16		cardNo;
	UInt16		dbID[2];

} SysAppLaunchCmdOpenDBType2;

typedef struct vfspath
{
	UInt16 volRefNum;
	Char path[256];
} vfspath;

#endif
#endif
