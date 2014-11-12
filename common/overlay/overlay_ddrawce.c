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
 * $Id: overlay_ddrawce.c 615 2006-01-26 16:57:51Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#ifndef STRICT
#define STRICT
#endif
#include <windows.h>

#if _MSC_VER > 1000
#pragma warning( push, 3 )
#endif

#include "ddrawce.h"
#include "overlay_ddraw.h"

static int AdjustDesc( DDSURFACEDESC* Desc )
{
	int Dir = 0;

	if (abs(Desc->lPitch) < abs(Desc->lXPitch))
	{
		DWORD Tmp;
		
		Tmp = Desc->dwWidth;
		Desc->dwWidth = Desc->dwHeight;
		Desc->dwHeight = Tmp;

		Tmp = Desc->lPitch;
		Desc->lPitch = Desc->lXPitch;
		Desc->lXPitch = Tmp;

		Dir |= DIR_SWAPXY;
	}

	if (Desc->lXPitch < 0)
	{
		Dir |= DIR_MIRRORLEFTRIGHT;
		Desc->lpSurface = (char*)Desc->lpSurface + Desc->lXPitch * (Desc->dwWidth-1);
		Desc->lXPitch = -Desc->lXPitch;
	}
	if (Desc->lPitch < 0)
	{
		Dir |= DIR_MIRRORUPDOWN;
		Desc->lpSurface = (char*)Desc->lpSurface + Desc->lPitch * (Desc->dwHeight-1);
		Desc->lPitch = -Desc->lPitch;
	}

	return Dir;
}

static void Desc2Surface( DDSURFACEDESC* Desc, video* p )
{
	memset(p,0,sizeof(video));
	p->Direction = AdjustDesc(Desc);
	p->Aspect = ASPECT_ONE;
	p->Width = Desc->dwWidth;
	p->Height = Desc->dwHeight;

	if (Desc->ddpfPixelFormat.dwFlags & DDPF_FOURCC)
	{
		p->Pixel.Flags = PF_FOURCC;
		p->Pixel.FourCC = Desc->ddpfPixelFormat.dwFourCC;
	}
	else
	if (Desc->ddpfPixelFormat.dwFlags & DDPF_RGB)
	{
		p->Pixel.Flags = PF_RGB;
		p->Pixel.BitCount = Desc->ddpfPixelFormat.dwRGBBitCount;
		p->Pixel.BitMask[0] = Desc->ddpfPixelFormat.dwRBitMask;
		p->Pixel.BitMask[1] = Desc->ddpfPixelFormat.dwGBitMask;
		p->Pixel.BitMask[2] = Desc->ddpfPixelFormat.dwBBitMask;
	}
	else
	if (Desc->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED)
	{
		p->Pixel.Flags = PF_PALETTE;
		p->Pixel.BitCount = 8;
	}
}

static bool_t Surface2Desc( const video* p, DDSURFACEDESC* Desc, bool_t PixelFormat )
{
	memset(Desc,0,sizeof(DDSURFACEDESC));
	Desc->dwSize = sizeof(DDSURFACEDESC);
	Desc->dwFlags = DDSD_WIDTH | DDSD_HEIGHT;

	Desc->dwWidth = p->Width;
	Desc->dwHeight = p->Height;

	if (PixelFormat)
	{
		Desc->dwFlags |= DDSD_PIXELFORMAT;
		Desc->ddpfPixelFormat.dwSize = sizeof(Desc->ddpfPixelFormat);

		if (p->Pixel.Flags & PF_FOURCC)
		{
			Desc->ddpfPixelFormat.dwFlags = DDPF_FOURCC;
			Desc->ddpfPixelFormat.dwFourCC = p->Pixel.FourCC;
		}
		else
		if (p->Pixel.Flags & PF_RGB)
		{
			Desc->ddpfPixelFormat.dwFlags = DDPF_RGB;
			Desc->ddpfPixelFormat.dwRGBBitCount = p->Pixel.BitCount;
			Desc->ddpfPixelFormat.dwRBitMask = p->Pixel.BitMask[0];
			Desc->ddpfPixelFormat.dwGBitMask = p->Pixel.BitMask[1];
			Desc->ddpfPixelFormat.dwBBitMask = p->Pixel.BitMask[2];
		}
		else
		if (p->Pixel.Flags & PF_PALETTE && p->Pixel.BitCount==8)
		{
			Desc->ddpfPixelFormat.dwFlags = DDPF_PALETTEINDEXED;
			Desc->ddpfPixelFormat.dwRGBBitCount = p->Pixel.BitCount;
		}
		else
			return 0;
	}

	return 1;
}

