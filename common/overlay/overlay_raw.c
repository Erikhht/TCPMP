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
 * $Id: overlay_raw.c 543 2006-01-07 22:06:24Z picard $
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

typedef struct raw
{
	overlay Overlay;
	void* Ptr;
	RawFrameBufferInfo Info;

} raw;

static const datatable Params[] = 
{
	{ RAW_WIDTH,	TYPE_INT, DF_SETUP | DF_RDONLY | DF_HIDDEN },
	{ RAW_HEIGHT,	TYPE_INT, DF_SETUP | DF_RDONLY | DF_HIDDEN },
	{ RAW_PITCHX,	TYPE_INT, DF_SETUP | DF_RDONLY | DF_HIDDEN },
	{ RAW_PITCHY,	TYPE_INT, DF_SETUP | DF_RDONLY | DF_HIDDEN },
	{ RAW_BPP,		TYPE_INT, DF_SETUP | DF_RDONLY | DF_HIDDEN },
	{ RAW_FORMAT,	TYPE_INT, DF_SETUP | DF_RDONLY | DF_HIDDEN },
	{ RAW_POINTER,	TYPE_INT, DF_SETUP | DF_RDONLY | DF_HEX | DF_HIDDEN },

	DATATABLE_END(RAW_ID),
};

static bool_t GetRawFrameBuffer(RawFrameBufferInfo* Info)
{
	HDC DC = GetDC(NULL);
	memset(Info,0,sizeof(RawFrameBufferInfo));

	ExtEscape(DC, GETRAWFRAMEBUFFER, 0, NULL, sizeof(RawFrameBufferInfo), (char*)Info);
	if (!Info->pFramePointer && QueryPlatform(PLATFORM_VER) >= 421)
	{
		//try gxinfo
		DWORD Code = GETGXINFO;
		if (ExtEscape(DC, ESC_QUERYESCSUPPORT, sizeof(DWORD), (char*)&Code, 0, NULL) > 0)
		{
			DWORD DCWidth = GetDeviceCaps(DC,HORZRES);
			DWORD DCHeight = GetDeviceCaps(DC,VERTRES);
			GXDeviceInfo GXInfo;
			memset(&GXInfo,0,sizeof(GXInfo));
			GXInfo.Version = 100;
			ExtEscape(DC, GETGXINFO, 0, NULL, sizeof(GXInfo), (char*)&GXInfo);

			// avoid VGA devices (or QVGA smartphones emulating 176x220)
			if (GXInfo.cbStride>0 && !(GXInfo.ffFormat & kfLandscape) &&
				((DCWidth == GXInfo.cxWidth && DCHeight == GXInfo.cyHeight) ||
				 (DCWidth == GXInfo.cyHeight && DCHeight == GXInfo.cxWidth)))
			{
				bool_t Detect = 0;
				TRY_BEGIN
				{
					int* p = (int*)GXInfo.pvFrameBuffer;
					COLORREF Old = GetPixel(DC,0,0);
					*p ^= -1;
					Detect = GetPixel(DC,0,0) != Old;
					*p ^= -1;
				}
				TRY_END

				if (Detect)
				{
					Info->pFramePointer = GXInfo.pvFrameBuffer;
					Info->cxPixels = GXInfo.cxWidth;
					Info->cyPixels = GXInfo.cyHeight;
					Info->cxStride = GXInfo.cBPP/8;
					Info->cyStride = GXInfo.cbStride;
					Info->wBPP = (WORD)GXInfo.cBPP;
					Info->wFormat = (WORD)GXInfo.ffFormat;
				}
			}
		}
	}

	ReleaseDC(NULL,DC);
	return Info->pFramePointer != NULL;
}

static int Enum(raw* p, int* No, datadef* Param)
{
	if (OverlayEnum(&p->Overlay,No,Param)==ERR_NONE)
		return ERR_NONE;
	return NodeEnumTable(No,Param,Params);
}

static int Get(raw* p,int No,void* Data,int Size)
{
	int Result = OverlayGet(&p->Overlay,No,Data,Size);
	switch (No)
	{
	case RAW_WIDTH: GETVALUE(p->Info.cxPixels,int); break;
	case RAW_HEIGHT: GETVALUE(p->Info.cyPixels,int); break;
	case RAW_PITCHX: GETVALUE(p->Info.cxStride,int); break;
	case RAW_PITCHY: GETVALUE(p->Info.cyStride,int); break;
	case RAW_BPP: GETVALUE(p->Info.wBPP,int); break;
	case RAW_FORMAT: GETVALUE(p->Info.wFormat,int); break;
	case RAW_POINTER: GETVALUE((int)p->Info.pFramePointer,int); break;
	}
	return Result;
}

