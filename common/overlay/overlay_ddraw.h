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
 * $Id: overlay_ddraw.h 615 2006-01-26 16:57:51Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __OVERLAY_DDRAW_H
#define __OVERLAY_DDRAW_H

// displaying methods:
#define MODE_OVERLAY	0
#define MODE_BLIT		1
#define MODE_PRIMARY	2

typedef struct ddraw
{
	overlay p;
	video Overlay;
	rect OverlayRect;
	blitfx SoftFX;
	blitfx OvlFX;
	RECT Src;
	RECT Dst;
	void* Wnd;

	bool_t SetupColorKey;
	bool_t SetupBlit;
	bool_t SetupBlitStretch;
	int Format;

	int Mode;
	int DstAlignSize;
	int DstAlignPos;
	int MinScale;
	int MaxScale;
	bool_t BufferPixelFormat;
	bool_t PhyMode;

	LPDIRECTDRAW DD;
	LPDIRECTDRAWSURFACE DDPrimary;
	LPDIRECTDRAWSURFACE DDBuffer;
	LPDIRECTDRAWSURFACE DDBackBuffer;
	LPDIRECTDRAWCLIPPER DDClipper;

	union
	{
		DDCAPS DDCaps;
		char DDCapsCE[256];
	};

} ddraw;

extern int DDrawCECreate(ddraw*,LPDIRECTDRAW DD);

#endif