static bool_t ClearBuffer(ddraw* p,LPDIRECTDRAWSURFACE Buffer)
{
	bool_t Result = 0;
	DDSURFACEDESC Desc;
	Desc.dwSize = sizeof(Desc);

	TRY_BEGIN
	if (IDirectDrawSurface_Lock(p->DDBuffer,NULL,&Desc,DDLOCK_WAITNOTBUSY,NULL) == DD_OK)
	{
		int v,x2,y2,pitch2;

		AdjustDesc(&Desc);
		FillInfo(&p->Overlay.Pixel);
		v = RGBToFormat(CRGB(0,0,0),&p->Overlay.Pixel);

		TRY_BEGIN
		if (PlanarYUV(&p->Overlay.Pixel,&x2,&y2,&pitch2) && p->Overlay.Pixel.BitCount==8)
		{
			uint8_t* i = (uint8_t*)Desc.lpSurface;
			FillColor(i,Desc.lPitch,0,0,p->Overlay.Width,p->Overlay.Height,8,v & 255);
			i += Desc.lPitch * p->Overlay.Height;
			FillColor(i,Desc.lPitch >> pitch2,0,0,p->Overlay.Width >> x2,p->Overlay.Height >> y2,8,(v>>8)&255);
			i += (Desc.lPitch >> x2) * (p->Overlay.Height >> y2);
			FillColor(i,Desc.lPitch >> pitch2,0,0,p->Overlay.Width >> x2,p->Overlay.Height >> y2,8,(v>>16)&255);
		}
		else
		if (PackedYUV(&p->Overlay.Pixel) && p->Overlay.Pixel.BitCount==16)
			FillColor(Desc.lpSurface,Desc.lPitch,0,0,p->Overlay.Width >> 1,p->Overlay.Height,32,v);
		else
			FillColor(Desc.lpSurface,Desc.lPitch,0,0,p->Overlay.Width,p->Overlay.Height,p->Overlay.Pixel.BitCount,v);
		Result = 1;
		TRY_END

		IDirectDrawSurface_Unlock(p->DDBuffer,NULL);
	}
	TRY_END
	return Result;
}

static HRESULT PASCAL EnumBackBuffer(LPDIRECTDRAWSURFACE p, LPDDSURFACEDESC Desc, LPVOID Context)
{
    if (*((LPDIRECTDRAWSURFACE *)Context)) 
	{
		IDirectDrawSurface_Release(p);
		return DDENUMRET_CANCEL;
	}
	*((LPDIRECTDRAWSURFACE *)Context) = p;
	return DDENUMRET_OK;
}

static void ReleaseBuffers(ddraw* p)
{
	if (p->DDBackBuffer) 
	{ 
		IDirectDrawSurface_Release(p->DDBackBuffer); 
		p->DDBackBuffer = NULL;
	}

	if (p->DDBuffer) 
	{ 
		if (p->Mode == MODE_OVERLAY)
			IDirectDrawSurface_UpdateOverlay(p->DDBuffer,NULL,p->DDPrimary,NULL,DDOVER_HIDE,NULL);
		IDirectDrawSurface_Release(p->DDBuffer); 
		p->DDBuffer = NULL;
	}
}

