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
 * $Id: blit.h 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __BLIT_H
#define __BLIT_H

// blitfx flags
#define	BLITFX_ARITHSTRETCH50		0x001
#define	BLITFX_ARITHSTRETCHALWAYS	0x100
#define	BLITFX_DITHER				0x002
#define	BLITFX_AVOIDTEARING			0x004
#define BLITFX_ONLYDIFF				0x020
#define BLITFX_COLOR_LOOKUP			0x040
#define	BLITFX_VMEMROTATED			0x010
#define	BLITFX_VMEMUPDOWN			0x080
#define BLITFX_ENLARGEIFNEEDED		0x200

typedef struct blitfx
{
	int Flags;			// combination of blitfx flags
	int ScaleX;			// 16.16 fixed point scale factor (before swapxy)
	int ScaleY;			// 16.16 fixed point scale factor (before swapxy)
	int Brightness;
	int Contrast;
	int Saturation;
	int Direction;
	int RGBAdjust[3];		// additional r,g,b brightness
} blitfx;

void Blit_Init();
void Blit_Done();

DLL struct blitpack* BlitCreate(const video* DstFormat,
			                    const video* SrcFormat, const blitfx* FX, int* OutCaps);

DLL void BlitAlign(struct blitpack*, rect* DstRect, rect* SrcRect);

DLL void BlitImage(struct blitpack*, const planes Dst, const constplanes Src, const constplanes SrcLast,
				   int DstPicth, int SrcPitch); // video pitch can be overridden

DLL void BlitRelease(struct blitpack*);

#endif
