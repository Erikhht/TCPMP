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
 * $Id: settings.h 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common/common.h"
#include "skin.h"
#include "../config.h"

#if defined(CONFIG_SKIN) && !defined(TARGET_PALMOS)
#include "../private/skin.c"
#else
void* SkinLoad(skin* p,void* Wnd,const tchar_t* Path) 
{
	memset(p,0,sizeof(skin)*MAX_SKIN);
	return NULL; 
}
void SkinFree(skin* p,void* Db) {}
void SkinDraw(const skin* p,void* DC,rect* r) {}
void SkinDrawItem(skin* p,void* Wnd,int n,rect* r) {}
void SkinUpdate(skin* p,player* Player,void* Wnd,rect* r) {}
int SkinMouse(skin* p,int no,int x,int y,player* Player,int* cmd,void* Wnd,rect* r) { return 0; }
#endif