static bool_t CreateBuffer(ddraw* p,bool_t PixelFormat)
{
	DDSURFACEDESC Desc;

	p->BufferPixelFormat = PixelFormat;

	ReleaseBuffers(p);

	if (!Surface2Desc(&p->Overlay,&Desc,PixelFormat))
		return 0;

	Desc.dwFlags |= DDSD_CAPS;
	Desc.ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY;
	if (p->Mode == MODE_OVERLAY)
	{
		Desc.ddsCaps.dwCaps |= DDSCAPS_OVERLAY;
		Desc.ddsCaps.dwCaps |= DDSCAPS_FLIP;
		Desc.dwFlags |= DDSD_BACKBUFFERCOUNT;
		Desc.dwBackBufferCount = 1;
	}

	DEBUG_MSG5(DEBUG_VIDEO,T("DDRAW CreateSurface %08x %dx%d %08x %08x"),Desc.ddsCaps.dwCaps,Desc.dwWidth,Desc.dwHeight,Desc.ddpfPixelFormat.dwFlags,Desc.ddpfPixelFormat.dwFourCC);

	if (IDirectDraw_CreateSurface(p->DD,&Desc,&p->DDBuffer,NULL) != DD_OK)
	{
		if (p->Mode == MODE_OVERLAY)
		{
			// try without backbuffer...
			Desc.ddsCaps.dwCaps &= ~(DDSCAPS_FLIP);
			Desc.dwFlags &= ~DDSD_BACKBUFFERCOUNT;
			IDirectDraw_CreateSurface(p->DD,&Desc,&p->DDBuffer,NULL);
		}

		if (!p->DDBuffer)
		{
			DEBUG_MSG(DEBUG_VIDEO,T("DDRAW CreateSurface Failed"));
			return 0;
		}
	}
	else
	if (p->Mode == MODE_OVERLAY)
		IDirectDrawSurface_EnumAttachedSurfaces(p->DDBuffer,&p->DDBackBuffer,EnumBackBuffer);

	if (IDirectDrawSurface_GetSurfaceDesc(p->DDBuffer,&Desc) == DD_OK)
		Desc2Surface(&Desc,&p->Overlay);

	DEBUG_MSG(DEBUG_VIDEO,T("DDRAW CreateSurface Ok"));

	if (!ClearBuffer(p,p->DDBuffer))
	{
		ReleaseBuffers(p);
		return 0;
	}

	if (p->DDBackBuffer)
		ClearBuffer(p,p->DDBackBuffer);

	return 1;
}

static int UpdateOverlay(ddraw* p)
{
	rect OverlayRect;
	rect DstAlignedRect;

	DstAlignedRect = p->p.GUIAlignedRect;
	PhyToVirt(&p->OverlayRect,&OverlayRect,&p->Overlay);

	p->Src.left = OverlayRect.x;
	p->Src.top = OverlayRect.y;
	p->Src.right = OverlayRect.x + OverlayRect.Width;
	p->Src.bottom = OverlayRect.y + OverlayRect.Height;
 
	p->Dst.left = DstAlignedRect.x;
	p->Dst.top = DstAlignedRect.y;
	p->Dst.right = DstAlignedRect.x + DstAlignedRect.Width;
	p->Dst.bottom = DstAlignedRect.y + DstAlignedRect.Height;

	if (p->Mode == MODE_OVERLAY && p->DDBuffer &&
		OverlayRect.Width>0 && OverlayRect.Height>0 &&
		DstAlignedRect.Width>0 && DstAlignedRect.Height>0)
	{
		DWORD hResult;
		int Flags = 0;

		if (p->p.Show)
			Flags |= DDOVER_SHOW;
		else
			Flags |= DDOVER_HIDE;

		if (p->p.ColorKey != RGB_NULL)
			Flags |= DDOVER_KEYDEST;

		hResult = IDirectDrawSurface_UpdateOverlay(p->DDBuffer,&p->Src,p->DDPrimary,&p->Dst,Flags,NULL);
		if (hResult != DD_OK)
		{
			DEBUG_MSG1(DEBUG_VIDEO,T("DDRAW UpdateOverlay failed %08x"),hResult);
		}
	}

	return ERR_NONE;
}

static int GetAlign(int i)
{
	int v=1;
	while (v<16 && v<i)
		v<<=1;
	return v;
}

