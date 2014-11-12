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
 * $Id: native86.c 292 2005-10-14 20:30:00Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"

#if defined(TARGET_PALMOS)

#include "pace.h"

Err SysGetEntryAddresses(UInt32 refNum, UInt32 entryNumStart, UInt32 entryNumEnd, void **addressP)
{
	return 1; //fail
}

Err SysFindModule(UInt32 dbType, UInt32 dbCreator, UInt16 rsrcID, UInt32 flags, UInt32 *refNumP)
{
	return 1;
}

Err SysLoadModule(UInt32 dbType, UInt32 dbCreator, UInt16 rsrcID, UInt32 flags, UInt32 *refNumP)
{
	return 1;
}

Err SysUnloadModule(UInt32 refNum)
{
	return 1;
}

#include <windows.h>

void** HALDispatch()
{
	static void** Table = NULL;
	if (!Table)
	{
		HMODULE Module = LoadLibrary(T("dal.dll"));
		void (__cdecl *GetDispath)(void***) = (void (__cdecl*)(void***)) GetProcAddress(Module,"__ExportDispatchTable");
		GetDispath(&Table);
		//FreeLibrary(Module);
	}
	return Table;
}

void SonyCleanDCache(void* p, UInt32 n) {}
void SonyInvalidateDCache(void* p, UInt32 n) {}
void HALDelay(UInt32 n) 
{
	((void (__cdecl*)(UInt32))HALDispatch()[0x0A])(n);
}
void HALDisplayWake() 
{
	((void (__cdecl*)())HALDispatch()[0x10])();
}
void HALDisplayOff_TREO650() 
{
	((void (__cdecl*)())HALDispatch()[0xD6])();
}

#endif
