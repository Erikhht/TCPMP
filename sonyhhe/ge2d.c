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
 * $Id: ge2d.c 543 2006-01-07 22:06:24Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 * Big thanks to Mobile Stream (http://www.mobile-stream.com)
 * for reverse enginerring the GE2D interface
 *
 ****************************************************************************/

#include "../common/common.h"

#if defined(TARGET_PALMOS)

#include "../common/palmos/pace.h"

#if defined(_M_IX86)
//#define GE2DEMU
#endif

#define GE2D_ID			FOURCC('G','E','2','D')

#define sonyGE2DLibType			'aexo'
#define sonyGE2DLibCreator		'SeGE'

enum {
	sonyGE2DFormatYCbCr422 = 1,
	sonyGE2DFormatYCbCr422Planar,
	sonyGE2DFormatYCbCr420Planar
};

#define sonyGE2DBlitPixelCoord	0x2

typedef UInt32 GE2DInsnType;

typedef struct GE2DBitmapType
{
	UInt8		format;		/* 0x00 */
	UInt8		zero;		/* 0x01 */
	UInt16		pitch;		/* 0x02 */
	UInt16		chromaPitch;/* 0x04 */
	UInt16		width;		/* 0x06 */
	UInt16		height;		/* 0x08 */
	UInt16		unused;		/* 0x0a */
	void *		plane1P;	/* 0x0c: Y */
	void *		plane2P;	/* 0x10: Cb */
	void *		plane3P;	/* 0x14: Cr */

} GE2DBitmapType;

typedef struct GE2DLibAPIType
{
	Err (*GE2DLibOpen)();
	Err (*GE2DLibClose)();
	Err (*GE2DProgramBlit)(GE2DInsnType *,
			void *, UInt16,
			const void *, UInt16,
			UInt16, UInt16, UInt8);
	Err (*GE2DBlitWithAlpha)(void *, UInt16,
			const void *, UInt16,
			UInt16, UInt16, UInt8);
	Err (*GE2DBlit)(void *dstBitsP, UInt16 dstPitch,
			const void *srcBitsP, UInt16 srcPitch,
			UInt16 srcWidth, UInt16 srcHeight, UInt8 ff);
	Err (*GE2DBlitWithMask)(void *dstBitsP, UInt16 dstPitch,
			const void *srcBitsP, UInt16 srcPitch,
			UInt16 srcWidth, UInt16 srcHeight,
			UInt8 ff, const void *maskBitsP);
	Err (*GE2DBlitBitmap)(void *dstP, Coord left, Coord top,
			UInt16 dstWidth, UInt16 dstHeight, GE2DBitmapType *);
	Err (*GE2DConvertBitmap)(GE2DBitmapType *, GE2DBitmapType *);
	Err (*GE2DProgramBlitBitmapWithMask)(GE2DInsnType *programP,
			void *dstP, Coord left, Coord top,
			UInt16 dstWidth, UInt16 dstHeight,
			GE2DBitmapType *bmpP, const void *p7);
	Err (*GE2DProgramBlitBitmap)(GE2DInsnType *programP,
			void *dstP, Coord left, Coord top,
			UInt16 dstWidth, UInt16 dstHeight, GE2DBitmapType *bmpP);
	Err (*GE2DScaleBitmap)(GE2DBitmapType *dstBmpP, GE2DBitmapType *srcBmpP);
	Err (*GE2DShowOverlay)(Coord left, Coord top, GE2DBitmapType *);
	Err (*GE2DHideOverlay)();
	Err (*GE2DRunProgram)(const GE2DInsnType *programP);
	Err (*GE2DRunProgram2)(const GE2DInsnType *programP);
	Err (*GE2D_60)(UInt16 x, UInt16 y);
	Err (*GE2D_64)();
	Err (*GE2D_68)(void *);
	Err (*GE2D_72)(RectangleType *);
	Err (*GE2D_76)(FormType *);
	Err (*GE2D_80)(Boolean);
	Err (*GE2D_84)();
	Err (*GE2DSetLock)(Boolean lock);

} GE2DLibAPIType;

typedef struct ge2d
{
	overlay p;
	point Offset;
	GE2DBitmapType Buffer;
	GE2DBitmapType Scale;
	int BufferSize;
	int ScaleSize;
	rect OverlayRect;
	blitfx SoftFX;
	char* Bits;

} ge2d;

static UInt32 LibRef = 0;

#if defined(GE2DEMU)
#include "ge2demu.c"
#else
static GE2DLibAPIType API;
#endif

extern char* QueryScreen(video* Video,int* Offset,int* Mode);

static void FreeBuffer(GE2DBitmapType* p,int* Allocated)
{
	free(p->plane1P);
	free(p->plane2P);
	free(p->plane3P);
	memset(p,0,sizeof(GE2DBitmapType));
	*Allocated = 0;
}

static bool_t AllocBuffer(GE2DBitmapType* p,int w,int h,int* Allocated)
{
	int size;
	w = (w+1) & ~1;
	h = (h+1) & ~1;
	size = w*h;

	p->format = sonyGE2DFormatYCbCr420Planar;
	p->zero = 0;
	p->pitch = (UInt16)w;
	p->chromaPitch = (UInt16)(w >> 1);
	p->width = (UInt16)w;
	p->height = (UInt16)h;
	p->unused = 0;

	if (*Allocated < size)
	{
		free(p->plane1P);
		free(p->plane2P);
		free(p->plane3P);
		p->plane1P = malloc(size);
		p->plane2P = malloc(size/4);
		p->plane3P = malloc(size/4);
		if (!p->plane1P || !p->plane2P || !p->plane3P)
		{
			FreeBuffer(p,Allocated);
			return 0;
		}
		*Allocated = size;
	}

	memset(p->plane1P,0,size);
	memset(p->plane2P,128,size/4);
	memset(p->plane2P,128,size/4);
	return 1;
}

static int GetMode(ge2d* p)
{
	int Offset;
	p->Bits = QueryScreen(&p->p.Output.Format.Video,&Offset,NULL);

	if (!p->p.Output.Format.Video.Pitch || !p->p.Output.Format.Video.Pixel.BitCount)
		return ERR_INVALID_PARAM;

	p->Offset.x = (Offset % p->p.Output.Format.Video.Pitch) / (p->p.Output.Format.Video.Pixel.BitCount/8);
	p->Offset.y = Offset / p->p.Output.Format.Video.Pitch;

	return ERR_NONE;
}

static int Init(ge2d* p)
{
	//p->p.SetFX = BLITFX_AVOIDTEARING;
	p->p.Overlay = 0;

	if (GetMode(p) != ERR_NONE)
		return ERR_INVALID_PARAM;

	if (!AllocBuffer(&p->Buffer,p->p.Input.Format.Video.Width,p->p.Input.Format.Video.Height,&p->BufferSize))
		return ERR_OUT_OF_MEMORY;

	return ERR_NONE;
}

static void Done(ge2d* p)
{
	FreeBuffer(&p->Buffer,&p->BufferSize);
	FreeBuffer(&p->Scale,&p->ScaleSize);
}

static int UpdateOverlay(ge2d* p)
{
	if (p->p.Overlay)
	{
		if (p->p.Show)
		{
			int x = p->p.DstAlignedRect.x;
			int y = p->p.DstAlignedRect.y;

			x += p->Offset.x;
			y += p->Offset.y;

			PALMCALL3(API.GE2DShowOverlay,(Coord)x,(Coord)y,p->Scale.format? &p->Scale : &p->Buffer);
		}
		else
			PALMCALL0(API.GE2DHideOverlay);
	}
	return ERR_NONE;
}

static int Update(ge2d* p)
{
	blitfx OvlFX = p->p.FX;
	video Buffer;

	if (p->p.Overlay)
		PALMCALL0(API.GE2DHideOverlay);

	p->SoftFX = p->p.FX;
	p->SoftFX.ScaleX = SCALE_ONE; // scale handled by hardware
	p->SoftFX.ScaleY = SCALE_ONE;
	OvlFX.Direction = 0;

	//align to 100%
	if (OvlFX.ScaleX > SCALE_ONE-(SCALE_ONE/16) && OvlFX.ScaleX < SCALE_ONE+(SCALE_ONE/16) &&
		OvlFX.ScaleY > SCALE_ONE-(SCALE_ONE/16) && OvlFX.ScaleY < SCALE_ONE+(SCALE_ONE/16))
	{
		OvlFX.ScaleX = SCALE_ONE;
		OvlFX.ScaleY = SCALE_ONE;
	}

	if (p->SoftFX.Direction & DIR_SWAPXY)
		SwapInt(&OvlFX.ScaleX,&OvlFX.ScaleY);

	memset(&Buffer,0,sizeof(Buffer));
	Buffer.Pixel.Flags = PF_YUV420;
	Buffer.Width = p->p.Input.Format.Video.Width;
	Buffer.Height = p->p.Input.Format.Video.Height;
	Buffer.Direction = CombineDir(p->p.InputDirection,p->p.OrigFX.Direction,p->p.Output.Format.Video.Direction);
	if (Buffer.Direction & DIR_SWAPXY)
		SwapInt(&Buffer.Width,&Buffer.Height);
	Buffer.Pitch = Buffer.Width;

	for (;;)
	{
		VirtToPhy(&p->p.Viewport,&p->p.DstAlignedRect,&p->p.Output.Format.Video);
		VirtToPhy(NULL,&p->OverlayRect,&Buffer);

		AnyAlign(&p->p.DstAlignedRect, &p->OverlayRect, &OvlFX, 2, 1, SCALE_ONE/256, SCALE_ONE*256);

		if ((OvlFX.ScaleX != SCALE_ONE || OvlFX.ScaleY != SCALE_ONE) && 
			(p->p.DstAlignedRect.Width != p->OverlayRect.Width && p->p.DstAlignedRect.Height != p->OverlayRect.Height))
		{
			if (!AllocBuffer(&p->Scale,p->p.DstAlignedRect.Width,p->p.DstAlignedRect.Height,&p->ScaleSize))
			{
				// fallback to 100%
				OvlFX.ScaleX = SCALE_ONE;
				OvlFX.ScaleY = SCALE_ONE;
				continue;
			}
		}
		else
			FreeBuffer(&p->Scale,&p->ScaleSize);
		break;
	}

	VirtToPhy(NULL,&p->p.SrcAlignedRect,&p->p.Input.Format.Video);

	BlitRelease(p->p.Soft);
	p->p.Soft = BlitCreate(&Buffer,&p->p.Input.Format.Video,&p->SoftFX,&p->p.Caps);
	BlitAlign(p->p.Soft,&p->OverlayRect,&p->p.SrcAlignedRect);

	if (!AllocBuffer(&p->Buffer,p->OverlayRect.Width,p->OverlayRect.Height,&p->BufferSize))
		return ERR_OUT_OF_MEMORY;

	Buffer.Width = p->OverlayRect.Width;
	Buffer.Height = p->OverlayRect.Height;
	Buffer.Pitch = Buffer.Width;
	p->OverlayRect.x = 0;
	p->OverlayRect.y = 0;

	BlitRelease(p->p.Soft);
	p->p.Soft = BlitCreate(&Buffer,&p->p.Input.Format.Video,&p->SoftFX,&p->p.Caps);
	BlitAlign(p->p.Soft,&p->OverlayRect,&p->p.SrcAlignedRect);

	if (!p->Scale.format) // no scaling -> adjust dst rectangle with soft yuv blitting alignement
	{
		p->p.DstAlignedRect.x += (p->p.DstAlignedRect.Width - p->OverlayRect.Width) >> 1;
		p->p.DstAlignedRect.y += (p->p.DstAlignedRect.Height - p->OverlayRect.Height) >> 1;
		p->p.DstAlignedRect.Width = p->OverlayRect.Width;
		p->p.DstAlignedRect.Height = p->OverlayRect.Height;
	}
	PhyToVirt(&p->p.DstAlignedRect,&p->p.GUIAlignedRect,&p->p.Output.Format.Video);

	return UpdateOverlay(p);
}

static INLINE void FlushDCache(void *p, int n)
{
	SonyCleanDCache(p,n);
	SonyInvalidateDCache(p,n);
}

static int Blit(ge2d* p, const constplanes Data, const constplanes DataLast )
{
	int n;
	planes Planes;

	Planes[0] = p->Buffer.plane1P;
	Planes[1] = p->Buffer.plane3P;
	Planes[2] = p->Buffer.plane2P;
	BlitImage(p->p.Soft,Planes,Data,DataLast,-1,-1);

	n = p->Buffer.width * p->Buffer.height;
	FlushDCache(Planes[0],n);
	FlushDCache(Planes[1],n/4);
	FlushDCache(Planes[2],n/4);

	if (p->Scale.format)
		PALMCALL2(API.GE2DScaleBitmap,&p->Scale,&p->Buffer);

	if (!p->p.Overlay)
	{
		PALMCALL6(API.GE2DBlitBitmap,p->Bits,
			(Coord)(p->p.DstAlignedRect.x + p->Offset.x),
			(Coord)(p->p.DstAlignedRect.y + p->Offset.y),
			(Coord)(p->p.Output.Format.Video.Pitch>>1),(Coord)p->p.Output.Format.Video.Height,
			p->Scale.format? &p->Scale : &p->Buffer);
	}

	return ERR_NONE;
}

static int Reset(ge2d* p)
{
	return GetMode(p);
}

static int Create(ge2d* p)
{
	p->p.Init = (ovlfunc)Init;
	p->p.Done = (ovldone)Done;
	p->p.Reset = (ovlfunc)Reset;
	p->p.Blit = (ovlblit)Blit;
	p->p.Update = (ovlfunc)Update;
	p->p.UpdateShow = (ovlfunc)UpdateOverlay;

	QueryDesktop(&p->p.Output.Format.Video);
	return ERR_NONE;
}

static const nodedef GE2D = 
{
	sizeof(ge2d)|CF_GLOBAL,
	GE2D_ID,
	OVERLAY_CLASS,
	PRI_DEFAULT+50,
	(nodecreate)Create,
};

void GE2D_Init() 
{
#if !defined(GE2DEMU)
	Err err;

//	UInt32 CompanyID;
//	FtrGet(sysFtrCreator, sysFtrNumOEMCompanyID, &CompanyID);
//	if (CompanyID != 'sony')
//		return;

	err = SysFindModule(sonyGE2DLibType, sonyGE2DLibCreator, 0, 0, &LibRef);
	if (err) 
		err = SysLoadModule(sonyGE2DLibType, sonyGE2DLibCreator, 0, 0, &LibRef);
	if (err)
		return;

	if (SysGetEntryAddresses(LibRef, 0, 22, (void **)(void *)&API) != errNone)
		return;

#endif

	PALMCALL0(API.GE2DLibOpen);
	NodeRegisterClass(&GE2D);
}

void GE2D_Done() 
{
	NodeUnRegisterClass(GE2D_ID);
	if (LibRef)
	{
		PALMCALL0(API.GE2DLibClose);
		//SysUnloadModule(LibRef);
		LibRef = 0;
	}
}

#else
void GE2D_Init() {}
void GE2D_Done() {}
#endif