static int Init(raw* p)
{
	RawFrameBufferInfo Info;

	if (!GetRawFrameBuffer(&p->Info))
		return ERR_DEVICE_ERROR;

	memcpy(&Info,&p->Info,sizeof(Info));

	if (abs(Info.cyStride) < abs(Info.cxStride))
	{
		if (abs(Info.cxStride)*8 < Info.cyPixels*Info.wBPP &&
			abs(Info.cxStride)*8 >= Info.cxPixels*Info.wBPP) //swapped resolution
			SwapInt(&Info.cxPixels,&Info.cyPixels);
	}
	else
	{
		if (abs(Info.cyStride)*8 < Info.cxPixels*Info.wBPP &&
			abs(Info.cyStride)*8 >= Info.cyPixels*Info.wBPP) //swapped resolution
			SwapInt(&Info.cxPixels,&Info.cyPixels);
	}

	memset(&p->Overlay.Output.Format.Video,0,sizeof(video));
	p->Overlay.Output.Format.Video.Width = Info.cxPixels;
	p->Overlay.Output.Format.Video.Height = Info.cyPixels;
	p->Overlay.Output.Format.Video.Direction = 0;
	p->Overlay.Output.Format.Video.Aspect = ASPECT_ONE;

	switch (Info.wFormat)
	{
	case kfDirect555:
	case kfDirect|kfDirect555:
	case kfDirect|kfDirect555|kfLandscape:
	case FORMAT_555:
		DefaultRGB(&p->Overlay.Output.Format.Video.Pixel,Info.wBPP,5,5,5,0,0,0);
		break;
	case kfDirect565:
	case kfDirect|kfDirect565:
	case kfDirect|kfDirect565|kfLandscape:
	case FORMAT_565:
		DefaultRGB(&p->Overlay.Output.Format.Video.Pixel,Info.wBPP,5,6,5,0,0,0);
		break;
	default:
		return ERR_DEVICE_ERROR;
	}

	// we need the physical start of the framebuffer
	if (Info.cxStride<0) 
		Info.pFramePointer += (Info.cxStride * (Info.cxPixels-1));
	if (Info.cyStride<0) 
		Info.pFramePointer += (Info.cyStride * (Info.cyPixels-1));

	if (abs(Info.cyStride) < abs(Info.cxStride))
	{
		p->Overlay.Output.Format.Video.Direction |= DIR_SWAPXY;
		p->Overlay.Output.Format.Video.Pitch = abs(Info.cxStride);
		SwapInt(&p->Overlay.Output.Format.Video.Width,&p->Overlay.Output.Format.Video.Height);

		if (Info.cxStride<0) 
			p->Overlay.Output.Format.Video.Direction |= DIR_MIRRORUPDOWN;
		if (Info.cyStride<0) 
			p->Overlay.Output.Format.Video.Direction |= DIR_MIRRORLEFTRIGHT;
	}
	else
	{
		p->Overlay.Output.Format.Video.Pitch = abs(Info.cyStride);

		if (Info.cxStride<0) 
			p->Overlay.Output.Format.Video.Direction |= DIR_MIRRORLEFTRIGHT;
		if (Info.cyStride<0) 
			p->Overlay.Output.Format.Video.Direction |= DIR_MIRRORUPDOWN;
	}

	if (((int)Info.pFramePointer & 15)==0 && (p->Overlay.Output.Format.Video.Pitch & 15)==0)
		p->Overlay.Output.Format.Video.Pixel.Flags |= PF_16ALIGNED;

	p->Ptr = Info.pFramePointer;

	if (p->Overlay.Output.Format.Video.Direction & DIR_SWAPXY)
		p->Overlay.SetFX |= BLITFX_VMEMROTATED;
	p->Overlay.SetFX = BLITFX_AVOIDTEARING;
	AdjustOrientation(&p->Overlay.Output.Format.Video,1);
	return ERR_NONE;
}

static void Done(raw* p)
{
}

static int Reset(raw* p)
{
	Done(p);
	Init(p);
	return ERR_NONE;
}

static int Lock(raw* p, planes Planes, bool_t OnlyAligned)
{
	Planes[0] = p->Ptr;
	return ERR_NONE;
}

static int Unlock(raw* p)
{
	return ERR_NONE;
}

static int Create(raw* p)
{
	// skip when GAPI is already available (but not for QVGA smartphones)
	if (NodeEnumObject(NULL,GAPI_ID) &&  
		GetSystemMetrics(SM_CXSCREEN)*GetSystemMetrics(SM_CYSCREEN) < 320*240)
		return ERR_NOT_SUPPORTED;

	p->Overlay.Node.Enum = Enum;
	p->Overlay.Node.Get = Get;
	p->Overlay.Init = Init;
	p->Overlay.Done = Done;
	p->Overlay.Reset = Reset;
	p->Overlay.Lock = Lock;
	p->Overlay.Unlock = Unlock;
	if (Init(p) != ERR_NONE) // check if supported
		return ERR_NOT_SUPPORTED;
	Done(p);
	return ERR_NONE;
}

static const nodedef RAW =
{
	sizeof(raw)|CF_GLOBAL, //no setup
	RAW_ID,
	OVERLAY_CLASS,
	PRI_DEFAULT+95,
	(nodecreate)Create,
};

void OverlayRAW_Init() 
{ 
	NodeRegisterClass(&RAW); 
}

void OverlayRAW_Done()
{
	NodeUnRegisterClass(RAW_ID);
}

#endif
