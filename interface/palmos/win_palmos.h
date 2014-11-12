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
 * $Id: win_palmos.h 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __WIN_PALMOS_H
#define __WIN_PALMOS_H

typedef int (*winhandler)(void*);
int WinHandler(win*,void*);

#define WINCREATE(Name) \
	static win* Name##Ptr; \
	static int Name##Handler(void* Event) { return WinHandler(Name##Ptr,Event); } \
	static void Name##Create(win* p) { Name##Ptr = p; p->Handler = Name##Handler; }

struct wincontrol
{
	pin Pin;
	wincontrol* Ref;
	wincontrol* Next; // controls chain
};

#define WINPRIVATE

#endif
