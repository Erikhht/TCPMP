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
 * $Id: overlay_s1d13806.c 543 2006-01-07 22:06:24Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"

#if defined(TARGET_WINCE) && defined(ARM)

#define WIN32_LEAN_AND_MEAN
#ifndef STRICT
#define STRICT
#endif
#include <windows.h>

extern int STDCALL IRQEnable();
extern int STDCALL IRQDisable();

typedef struct s1d13806
{
	overlay Overlay;
	planes Planes;
	bool_t KernelMode;
	void* Buffer;
	bool_t PhyAddr;
	volatile unsigned char* Regs;
	volatile unsigned short* FIFO;
	HBRUSH Brush;

} s1d13806;

static INLINE void Reg3(s1d13806* p,int i,int v)
{
	p->Regs[i+0] = (unsigned char)(v);
	p->Regs[i+1] = (unsigned char)(v >> 8);
	p->Regs[i+2] = (unsigned char)(v >> 16);
}

static INLINE void Reg2(s1d13806* p,int i,int v)
{
	p->Regs[i+0] = (unsigned char)(v);
	p->Regs[i+1] = (unsigned char)(v >> 8);
}

static int Blit(overlay* p, const constplanes Data, const constplanes DataLast)
{
	planes Planes;
	int Result = p->Lock(p,Planes,1);
	if (Result==ERR_NONE)
	{
		BlitImage(p->Soft,Planes,Data,DataLast,-1,-1);
		p->Unlock(p);
	}
	return Result;
}

static int Update(overlay* p)
{
	blitfx FX = p->FX;
	video Output = p->Output.Format.Video;
	rect OldGUI = p->GUIAlignedRect;
	rect Old = p->DstAlignedRect;

	VirtToPhy(&p->Viewport,&p->DstAlignedRect,&p->Output.Format.Video);
	VirtToPhy(NULL,&p->SrcAlignedRect,&p->Input.Format.Video);

	p->DstAlignedRect.Height >>= 1;
	p->DstAlignedRect.y >>= 1;
	Output.Height >>= 1;
	Output.Pitch <<= 1;
	if (FX.Direction & DIR_SWAPXY)
		FX.ScaleX >>= 1;
	else
		FX.ScaleY >>= 1;

	if (p->Output.Format.Video.Pixel.BitCount!=16)
		FX.Flags |= BLITFX_DITHER;

	BlitRelease(p->Soft);
	p->Soft = BlitCreate(&Output,&p->Input.Format.Video,&FX,&p->Caps);

	BlitAlign(p->Soft,&p->DstAlignedRect, &p->SrcAlignedRect);

	p->DstAlignedRect.Height <<= 1;
	p->DstAlignedRect.y <<= 1;

	if (p->Output.Format.Video.Pixel.BitCount!=16)
		p->Caps &= ~VC_DITHER;

	PhyToVirt(&p->DstAlignedRect,&p->GUIAlignedRect,&p->Output.Format.Video);

	if (!EqRect(&Old,&p->DstAlignedRect) && p->Show && p->Primary)
	{
		WinInvalidate(&OldGUI,0);
		WinInvalidate(&p->Viewport,1);
		WinValidate(&p->GUIAlignedRect);
	}

	if (p->Show && (p->FullScreenViewport || !p->Primary))
		OverlayClearBorder(p);

	return ERR_NONE;
}

static int Init(s1d13806* p)
{
	HKEY Key;
	video* Output = &p->Overlay.Output.Format.Video;

	QueryDesktop(Output);
	DefaultPitch(Output);

	p->Regs = NULL;
	p->Buffer = NULL;

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, T("Drivers\\Display\\S1D13806"), 0, KEY_READ, &Key) == ERROR_SUCCESS)
	{
		DWORD Value;
		DWORD RegSize;
		DWORD RegType;

		RegSize = sizeof(Value);
		if (RegQueryValueEx(Key, T("RegBase"), 0, &RegType, (LPBYTE)&Value, &RegSize) == ERROR_SUCCESS)
			p->Regs = (unsigned char*)Value;

		RegSize = sizeof(Value);
		if (RegQueryValueEx(Key, T("MemBase"), 0, &RegType, (LPBYTE)&Value, &RegSize) == ERROR_SUCCESS)
			p->Buffer = (void*)Value;

		RegCloseKey(Key);
	}

	if (!p->Regs || !p->Buffer)
		return ERR_NOT_SUPPORTED;

	p->PhyAddr = (uint32_t)p->Regs < 0x80000000;

	if (p->PhyAddr)
	{
		p->Regs = PhyMemBegin((uint32_t)p->Regs,2*1024*1024,0);
		if (!p->Regs)
			return ERR_NOT_SUPPORTED;

		p->Buffer = PhyMemBegin((uint32_t)p->Buffer,2*1024*1024,1);
		if (!p->Buffer)
		{
			PhyMemEnd((void*)p->Regs);
			p->Regs = NULL;
			return ERR_NOT_SUPPORTED;
		}
	}

	p->FIFO = (unsigned short*)(p->Regs+1024*1024);

	if (((int)p->Buffer & 15)==0 && (Output->Pitch & 15)==0)
		Output->Pixel.Flags |= PF_16ALIGNED;

	p->Planes[0] = p->Buffer;
	AdjustOrientation(Output,0);
	p->Overlay.SetFX = BLITFX_AVOIDTEARING;

	p->Brush = (HBRUSH)GetStockObject(BLACK_BRUSH);

	return ERR_NONE;
}

