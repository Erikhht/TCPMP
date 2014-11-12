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
 * $Id: overlay_gapi.c 543 2006-01-07 22:06:24Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"

#ifdef TARGET_WINCE

#define WIN32_LEAN_AND_MEAN
#ifndef STRICT
#define STRICT
#endif
#include <windows.h>

typedef struct GXDisplayProperties 
{
	int cxWidth;
	int cyHeight;
	int cbxPitch;
	int cbyPitch;
	int cBPP;
	int ffFormat;
} GXDisplayProperties;

#define SHFS_SHOWTASKBAR            0x0001
#define SHFS_HIDETASKBAR            0x0002
#define SHFS_SHOWSIPBUTTON          0x0004
#define SHFS_HIDESIPBUTTON          0x0008
#define SHFS_SHOWSTARTICON          0x0010
#define SHFS_HIDESTARTICON          0x0020

#define GX_FULLSCREEN		0x01
#define kfLandscape			0x08
#define kfPalette			0x10
#define kfDirect			0x20
#define kfDirect555			0x40
#define kfDirect565			0x80
#define kfDirect888			0x100
#define kfDirect444			0x200
#define kfDirectInverted	0x400

typedef struct gapi
{
	overlay Overlay;

	bool_t Windows;
	bool_t Opened;
	bool_t DRAM;
	void* Wnd;
	int AdjustPtr;
	uint8_t* DirectPtr;
	uint8_t* Pointer;
	HMODULE GX;
	HMODULE AygShell;
	GXDisplayProperties Info;

	int (*GXOpenDisplay)(HWND hWnd, DWORD dwFlags);
	int (*GXCloseDisplay)();
	void* (*GXBeginDraw)();
	int (*GXEndDraw)();
	GXDisplayProperties (*GXGetDisplayProperties)();
	int (*GXSetViewport)(DWORD dwTop, DWORD dwHeight, DWORD, DWORD);
	BOOL (*GXIsDisplayDRAMBuffer)();
	BOOL (WINAPI* SHFullScreen)(HWND, DWORD);

} gapi;

static const datatable Params[] = 
{
	{ GAPI_WIDTH,	TYPE_INT, DF_SETUP | DF_RDONLY | DF_HIDDEN },
	{ GAPI_HEIGHT,	TYPE_INT, DF_SETUP | DF_RDONLY | DF_HIDDEN },
	{ GAPI_PITCHX,	TYPE_INT, DF_SETUP | DF_RDONLY | DF_HIDDEN },
	{ GAPI_PITCHY,	TYPE_INT, DF_SETUP | DF_RDONLY | DF_HIDDEN },
	{ GAPI_BPP,		TYPE_INT, DF_SETUP | DF_RDONLY | DF_HIDDEN },
	{ GAPI_FORMAT,	TYPE_INT, DF_SETUP | DF_RDONLY|DF_HEX | DF_HIDDEN },
	{ GAPI_DRAM,	TYPE_BOOL, DF_SETUP | DF_RDONLY | DF_HIDDEN },
	{ GAPI_POINTER,	TYPE_INT, DF_SETUP | DF_RDONLY|DF_HEX | DF_HIDDEN },

	DATATABLE_END(GAPI_ID)
};

static int Enum(gapi* p, int* No, datadef* Param)
{
	if (OverlayEnum(&p->Overlay,No,Param)==ERR_NONE)
		return ERR_NONE;
	return NodeEnumTable(No,Param,Params);
}

static int Get(gapi* p,int No,void* Data,int Size)
{
	int Result = OverlayGet(&p->Overlay,No,Data,Size);
	switch (No)
	{
	case GAPI_WIDTH: GETVALUE(p->Info.cxWidth,int); break;
	case GAPI_HEIGHT: GETVALUE(p->Info.cyHeight,int); break;
	case GAPI_PITCHX: GETVALUE(p->Info.cbxPitch,int); break;
	case GAPI_PITCHY: GETVALUE(p->Info.cbyPitch,int); break;
	case GAPI_BPP: GETVALUE(p->Info.cBPP,int); break;
	case GAPI_FORMAT: GETVALUE(p->Info.ffFormat,int); break;
	case GAPI_DRAM: GETVALUE(p->DRAM,bool_t); break;
	case GAPI_POINTER: GETVALUE((int)p->Pointer,int); break;
	}
	return Result;
}