static int Init(ddraw* p)
{
	DDSURFACEDESC Desc;

	p->p.ColorKey = RGB_NULL;
	if (p->SetupColorKey && IsWindow(Context()->Wnd))
		p->p.ColorKey = COLORKEY;

	if (p->DDCaps.dwMinOverlayStretch && p->DDCaps.dwMaxOverlayStretch)
	{
		p->MinScale = (p->DDCaps.dwMinOverlayStretch * SCALE_ONE) / 1000;
		p->MaxScale = (p->DDCaps.dwMaxOverlayStretch * SCALE_ONE) / 1000;
	}
	else
	{
		p->MinScale = SCALE_ONE;
		p->MaxScale = SCALE_ONE;
	}
	p->DstAlignPos = GetAlign(p->DDCaps.dwAlignBoundaryDest);
	p->DstAlignSize = GetAlign(p->DDCaps.dwAlignSizeDest);

	// get primary surface
	memset(&Desc,0,sizeof(DDSURFACEDESC));
	Desc.dwSize = sizeof(DDSURFACEDESC);
	Desc.dwFlags = DDSD_CAPS;
	Desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
	if (IDirectDraw_CreateSurface(p->DD,&Desc,&p->DDPrimary,NULL) != DD_OK)
		return ERR_DEVICE_ERROR;

	if (IDirectDrawSurface_GetSurfaceDesc(p->DDPrimary,&Desc) != DD_OK)
		return ERR_DEVICE_ERROR;

	//{ int i; for (i=0;i<sizeof(Desc)/4;++i)
	//	DEBUG_MSG(DEBUG_VIDEO,T("DDRAW PrimaryDesc %02x:%08x"),i*4,((int*)&Desc)[i]); }

	Desc2Surface(&Desc,&p->p.Output.Format.Video);
	FillInfo(&p->p.Output.Format.Video.Pixel);

	//{ int i; for (i=0;i<sizeof(p->p.Output.Format.Video)/4;++i)
	//	DEBUG_MSG(DEBUG_VIDEO,T("DDRAW Output %02x:%08x"),i*4,((int*)&p->p.Output.Format.Video)[i]); }

	p->p.Overlay = 1;
	p->p.UpdateShow = UpdateOverlay;
	p->Overlay = p->p.Input.Format.Video;
	p->Mode = MODE_OVERLAY;

	// first try the optimal overlay format
	if (!p->SetupBlit && (p->Format != DDRAWFORMAT_AUTO || !CreateBuffer(p,1)))
	{
		if (PlanarYUV(&p->Overlay.Pixel,NULL,NULL,NULL))
		{
			// try other formats
			static const uint32_t FourCC[] = 
			{ 
				// prefer planar420 formats
				FOURCC_YV12, DDRAWFORMAT_YV12, 
				FOURCC_IYUV, DDRAWFORMAT_YV12,
				FOURCC_I420, DDRAWFORMAT_YV12,
				FOURCC_IMC2, DDRAWFORMAT_YV12, 
				FOURCC_IMC4, DDRAWFORMAT_YV12, 
				// next planar422 formats
				FOURCC_YV16, DDRAWFORMAT_YV12,
				// next packed formats
				FOURCC_YUY2, DDRAWFORMAT_YUY2, 
				FOURCC_YUNV, DDRAWFORMAT_YUY2,  
				FOURCC_V422, DDRAWFORMAT_YUY2,  
				FOURCC_YUYV, DDRAWFORMAT_YUY2,  
				FOURCC_YVYU, DDRAWFORMAT_YUY2,  
				FOURCC_UYVY, DDRAWFORMAT_YUY2,  
				FOURCC_Y422, DDRAWFORMAT_YUY2,  
				FOURCC_UYNV, DDRAWFORMAT_YUY2,
				0 
			};
			const uint32_t* i;
			for (i=FourCC;i[0];i+=2)
			{
				p->Overlay.Pixel.Flags = PF_FOURCC;
				p->Overlay.Pixel.FourCC = i[0];
				if ((p->Format == DDRAWFORMAT_AUTO || p->Format == (int)i[1]) && CreateBuffer(p,1))
					break;
			}
		}

		// last hope is the device's current RGB mode (still better as blit mode)
		if (!p->DDBuffer && (p->Format == DDRAWFORMAT_AUTO || p->Format == DDRAWFORMAT_RGB))
			CreateBuffer(p,0);
	}

	if (!p->DDBuffer)
	{
		// try blit mode

		p->p.Overlay = 0;
		p->p.UpdateShow = NULL;
		p->p.ColorKey = RGB_NULL;
		p->DstAlignPos = 1;
		p->DstAlignSize = 1;
		p->MinScale = SCALE_ONE/256;
		p->MaxScale = SCALE_ONE*256;

		if (p->SetupBlitStretch)
		{
			p->Mode = MODE_BLIT; 
			CreateBuffer(p,0);
		}

		if (!p->DDBuffer)
		{
			p->Mode = MODE_PRIMARY; // use primary mode
			DEBUG_MSG(DEBUG_VIDEO,T("DDRAW Blit Primary Mode"));
		}
		else
		{
			DEBUG_MSG(DEBUG_VIDEO,T("DDRAW Blit Stretch Mode"));
		}
	}
	else
	{
		DEBUG_MSG(DEBUG_VIDEO,T("DDRAW Overlay Mode"));
	}

	if (p->Mode != MODE_BLIT)
		p->p.SetFX = BLITFX_AVOIDTEARING;

	return ERR_NONE;
}

