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
 * $Id: ge2demu.c 343 2005-11-16 20:11:07Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 * Big thanks to Mobile Stream (http://www.mobile-stream.com)
 * for reverse enginerring the GE2D interface
 *
 ****************************************************************************/

#define WIN32_LEAN_AND_MEAN
#ifndef STRICT
#define STRICT
#endif
#include <windows.h>
#include "../common/overlay/ddraw.h"

HMODULE Module = NULL;
LPDIRECTDRAW DD = NULL;
HANDLE Thread = NULL;
bool_t ThreadRun = 0;
RECT DDSrc;
RECT DDDst;
int DDLeft,DDTop;
LPDIRECTDRAWSURFACE DDPrimary;
LPDIRECTDRAWSURFACE DDBuffer;

Err GE2DLibOpen()
{
	DDSURFACEDESC Desc;

	HRESULT (WINAPI* DirectDrawCreate)( void*, LPDIRECTDRAW*, void* );
	Module = LoadLibrary(T("DDRAW.DLL"));
	*(FARPROC*)&DirectDrawCreate = GetProcAddress(Module,"DirectDrawCreate");

	if (!DirectDrawCreate)
		return 1;

	if (DirectDrawCreate(NULL,&DD,NULL)!=DD_OK)
		return 1;

	IDirectDraw_SetCooperativeLevel(DD, NULL, DDSCL_NORMAL);

	// get primary surface
	memset(&Desc,0,sizeof(DDSURFACEDESC));
	Desc.dwSize = sizeof(DDSURFACEDESC);
	Desc.dwFlags = DDSD_CAPS;
	Desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
	if (IDirectDraw_CreateSurface(DD,&Desc,&DDPrimary,NULL) != DD_OK)
		return 1;

	return errNone;
}

Err GE2DLibClose()
{
	if (DDPrimary) 
	{ 
		IDirectDrawSurface_Release(DDPrimary); 
		DDPrimary = NULL;
	}
	if (DD)
	{
		IDirectDraw_Release(DD);
		DD = NULL;
	}
	if (Module)
		FreeLibrary(Module);
	return errNone;
}

Err GE2DScaleBitmap(GE2DBitmapType *dstBmpP, GE2DBitmapType *srcBmpP)
{
	int x,y,sn,dn;
	uint8_t *d,*s;
	int dx = (1024 * srcBmpP->width) / dstBmpP->width; 
	int dy = (1024 * srcBmpP->height) / dstBmpP->height; 

	if (dstBmpP->format != sonyGE2DFormatYCbCr420Planar)
		return 1;
	if (srcBmpP->format != sonyGE2DFormatYCbCr420Planar)
		return 1;

	sn = srcBmpP->width * srcBmpP->height;
	dn = dstBmpP->width * dstBmpP->height;

	for (y=0;y<dstBmpP->height;++y)
	{
		d = (uint8_t*)dstBmpP->plane1P + y*dstBmpP->pitch;
		s = (uint8_t*)srcBmpP->plane1P + ((y*dy)/1024)*srcBmpP->pitch;
		for (x=0;x<dstBmpP->width;++x,++d)
			*d = s[(x*dx)/1024];
	}

	for (y=0;y<dstBmpP->height/2;++y)
	{
		d = (uint8_t*)dstBmpP->plane2P + y*dstBmpP->chromaPitch;
		s = (uint8_t*)srcBmpP->plane2P + ((y*dy)/1024)*srcBmpP->chromaPitch;
		for (x=0;x<dstBmpP->width/2;++x,++d)
			*d = s[(x*dx)/1024];
	}

	for (y=0;y<dstBmpP->height/2;++y)
	{
		d = (uint8_t*)dstBmpP->plane3P + y*dstBmpP->chromaPitch;
		s = (uint8_t*)srcBmpP->plane3P + ((y*dy)/1024)*srcBmpP->chromaPitch;
		for (x=0;x<dstBmpP->width/2;++x,++d)
			*d = s[(x*dx)/1024];
	}

	return 0;
}

Err GE2DHideOverlay()
{
	if (Thread)
	{
		ThreadRun = 0;
		if (Thread && WaitForSingleObject(Thread,5000) == WAIT_TIMEOUT)
			TerminateThread(Thread,0);
		Thread = NULL;
	}

	if (DDBuffer)
	{
		IDirectDrawSurface_UpdateOverlay(DDBuffer,&DDSrc,DDPrimary,&DDDst,DDOVER_HIDE,NULL);
		IDirectDrawSurface_Release(DDBuffer); 
		DDBuffer = NULL;
	}

	return 0;
}

static void DDUpdate(GE2DBitmapType *p)
{
	HWND Wnd = FindWindow("PalmOSEmulatorDisplay",NULL);
	POINT Origin = {0,0};
	if (Wnd)
		ClientToScreen(Wnd,&Origin);

	DDSrc.left = 0;
	DDSrc.top = 0;
	DDSrc.right = p->width;
	DDSrc.bottom = p->height;
 
	if (DDDst.left != DDLeft + Origin.x ||
		DDDst.top != DDTop + Origin.y)
	{
		DDDst.left = DDLeft + Origin.x;
		DDDst.top = DDTop + Origin.y;
		DDDst.right = DDDst.left + p->width;
		DDDst.bottom = DDDst.top + p->height;

		IDirectDrawSurface_UpdateOverlay(DDBuffer,&DDSrc,DDPrimary,&DDDst,DDOVER_SHOW,NULL);
	}
}

static Err GE2DBlitBitmap(void *dstP, Coord left, Coord top,
		UInt16 dstWidth, UInt16 dstHeight, GE2DBitmapType* p)
{
	video Src;
	video Dst;
	planes SrcPlanes;
	planes DstPlanes;

	memset(&Src,0,sizeof(Src));
	Src.Pixel.Flags = PF_YUV420;
	Src.Width = p->width;
	Src.Height = p->height;
	Src.Pitch = p->pitch;
	SrcPlanes[0] = p->plane1P;
	SrcPlanes[1] = p->plane3P;
	SrcPlanes[2] = p->plane2P;

	memset(&Dst,0,sizeof(Dst));
	DefaultRGB(&Dst.Pixel,16,5,6,5,0,0,0);
	Dst.Width = p->width;
	Dst.Height = p->height;
	Dst.Pitch = dstWidth*2;
	DstPlanes[0] = (char*)dstP + top * Dst.Pitch + left * 2;

	SurfaceCopy(&Src,&Dst,SrcPlanes,DstPlanes,NULL);
	return 0;
}

static void DDCopy(GE2DBitmapType *p)
{
	DDSURFACEDESC Desc;
	memset(&Desc,0,sizeof(DDSURFACEDESC));
	Desc.dwSize = sizeof(DDSURFACEDESC);

	if (IDirectDrawSurface_Lock(DDBuffer,NULL,&Desc,DDLOCK_WAIT,NULL) == DD_OK)
	{
		int y,n;
		uint8_t* i = (uint8_t*)Desc.lpSurface;
		n = Desc.lPitch*p->height;
		for (y=0;y<p->height;++y)
			memcpy(i+y*Desc.lPitch,(char*)p->plane1P+p->pitch*y,p->width);
		for (y=0;y<(p->height/2);++y)
		{
			memcpy(i+n+y*(Desc.lPitch/2),(char*)p->plane2P+p->chromaPitch*y,p->width/2);
			memcpy(i+n+n/4+y*(Desc.lPitch/2),(char*)p->plane3P+p->chromaPitch*y,p->width/2);
		}
		IDirectDrawSurface_Unlock(DDBuffer,NULL);
	}
}

static int ThreadCopy(void* p)
{
	while (ThreadRun)
	{
		Sleep(100);
		if (!ThreadRun)
			break;
		DDCopy((GE2DBitmapType*)p);
		DDUpdate((GE2DBitmapType*)p);
	}
	return 0;
}

Err GE2DShowOverlay(Coord left, Coord top, GE2DBitmapType *p)
{
	DWORD Id;
	DDSURFACEDESC Desc;

	if (p->format != sonyGE2DFormatYCbCr420Planar)
		return 1;

	GE2DHideOverlay();

	memset(&Desc,0,sizeof(DDSURFACEDESC));
	Desc.dwSize = sizeof(DDSURFACEDESC);
	Desc.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_PIXELFORMAT;
	Desc.dwWidth = p->width;
	Desc.dwHeight = p->height;
	Desc.ddpfPixelFormat.dwSize = sizeof(Desc.ddpfPixelFormat);
	Desc.ddpfPixelFormat.dwFlags = DDPF_FOURCC;
	Desc.ddpfPixelFormat.dwFourCC = FOURCC_YV12;
	Desc.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY | DDSCAPS_OVERLAY;

	if (IDirectDraw_CreateSurface(DD,&Desc,&DDBuffer,NULL) != DD_OK)
		return 1;

	memset(&DDDst,-1,sizeof(DDDst));
	DDTop = top;
	DDLeft = left;

	DDCopy(p);
	DDUpdate(p);

	ThreadRun = 1;
	Thread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)ThreadCopy,p,0,&Id);
	return 0;
}

Err GE2DProgramBlitBitmap(GE2DInsnType *programP,void *dstP, Coord left, Coord top,
	UInt16 dstWidth, UInt16 dstHeight, GE2DBitmapType *bmpP)
{
	programP[0] = (GE2DInsnType)dstP;
	programP[1] = left;
	programP[2] = top;
	programP[3] = dstWidth;
	programP[4] = dstHeight;
	programP[5] = (GE2DInsnType)bmpP;
	return 0;
}

Err GE2DRunProgram(const GE2DInsnType *programP)
{
	return GE2DBlitBitmap((void*)programP[0],(Coord)programP[1],(Coord)programP[2],
		(UInt16)programP[3],(UInt16)programP[4],(GE2DBitmapType*)programP[5]);
}

static GE2DLibAPIType API = 
{
	GE2DLibOpen,
	GE2DLibClose,
	NULL,
	NULL,
	NULL,
	NULL,
	GE2DBlitBitmap,
	NULL,
	NULL,
	GE2DProgramBlitBitmap,
	GE2DScaleBitmap,
	GE2DShowOverlay,
	GE2DHideOverlay,
	GE2DRunProgram,
	GE2DRunProgram,
};