static bool_t LoadDriver(gapi* p)
{
	HMODULE GX;
	tchar_t Path[MAXPATH];

	if (p->Windows)
		GetSystemPath(Path,TSIZEOF(Path),T("gx.dll"));
	else
		tcscpy_s(Path,TSIZEOF(Path),T("gx.dll"));

	if ((GX = LoadLibrary(Path))==NULL)
	{
		if (!p->Windows)
		{
			p->Windows = 1;
			return LoadDriver(p);
		}
		return 0;
	}

	if (p->GX)
		FreeLibrary(p->GX);
	p->GX = GX;

	GetProc(&p->GX,&p->GXOpenDisplay,T("?GXOpenDisplay@@YAHPAUHWND__@@K@Z"),0);
	GetProc(&p->GX,&p->GXCloseDisplay,T("?GXCloseDisplay@@YAHXZ"),0);
	GetProc(&p->GX,&p->GXBeginDraw,T("?GXBeginDraw@@YAPAXXZ"),0);
	GetProc(&p->GX,&p->GXEndDraw,T("?GXEndDraw@@YAHXZ"),0);
	GetProc(&p->GX,&p->GXGetDisplayProperties,T("?GXGetDisplayProperties@@YA?AUGXDisplayProperties@@XZ"),0);
	GetProc(&p->GX,&p->GXSetViewport,T("?GXSetViewport@@YAHKKKK@Z"),1);
	GetProc(&p->GX,&p->GXIsDisplayDRAMBuffer,T("?GXIsDisplayDRAMBuffer@@YAHXZ"),1);

	return p->GX!=NULL;
}