static void Done(ddraw* p)
{
	ReleaseBuffers(p);

	if (p->DDPrimary) 
	{ 
		IDirectDrawSurface_Release(p->DDPrimary); 
		p->DDPrimary = NULL;
	}
}

static int Reset(ddraw* p)
{
	Done(p);
	Init(p);
	return ERR_NONE;
}

static int Update(ddraw* p)
{
	rect GUIOverlayRect;
	int OvlWidth = p->p.Input.Format.Video.Width;
	int OvlHeight = p->p.Input.Format.Video.Height;

	if (p->Mode == MODE_PRIMARY)
		return OverlayUpdateAlign(&p->p);

	p->OvlFX = p->p.FX;
	p->SoftFX = p->p.FX;

	p->OvlFX.Brightness = 0;
	p->OvlFX.Contrast = 0;
	p->OvlFX.Saturation = 0;
	p->OvlFX.RGBAdjust[0] = p->OvlFX.RGBAdjust[1] = p->OvlFX.RGBAdjust[2] = 0;
	p->OvlFX.Direction &= ~(DIR_SWAPXY|DIR_MIRRORLEFTRIGHT|DIR_MIRRORUPDOWN); //rotate handled by SoftFX

	p->SoftFX.ScaleX = SCALE_ONE; // scale handled by overlay or blit
	p->SoftFX.ScaleY = SCALE_ONE;

	if (p->SoftFX.Direction & DIR_SWAPXY)
		SwapInt(&p->OvlFX.ScaleX,&p->OvlFX.ScaleY);

	if (CombineDir(p->p.InputDirection,p->p.OrigFX.Direction,p->Overlay.Direction) & DIR_SWAPXY)
		SwapInt(&OvlWidth,&OvlHeight);

	if (p->Overlay.Width != OvlWidth ||	p->Overlay.Height != OvlHeight)
	{
		p->Overlay.Direction = 0;
		p->Overlay.Width = p->p.Input.Format.Video.Width;
		p->Overlay.Height = p->p.Input.Format.Video.Height;
		if (CombineDir(p->p.InputDirection,p->p.OrigFX.Direction,0) & DIR_SWAPXY)
			SwapInt(&p->Overlay.Width,&p->Overlay.Height);

		if (!CreateBuffer(p,p->BufferPixelFormat))
		{
			p->Overlay.Width = 0;
			p->Overlay.Height = 0;
			return ERR_OUT_OF_MEMORY;
		}
	}

	p->SoftFX.Direction = CombineDir(p->p.InputDirection,p->p.OrigFX.Direction,p->Overlay.Direction);

	p->p.GUIAlignedRect = p->p.Viewport;
	PhyToVirt(NULL,&GUIOverlayRect,&p->Overlay);

	AnyAlign(&p->p.GUIAlignedRect, &GUIOverlayRect, &p->OvlFX,
		p->DstAlignSize,p->DstAlignPos,p->MinScale,p->MaxScale);

	VirtToPhy(&p->p.GUIAlignedRect,&p->p.DstAlignedRect,&p->p.Output.Format.Video);
	VirtToPhy(NULL,&p->p.SrcAlignedRect,&p->p.Input.Format.Video);
	VirtToPhy(&GUIOverlayRect,&p->OverlayRect,&p->Overlay);

	BlitRelease(p->p.Soft);
	p->p.Soft = BlitCreate(&p->Overlay,&p->p.Input.Format.Video,&p->SoftFX,&p->p.Caps);

	BlitAlign(p->p.Soft,&p->OverlayRect,&p->p.SrcAlignedRect);

	if (p->p.ColorKey!=RGB_NULL && p->DDPrimary)
	{ 
		DWORD hResult;
		DDCOLORKEY Key;

		Key.dwColorSpaceLowValue = RGBToFormat(p->p.ColorKey,&p->p.Output.Format.Video.Pixel);
		Key.dwColorSpaceHighValue = 0;

		WinInvalidate(&p->p.Viewport,1);

		if ((hResult = IDirectDrawSurface_SetColorKey(p->DDPrimary,DDCKEY_DESTOVERLAY,&Key))!=DD_OK)
		{
			DEBUG_MSG1(DEBUG_VIDEO,T("DDRAW SetColorKey failed %08x"),hResult);
			return ERR_NOT_SUPPORTED;
		}
	}

	return UpdateOverlay(p);
}

