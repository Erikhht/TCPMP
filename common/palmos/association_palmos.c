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
 * $Id: association_palmos.c 345 2005-11-19 15:57:54Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"

#if defined(TARGET_PALMOS)

#include "pace.h"

static void RegisterDefaultDirectory()
{
	int* i;
	array List;
	NodeEnumClass(&List,MEDIA_CLASS);
	for (i=ARRAYBEGIN(List,int);i!=ARRAYEND(List,int);++i)
	{
		const tchar_t* Exts = LangStr(*i,NODE_EXTS);
		while (Exts)
		{
			const tchar_t* p = tcschr(Exts,':');
			if (p)
			{
				tchar_t Ext[MAXPATH];
				const char* Dir;

				switch (p[1])
				{
				case FTYPE_VIDEO:
					Dir = T("/DCIM/");
					break;
				case FTYPE_PLAYLIST:
				case FTYPE_AUDIO:
				default:
					Dir = T("/AUDIO/");
					break;
				}

				Ext[0] = '.';
				memcpy(Ext+1,Exts,p-Exts);
				Ext[p-Exts+1] = 0;

				VFSRegisterDefaultDirectory (Ext, expMediaType_Any, Dir);

				p = tcschr(p,';');
				if (p) ++p;
			}
			Exts = p;
		}
	}
	ArrayClear(&List);
}

//ExgGetDefaultApplication
//ExgRegisterDatatype(myCreator,exgRegExtensionID, "TXT", NULL, 0); 
//ExgSetDefaultApplication
//sysAppLaunchCmdExgReceiveData ???

void Association_Init()
{
	RegisterDefaultDirectory();
}

void Association_Done()
{
}

#endif