static int Init(gapi* p)
{
	GXDisplayProperties Info;
	video GDI;
	int Caps = QueryPlatform(PLATFORM_CAPS);

	if (!p->Opened)
	{
		ShowError(p->Overlay.Node.Class,GAPI_ID,GAPI_OPEN_ERROR);
		return ERR_NOT_SUPPORTED; // don't show device error
	}

	Info = p->Info;

	if (Info.cxWidth == GetSystemMetrics(SM_CXSCREEN) &&
		Info.cyHeight < GetSystemMetrics(SM_CYSCREEN))
	{
		// HPC devices we need the taskbar counted in the height too
		RECT WorkArea;
		SystemParametersInfo(SPI_GETWORKAREA,0,&WorkArea,0);
		if (WorkArea.top == 0 && WorkArea.bottom == Info.cyHeight)
			Info.cyHeight = GetSystemMetrics(SM_CYSCREEN);
	}

	p->Overlay.Output.Format.Video.Direction = 0;
	p->Overlay.Output.Format.Video.Aspect = ASPECT_ONE;

	if (Info.ffFormat & kfPalette)
	{
		QueryDesktop(&GDI);
		p->Overlay.Output.Format.Video.Pixel.Flags = PF_PALETTE;
		p->Overlay.Output.Format.Video.Pixel.BitCount = Info.cBPP;
		p->Overlay.Output.Format.Video.Pixel.Palette = GDI.Pixel.Palette;
	}
	else
	if (Info.ffFormat & kfDirect444)
		DefaultRGB(&p->Overlay.Output.Format.Video.Pixel,Info.cBPP,4,4,4,0,0,0);
	else
	if (Info.ffFormat & kfDirect565)
	{
		if (Caps & CAPS_ONLY12BITRGB)
			DefaultRGB(&p->Overlay.Output.Format.Video.Pixel,Info.cBPP,4,4,4,1,2,1);
		else
			DefaultRGB(&p->Overlay.Output.Format.Video.Pixel,Info.cBPP,5,6,5,0,0,0);
	}
	else
	if (Info.ffFormat & kfDirect555)
		DefaultRGB(&p->Overlay.Output.Format.Video.Pixel,Info.cBPP,5,5,5,0,0,0);
	else
	if (Info.ffFormat & kfDirect888)
		DefaultRGB(&p->Overlay.Output.Format.Video.Pixel,Info.cBPP,8,8,8,0,0,0);

	if (Info.ffFormat & kfDirectInverted)
		p->Overlay.Output.Format.Video.Pixel.Flags |= PF_INVERTED;

	// get signed x/y pitches in bits
	Info.cbxPitch <<= 3;
	Info.cbyPitch <<= 3;

	// one pitch is probably zero when BPP<8
	if (!Info.cbxPitch) 
		if (Info.cbyPitch < 0)
			Info.cbxPitch = -Info.cBPP;
		else
			Info.cbxPitch = Info.cBPP;
	else
	if (!Info.cbyPitch)
		if (Info.cbxPitch < 0)
			Info.cbyPitch = Info.cBPP;
		else
			Info.cbyPitch = -Info.cBPP;

	// one of the pitches absolute value must be BPP
	if (abs(Info.cbyPitch) < abs(Info.cbxPitch))
	{
		if (abs(Info.cbxPitch) < Info.cyHeight*Info.cBPP &&
			abs(Info.cbxPitch) >= Info.cxWidth*Info.cBPP) //swapped gapi resolution
			SwapInt(&Info.cxWidth,&Info.cyHeight);

		Info.ffFormat |= kfLandscape;
		Info.cbyPitch = Info.cbyPitch<0 ? -Info.cBPP : Info.cBPP;
	}
	else
	{
		if (abs(Info.cbyPitch) < Info.cxWidth*Info.cBPP &&
			abs(Info.cbyPitch) >= Info.cyHeight*Info.cBPP) //swapped gapi resolution
			SwapInt(&Info.cxWidth,&Info.cyHeight);

		Info.ffFormat &= ~kfLandscape;
		Info.cbxPitch = Info.cbxPitch<0 ? -Info.cBPP : Info.cBPP;
	}

	p->Overlay.Output.Format.Video.Width = Info.cxWidth;
	p->Overlay.Output.Format.Video.Height = Info.cyHeight;

	// we need the physical start of the framebuffer
	p->AdjustPtr = 0;
	if (Info.cbxPitch<0) 
		p->AdjustPtr += (Info.cbxPitch * (Info.cxWidth-1)) >> 3;
	if (Info.cbyPitch<0) 
		p->AdjustPtr += (Info.cbyPitch * (Info.cyHeight-1)) >> 3;

	if (Info.ffFormat & kfLandscape)
	{
		p->Overlay.Output.Format.Video.Direction |= DIR_SWAPXY;
		p->Overlay.Output.Format.Video.Pitch = abs(Info.cbxPitch) >> 3;
		SwapInt(&p->Overlay.Output.Format.Video.Width,&p->Overlay.Output.Format.Video.Height);

		if (Info.cbxPitch<0) 
			p->Overlay.Output.Format.Video.Direction |= DIR_MIRRORUPDOWN;
		if (Info.cbyPitch<0) 
			p->Overlay.Output.Format.Video.Direction |= DIR_MIRRORLEFTRIGHT;
	}
	else
	{
		p->Overlay.Output.Format.Video.Pitch = abs(Info.cbyPitch) >> 3;

		if (Info.cbxPitch<0) 
			p->Overlay.Output.Format.Video.Direction |= DIR_MIRRORLEFTRIGHT;
		if (Info.cbyPitch<0) 
			p->Overlay.Output.Format.Video.Direction |= DIR_MIRRORUPDOWN;
	}

	if (!p->DRAM)
	{
		p->Overlay.SetFX = BLITFX_AVOIDTEARING;

		if ((Info.cxWidth > Info.cyHeight) && (Caps & CAPS_PORTRAIT))
			p->Overlay.SetFX |= BLITFX_VMEMROTATED;

		if ((Info.cyHeight > Info.cxWidth) && (Caps & CAPS_LANDSCAPE))
			p->Overlay.SetFX |= BLITFX_VMEMROTATED;
	}
	else
		p->Overlay.ClearFX = BLITFX_ONLYDIFF;

	// wm2003se gapi emulation?
	if (QueryPlatform(PLATFORM_VER) >= 421)
	{
		if (p->Overlay.Output.Format.Video.Width == 240 && p->Overlay.Output.Format.Video.Height == 320 && p->Overlay.Output.Format.Video.Pitch == 640)
			p->Overlay.Output.Format.Video.Pitch = 480;
	}

	if (Info.cxWidth == 240 && Info.cyHeight == 320)
		AdjustOrientation(&p->Overlay.Output.Format.Video,0);

	return ERR_NONE;
}

static void Done(gapi* p)
{
}

static int Reset(gapi* p)
{
	Done(p);
	Init(p);
	return ERR_NONE;
}

static int Lock(gapi* p, planes Planes, bool_t OnlyAligned)
{
	if (!p->Opened)
		return ERR_INVALID_PARAM;

	if (p->DirectPtr)
		p->Pointer = p->DirectPtr + p->AdjustPtr;
	else
	{
		if (p->DRAM && p->GXSetViewport)
		{
			int Top,Height;
			if (OnlyAligned)
			{
				Top = p->Overlay.GUIAlignedRect.y;
				Height = p->Overlay.GUIAlignedRect.Height;
			}
			else
			{
				Top = p->Overlay.Viewport.y;
				Height = p->Overlay.Viewport.Height;
			}

			if (p->Overlay.Output.Format.Video.Pixel.Flags & PF_PIXELDOUBLE)
			{
				Top >>= 1;
				Height >>= 1;
			}

			p->GXSetViewport(Top,Height,0,0); // coords in windows space
		}

		p->Pointer = (uint8_t*)p->GXBeginDraw();
		if (!p->Pointer)
			return ERR_NOT_SUPPORTED;

		p->Pointer += p->AdjustPtr;
	}

	Planes[0] = p->Pointer;
	return ERR_NONE;
}