static int Blit(ddraw* p, const constplanes Data, const constplanes DataLast )
{
	DDSURFACEDESC Desc;
	HRESULT hResult;
	planes Planes;
	LPDIRECTDRAWSURFACE Output = p->Mode == MODE_PRIMARY ? p->DDPrimary:p->DDBuffer;

	if (p->Mode == MODE_OVERLAY && p->DDBackBuffer && p->p.CurrTime!=TIME_BENCH)
		Output = p->DDBackBuffer;

	if (!Output)
	{
		DEBUG_MSG(DEBUG_VIDEO,T("DDRAW Blit no output"));
		return ERR_NOT_SUPPORTED;
	}

	Desc.dwSize = sizeof(Desc);
	while ((hResult = IDirectDrawSurface_Lock(Output,NULL,&Desc,DDLOCK_WAITNOTBUSY,NULL)) != DD_OK)
	{
		if (hResult != DDERR_SURFACELOST || IDirectDrawSurface_Restore(Output) != DD_OK)
		{
			DEBUG_MSG(DEBUG_VIDEO,T("DDRAW surface Lock() failed"));
			return ERR_NOT_SUPPORTED;
		}
	}

	//{ int i; for (i=0;i<sizeof(Desc)/4;++i)
	//	DEBUG_MSG(DEBUG_VIDEO,T("DDRAW LockDesc %02x:%08x"),i*4,((int*)&Desc)[i]); }

	AdjustDesc(&Desc);
	Planes[0] = Desc.lpSurface;

	DEBUG_MSG4(DEBUG_VIDEO,T("DDRAW Blit %08x,%08x,%08x,%08x"),p->p.Soft,Planes[0],Desc.lPitch,Data);

	BlitImage(p->p.Soft,Planes,Data,DataLast,Desc.lPitch,-1);

	IDirectDrawSurface_Unlock(Output,NULL);

	if (p->Mode == MODE_BLIT)
	{
		while ((hResult = IDirectDrawSurface_Blt(p->DDPrimary,&p->Dst,Output,&p->Src,DDBLT_WAITNOTBUSY,NULL)) != DD_OK)
			if (hResult != DDERR_SURFACELOST || IDirectDrawSurface_Restore(p->DDPrimary) != DD_OK)
			{
				DEBUG_MSG(DEBUG_VIDEO,T("DDRAW Blt() failed"));
				break;
			}
	}
	else 
	if (p->Mode == MODE_OVERLAY && Output == p->DDBackBuffer)
		IDirectDrawSurface_Flip(p->DDBuffer,NULL,DDFLIP_WAITNOTBUSY);

	return ERR_NONE;
}

int DDrawCECreate(ddraw* p,LPDIRECTDRAW DD)
{
	int Model = QueryPlatform(PLATFORM_MODEL);
	if (Model==MODEL_AXIM_X50)
		return ERR_NOT_SUPPORTED; // buggy WM5 driver...

	if (IDirectDraw_QueryInterface(DD,&IID_IDirectDraw,&p->DD)!=DD_OK)
		return ERR_DEVICE_ERROR;

	IDirectDraw_Release(DD);
	IDirectDraw_SetCooperativeLevel(p->DD, NULL, DDSCL_NORMAL);

	p->DDCaps.dwSize = sizeof(p->DDCaps);
	IDirectDraw_GetCaps(p->DD,&p->DDCaps,NULL);

	//{ int i; for (i=0;i<sizeof(p->DDCaps)/4;++i)
	//	DEBUG_MSG(DEBUG_VIDEO,T("DDRAW Caps %02x:%08x"),i*4,((int*)&p->DDCaps)[i]); }

	p->SetupColorKey = (p->DDCaps.dwOverlayCaps & DDOVERLAYCAPS_CKEYDEST) != 0;
	p->SetupBlit = (p->DDCaps.dwOverlayCaps & DDOVERLAYCAPS_OVERLAYSUPPORT) == 0;
	p->SetupBlitStretch = 1;

	if (p->SetupBlit && !p->DDCaps.dwVidMemTotal && 
		(NodeEnumClass(NULL,RAW_ID) || NodeEnumClass(NULL,GAPI_ID)))
		return ERR_NOT_SUPPORTED; // fallback to RawFrameBuffer or GAPI until things get fixed

	p->p.DoPowerOff = 1;
	p->p.Init = Init;
	p->p.Done = Done;
	p->p.Reset = Reset;
	p->p.Blit = Blit;
	p->p.Update = Update;
	p->p.UpdateShow = UpdateOverlay;
	return ERR_NONE;
}

#endif
