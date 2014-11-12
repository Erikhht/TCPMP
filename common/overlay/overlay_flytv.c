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
 * $Id: overlay_flytv.c 543 2006-01-07 22:06:24Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"

// not for smartphones
#if defined(TARGET_WINCE) && !defined(WIN32_PLATFORM_WFSP)

#define WIN32_LEAN_AND_MEAN
#ifndef STRICT
#define STRICT
#endif
#include <windows.h>

#define OUTPUT_VGA		17
#define Format320X240	2

typedef struct flytv
{
	overlay Overlay;
	planes Planes;
	bool_t Inited;
	bool_t ErrorShowed;

	WCHAR* (*DriverVersion)();
	int (*Initial_FLY_TV)(int,int);
	void (*UnInitial_FLY_TV)();
	int (*Zoom)(int);
	int (*SendFrame)(const void*);
} flytv;

static int Init(flytv* p)
{
	int Size = 2*p->Overlay.Output.Format.Video.Height * p->Overlay.Output.Format.Video.Width;
	p->ErrorShowed = 0;
	p->Inited = 0;
	p->Planes[0] = Alloc16(Size);
	if (!p->Planes[0])
		return ERR_OUT_OF_MEMORY;

	memset(p->Planes[0],0,Size);
	p->Overlay.ClearFX = BLITFX_ONLYDIFF;
	return ERR_NONE;
}

static void Done(flytv* p)
{
	Free16(p->Planes[0]); 
	p->Planes[0] = NULL;
}

static int UpdateShow(flytv* p)
{
	if (p->Overlay.Show)
	{
		if (!p->Initial_FLY_TV(OUTPUT_VGA,Format320X240))
		{
			if (!p->ErrorShowed)
			{
				p->ErrorShowed = 1;
				ShowError(p->Overlay.Node.Class,ERR_ID,ERR_DEVICE_ERROR);
			}
			return ERR_DEVICE_ERROR;
		}
		p->Inited = 1;
		p->Zoom(2);
	}
	else
	{
		p->UnInitial_FLY_TV();
		p->Inited = 0;
	}
	return ERR_NONE;
}

static int Lock(flytv* p, planes Planes, bool_t OnlyAligned )
{
	Planes[0] = p->Planes[0];
	return ERR_NONE;
}

static int Unlock(flytv* p )
{
	return ERR_NONE;
}

static int Blit(flytv* p, const constplanes Data, const constplanes DataLast )
{
	if (p->Inited)
	{
		BlitImage(p->Overlay.Soft,p->Planes,Data,DataLast,-1,-1);
		p->SendFrame(p->Planes[0]);
	}
	return ERR_NONE;
}

static int Create(flytv* p)
{
	p->Overlay.Module = LoadLibrary(T("FlyPresenter_TV.DLL"));
	GetProc(&p->Overlay.Module,&p->DriverVersion,T("DriverVersion"),0);
	GetProc(&p->Overlay.Module,&p->Initial_FLY_TV,T("Initial_FLY_TV"),0);
	GetProc(&p->Overlay.Module,&p->UnInitial_FLY_TV,T("UnInitial_FLY_TV"),0);
	GetProc(&p->Overlay.Module,&p->Zoom,T("Zoom"),0);
	GetProc(&p->Overlay.Module,&p->SendFrame,T("SendFrame"),0);
	if (!p->Overlay.Module)
		return ERR_DEVICE_ERROR;

	p->Overlay.Primary = 0;
	p->Overlay.Output.Format.Video.Width = 320;
	p->Overlay.Output.Format.Video.Height = 240;
	p->Overlay.Output.Format.Video.Direction = 0;
	p->Overlay.Output.Format.Video.Aspect = ASPECT_ONE;
	p->Overlay.Output.Format.Video.Pitch = p->Overlay.Output.Format.Video.Width*2;
	DefaultRGB(&p->Overlay.Output.Format.Video.Pixel,16,5,6,5,0,0,0);
	p->Overlay.Output.Format.Video.Pixel.Flags |= PF_16ALIGNED;
	p->Overlay.Init = Init;
	p->Overlay.Done = Done;
	p->Overlay.Blit = Blit;
	p->Overlay.UpdateShow = UpdateShow;
	p->Overlay.Lock = Lock;
	p->Overlay.Unlock = Unlock;
	return ERR_NONE;
}

static const nodedef FlyTV = 
{
	sizeof(flytv)|CF_GLOBAL,
	FLYTV_ID,
	OVERLAY_CLASS,
	PRI_DEFAULT-10,
	(nodecreate)Create,
};

void OverlayFlyTV_Init() 
{ 
	NodeRegisterClass(&FlyTV);
}
void OverlayFlyTV_Done()
{
	NodeUnRegisterClass(FLYTV_ID);
}

#else
void OverlayFlyTV_Init() {}
void OverlayFlyTV_Done() {}
#endif