static int Unlock(gapi* p)
{
	if (!p->DirectPtr)
		p->GXEndDraw();
	return ERR_NONE;
}

static int UpdateWnd(gapi* p)
{
	void* Wnd = Context()->Wnd;
	if (p->Wnd != Wnd)
	{
		p->Wnd = Wnd;
		if (p->Wnd && !p->Opened)
		{
			HWND WndTask;

			while (!p->GX || !p->GXOpenDisplay(p->Wnd,GX_FULLSCREEN))
			{
				if (!p->Windows)
				{
					p->Windows = 1; // try gx.dll in \windows directory (HPC with .NET)
					if (LoadDriver(p))
						continue;
				}
				return ERR_NOT_SUPPORTED;
			}

			// just in case GXOpenDisplay hides these things
			if (p->SHFullScreen)
			{
				p->SHFullScreen(p->Wnd,SHFS_SHOWSTARTICON);
				p->SHFullScreen(p->Wnd,SHFS_SHOWTASKBAR);
			}

			WndTask = FindWindow(T("HHTaskbar"),NULL);
			if (WndTask)
				ShowWindow(WndTask,SW_SHOWNA);

			p->Info = p->GXGetDisplayProperties();
			if (!p->Info.cxWidth || !p->Info.cyHeight) // this shouldn't happen
			{
				p->GXCloseDisplay();
				return ERR_DEVICE_ERROR;
			}

			p->DirectPtr = NULL;
			p->Opened = 1;
			p->DRAM = 0;

			if (p->GXIsDisplayDRAMBuffer)
				p->DRAM = p->GXIsDisplayDRAMBuffer() != 0;

			// detect fake DRAM
			if (p->DRAM && !(p->Info.ffFormat & kfLandscape) && QueryPlatform(PLATFORM_MODEL)==MODEL_SPV_C500_ORIGROM)
			{
				p->GXSetViewport(0,0,0,0);
				p->DirectPtr = p->GXBeginDraw();
			}

			if (QueryPlatform(PLATFORM_VER) >= 421)
				IsOrientationChanged(); // force requery of orientation (GXOpenDisplay changes to portrait)

			Init(p); // we need the output format
		}
		else
		if (p->Opened && !p->Wnd && !p->DirectPtr)
		{
			p->GXCloseDisplay();
			p->Opened = 0;
		}
	}
	return ERR_NONE;
}

static int Set(gapi* p,int No,const void* Data,int Size)
{
	int Result = OverlaySet(&p->Overlay,No,Data,Size);
	switch (No)
	{
	case NODE_SETTINGSCHANGED: 
		Result = UpdateWnd(p);
		break;
	}
	return Result;
}

static int Create(gapi* p)
{
	if (NodeEnumObject(NULL,RAW_ID))
		return ERR_NOT_SUPPORTED;

	p->Overlay.Node.Enum = Enum;
	p->Overlay.Node.Get = Get;
	p->Overlay.Node.Set = Set;
	p->Overlay.Lock = Lock;
	p->Overlay.Unlock = Unlock;
	p->Overlay.Init = Init;
	p->Overlay.Done = Done;
	p->Overlay.Reset = Reset;
	p->Overlay.Node.Get = Get;
	p->Overlay.Node.Enum = Enum;

	if (!LoadDriver(p))
		return ERR_NOT_SUPPORTED;

	p->AygShell = LoadLibrary(T("aygshell.dll"));
	GetProc(&p->AygShell,&p->SHFullScreen,T("SHFullScreen"),1);
	return ERR_NONE;
}

static void Delete(gapi* p)
{
	if (p->GX)
		FreeLibrary(p->GX);
	if (p->AygShell)
		FreeLibrary(p->AygShell);
}

static const nodedef GAPI =
{
	sizeof(gapi)|CF_GLOBAL,
	GAPI_ID,
	OVERLAY_CLASS,
	PRI_DEFAULT+50,
	(nodecreate)Create,
	(nodedelete)Delete,
};

void OverlayGAPI_Init() 
{ 
	NodeRegisterClass(&GAPI);
}

void OverlayGAPI_Done()
{
	NodeUnRegisterClass(GAPI_ID);
}

#endif
