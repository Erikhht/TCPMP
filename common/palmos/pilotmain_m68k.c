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
 * $Id: pilotmain_m68k.c 292 2005-10-14 20:30:00Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include <PalmOS.h>
#include <MemGlue.h>
#include "peal/m68k/peal.h"
#include <stdio.h>
#include <PceNativeCall.h>

#define SWAP16(a) ((((UInt16)(a) >> 8) & 0x00FF) | (((UInt16)(a) << 8) & 0xFF00))

#define SWAP32(a) ((((UInt32)(a) >> 24) & 0x000000FF) | (((UInt32)(a) >> 8)  & 0x0000FF00) |\
	 	(((UInt32)(a) << 8)  & 0x00FF0000) | (((UInt32)(a) << 24) & 0xFF000000))

typedef struct vfspath
{
	UInt16 volRefNum;
	Char path[256];
} vfspath;

typedef struct launch
{
	MemPtr PealCall;
	PealModule* Module;
	void* LoadModule;
	void* FreeModule;
	void* GetSymbol;

	MemPtr launchParameters;
	UInt16 launchCode;
	UInt16 launchFlags;

} launch;

static Err RomVersionCheck(UInt16 launchFlags)
{
	UInt32 Version;
	FtrGet(sysFtrCreator, sysFtrNumROMVersion, &Version);

	if (Version < sysMakeROMVersion(5,0,0,sysROMStageDevelopment,0))
	{
		if ((launchFlags & sysAppLaunchFlagNewGlobals) != 0 &&
			(launchFlags & sysAppLaunchFlagUIApp) != 0)
		{
			FrmCustomAlert(WarningOKAlert, "System version 5.0 or greater is required to run this application!", " ", " ");

			// Palm OS 1.0 requires you to explicitly launch another app
			if (Version < sysMakeROMVersion(1,0,0,sysROMStageRelease,0))
			{
				AppLaunchWithCommand(sysFileCDefaultApp,
						sysAppLaunchCmdNormalLaunch, NULL);
			}
		}

		return sysErrRomIncompatible;
	}
	return errNone;
}

static PealModule* Module = NULL;
static void* PaceMain = NULL;

static void* LookupSymbol86(void* Module,const char* Name)
{
	return NULL;
}

static void* LoadModule(UInt16 ftrId,Boolean mem,Boolean onlyftr,Boolean memsema)
{
	return PealLoadFromResources('armc',1000,Module,'TPLY',ftrId,mem,onlyftr,memsema);
}

static UInt32 PealCall86(void* Module,void* Func,void* Param)
{
	return PceNativeCall((NativeFuncType*)Func,Param);
}

static UInt16 GetHeapId()
{
	MemPtr p=MemPtrNew(8);   
	UInt16 Id=MemPtrHeapID(p);
	MemPtrFree(p);
	return Id;
}

UInt32 PilotMain(UInt16 launchCode, MemPtr launchParameters, UInt16 launchFlags)
{
	UInt32 Value;
	UInt32 Result = errNone;
	UInt32 CPU;
	launch Launch;
	Launch.launchParameters = launchParameters;
	Launch.launchCode = launchCode;
	Launch.launchFlags = launchFlags;

	if ((launchCode == sysAppLaunchCmdNormalLaunch ||
		launchCode == sysAppLaunchCmdOpenDB ||
		launchCode == sysAppLaunchCmdCustomBase) && !RomVersionCheck(launchFlags))
	{
		FtrGet(sysFileCSystem, sysFtrNumProcessorID, &CPU);
		if (CPU == sysFtrNumProcessorx86)
		{
			Module = PealLoadFromResources('armc', 1000, NULL, 'TPLY',32,0,0,0); // just for testing

			Launch.FreeModule = PealUnload;
			Launch.LoadModule = LoadModule;
			Launch.GetSymbol = LookupSymbol86;
			Launch.PealCall = PealCall86;
			Launch.Module = Module;

			PceNativeCall((NativeFuncType*)"tcpmp.dll\0PaceMain86",&Launch);

			if (Module)
				PealUnload(Module);
		}
		else
		if (sysFtrNumProcessorIsARM(CPU))
		{
			UInt32 Version;
			Boolean MemSema;

			FtrGet(sysFtrCreator, sysFtrNumROMVersion, &Version);
			MemSema = Version < sysMakeROMVersion(6,0,0,sysROMStageDevelopment,0);

			Module = PealLoadFromResources('armc', 1000, NULL, 'TPLY',32,0,0,MemSema);
			if (Module)
			{
				PaceMain = PealLookupSymbol(Module, "PaceMain");
				if (PaceMain)
				{
					Launch.FreeModule = PealUnload;
					Launch.LoadModule = LoadModule;
					Launch.GetSymbol = PealLookupSymbol;
					Launch.PealCall = PealCall;
					Launch.Module = Module;

					Result = PealCall(Module,PaceMain,&Launch);
				}
				PealUnload(Module);
				MemHeapCompact(GetHeapId()); 
			}
		}
		else
			FrmCustomAlert(WarningOKAlert, "ARM processor is required to run this application!", " ", " ");

		if (FtrGet('TPLY',20,&Value)==errNone)
			FtrPtrFree('TPLY',20);
	}
	else
	if (launchCode == sysAppLaunchCmdNotify && (launchFlags & sysAppLaunchFlagSubCall)!=0)
	{
		FtrGet(sysFileCSystem, sysFtrNumProcessorID, &CPU);
		if (CPU == sysFtrNumProcessorx86)
			Result = PceNativeCall((NativeFuncType*)"tcpmp.dll\0PaceMain86",&Launch);
		else
		if (sysFtrNumProcessorIsARM(CPU) && Module && PaceMain)
			Result = PealCall(Module,PaceMain,&Launch);
	}
	else
	if (launchCode == sysAppLaunchCmdCardLaunch && launchParameters)
	{
		SysAppLaunchCmdCardType* p = (SysAppLaunchCmdCardType*)launchParameters;
		vfspath* VFSPath;
		if (FtrGet('TPLY',20,&Value)==errNone)
			FtrPtrFree('TPLY',20);
		if (FtrPtrNew('TPLY',20,sizeof(vfspath),(void**)&VFSPath)==errNone)
		{
			vfspath Path;
			memset(&Path,0,sizeof(vfspath));
			Path.volRefNum = SWAP16(p->volRefNum);
			StrNCopy(Path.path,p->path,255);
			DmWrite(VFSPath,0,&Path,sizeof(vfspath));
		}
	}
	return Result;
}