static void Done(s1d13806* p)
{
	if (p->PhyAddr)
	{
		if (p->Buffer)
			PhyMemEnd(p->Buffer);
		if (p->Regs)
			PhyMemEnd((void*)p->Regs);
	}
	p->Buffer = NULL;
	p->Regs = NULL;
}

static int Reset(s1d13806* p)
{
	Done(p);
	Init(p);
	return ERR_NONE;
}

static int Lock(s1d13806* p, planes Planes, bool_t OnlyAligned)
{
	p->KernelMode = KernelMode(1);
	Planes[0] = p->Planes[0];
	return ERR_NONE;
}

static int Unlock(s1d13806* p)
{
	if (p->Regs && p->Overlay.DstAlignedRect.Width>0 && p->Overlay.DstAlignedRect.Height>1)
	{
		int ReTry;
		HDC DC = GetDC(NULL);

		for (ReTry=0;ReTry<32;++ReTry)
		{
			// we have to make sure not to interfere with the GDI driver
			// use FillRect to make the GDI driver use the epson chip
			// disable the interrupt and verify we still in our FillRect blitting
			// wait until it's finished and start our own blitting

			RECT R;
			R.left = 0;
			R.top = 0;
			R.right = 1;
			R.bottom = p->Overlay.Output.Format.Video.Height-1;
			FillRect(DC,&R,p->Brush);

			IRQDisable();

			if ((p->Regs[0x100] & 128)==0 ||
				p->Regs[0x110] != 0 || p->Regs[0x111] != 0 ||
				p->Regs[0x112] != ((R.bottom-1) & 255) || p->Regs[0x113] != ((R.bottom-1) >> 8))
			{
				IRQEnable();
			}
			else
			{
				int Ofs = p->Overlay.DstAlignedRect.x*(p->Overlay.Output.Format.Video.Pixel.BitCount>>3) + 
					p->Overlay.DstAlignedRect.y*p->Overlay.Output.Format.Video.Pitch;

				while ((p->Regs[0x100] & 128)!=0);
				ReTry += p->FIFO[0];

				Reg3(p,0x104,Ofs);
				Reg3(p,0x108,Ofs + p->Overlay.Output.Format.Video.Pitch);

				Reg2(p,0x110,p->Overlay.DstAlignedRect.Width-1);
				Reg2(p,0x112,(p->Overlay.DstAlignedRect.Height/2)-1);
				p->Regs[0x103] = 2;
				p->Regs[0x102] = 0x0C;
				p->Regs[0x101] = (char)(p->Overlay.Output.Format.Video.Pixel.BitCount==16?1:0);
				Reg2(p,0x10C,p->Overlay.Output.Format.Video.Pitch);

				p->Regs[0x100] = 0x80;

				while ((p->Regs[0x100] & 128)==0);
				IRQEnable();
				break;
			}
		}

		ReleaseDC(NULL,DC);
	}

	KernelMode(p->KernelMode);
	return ERR_NONE;
}

static int Create(s1d13806* p)
{
	if (Init(p) != ERR_NONE) // check if supported
		return ERR_NOT_SUPPORTED;
	Done(p);

	p->Overlay.Init = (ovlfunc)Init;
	p->Overlay.Done = (ovldone)Done;
	p->Overlay.Reset = (ovlfunc)Reset;
	p->Overlay.Lock = (ovllock)Lock;
	p->Overlay.Unlock = (ovlfunc)Unlock;
	p->Overlay.Blit = (ovlblit)Blit;
	p->Overlay.Update = (ovlfunc)Update;
	return ERR_NONE;
}

static const nodedef S1D13806 = 
{
	sizeof(s1d13806)|CF_GLOBAL,
	S1D13806_ID,
	OVERLAY_CLASS,
	PRI_DEFAULT+92,
	(nodecreate)Create,
};

void OverlayS1D13806_Init()
{ 
	NodeRegisterClass(&S1D13806);
}

void OverlayS1D13806_Done()
{
	NodeUnRegisterClass(S1D13806_ID);
}

#else
void OverlayS1D13806_Init() {}
void OverlayS1D13806_Done() {}
#endif
