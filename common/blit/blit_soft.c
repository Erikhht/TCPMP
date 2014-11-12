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
 * $Id: blit_soft.c 607 2006-01-22 20:58:29Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"
#include "../dyncode/dyncode.h"
#include "../cpu/cpu.h"
#include "blit_soft.h"

//#define BLITTEST

/*
TV range:
+16  (219)
+128 (224)
+128 (224)

PC range:
+0   (255)
+128 (255)
+128 (255)

ranges:
R,G,B,Y [0..1]
Cb,Cr   [-0.5..0.5]

Y' = Kr * R' + (1 - Kr - Kb) * G' + Kb * B'
Cb = 0.5 * (B' - Y') / (1 - Kb)
Cr = 0.5 * (R' - Y') / (1 - Kr)

Kb = 0.114
Kr = 0.299

ITU-R BT 601
    Y'= 0.299   *R' + 0.587   *G' + 0.114   *B'
    Cb=-0.168736*R' - 0.331264*G' + 0.5     *B'
    Cr= 0.5     *R' - 0.418688*G' - 0.081312*B'

    R'= Y'            + 1.403*Cr
    G'= Y' - 0.344*Cb - 0.714*Cr
    B'= Y' + 1.773*Cb 

Kb = 0.0722
Kr = 0.2126

ITU-R BT 709
    Y'= 0.2215*R' + 0.7154*G' + 0.0721*B'
    Cb=-0.1145*R' - 0.3855*G' + 0.5000*B'
    Cr= 0.5016*R' - 0.4556*G' - 0.0459*B'

    R'= Y'             + 1.5701*Cr
    G'= Y' - 0.1870*Cb - 0.4664*Cr
    B'= Y' - 1.8556*Cb 
*/

#define CM(i) ((int16_t)((i)*8192))

#if defined(_M_IX86) && !defined(TARGET_SYMBIAN)

static const int16_t YUVToRGB[4][8] =
{
	// TV range BT-601
	{ CM(1.164),-16,CM(1.596),CM(-0.813),-128,CM(2.017),CM(-0.392),-128 },
	// TV range BT-709
	{ CM(1.164),-16,CM(1.786),CM(-0.530),-128,CM(2.111),CM(-0.213),-128 },
	// PC range BT-601
	{ CM(1),0,CM(1.402),CM(-0.71414),-128,CM(1.772),CM(-0.34414),-128 },
	// PC range BT-709
	{ CM(1),0,CM(1.5701),CM(-0.4664),-128,CM(1.8556),CM(-0.1870),-128 },
};

static const int16_t* GetYUVToRGB(const pixel* Src)
{
	int i = 0;
	if (Src->Flags & PF_YUV_BT709)
		i += 1;
	if (Src->Flags & PF_YUV_PC)
		i += 2;

	return YUVToRGB[i];
}

#endif

#define SAT(Value) (Value < 0 ? 0: (Value > 255 ? 255: Value))

static const rgbval_t Gray1[2] = { 
	CRGB(0,0,0),CRGB(255,255,255)
};
static const rgbval_t Gray2[4] = { 
	CRGB(0,0,0),CRGB(85,85,85),CRGB(170,170,170),CRGB(255,255,255) 
};
static const rgbval_t Gray4[16] = {
	CRGB(0,0,0),CRGB(17,17,17),CRGB(34,34,34),CRGB(51,51,51),
	CRGB(68,68,68),CRGB(85,85,85),CRGB(102,102,102),CRGB(119,119,119),
	CRGB(136,136,136),CRGB(153,153,153),CRGB(170,170,170),CRGB(187,187,187),
	CRGB(204,204,204),CRGB(221,221,221),CRGB(238,238,238),CRGB(255,255,255)
};

#if defined(_M_IX86) && !defined(TARGET_SYMBIAN)

#define DECLARE_BLITMMX(name) \
extern void STDCALL name##_mmx(blit_soft* This, uint8_t** DstPtr,uint8_t** SrcPtr,int DstPitch,int SrcPitch,int Width,int Height,uintptr_t Src2SrcLast); \
extern void STDCALL name##_mmx2(blit_soft* This, uint8_t** DstPtr,uint8_t** SrcPtr,int DstPitch,int SrcPitch,int Width,int Height,uintptr_t Src2SrcLast); \
extern void STDCALL name##_3dnow(blit_soft* This, uint8_t** DstPtr,uint8_t** SrcPtr,int DstPitch,int SrcPitch,int Width,int Height,uintptr_t Src2SrcLast);

DECLARE_BLITMMX(blit_i420_i420)
DECLARE_BLITMMX(blit_i420_yuy2)
DECLARE_BLITMMX(blit_i420_rgb32)
DECLARE_BLITMMX(blit_i420_rgb24)
DECLARE_BLITMMX(blit_i420_bgr32)
DECLARE_BLITMMX(blit_i420_bgr24)
DECLARE_BLITMMX(blit_rgb32_rgb32)
DECLARE_BLITMMX(blit_rgb24_rgb24)
DECLARE_BLITMMX(blit_rgb16_rgb16)

typedef struct blitmmx
{
	uint32_t In;
	uint32_t Out;
	blitsoftentry Func[3];

} blitmmx;

static const blitmmx BlitMMX[] = 
{
	{ FOURCC_I420, FOURCC_I420, { blit_i420_i420_mmx, blit_i420_i420_mmx2, blit_i420_i420_3dnow }},
	{ FOURCC_I420, FOURCC_YUY2, { blit_i420_yuy2_mmx, blit_i420_yuy2_mmx2, blit_i420_yuy2_3dnow }},
	{ FOURCC_I420, FOURCC_RGB32,{ blit_i420_rgb32_mmx,blit_i420_rgb32_mmx2,blit_i420_rgb32_3dnow }},
	{ FOURCC_I420, FOURCC_RGB24,{ blit_i420_rgb24_mmx,blit_i420_rgb24_mmx2,blit_i420_rgb24_3dnow }},
	{ FOURCC_I420, FOURCC_BGR32,{ blit_i420_bgr32_mmx,blit_i420_bgr32_mmx2,blit_i420_bgr32_3dnow }},
	{ FOURCC_I420, FOURCC_BGR24,{ blit_i420_bgr24_mmx,blit_i420_bgr24_mmx2,blit_i420_bgr24_3dnow }},
	{ FOURCC_RGB32,FOURCC_RGB32,{ blit_rgb32_rgb32_mmx,blit_rgb32_rgb32_mmx2,blit_rgb32_rgb32_3dnow }},
	{ FOURCC_RGB24,FOURCC_RGB24,{ blit_rgb24_rgb24_mmx,blit_rgb24_rgb24_mmx2,blit_rgb24_rgb24_3dnow }},
	{ FOURCC_RGB16,FOURCC_RGB16,{ blit_rgb16_rgb16_mmx,blit_rgb16_rgb16_mmx2,blit_rgb16_rgb16_3dnow }},
	{ FOURCC_RGB15,FOURCC_RGB15,{ blit_rgb16_rgb16_mmx,blit_rgb16_rgb16_mmx2,blit_rgb16_rgb16_3dnow }},
	{ FOURCC_BGR32,FOURCC_BGR32,{ blit_rgb32_rgb32_mmx,blit_rgb32_rgb32_mmx2,blit_rgb32_rgb32_3dnow }},
	{ FOURCC_BGR24,FOURCC_BGR24,{ blit_rgb24_rgb24_mmx,blit_rgb24_rgb24_mmx2,blit_rgb24_rgb24_3dnow }},
	{ FOURCC_BGR16,FOURCC_BGR16,{ blit_rgb16_rgb16_mmx,blit_rgb16_rgb16_mmx2,blit_rgb16_rgb16_3dnow }},
	{ FOURCC_BGR15,FOURCC_BGR15,{ blit_rgb16_rgb16_mmx,blit_rgb16_rgb16_mmx2,blit_rgb16_rgb16_3dnow }},
	{0},
};

#endif

typedef struct blitpack
{
	blitfx FX;
	video Dst;
	video Src;
	rect DstRect;
	rect SrcRect;
	blit_soft Code[2];
	bool_t SafeBorder;
	int RScaleX;
	int RScaleY;
	struct blitpack* Next;

} blitpack;

static NOINLINE void FreeBlit(blit_soft* p)
{
	CodeDone(&p->Code);
	free(p->LookUp_Data);
	p->LookUp_Data = NULL;
}

static INLINE struct blitpack* _BlitAlloc()
{
	blitpack* p = (blitpack*) malloc(sizeof(blitpack));
	if (!p) return NULL;

	memset(p,0,sizeof(blitpack));
	CodeInit(&p->Code[0].Code);
	CodeInit(&p->Code[1].Code);
	return p;
}

static INLINE void _BlitFree(struct blitpack* p)
{
	FreeBlit(&p->Code[0]);
	FreeBlit(&p->Code[1]);
	free(p);
}

#ifdef CONFIG_CONTEXT

void Blit_Init()
{
}

void Blit_Done()
{
	blitpack* p;
	while ((p = Context()->Blit)!=NULL)
	{
		Context()->Blit = p->Next;
		_BlitFree(p);
	}
}

static INLINE void BlitFree(struct blitpack* p)
{
	p->Next = Context()->Blit;
	Context()->Blit = p;
}

static INLINE struct blitpack* BlitAlloc()
{
	blitpack* p = Context()->Blit;
	if (p)
		Context()->Blit = p->Next;
	else
		p = _BlitAlloc();
	return p;
}

#else
#define BlitAlloc _BlitAlloc
#define BlitFree _BlitFree
#endif

static const rgb* DefaultPal(const pixel* Format)
{
	if (Format->Flags & PF_PALETTE)
	{
		if (!Format->Palette)
			switch (Format->BitCount)
			{
			case 1: return (const rgb*)Gray1;
			case 2: return (const rgb*)Gray2;
			case 4: return (const rgb*)Gray4;
			}
		return Format->Palette;
	}
	return NULL;
}

static int CalcScale(int v, int Min, int Max)
{
	if (v > Max) v = Max;
	if (v < Min) v = Min;
	return v;
}

static int CalcRScale(int v, int Gray)
{
	if (v<=0) return 16;

	v = (16*1024 << 16) / v;

	if (Gray) // only 100% and 200% scale
		return v > 12288 ? 16:8;

	//align to 100%
	if (v > 16834-1024 && v < 16384+1024) 
		v = 16384;
	//align to 200%
	if (v > 8192-1024 && v < 8192+1024) 
		v = 8192;
	//align to 50%
	if (v > 32768-2048 && v < 32768+2048) 
		v = 32768;

#if defined(SH3)
	if (v < 12288)
		return 8;
	return 16;
//	if (v<1024) v=1024;
//	return ((v+1024) >> 11) << 1;
#else
	if (v<512) v=512;
	return (v+512) >> 10;
#endif
}

static NOINLINE bool_t EnlargeIfNeeded(int* v,int Align,int Side,int Limit)
{
	int Needed = Align - (*v & (Align-1));
	if (Needed < Align && Needed <= (Limit-Side))
	{
		*v += Needed;
		return 1;
	}
	return 0;
}

void BlitAlign(blitpack* p, rect* DstRect, rect* SrcRect)
{
	int i;
	int ShrinkX,ShrinkY;
	int SrcRight;
	int SrcBottom;
	int SrcAdjWidth,SrcAdjHeight;
	int RScaleX,RScaleY;
	blit_soft* Code;

	if (!p) return;

	RScaleX = p->RScaleX;
	RScaleY = p->RScaleY;
	Code = &p->Code[0];

	p->SafeBorder = 0;

	if (Code->ArithStretch && (RScaleX != 16) && (RScaleX != 32))
	{
		//avoid bilinear scale overrun (shrink source)

		if ((p->Src.Pixel.Flags & PF_SAFEBORDER) && Code->DstAlignSize > 2)
			p->SafeBorder = 1; // build a one pixel border before blitting on right and bottom side
		else
		{
			//only horizontal bilinear filtering is supported (arm_stretch)
			if (SrcRect->Width>2) 
				SrcRect->Width -= 2;
		}
	}

	// convert source to destination space
	if (p->FX.Direction & DIR_SWAPXY)
	{
		SwapInt(&RScaleX,&RScaleY);
		SwapRect(SrcRect);
	}

	SrcRight = SrcRect->x + SrcRect->Width;
	SrcBottom = SrcRect->y + SrcRect->Height;

	SrcAdjWidth = SrcRect->Width * 16 / RScaleX;
	SrcAdjHeight = SrcRect->Height * 16 / RScaleY;

	if (p->FX.Flags & BLITFX_ENLARGEIFNEEDED)
	{
		SrcRight = p->Src.Width;
		SrcBottom = p->Src.Height;
		if (p->FX.Direction & DIR_SWAPXY)
			SwapInt(&SrcRight,&SrcBottom);

		if (p->Src.Pixel.Flags & PF_SAFEBORDER)
		{
			SrcRight += 16;
			SrcBottom += 16;
		}

		if (EnlargeIfNeeded(&SrcAdjWidth,Code->DstAlignSize,SrcRect->x+SrcRect->Width,SrcRight))
			SrcRect->Width = -1; // need calc
		if (EnlargeIfNeeded(&SrcAdjHeight,Code->DstAlignSize,SrcRect->y+SrcRect->Height,SrcBottom))
			SrcRect->Height = -1; // need calc
	}

	ShrinkX = DstRect->Width - SrcAdjWidth;
	if (ShrinkX>=0) //shrink destination?
	{
		ShrinkX >>= 1;
		DstRect->x += ShrinkX;
		DstRect->Width = SrcAdjWidth;
	}
	else //adjust source position
	{
		ShrinkX = 0;
		SrcRect->x += (SrcAdjWidth - DstRect->Width) * RScaleX >> 5;
		SrcRect->Width = -1; // need calc
	}
	
	ShrinkY = DstRect->Height - SrcAdjHeight;
	if (ShrinkY>=0) //shrink Dst?
	{
		ShrinkY >>= 1;
		DstRect->y += ShrinkY;
		DstRect->Height = SrcAdjHeight;
	}
	else //adjust source position
	{
		ShrinkY = 0;
		SrcRect->y += (SrcAdjHeight - DstRect->Height) * RScaleY >> 5;
		SrcRect->Height = -1; // need calc
	}

	i = DstRect->Width & (Code->DstAlignSize-1); 
	DstRect->Width -= i; 
	i >>= 1;
	ShrinkX += i;
	DstRect->x += i;

	i = DstRect->Height & (Code->DstAlignSize-1);
	DstRect->Height -= i;
	i >>= 1;
	ShrinkY += i;
	DstRect->y += i;

	i = DstRect->x & (Code->DstAlignPos-1);
	if (i && ShrinkX < i)
	{
		DstRect->Width -= Code->DstAlignPos - i;
		DstRect->Width &= ~(Code->DstAlignSize-1); 
		DstRect->x += Code->DstAlignPos - i;
	}
	else
		DstRect->x -= i;

	i = DstRect->y & (Code->DstAlignPos-1);
	if (i && ShrinkY < i)
	{
		DstRect->Height -= Code->DstAlignPos - i;
		DstRect->Height &= ~(Code->DstAlignSize-1);
		DstRect->y += Code->DstAlignPos - i;
	}
	else
		DstRect->y -= i;

	SrcRect->x &= ~(Code->SrcAlignPos-1);
	SrcRect->y &= ~(Code->SrcAlignPos-1);

	// convert source back to it's space (if needed)
	if (SrcRect->Width < 0)
		SrcRect->Width = (DstRect->Width * RScaleX / 16 + 1) & ~1;
	if (SrcRect->Height < 0)
		SrcRect->Height = (DstRect->Height * RScaleY / 16 + 1) & ~1;

	if (SrcRect->x + SrcRect->Width > SrcRight)
		SrcRect->Width = SrcRight - SrcRect->x;

	if (SrcRect->y + SrcRect->Height > SrcBottom)
		SrcRect->Height = SrcBottom - SrcRect->y;

	if (p->FX.Direction & DIR_SWAPXY)
		SwapRect(SrcRect);

	p->DstRect = *DstRect;
	p->SrcRect = *SrcRect;
}

static NOINLINE void CodeRelease(blit_soft* p)
{
	//todo... better palette handling
	if (p->Dst.Palette)
		memset(&p->Dst,0,sizeof(p->Dst));
	if (p->Src.Palette)
		memset(&p->Src,0,sizeof(p->Src));
}

void BlitRelease(blitpack* p)
{
	if (p)
	{
		CodeRelease(&p->Code[0]);
		CodeRelease(&p->Code[1]);
		BlitFree(p);
	}
}

int AnyAlign(rect* DstRect, rect* SrcRect, const blitfx* FX, 
			  int DstAlignSize, int DstAlignPos,
			  int MinScale, int MaxScale)
{
	int i,ShrinkX,ShrinkY;
	int ScaleX,ScaleY;
	int SrcRight;
	int SrcBottom;
	int SrcAdjWidth,SrcAdjHeight;

	if (!DstRect || !SrcRect || !FX)
		return ERR_INVALID_PARAM;

	ScaleX = CalcScale(FX->ScaleX,MinScale,MaxScale);
	ScaleY = CalcScale(FX->ScaleY,MinScale,MaxScale);
	SrcRight = SrcRect->x + SrcRect->Width;
	SrcBottom = SrcRect->y + SrcRect->Height;

	// convert source to destination space
	if (FX->Direction & DIR_SWAPXY)
	{
		SwapInt(&ScaleX,&ScaleY);
		SwapRect(SrcRect);
	}

	SrcAdjWidth = (SrcRect->Width * ScaleX + 32768) >> 16;
	SrcAdjHeight = (SrcRect->Height * ScaleY + 32768) >> 16;

	ShrinkX = DstRect->Width - SrcAdjWidth;
	if (ShrinkX>0) //shrink destination?
	{
		ShrinkX >>= 1;
		DstRect->x += ShrinkX;
		DstRect->Width = SrcAdjWidth;
	}
	else //adjust source position
	{
		ShrinkX = 0;
		SrcRect->x += ((SrcAdjWidth - DstRect->Width) << 15) / ScaleX;
		SrcRect->Width = -1;
	}
	
	ShrinkY = DstRect->Height - SrcAdjHeight;
	if (ShrinkY>0) //shrink Dst?
	{
		ShrinkY >>= 1;
		DstRect->y += ShrinkY;
		DstRect->Height = SrcAdjHeight;
	}
	else //adjust source position
	{
		ShrinkY = 0;
		SrcRect->y += ((SrcAdjHeight - DstRect->Height) << 15) / ScaleY;
		SrcRect->Height = -1;
	}

	// final alignment

	i = DstRect->Width & (DstAlignSize-1); 
	DstRect->Width -= i; 
	i >>= 1;
	ShrinkX += i;
	DstRect->x += i;

	i = DstRect->Height & (DstAlignSize-1);
	DstRect->Height -= i;
	i >>= 1;
	ShrinkY += i;
	DstRect->y += i;

	i = DstRect->x & (DstAlignPos-1);
	if (i && ShrinkX < i)
	{
		DstRect->Width -= DstAlignPos - i;
		DstRect->Width &= ~(DstAlignSize-1); 
		DstRect->x += DstAlignPos - i;
	}
	else
		DstRect->x -= i;

	i = DstRect->y & (DstAlignPos-1);
	if (i && ShrinkY < i)
	{
		DstRect->Height -= DstAlignPos - i;
		DstRect->Height &= ~(DstAlignSize-1);
		DstRect->y += DstAlignPos - i;
	}
	else
		DstRect->y -= i;

	SrcRect->x &= ~1;
	SrcRect->y &= ~1;

	if (SrcRect->Width < 0)
		SrcRect->Width = ((DstRect->Width << 16) / ScaleX +1) & ~1;
	if (SrcRect->Height < 0)
		SrcRect->Height = ((DstRect->Height << 16) / ScaleY +1) & ~1;

	if (FX->Direction & DIR_SWAPXY)
		SwapRect(SrcRect);

	if (SrcRect->x + SrcRect->Width > SrcRight)
		SrcRect->Width = SrcRight - SrcRect->x;

	if (SrcRect->y + SrcRect->Height > SrcBottom)
		SrcRect->Height = SrcBottom - SrcRect->y;

	return ERR_NONE;
}

static INLINE void SurfacePtr(uint8_t** Ptr, const planes Planes, const video* Format, int BPP, int x, int y, int Pitch)
{
	int Adj = (x & 1) << 1;
   	Ptr[0] = (uint8_t*)Planes[0] + ((x * BPP) >> 3) + y * Pitch;

	if (Format->Pixel.Flags & (PF_YUV420|PF_YUV422|PF_YUV444|PF_YUV410))
	{
		if (Format->Pixel.Flags & PF_YUV420)
		{
			Ptr[1] = (uint8_t*)Planes[1] + (x >> 1) + (y >> 1) * (Pitch >> 1);
			Ptr[2] = (uint8_t*)Planes[2] + (x >> 1) + (y >> 1) * (Pitch >> 1);
		}
		else
		if (Format->Pixel.Flags & PF_YUV422)
		{
			Ptr[1] = (uint8_t*)Planes[1] + (x >> 1) + y * (Pitch >> 1);
			Ptr[2] = (uint8_t*)Planes[2] + (x >> 1) + y * (Pitch >> 1);
		}
		else
		if (Format->Pixel.Flags & PF_YUV444)
		{
			Ptr[1] = (uint8_t*)Planes[1] + x + y * Pitch;
			Ptr[2] = (uint8_t*)Planes[2] + x + y * Pitch;
		}
		else
		if (Format->Pixel.Flags & PF_YUV410)
		{
			Ptr[1] = (uint8_t*)Planes[1] + (x >> 2) + (y >> 2) * (Pitch >> 2);
			Ptr[2] = (uint8_t*)Planes[2] + (x >> 2) + (y >> 2) * (Pitch >> 2);
		}
	}
	else
	if (Format->Pixel.Flags & PF_FOURCC)
		switch (Format->Pixel.FourCC)
		{
		case FOURCC_IMC2:
			Ptr[2] = (uint8_t*)Planes[0] + Format->Height * Pitch + (x >> 1) + (y >> 1) * Pitch;
			Ptr[1] = Ptr[1] + (Pitch >> 1);
			break;
		case FOURCC_IMC4:
			Ptr[1] = (uint8_t*)Planes[0] + Format->Height * Pitch + (x >> 1) + (y >> 1) * Pitch;
			Ptr[2] = Ptr[1] + (Pitch >> 1);
			break;
		case FOURCC_I420:
		case FOURCC_IYUV:
			Ptr[1] = (uint8_t*)Planes[0] + Format->Height * Pitch + (x >> 1) + (y >> 1) * (Pitch >> 1);
			Ptr[2] = Ptr[1] + ((Format->Height * Pitch) >> 2);
			break;
		case FOURCC_YV16: 
			Ptr[2] = (uint8_t*)Planes[0] + Format->Height * Pitch + (x >> 1) + y * (Pitch >> 1);
			Ptr[1] = Ptr[2] + ((Format->Height * Pitch) >> 1);
			break;
		case FOURCC_YVU9: 
			Ptr[2] = (uint8_t*)Planes[0] + Format->Height * Pitch + (x >> 2) + (y >> 2) * (Pitch >> 2);
			Ptr[1] = Ptr[2] + ((Format->Height * Pitch) >> 4);
			break;
		case FOURCC_YUV9: 
			Ptr[1] = (uint8_t*)Planes[0] + Format->Height * Pitch + (x >> 2) + (y >> 2) * (Pitch >> 2);
			Ptr[2] = Ptr[1] + ((Format->Height * Pitch) >> 4);
			break;
		case FOURCC_YV12: 
			Ptr[2] = (uint8_t*)Planes[0] + Format->Height * Pitch + (x >> 1) + (y >> 1) * (Pitch >> 1);
			Ptr[1] = Ptr[2] + ((Format->Height * Pitch) >> 2);
			break;
		case FOURCC_YUY2:
		case FOURCC_YUNV:
		case FOURCC_V422:
		case FOURCC_YUYV:
			Ptr[1] = Ptr[0]+1-Adj;
			Ptr[2] = Ptr[0]+3-Adj;
			break;
		case FOURCC_YVYU:
			Ptr[1] = Ptr[0]+3-Adj;
			Ptr[2] = Ptr[0]+1-Adj;
			break;
		case FOURCC_UYVY:
		case FOURCC_Y422:
		case FOURCC_UYNV:
			Ptr[1] = Ptr[0]-Adj;
			Ptr[2] = Ptr[0]+2-Adj;
			Ptr[0]++;
			break;
		case FOURCC_VYUY:
			Ptr[2] = Ptr[0]-Adj;
			Ptr[1] = Ptr[0]+2-Adj;
			Ptr[0]++;
			break;
		}
}

void BlitImage(blitpack* Pack, const planes Dst, const constplanes Src, const constplanes SrcLast, int DstPitch, int SrcPitch)
{
	uint8_t* DstPtr[MAXPLANES];
	uint8_t* SrcPtr[MAXPLANES];
	bool_t OnlyDiff;
	int Width,Height;
	blit_soft* p;
	uintptr_t Src2SrcLast;
	int DstStepX;
	int DstStepY;
	int DstX;
	int DstY;
	int SrcY;

	// nothing to do?
	if (!Pack || Pack->DstRect.Width<=0 || Pack->DstRect.Height<=0)
		return;

	OnlyDiff = (Pack->FX.Flags & BLITFX_ONLYDIFF) && SrcLast && SrcLast[0] != NULL;

	p = &Pack->Code[OnlyDiff];

	// calculate the Src and Dst pointers
	//   Src: always upperleft corner
	//   Dst: according to swapxy and mirroring

	Width = Pack->DstRect.Width;
	Height = Pack->DstRect.Height;

	if (p->SwapXY)
		SwapInt(&Width,&Height);

	if (DstPitch < 0)
		DstPitch = Pack->Dst.Pitch;
	if (SrcPitch < 0)
		SrcPitch = Pack->Src.Pitch;

	SrcY = Pack->SrcRect.y;
	if (p->SrcUpDown)
		SrcY += Pack->SrcRect.Height-1;

	SurfacePtr(SrcPtr,*(const planes*)Src,&Pack->Src,p->SrcBPP,Pack->SrcRect.x,SrcY,SrcPitch);

	if (p->SrcUpDown)
		SrcPitch = -SrcPitch;

	Src2SrcLast = 0;
	if (OnlyDiff)
		Src2SrcLast = (uint8_t*)SrcLast[0] - (uint8_t*)Src[0];

	DstStepX = p->DstBPP;
	DstStepY = DstPitch*8;
	DstX = Pack->DstRect.x;
	DstY = Pack->DstRect.y;

	if (p->DstLeftRight)
	{
		DstX += Pack->DstRect.Width-1;
		DstStepX = -DstStepX;
	}		

	if (p->DstUpDown)
	{
		DstY += Pack->DstRect.Height-1;
		DstStepY = -DstStepY;
	}

	if (p->SwapXY)
		SwapInt(&DstStepX,&DstStepY);

	SurfacePtr(DstPtr,Dst,&Pack->Dst,p->DstBPP,DstX,DstY,DstPitch);

	if (p->DstUpDown)
		DstPitch = -DstPitch;

	if (p->Slices)
	{
		const int DstBlock2 = 5;
		const int DstBlock = 32;

		int SrcBlock = (DstBlock * p->RScaleX) >> 4; //SrcBlock has to be even because of YUV
		int DstNext = (DstBlock * DstStepX) >> 3;
		int DstNextUV = DstNext >> (p->SwapXY ? p->DstUVPitch2+p->DstUVY2 : p->DstUVX2);
		int SrcNext = (SrcBlock * p->SrcBPP) >> 3;
		int SrcNextUV = SrcNext >> p->SrcUVX2;

		if (Width > DstBlock && p->SlicesReverse) // reverse order?
		{
			int Quot = Width >> DstBlock2;
			int Rem = Width & (DstBlock-1);

			DstPtr[0] += Quot*DstNext;
			DstPtr[1] += Quot*DstNextUV;
			DstPtr[2] += Quot*DstNextUV;

			SrcPtr[0] += Quot*SrcNext;
			SrcPtr[1] += Quot*SrcNextUV;
			SrcPtr[2] += Quot*SrcNextUV;

			if (Rem)
			{
				p->Entry(p,DstPtr,SrcPtr,DstPitch,SrcPitch,Rem,Height,Src2SrcLast);

				Width -= Rem;
			}

			DstNext = -DstNext;
			DstNextUV = -DstNextUV;
			SrcNext = -SrcNext;
			SrcNextUV = -SrcNextUV;

			DstPtr[0] += DstNext;
			DstPtr[1] += DstNextUV;
			DstPtr[2] += DstNextUV;

			SrcPtr[0] += SrcNext;
			SrcPtr[1] += SrcNextUV;
			SrcPtr[2] += SrcNextUV;
		}

		for (;Width > DstBlock;Width -= DstBlock)
		{
			p->Entry(p,DstPtr,SrcPtr,DstPitch,SrcPitch,DstBlock,Height,Src2SrcLast);

			DstPtr[0] += DstNext;
			DstPtr[1] += DstNextUV;
			DstPtr[2] += DstNextUV;

			SrcPtr[0] += SrcNext;
			SrcPtr[1] += SrcNextUV;
			SrcPtr[2] += SrcNextUV;
		}
	}

	p->Entry(p,DstPtr,SrcPtr,DstPitch,SrcPitch,Width,Height,Src2SrcLast); 
}

void BuildPalLookUp(blit_soft* p,bool_t YUV)
{
	//create a palette lookup with 3x3 bits RGB input
	int a,b,c;
	int Size = 1 << p->Dst.BitCount;
	uint8_t* LookUp;

	p->LookUp_Data = malloc(16*16*16*4);
	if (p->LookUp_Data)
	{
		LookUp = (uint8_t*) p->LookUp_Data;

		for (a=16;a<512;a+=32)
			for (b=16;b<512;b+=32)
				for (c=16;c<512;c+=32)
				{
					const rgb* q = p->DstPalette;
					int BestMatch = 0;
					int BestDiff = 0x7FFFFFFF;
					int i,v[3];

					v[0] = a; 
					v[1] = b;
					v[2] = c;
					if (v[0] >= 384) v[0] -= 512;
					if (v[1] >= 384) v[1] -= 512;
					if (v[2] >= 384) v[2] -= 512;

					if (YUV)
					{
						int w[3];

						w[0] = (v[0]*p->_YMul +                  v[2]*p->_RVMul + p->_RAdd) >> 16;
						w[1] = (v[0]*p->_YMul + v[1]*p->_GUMul + v[2]*p->_GVMul + p->_GAdd) >> 16;
						w[2] = (v[0]*p->_YMul + v[1]*p->_BUMul +               p->_BAdd) >> 16;
						v[0]=w[0];
						v[1]=w[1];
						v[2]=w[2];
					}

					for (i=0;i<Size;++i,++q)
					{
						int Diff = (q->c.r-v[0])*(q->c.r-v[0])+
								   (q->c.g-v[1])*(q->c.g-v[1])+
								   (q->c.b-v[2])*(q->c.b-v[2]);

						if (Diff < BestDiff)
						{
							BestMatch = i;
							BestDiff = Diff;
						}
					}

					q = p->DstPalette + BestMatch;
					if (YUV)
					{
						v[0] = ((2105 * q->c.r) + (4128 * q->c.g) + (802 * q->c.b))/0x2000 + 16;
						v[1] = (-(1212 * q->c.r) - (2384 * q->c.g) + (3596 * q->c.b))/0x2000 + 128;
						v[2] = ((3596 * q->c.r) - (3015 * q->c.g) - (582 * q->c.b))/0x2000 + 128;

						v[0]=SAT(v[0]);
						v[1]=SAT(v[1]);
						v[2]=SAT(v[2]);
					}
					else
					{
						v[0] = q->c.r;
						v[1] = q->c.g;
						v[2] = q->c.b;
					}

					LookUp[0] = (uint8_t)BestMatch;
#if defined(SH3)
					LookUp[3] = (uint8_t)(v[0] >> 1);
					LookUp[1] = (uint8_t)(v[1] >> 1);
					LookUp[2] = (uint8_t)(v[2] >> 1);
#else
					LookUp[1] = (uint8_t)v[0];
					LookUp[2] = (uint8_t)v[1];
					LookUp[3] = (uint8_t)v[2];
#endif
					LookUp += 4;
				}
	}
}

int UniversalType(const pixel* p, bool_t YUV)
{
	if (PlanarYUV(p,NULL,NULL,NULL)) return YUV ? 12:10;
	if (PackedYUV(p)) return YUV ? 13:11;

	if (p->Flags & PF_PALETTE)
	{
		if (p->BitCount==1) return 1;
		if (p->BitCount==2) return 2;
		if (p->BitCount==4) return 3;
		if (p->BitCount==8) return YUV ? 14:4;
	}

	if (p->Flags & PF_RGB)
	{
		if (p->BitCount==8)  return 5;
		if (p->BitCount==16) return 6;
		if (p->BitCount==24) return 7;
		if (p->BitCount==32) return 8;
	}

	return -1;
}

// !SwapXY
// !DstLeftRight
// SrcType == 12
// DstType == 12
// RScaleX == 32/16/8
// RScaleY == 32/16/8 
// SrcUVX2 == DstUVX2
// SrcUVY2 == DstUVY2

static void Blit_PYUV_PYUV_2Plane(const uint8_t* Src,uint8_t* Dst,int Width,int Height,int SrcPitch,int DstPitch,int ScaleX,int ScaleY)
{
	int y;
	for (y=0;y<Height;++y)
	{
		const uint8_t* s = Src;
		uint8_t* d = Dst;
		uint8_t* de = Dst + Width;

		switch (ScaleX)
		{
		case 4:
			while (d<de)
			{
				d[0] = d[1] = d[2] = d[3] = *s;
				++s;
				d+=4;
			}
			break;
		case 8:
			while (d<de)
			{
				d[0] = *s;
				d[1] = *s;
				++s;
				d+=2;
			}
			break;
		case 16:
			memcpy(d,s,Width);
			break;
		case 32:
			while (d<de)
			{
				*d = *s;
				++d;
				s+=2;
			}
			break;
		case 64:
			while (d<de)
			{
				*d = *s;
				++d;
				s+=4;
			}
			break;
		}

		Dst += DstPitch;

		switch (ScaleY)
		{
		case 4:
			if ((y&3)==3) Src += SrcPitch;
			break;
		case 8:
			if (y&1) Src += SrcPitch;
			break;
		case 16:
			Src += SrcPitch;
			break;
		case 32:
			Src += SrcPitch*2;
			break;
		case 64:
			Src += SrcPitch*4;
			break;
		}
	}
}

// needed for half/quarter software idct mode changes
static void STDCALL Blit_PYUV_PYUV_2(blit_soft* This, uint8_t** DstPtr,uint8_t** SrcPtr,int DstPitch,int SrcPitch,
	                  int Width,int Height,uintptr_t Src2SrcLast) 
{
	Blit_PYUV_PYUV_2Plane(SrcPtr[0],DstPtr[0],Width,Height,SrcPitch,DstPitch,This->RScaleX,This->RScaleY);

	Width >>= This->SrcUVX2;
	Height >>= This->SrcUVY2;
	SrcPitch >>= This->SrcUVPitch2;
	DstPitch >>= This->DstUVPitch2;

	Blit_PYUV_PYUV_2Plane(SrcPtr[1],DstPtr[1],Width,Height,SrcPitch,DstPitch,This->RScaleX,This->RScaleY);
	Blit_PYUV_PYUV_2Plane(SrcPtr[2],DstPtr[2],Width,Height,SrcPitch,DstPitch,This->RScaleX,This->RScaleY);
}

#if defined(_M_IX86) || !defined(CONFIG_DYNCODE) || defined(BLITTEST)

// !SwapXY
// !DstLeftRight
// SrcType == 12
// DstType == 12
// RScaleX == 16 && RScaleY == 16
// SrcUVX2 == DstUVX2

static void STDCALL Blit_PYUV_PYUV(blit_soft* This, uint8_t** DstPtr,uint8_t** SrcPtr,int DstPitch,int SrcPitch,
	               int Width,int Height,uintptr_t Src2SrcLast) 
{
	uint8_t* Src[3];
	uint8_t* Dst[3];

	uint8_t* LookUp = This->LookUp_Data;
	int UVY = 1 << This->DstUVY2;
	int UVDup = 1;
	int SrcPitchUV = SrcPitch >> This->SrcUVPitch2;
	int DstPitchUV = DstPitch >> This->DstUVPitch2;
	int WidthUV = Width >> This->SrcUVX2;
	int YAdd = This->FX.Brightness;
	int x,y,i;

	for (i=0;i<3;++i)
	{
		Src[i] = SrcPtr[i];
		Dst[i] = DstPtr[i];
	}

	// skip some UV lines?
	if (This->DstUVY2 > This->SrcUVY2)
		SrcPitchUV <<= This->DstUVY2 - This->SrcUVY2;
	else
		UVDup = 1 << (This->SrcUVY2 - This->DstUVY2);

	Height >>= This->DstUVY2;

	for (y=0;y<Height;++y)
	{
		for (i=0;i<UVY;++i)
		{
			if (LookUp)
			{
				uint8_t* s = Src[0];
				uint8_t* d = Dst[0];
				for (x=0;x<Width;++x,++s,++d)
					*d = LookUp[*s];
			}
			else
			if (YAdd)
			{
				uint8_t* s = Src[0];
				uint8_t* d = Dst[0];
				for (x=0;x<Width;++x,++s,++d)
				{
					int Y = *s + YAdd;
					Y = SAT(Y);
					*d = (uint8_t)Y;
				}
			}
			else
				memcpy(Dst[0],Src[0],Width);

			Src[0] += SrcPitch;
			Dst[0] += DstPitch;
		}

		for (i=0;i<UVDup;++i)
		{
			if (LookUp)
			{
				for (x=0;x<WidthUV;++x)
				{
					Dst[1][x] = LookUp[256+Src[1][x]];
					Dst[2][x] = LookUp[512+Src[2][x]];
				}
			}
			else
			{
				memcpy(Dst[1],Src[1],WidthUV);
				memcpy(Dst[2],Src[2],WidthUV);
			}

			Dst[1] += DstPitchUV;
			Dst[2] += DstPitchUV;
		}

		Src[1] += SrcPitchUV;
		Src[2] += SrcPitchUV;
	}
}

// !SwapXY
// !DstLeftRight
// SrcType == 10
// SrcUVX2 == 1
// DstType == 8		00000000rrrrrrrrggggggggbbbbbbbb

static void STDCALL Blit_PYUV_RGB32(blit_soft* This, uint8_t** DstPtr,uint8_t** SrcPtr,int DstPitch,int SrcPitch,
	                 int Width,int Height,uintptr_t Src2SrcLast) 
{
	int i,x,y;
	uint8_t* Src[3];
	uint8_t* Dst;
	int SrcStepX[16];
	int SrcStepY[16];
	int SrcUVY2 = This->SrcUVY2;
	int SrcUVPitch2 = This->SrcUVPitch2;

	int YAdd = This->FX.Brightness;
	int sy0;
	int sy=0;

	for (i=0;i<3;++i)
		Src[i] = SrcPtr[i];
	Dst = DstPtr[0];

	for (i=0;i<16;++i)
	{
		SrcStepX[i] = ((This->RScaleX * (i+1)) >> 4) - ((This->RScaleX * i) >> 4);
		SrcStepY[i] = ((This->RScaleY * (i+1)) >> 4) - ((This->RScaleY * i) >> 4);
	}

	for (y=0;y<Height;++y)
	{
		uint8_t* dx = Dst;
		int sx = 0;

		for (x=0;x<Width;++x)
		{
			int cR,cG,cB;
			int sx2 = sx>>1;

			cR = ((Src[0][sx]+YAdd-16)*0x2568						      + 0x3343*(Src[2][sx2]-128)) /0x2000;
			cG = ((Src[0][sx]+YAdd-16)*0x2568 - 0x0c92*(Src[1][sx2]-128)  - 0x1a1e*(Src[2][sx2]-128)) /0x2000;
			cB = ((Src[0][sx]+YAdd-16)*0x2568 + 0x40cf*(Src[1][sx2]-128))                             /0x2000;
			
			cR = SAT(cR);
			cG = SAT(cG);
			cB = SAT(cB);

			do
			{
				dx[0]=(uint8_t)cB;
				dx[1]=(uint8_t)cG;
				dx[2]=(uint8_t)cR;
				dx+=4;
			} while (SrcStepX[x&15]==0 && ++x<Width);

			sx += SrcStepX[x&15];
		}
		Dst += DstPitch;

		while (SrcStepY[y&15]==0 && ++y<Height)
		{
			memcpy(Dst,Dst-DstPitch,4*Width);
			Dst += DstPitch;
		}

		sy0 = sy;
		sy += SrcStepY[y&15];
		Src[0] += SrcPitch * (sy - sy0);
		Src[1] += (SrcPitch >> SrcUVPitch2) * ((sy >> SrcUVY2) - (sy0 >> SrcUVY2));
		Src[2] += (SrcPitch >> SrcUVPitch2) * ((sy >> SrcUVY2) - (sy0 >> SrcUVY2));
	}
}

// !SwapXY
// !DstLeftRight
// SrcType == 10
// SrcUVX2 == 1
// DstType == 6		rrrrrggggggbbbbb

static void STDCALL Blit_PYUV_RGB16(blit_soft* This, uint8_t** DstPtr,uint8_t** SrcPtr,int DstPitch,int SrcPitch,
	                 int Width,int Height,uintptr_t Src2SrcLast) 
{
	int i,x,y;
	uint8_t* Src[3];
	uint8_t* Dst;
	int SrcStepX[16];
	int SrcStepY[16];
	int SrcUVY2 = This->SrcUVY2;
	int SrcUVPitch2 = This->SrcUVPitch2;

	int YAdd = This->FX.Brightness;
	int sy0;
	int sy=0;

	for (i=0;i<3;++i)
		Src[i] = SrcPtr[i];
	Dst = DstPtr[0];

	for (i=0;i<16;++i)
	{
		SrcStepX[i] = ((This->RScaleX * (i+1)) >> 4) - ((This->RScaleX * i) >> 4);
		SrcStepY[i] = ((This->RScaleY * (i+1)) >> 4) - ((This->RScaleY * i) >> 4);
	}

	for (y=0;y<Height;++y)
	{
		uint16_t* dx = (uint16_t*)Dst;
		int sx = 0;

		for (x=0;x<Width;++x)
		{
			int cR,cG,cB;
			int sx2 = sx>>1;

			cR = ((Src[0][sx]+YAdd-16)*0x2568						      + 0x3343*(Src[2][sx2]-128)) /0x2000;
			cG = ((Src[0][sx]+YAdd-16)*0x2568 - 0x0c92*(Src[1][sx2]-128)  - 0x1a1e*(Src[2][sx2]-128)) /0x2000;
			cB = ((Src[0][sx]+YAdd-16)*0x2568 + 0x40cf*(Src[1][sx2]-128))                             /0x2000;
			
			cR = SAT(cR);
			cG = SAT(cG);
			cB = SAT(cB);

			do
			{
				*dx = (uint16_t)(((cR << 8)&0xF800)|((cG << 3)&0x07E0)|(cB >> 3));
				++dx;
			} while (SrcStepX[x&15]==0 && ++x<Width);

			sx += SrcStepX[x&15];
		}
		Dst += DstPitch;

		while (SrcStepY[y&15]==0 && ++y<Height)
		{
			memcpy(Dst,Dst-DstPitch,2*Width);
			Dst += DstPitch;
		}

		sy0 = sy;
		sy += SrcStepY[y&15];
		Src[0] += SrcPitch * (sy - sy0);
		Src[1] += (SrcPitch >> SrcUVPitch2) * ((sy >> SrcUVY2) - (sy0 >> SrcUVY2));
		Src[2] += (SrcPitch >> SrcUVPitch2) * ((sy >> SrcUVY2) - (sy0 >> SrcUVY2));
	}
}

static void STDCALL BlitUniversal(blit_soft* This, uint8_t** DstPtr,uint8_t** SrcPtr,int DstPitch,int SrcPitch,
	               int Width,int Height,uintptr_t Src2SrcLast) 
{
	//this will be very-very slow, only for compability and testing

	int i,j,x,y;

	uint8_t* Src[3];
	uint8_t* Dst[3];

	int SrcType = This->SrcType;
	int DstType = This->DstType;

	int DstMask[3],DstPos[3];
	int SrcMask[3],SrcPos[3];
	int DitherMask[3];

	uint8_t* PalLookUp = (uint8_t*)This->LookUp_Data;
	uint8_t* wp;
	int Flags = This->FX.Flags;
	bool_t Dither = (Flags & BLITFX_DITHER) != 0 && !(This->Dst.Flags & PF_FOURCC);
	bool_t PalDither = Dither && (This->Dst.Flags & PF_PALETTE);

	const rgb* SrcPalette = This->SrcPalette;

	int SrcStepX[16];
	int SrcStepY[16];
	int RScaleX = This->RScaleX;
	int RScaleY = This->RScaleY;

	int SrcUVPitch2 = This->SrcUVPitch2;
	int SrcUVX2 = This->SrcUVX2;
	int SrcUVY2 = This->SrcUVY2;
	int DstUVX2 = This->DstUVX2;
	int DstUVY2 = This->DstUVY2;
	int DstStepX = This->DstBPP;
	int DstStepX2 = This->DstBPP;
	int DstStepY = DstPitch << 3;
	int DstStepY2 = DstStepY >> This->DstUVPitch2;
	int dy = 0;
	int dy2 = 0;

	int cR,cG,cB;
	int Y,U,V;
	int sy0;
	int sy=0;
	int YAdd = This->FX.Brightness;

	uint32_t SrcInvert = 0;
	uint32_t DstInvert = 0;

	SrcMask[0] = This->Src.BitMask[0];
	SrcMask[1] = This->Src.BitMask[1];
	SrcMask[2] = This->Src.BitMask[2];

	DstMask[0] = This->Dst.BitMask[0];
	DstMask[1] = This->Dst.BitMask[1];
	DstMask[2] = This->Dst.BitMask[2];

	for (i=0;i<3;++i)
	{
		DitherMask[i] = Dither ? (1 << (8 - BitMaskSize(DstMask[i]))) - 1 : 0;
		SrcPos[i] = BitMaskPos(SrcMask[i]) + BitMaskSize(SrcMask[i]);
		DstPos[i] = BitMaskPos(DstMask[i]) + BitMaskSize(DstMask[i]);
		SrcMask[i] <<= 8;
		DstMask[i] <<= 8;
		Src[i] = SrcPtr[i];
		Dst[i] = DstPtr[i];
	}

	if (This->DstPalette)
	{
		DstPos[0] = 11;
		DstPos[1] = 7;
		DstPos[2] = 3; 
		DstMask[0] = 0xF0000;
		DstMask[1] = 0x0F000;
		DstMask[2] = 0x00F00;
	}

	for (j=0;j<16;++j)
	{
		SrcStepX[j] = ((RScaleX * (j+1)) >> 4) - ((RScaleX * j) >> 4);
		SrcStepY[j] = ((RScaleY * (j+1)) >> 4) - ((RScaleY * j) >> 4);
	}

	if (This->DstLeftRight)
	{	
		DstStepX = -DstStepX;
		DstStepX2 = -DstStepX2;
		if (This->DstBPP < 8)
			dy = 8 - This->DstBPP;
	}

	if (This->SwapXY)
	{
		SwapInt(&DstUVX2,&DstUVY2);
		SwapInt(&DstStepX,&DstStepY);
		SwapInt(&DstStepX2,&DstStepY2);
	}

	cR = (DitherMask[0] >> 1);
	cG = (DitherMask[1] >> 1);
	cB = (DitherMask[2] >> 1);
	Y=U=V=0;

	if (This->Src.Flags & PF_INVERTED)
		SrcInvert = (This->Src.BitCount>=32?0:(1 << This->Src.BitCount))-1;
	if (This->Dst.Flags & PF_INVERTED)
		DstInvert = (This->Dst.BitCount>=32?0:(1 << This->Dst.BitCount))-1;

	for (y=0;y<Height;++y)
	{
		int dx=dy;
		int dx2=dy2;
		int sx=0;

		for (x=0;x<Width;sx+=SrcStepX[x&15],++x,dx+=DstStepX)
		{
			uint8_t* q;
			const rgb* p;
			uint32_t v,w;

			switch (SrcType)
			{
			case 10: //Planar YUV->RGB
				cR += ((Src[0][sx]+YAdd-16)*0x2568						              + 0x3343*(Src[2][sx>>SrcUVX2]-128)) /0x2000;
				cG += ((Src[0][sx]+YAdd-16)*0x2568 - 0x0c92*(Src[1][sx>>SrcUVX2]-128)  - 0x1a1e*(Src[2][sx>>SrcUVX2]-128)) /0x2000;
				cB += ((Src[0][sx]+YAdd-16)*0x2568 + 0x40cf*(Src[1][sx>>SrcUVX2]-128))                                     /0x2000;

				cR=SAT(cR);
				cG=SAT(cG);
				cB=SAT(cB);
				break;
			case 11: //Packed YUV->RGB
				cR += ((Src[0][sx*2]+YAdd-16)*0x2568									  + 0x3343*(Src[2][4*(sx>>1)]-128)) /0x2000;
				cG += ((Src[0][sx*2]+YAdd-16)*0x2568 - 0x0c92*(Src[1][4*(sx>>1)]-128)  - 0x1a1e*(Src[2][4*(sx>>1)]-128)) /0x2000;
				cB += ((Src[0][sx*2]+YAdd-16)*0x2568 + 0x40cf*(Src[1][4*(sx>>1)]-128))                                   /0x2000;

				cR=SAT(cR);
				cG=SAT(cG);
				cB=SAT(cB);
				break;
			case 12: //Planar YUV->YUV
				Y += Src[0][sx];
				U += Src[1][sx>>SrcUVX2];
				V += Src[2][sx>>SrcUVX2];

				//Y=SAT(Y);
				//U=SAT(U);
				//V=SAT(V);
				break;
			case 13: //Packed YUV->YUV
				Y += Src[0][sx*2];
				U += Src[1][4*(sx>>1)];
				V += Src[2][4*(sx>>1)];

				//Y=SAT(Y);
				//U=SAT(U);
				//V=SAT(V);
				break;
			case 1: //Pal1->RGB
				p = &SrcPalette[ ((Src[0][sx>>3] >> ((~sx)&7)) & 1) ^ SrcInvert];
				cR += p->c.r; cG += p->c.g; cB += p->c.b;

				cR=SAT(cR);
				cG=SAT(cG);
				cB=SAT(cB);
				break;
			case 2: //Pal2->RGB
				p = &SrcPalette[ ((Src[0][sx>>2] >> (((~sx)&3)*2)) & 3) ^ SrcInvert];
				cR += p->c.r; cG += p->c.g; cB += p->c.b;

				cR=SAT(cR);
				cG=SAT(cG);
				cB=SAT(cB);
				break;
			case 3: //Pal4->RGB
				p = &SrcPalette[ ((Src[0][sx>>1] >> (((~sx)&1)*4)) & 15) ^ SrcInvert];
				cR += p->c.r; cG += p->c.g; cB += p->c.b;

				cR=SAT(cR);
				cG=SAT(cG);
				cB=SAT(cB);
				break;
			case 4: //Pal8->RGB
				p = &SrcPalette[Src[0][sx] ^ SrcInvert];
				cR += p->c.r; cG += p->c.g; cB += p->c.b;

				cR=SAT(cR);
				cG=SAT(cG);
				cB=SAT(cB);
				break;
			case 5: //RGB8->RGB
				v = Src[0][sx];
				v ^= SrcInvert;
				v <<= 8;
				cR += (v & SrcMask[0]) >> SrcPos[0];
				cG += (v & SrcMask[1]) >> SrcPos[1];
				cB += (v & SrcMask[2]) >> SrcPos[2];

				cR=SAT(cR);
				cG=SAT(cG);
				cB=SAT(cB);
				break;
			case 6: //RGB16->RGB
				v = ((uint16_t*)Src[0])[sx];
				v ^= SrcInvert;
				v <<= 8;
				cR += (v & SrcMask[0]) >> SrcPos[0];
				cG += (v & SrcMask[1]) >> SrcPos[1];
				cB += (v & SrcMask[2]) >> SrcPos[2];

				cR=SAT(cR);
				cG=SAT(cG);
				cB=SAT(cB);
				break;
			case 7: //RGB24->RGB
				v = Src[0][sx*3] | (Src[0][sx*3+1] << 8) | (Src[0][sx*3+2] << 16);
				v ^= SrcInvert;
				v <<= 8;
				cR += (v & SrcMask[0]) >> SrcPos[0];
				cG += (v & SrcMask[1]) >> SrcPos[1];
				cB += (v & SrcMask[2]) >> SrcPos[2];

				cR=SAT(cR);
				cG=SAT(cG);
				cB=SAT(cB);
				break;
			default: //RGB32->RGB
				v = ((uint32_t*)Src[0])[sx];
				v ^= SrcInvert;
				v <<= 8;
				cR += (v & SrcMask[0]) >> SrcPos[0];
				cG += (v & SrcMask[1]) >> SrcPos[1];
				cB += (v & SrcMask[2]) >> SrcPos[2];

				cR=SAT(cR);
				cG=SAT(cG);
				cB=SAT(cB);
				break;
			}

			q = Dst[0]+(dx >> 3);
			switch (DstType)
			{
			case 10: //RGB->Planar YUV
				Y = ((2105 * cR) + (4128 * cG) + (802 * cB))/0x2000 + 16;
				U = (-(1212 * cR) - (2384 * cG) + (3596 * cB))/0x2000 + 128;
				V = ((3596 * cR) - (3015 * cG) - (582 * cB))/0x2000 + 128;

				*q = (uint8_t)Y;
				Dst[1][dx2 >> 3] = (uint8_t)U;
				Dst[2][dx2 >> 3] = (uint8_t)V;
				if ((x & 1) || DstUVX2==0)
					dx2+=DstStepX2;
				cR=cG=cB=0;
				break;
			case 11: //RGB->Packed YUV
				Y = ((2105 * cR) + (4128 * cG) + (802 * cB))/0x2000 + 16;
				U = (-(1212 * cR) - (2384 * cG) + (3596 * cB))/0x2000 + 128;
				V = ((3596 * cR) - (3015 * cG) - (582 * cB))/0x2000 + 128;
				Y=SAT(Y);
				U=SAT(U);
				V=SAT(V);
				*q = (uint8_t)Y;
				Dst[1][4*(dx >> 5)] = (uint8_t)U;
				Dst[2][4*(dx >> 5)] = (uint8_t)V;
				cR=cG=cB=0;
				break;
			case 12: //YUV->Planar YUV
				Y += YAdd;
				Y=SAT(Y);
				*q = (uint8_t)Y;
				Dst[1][dx2 >> 3] = (uint8_t)U;
				Dst[2][dx2 >> 3] = (uint8_t)V;
				if ((x & 1) || DstUVX2==0)
					dx2+=DstStepX2;
				Y=U=V=0;
				break;
			case 13: //YUV->Packed YUV
				Y += YAdd;
				Y=SAT(Y);
				*q = (uint8_t)Y;
				Dst[1][4*(dx >> 5)] = (uint8_t)U;
				Dst[2][4*(dx >> 5)] = (uint8_t)V;
				Y=U=V=0;
				break;
			case 1: //RGB->Pal1
				w = ((cR << DstPos[0]) & DstMask[0]) |
					((cG << DstPos[1]) & DstMask[1]) |
					((cB << DstPos[2]) & DstMask[2]);

				wp = PalLookUp + (w >> 8)*4;
				w = wp[0];
				w ^= DstInvert;
				*q &= ~(1 << ((~dx)&7));
				*q |=  (w << ((~dx)&7));

				if (PalDither)
				{
					cR -= wp[1];
					cG -= wp[2];
					cB -= wp[3];
				}
				else
					cR=cG=cB=0;
				break;
			case 2: //RGB->Pal2
				w = ((cR << DstPos[0]) & DstMask[0]) |
					((cG << DstPos[1]) & DstMask[1]) |
					((cB << DstPos[2]) & DstMask[2]);

				wp = PalLookUp + (w >> 8)*4;
				w = wp[0];
				w ^= DstInvert;
				*q &= ~(3 << ((~dx)&6));
				*q |=  (w << ((~dx)&6));

				if (PalDither)
				{
					cR -= wp[1];
					cG -= wp[2];
					cB -= wp[3];
				}
				else
					cR=cG=cB=0;
				break;
			case 3: //RGB->Pal4
				w = ((cR << DstPos[0]) & DstMask[0]) |
					((cG << DstPos[1]) & DstMask[1]) |
					((cB << DstPos[2]) & DstMask[2]);

				wp = PalLookUp + (w >> 8)*4;
				w = wp[0];
				w ^= DstInvert;
				*q &= ~(15 << ((~dx)&4));
				*q |=  (w  << ((~dx)&4));

				if (PalDither)
				{
					cR -= wp[1];
					cG -= wp[2];
					cB -= wp[3];
				}
				else
					cR=cG=cB=0;
				break;
			case 14: //YUV->Pal8
				w = ((Y << DstPos[0]) & DstMask[0]) |
					((U << DstPos[1]) & DstMask[1]) |
					((V << DstPos[2]) & DstMask[2]);

				wp = PalLookUp + (w >> 8)*4;
				w = wp[0];
				w ^= DstInvert;
				*q = (uint8_t)w;

				if (PalDither)
				{
					Y -= wp[1];
					U -= wp[2];
					V -= wp[3];
				}
				else
					Y=U=V=0;
				break;

			case 4: //RGB->Pal8
				w = ((cR << DstPos[0]) & DstMask[0]) |
					((cG << DstPos[1]) & DstMask[1]) |
					((cB << DstPos[2]) & DstMask[2]);

				wp = PalLookUp + (w >> 8)*4;
				w = wp[0];
				w ^= DstInvert;
				*q = (uint8_t)w;
				if (PalDither)
				{
					cR -= wp[1];
					cG -= wp[2];
					cB -= wp[3];
				}
				else
					cR=cG=cB=0;
				break;
			case 5: //RGB->RGB8
				w = ((cR << DstPos[0]) & DstMask[0]) |
					((cG << DstPos[1]) & DstMask[1]) |
					((cB << DstPos[2]) & DstMask[2]);
				*q = (uint8_t)((w >> 8) ^ DstInvert);
				cR &= DitherMask[0];
				cG &= DitherMask[1];
				cB &= DitherMask[2];
				break;
			case 6: //RGB->RGB16
				w = ((cR << DstPos[0]) & DstMask[0]) |
					((cG << DstPos[1]) & DstMask[1]) |
					((cB << DstPos[2]) & DstMask[2]);
				*(uint16_t*)q = (uint16_t)((w >> 8) ^ DstInvert);
				cR &= DitherMask[0];
				cG &= DitherMask[1];
				cB &= DitherMask[2];
				break;
			case 7: //RGB->RGB24
				w = ((cR << DstPos[0]) & DstMask[0]) |
					((cG << DstPos[1]) & DstMask[1]) |
					((cB << DstPos[2]) & DstMask[2]);
				w >>= 8;
				w ^= DstInvert;
				q[0] = (uint8_t)(w);
				q[1] = (uint8_t)(w >> 8);
				q[2] = (uint8_t)(w >> 16);
				cR &= DitherMask[0];
				cG &= DitherMask[1];
				cB &= DitherMask[2];
				break;
			default: //RGB->RGB32
				w = ((cR << DstPos[0]) & DstMask[0]) |
					((cG << DstPos[1]) & DstMask[1]) |
					((cB << DstPos[2]) & DstMask[2]);
				*(uint32_t*)q = (w >> 8) ^ DstInvert;
				cR &= DitherMask[0];
				cG &= DitherMask[1];
				cB &= DitherMask[2];
				break;
			}
		}

		dy += DstStepY;
		if ((y & 1) || DstUVY2==0)
			dy2 += DstStepY2;

		sy0 = sy;
		sy += SrcStepY[y&15];
		Src[0] += SrcPitch * (sy - sy0);
		Src[1] += (SrcPitch >> SrcUVPitch2) * ((sy >> SrcUVY2) - (sy0 >> SrcUVY2));
		Src[2] += (SrcPitch >> SrcUVPitch2) * ((sy >> SrcUVY2) - (sy0 >> SrcUVY2));
	}
}

#endif

static bool_t BlitCompile(blit_soft* p,
				   const pixel* NewDst,const pixel* NewSrc,
		  		   const blitfx* NewFX,bool_t NewOnlyDiff)
{
	int i;
	bool_t Gray,SrcPlanarYUV,DstPlanarYUV;

	if (EqPixel(&p->Dst,NewDst) && 
		EqPixel(&p->Src,NewSrc) && 
		EqBlitFX(&p->FX,NewFX) &&
		NewOnlyDiff == p->OnlyDiff)
		return p->Entry != NULL;

	CodeStart(&p->Code);

	// defaults
	p->Caps = VC_BRIGHTNESS|VC_DITHER|VC_SATURATION|VC_CONTRAST|VC_RGBADJUST;
	p->SrcAlignPos = 2;
	p->DstAlignPos = 2;
	p->DstAlignSize = 2;

	p->Dst = *NewDst;
	p->Src = *NewSrc;
	p->FX = *NewFX;
	p->OnlyDiff = (boolmem_t)NewOnlyDiff;
	p->SwapXY = (boolmem_t)((p->FX.Direction & DIR_SWAPXY) != 0);
	p->DstLeftRight = (boolmem_t)((p->FX.Direction & DIR_MIRRORLEFTRIGHT) != 0);
	p->DstUpDown = (boolmem_t)((p->FX.Direction & DIR_MIRRORUPDOWN) != 0);
	p->SrcUpDown = (boolmem_t)((p->FX.Flags & BLITFX_AVOIDTEARING) && !p->SwapXY && p->DstUpDown != ((p->FX.Flags & BLITFX_VMEMUPDOWN) != 0));
	if (p->SrcUpDown)
		p->DstUpDown = (boolmem_t)!p->DstUpDown;

	// it's faster using slices with rotation (not just with AVOIDTEARING)
	// probably because of ram page trashing (during vertical writing)
	p->Slices = (boolmem_t)(p->SwapXY != ((p->FX.Flags & BLITFX_VMEMROTATED) != 0));
	p->SlicesReverse = (boolmem_t)((p->SwapXY ? p->DstUpDown : p->DstLeftRight) == ((p->FX.Flags & BLITFX_VMEMUPDOWN) != 0));

	Gray = (p->Dst.Flags & PF_PALETTE) && (p->Dst.BitCount == 4 || p->Dst.BitCount == 2);
	p->RScaleX = CalcRScale(p->FX.ScaleX,Gray);
	p->RScaleY = CalcRScale(p->FX.ScaleY,Gray);

	// important these integeres should be 1 or 0
	p->DstHalfX = p->SrcHalfX = p->RScaleX == 32;
	p->DstHalfY = p->SrcHalfY = p->RScaleY == 32;
	p->DstDoubleX = p->SrcDoubleX = p->RScaleX == 8;
	p->DstDoubleY = p->SrcDoubleY = p->RScaleY == 8;
	if (p->SwapXY)
	{
		SwapInt(&p->DstHalfX,&p->DstHalfY);
		SwapInt(&p->DstDoubleX,&p->DstDoubleY);
	}

	p->SrcBPP = GetBPP(&p->Src);
	p->SrcBPP2 = -3;
	for (i=p->SrcBPP;i>1;i>>=1)
		++p->SrcBPP2;

	p->DstBPP = GetBPP(&p->Dst);
	p->DstBPP2 = -3;
	for (i=p->DstBPP;i>1;i>>=1)
		++p->DstBPP2;

	p->SrcYUV = (boolmem_t)AnyYUV(&p->Src);
	p->SrcPalette = DefaultPal(&p->Src);
	p->DstPalette = DefaultPal(&p->Dst);

	free(p->LookUp_Data);
	p->LookUp_Data = NULL;

	p->ColorLookup = (boolmem_t)((p->FX.Flags & BLITFX_COLOR_LOOKUP) != 0);
#ifdef ARM
	p->ARM5 = (boolmem_t)((QueryPlatform(PLATFORM_CAPS) & CAPS_ARM_5E)!=0);
	p->WMMX = (boolmem_t)((QueryPlatform(PLATFORM_CAPS) & CAPS_ARM_WMMX)!=0 && 
	          !QueryAdvanced(ADVANCED_NOWMMX) && (p->Dst.Flags & PF_16ALIGNED) && (p->Src.Flags & PF_16ALIGNED));
	p->QAdd = (boolmem_t)((QueryPlatform(PLATFORM_CAPS) & CAPS_ARM_5E)!=0 && 
		      !p->DstPalette && !p->WMMX && !(p->FX.Flags & BLITFX_DITHER) && !p->ColorLookup);
#endif
	CalcColor(p);
	if (p->DstPalette)
		BuildPalLookUp(p,p->SrcYUV);

	p->ArithStretch = (boolmem_t)((p->FX.Flags & BLITFX_ARITHSTRETCHALWAYS) != 0);
	if ((p->FX.Flags & BLITFX_ARITHSTRETCH50) && p->RScaleX==32 && p->RScaleY==32)
		p->ArithStretch = 1;
	if (p->DstPalette)
		p->ArithStretch = 0;

	SrcPlanarYUV = PlanarYUV(&p->Src,&p->SrcUVX2,&p->SrcUVY2,&p->SrcUVPitch2);
	DstPlanarYUV = PlanarYUV(&p->Dst,&p->DstUVX2,&p->DstUVY2,&p->DstUVPitch2);

	p->DirX = p->DstLeftRight ? -1:1;
	for (i=0;i<3;++i)
	{
		p->DstSize[i] = BitMaskSize(p->Dst.BitMask[i]);
		p->DstPos[i] = BitMaskPos(p->Dst.BitMask[i]);
		p->SrcSize[i] = BitMaskSize(p->Src.BitMask[i]);
		p->SrcPos[i] = BitMaskPos(p->Src.BitMask[i]);
	}

#if defined(ARM)
	if ((p->Dst.Flags & PF_RGB) && p->Dst.BitCount==16 && !p->SrcYUV)
		Any_RGB_RGB(p);
	if (!p->SrcYUV && DstPlanarYUV && p->RScaleX==16 && p->RScaleY==16)
		Fix_Any_YUV(p);
#endif

#if (defined(ARM) || defined(SH3) || defined(MIPS)) && defined(CONFIG_DYNCODE)
	if (SrcPlanarYUV)
	{
#if defined(ARM)
		if (DstPlanarYUV && p->RScaleX==16 && p->RScaleY==16 && 
			((p->SrcUVX2==p->DstUVX2 && p->SrcUVY2==p->DstUVY2 && !(p->SwapXY && p->SrcUVX2 != p->SrcUVY2)) || 
			(p->DstUVX2==1 && p->DstUVY2==1)))
			Fix_Any_YUV(p);

		if (PackedYUV(&p->Dst) && p->RScaleX==16 && p->RScaleY==16 && p->SrcUVY2<2 && p->SrcUVX2<2)
			Fix_PackedYUV_YUV(p);
#endif
		if ((p->Dst.Flags & (PF_RGB|PF_PALETTE)) && (p->Dst.BitCount == 8 || p->Dst.BitCount == 16 || p->Dst.BitCount == 32))
		{
#if defined(ARM)
			if (p->Dst.BitCount == 16 && p->WMMX && PlanarYUV420(&p->Src) && 
				(p->RScaleX==16 || p->RScaleX==8 || p->RScaleX==32) && (p->RScaleY==16 || p->RScaleY==8 || p->RScaleY==32))
				WMMXFix_RGB_UV(p);
			else
			if ((p->Dst.BitCount == 16 || p->Dst.BitCount==32) && p->SrcUVX2==1 && p->SrcUVY2==1 && p->RScaleX == 16 && p->RScaleY == 16)
				Fix_RGB_UV(p);
			else
			if (p->Dst.BitCount == 16 && p->SrcUVX2==1 && p->SrcUVY2==1 &&
				(p->RScaleX == 8 || p->RScaleX == 16) && 
				(p->RScaleY == 8 || p->RScaleY == 16) && !p->ArithStretch)
				Fix_RGB_UV(p);
			else
			if (p->Dst.BitCount == 16 && p->SrcUVX2==1 && p->SrcUVY2==1 && p->RScaleX == 32 && p->RScaleY == 32)
				Half_RGB_UV(p);
			else
				Stretch_RGB_UV(p);
#else
#if !defined(MIPS)
			if (p->SrcUVX2==1 && p->SrcUVY2==1 && 
				(p->RScaleX == 8 || p->RScaleX == 16) &&
				(p->RScaleY == 8 || p->RScaleY == 16))
#endif
				Fix_RGB_UV(p);
#endif
		}
		else
		if (Gray)
			Fix_Gray_UV(p);
	}
#endif

	CodeBuild(&p->Code);
	if (p->Code.Size)
		p->Entry = (blitsoftentry)p->Code.Code;
	else
		p->Entry = NULL;

#if defined(_M_IX86) && !defined(TARGET_SYMBIAN)

	if (p->FX.Direction==0 && p->RScaleX==16 && p->RScaleY==16)
	{
		uint32_t In = DefFourCC(&p->Src);
		uint32_t Out = DefFourCC(&p->Dst);
#ifdef CONFIG_CONTEXT
		int Caps = QueryPlatform(PLATFORM_CAPS);
#else
		int Caps = CPUCaps();
#endif
		const blitmmx* i;
		for (i=BlitMMX;i->In;++i)
			if (i->In==In && i->Out==Out)
			{
				p->Caps &= ~VC_DITHER;
				if (AnyYUV(&p->Src))
					CalcYUVMMX(p);
				if (Caps & CAPS_X86_MMX2)
					p->Entry = i->Func[1];
				else if (Caps & CAPS_X86_3DNOW)
					p->Entry = i->Func[2];
				else
					p->Entry = i->Func[0];
				break;
			}
	}

#endif

	if (!p->Entry)
	{
		// use YUV internal calulaction?
		bool_t YUV = (AnyYUV(&p->Dst) || 
			(p->Dst.BitCount==8 && p->Dst.Flags & PF_PALETTE)) && p->SrcYUV;

		p->DstType = UniversalType(&p->Dst,YUV);
		p->SrcType = UniversalType(&p->Src,YUV);

#if defined(_M_IX86) || !defined(CONFIG_DYNCODE) || defined(BLITTEST)

		if (p->SrcType>=0 && p->DstType>=0)
		{
			// universal
			if (AnyYUV(&p->Dst)) 
				p->Caps &= ~VC_DITHER;
			else
				p->Caps = VC_BRIGHTNESS | VC_DITHER;
			p->Entry = BlitUniversal; 
		}

		if (p->SrcUVX2 == 1 && !p->SwapXY && !p->DstLeftRight && p->SrcType == 10)
		{
			if (p->DstType == 8 && 
				p->Dst.BitMask[0] == 0xFF0000 &&
				p->Dst.BitMask[1] == 0x00FF00 &&
				p->Dst.BitMask[2] == 0x0000FF)
			{
				p->Caps = VC_BRIGHTNESS;
				p->Entry = Blit_PYUV_RGB32;
			}

			if (p->DstType == 6 && 
				p->Dst.BitMask[0] == 0xF800 &&
				p->Dst.BitMask[1] == 0x07E0 &&
				p->Dst.BitMask[2] == 0x001F)
			{
				p->Caps = VC_BRIGHTNESS;
				p->Entry = Blit_PYUV_RGB16;
			}
		}

		if (p->SrcUVX2 == p->DstUVX2 &&	!p->SwapXY && !p->DstLeftRight &&
			p->RScaleX == 16 && p->RScaleY==16 &&
			p->SrcType == 12 &&	p->DstType == 12)
		{
			CalcYUVLookUp(p);
			p->Caps &= ~VC_DITHER;
			p->Entry = Blit_PYUV_PYUV;
		}
#endif
		if (p->SrcUVX2 == p->DstUVX2 &&	
			p->SrcUVY2 == p->DstUVY2 &&	!p->SwapXY && !p->DstLeftRight &&
			(p->RScaleX != 16 || p->RScaleY != 16) &&
			(p->RScaleX == 16 || p->RScaleX == 8 || p->RScaleX == 32 || p->RScaleX == 4 || p->RScaleX == 64) && 
			(p->RScaleY == 16 || p->RScaleY == 8 || p->RScaleY == 32 || p->RScaleX == 4 || p->RScaleX == 64) && 
			p->SrcType == 12 &&	p->DstType == 12 &&
			!p->FX.Saturation && !p->FX.Contrast && !p->FX.RGBAdjust[0] && 
			!p->FX.RGBAdjust[1] && !p->FX.RGBAdjust[2] && !p->FX.Brightness)
		{
			p->Caps = 0;
			p->Entry = Blit_PYUV_PYUV_2;
		}
	}

	return p->Entry != NULL;
}

blitpack* BlitCreate(const video* Dst, 
			         const video* Src, const blitfx* FX, int* OutCaps)
{
	blitfx CopyFX;
	bool_t Gray;

	blitpack* p = BlitAlloc();
	if (!p)	return NULL;

	if (!FX)
	{
		memset(&CopyFX,0,sizeof(CopyFX));
		CopyFX.ScaleX = SCALE_ONE;
		CopyFX.ScaleY = SCALE_ONE;
		FX = &CopyFX;
	}

	if (!BlitCompile(&p->Code[0],&Dst->Pixel,&Src->Pixel,FX,0) ||
		((FX->Flags & BLITFX_ONLYDIFF) && !BlitCompile(&p->Code[1],&Dst->Pixel,&Src->Pixel,FX,1)))
	{
		BlitRelease(p);
		return NULL;
	}

	p->FX = *FX;
	p->Dst = *Dst;
	p->Src = *Src;

	Gray = (Dst->Pixel.Flags & PF_PALETTE) && 
		   (Dst->Pixel.BitCount == 4 || Dst->Pixel.BitCount == 2);

	p->RScaleX = CalcRScale(FX->ScaleX,Gray);
	p->RScaleY = CalcRScale(FX->ScaleY,Gray);

	if (OutCaps)
		*OutCaps = p->Code[0].Caps;

	return p;
}

static NOINLINE int CMul(blit_soft* p, int64_t* r, int64_t v, bool_t UV)
{
	int m;

	if (UV)
	{
		m = p->FX.Saturation;
		if (m<0) m >>= 1; // adjust negtive interval: -128..0 -> -64..0
		m = 4*m+256;
		v *= m;
		if (v<0) 
			v -= 128; 
		else 
			v += 128;
		v >>= 8;
	}

	m = p->FX.Contrast;
	if (m<0) m >>= 1; // adjust negtive interval: -128..0 -> -64..0
	m = 4*m + 256;
	v *= m;
	if (v<0) 
		v -= 128; 
	else 
		v += 128;
	v >>= 8;

	if (r) *r = v;
	return (int)v;
}

static NOINLINE int CAdd(blit_soft* p, int64_t* r, int64_t v, int v0, int vUV)
{
	int m;

	m = p->FX.Saturation;
	if (m<0) m >>= 1; // adjust negtive interval: -128..0 -> -64..0
	m = 4*m+256;
	v += ((256 - m) * vUV) >> 1;

	m = p->FX.Contrast;
	if (m<0) m >>= 1; // adjust negtive interval: -128..0 -> -64..0
	m = 4*m + 256;
	v += ((256 - m)*(v0 - v)) >> 8;

	if (r) *r = v;
	return (int)v;
}

static INLINE bool_t InRange32(int64_t v)
{
	return v >= -MAX_INT && v <= MAX_INT;
}

static INLINE uint8_t RotateRight8(int v,int Bits)
{
	Bits &= 7;
	return (uint8_t)(((v >> Bits) & ((1 << (8-Bits))-1)) | (v << (8-Bits)));
}

void CalcPalYUVLookUp(blit_soft* p)
{
	p->LookUp_Size = 256+4+256+256;
	p->LookUp_Data = malloc(p->LookUp_Size);
	if (p->LookUp_Data)
	{
		int v,i,n = 1<<p->Src.BitCount;
		rgb* Pal = p->Src.Palette;
		uint8_t* LookUp = p->LookUp_Data;
		for (i=0;i<n;++i)
		{
			v = (2105*Pal[i].c.r + 4128*Pal[i].c.g + 802*Pal[i].c.b)/0x2000 + 16 + p->FX.Brightness;
			LookUp[i] = (uint8_t)SAT(v);
			v = (-1212*Pal[i].c.r -2384*Pal[i].c.g + 3596*Pal[i].c.b)/0x2000 + 128;
			LookUp[i+256+4] = (uint8_t)SAT(v);
			v = (3596*Pal[i].c.r -3015*Pal[i].c.g -582*Pal[i].c.b)/0x2000 + 128;
			LookUp[i+256+4+256] = (uint8_t)SAT(v);
		}
	}
}

void CalcPalRGBLookUp(blit_soft* p)
{
	int i,n = 1<<p->Src.BitCount;
	int w = p->Dst.BitCount>>3;
	if (w<2 || w>4) return;
	if (w==3) w=4;

	p->LookUp_Size = w*n;
	p->LookUp_Data = malloc(p->LookUp_Size);
	if (p->LookUp_Data)
	{
		uint16_t* Pal = p->LookUp_Data;
		for (i=0;i<n;++i)
		{
			uint32_t c = RGBToFormat(p->Src.Palette[i].v,&p->Dst);
			if (w==2)
				*(Pal++) = (uint16_t)c;
			else
			{
				*(uint32_t*)Pal = c;
				Pal += 2;
			}
		}
	}
}

void CalcYUVLookUp(blit_soft* p)
{
	if (p->FX.Saturation || p->FX.Contrast || (p->FX.Flags & BLITFX_DITHER) ||
		p->FX.RGBAdjust[0] || p->FX.RGBAdjust[1] || p->FX.RGBAdjust[2])
	{
		p->LookUp_Size = 3*256+4;
		p->LookUp_Data = malloc(p->LookUp_Size);

		if (p->LookUp_Data)
		{
			int Ofs = (p->FX.Flags & BLITFX_DITHER)?-2:0;
			int n;
			uint8_t* i=p->LookUp_Data;
			for (n=0;n<256+4;++n,++i)
			{
				int y = CMul(p,NULL,n+Ofs+p->FX.Brightness+p->FX.RGBAdjust[1]-128,0)+128;
				*i = (uint8_t)SAT(y);
			}
			for (n=0;n<256;++n,++i)
			{
				int u = CMul(p,NULL,n-128-p->FX.RGBAdjust[1]+p->FX.RGBAdjust[2],1)+128;
				*i = (uint8_t)SAT(u);
			}
			for (n=0;n<256;++n,++i)
			{
				int v = CMul(p,NULL,n-128-p->FX.RGBAdjust[1]+p->FX.RGBAdjust[0],1)+128;
				*i = (uint8_t)SAT(v);
			}
		}
	}
}

void CalcLookUp(blit_soft* p, bool_t Dither)
{
#if defined(ARM)
	const int Safe = 512;

	// 4*256:Y + (256+Safe*2)*n:SAT + 4*256:U + 4*256:V 

	int SatOfsR = 4*256+Safe;
	int SatOfsG = 4*256+Safe;
	int SatOfsB = 4*256+Safe;

	p->LookUp_Size = 4*256*3 + (256+Safe*2);
	if (Dither)
	{
		if (p->DstSize[1] == p->DstSize[0])
			SatOfsG = SatOfsR;
		else
		{
			p->LookUp_Size += (256+Safe*2);
			SatOfsG = SatOfsR + (256+Safe*2);
		}

		if (p->DstSize[2] == p->DstSize[0])
			SatOfsB = SatOfsR;
		else
		if (p->DstSize[2] == p->DstSize[1])
			SatOfsB = SatOfsG;
		else
		{
			p->LookUp_Size += (256+Safe*2);
			SatOfsB = SatOfsG + (256+Safe*2);
		}
	}
	p->LookUp_U = (p->LookUp_Size - 4*256*2) >> 2;
	p->LookUp_V = (p->LookUp_Size - 4*256) >> 2;
	p->LookUp_Data = malloc(p->LookUp_Size);

	if (p->LookUp_Data)
	{
		int v;
		int32_t* YMul = (int32_t*)p->LookUp_Data;
		int32_t* UMul = (int32_t*)p->LookUp_Data + p->LookUp_U;
		int32_t* VMul = (int32_t*)p->LookUp_Data + p->LookUp_V;
		uint8_t* SatR = (uint8_t*)p->LookUp_Data + SatOfsR;
		uint8_t* SatG = (uint8_t*)p->LookUp_Data + SatOfsG;
		uint8_t* SatB = (uint8_t*)p->LookUp_Data + SatOfsB;

		memset(p->LookUp_Data,0,p->LookUp_Size);

		for (v=0;v<256;++v)
		{
			*YMul = (p->_YMul * v) << LOOKUP_FIX;
			*UMul = (((p->_BUMul * v + p->_BAdd + (SatOfsB << 16)) << LOOKUP_FIX) & 0xFFFF0000) |
					(((p->_GUMul * v) >> (16-LOOKUP_FIX)) & 0xFFFF); //BUMul | GUMul
			*VMul = (((p->_RVMul * v + p->_RAdd + (SatOfsR << 16)) << LOOKUP_FIX) & 0xFFFF0000) |
					(((p->_GVMul * v + p->_GAdd + (SatOfsG << 16)) >> (16-LOOKUP_FIX)) & 0xFFFF); //BUMul | GUMul

			++YMul;
			++UMul;
			++VMul;
		}

		if (Dither)
		{
			for (v=-Safe;v<256+Safe;++v)
			{
				SatR[v] = RotateRight8(SAT(v),8-p->DstSize[0]);
				SatG[v] = RotateRight8(SAT(v),8-p->DstSize[1]);
				SatB[v] = RotateRight8(SAT(v),8-p->DstSize[2]);
			}
		}
		else
		{
			for (v=-Safe;v<256+Safe;++v)
				SatR[v] = (uint8_t)SAT(v);
		}
	}
#elif defined(MIPS)
	const int Safe = 256;

	// 256:Y + (128+Safe*2)*n:SAT + 8*256:U + 8*256:V 

	int SatOfsR = 256+Safe;
	int SatOfsG = 256+Safe;
	int SatOfsB = 256+Safe;

	p->LookUp_Size = 256+8*256*2 + (128+Safe*2);
	if (p->DstSize[1] == p->DstSize[0])
		SatOfsG = SatOfsR;
	else
	{
		p->LookUp_Size += (128+Safe*2);
		SatOfsG = SatOfsR + (128+Safe*2);
	}

	if (p->DstSize[2] == p->DstSize[0])
		SatOfsB = SatOfsR;
	else
	if (p->DstSize[2] == p->DstSize[1])
		SatOfsB = SatOfsG;
	else
	{
		p->LookUp_Size += (128+Safe*2);
		SatOfsB = SatOfsG + (128+Safe*2);
	}
	p->LookUp_U = p->LookUp_Size - 8*256*2;
	p->LookUp_V = p->LookUp_Size - 8*256;
	p->LookUp_Data = malloc(p->LookUp_Size);

	if (p->LookUp_Data)
	{
		int v;
		uint8_t* YMul = (uint8_t*)p->LookUp_Data;
		uint8_t** UMul = (uint8_t**)((uint8_t*)p->LookUp_Data + p->LookUp_U);
		uint8_t** VMul = (uint8_t**)((uint8_t*)p->LookUp_Data + p->LookUp_V);

		uint8_t* SatR = (uint8_t*)p->LookUp_Data + SatOfsR;
		uint8_t* SatG = (uint8_t*)p->LookUp_Data + SatOfsG;
		uint8_t* SatB = (uint8_t*)p->LookUp_Data + SatOfsB;

		memset(p->LookUp_Data,0,p->LookUp_Size);

		for (v=0;v<256;++v)
		{
			*YMul = (uint8_t)((p->_YMul * v) >> 17);

			UMul[0] = SatB + ((p->_BUMul * v + p->_BAdd) >> 17);
			*(int*)&UMul[1] = (p->_GUMul * v) >> 17;

			VMul[0] = SatR + ((p->_RVMul * v + p->_RAdd) >> 17);
			VMul[1] = SatG + ((p->_GVMul * v + p->_GAdd) >> 17);

			++YMul;
			UMul+=2;
			VMul+=2;
		}

		for (v=-Safe;v<128+Safe;++v)
		{
			SatR[v] = (uint8_t)(SAT(v*2) >> (8-p->DstSize[0]));
			SatG[v] = (uint8_t)(SAT(v*2) >> (8-p->DstSize[1]));
			SatB[v] = (uint8_t)(SAT(v*2) >> (8-p->DstSize[2]));
		}
	}
#elif defined(SH3)
	const int Safe = 256;

	//LookUp_Data:
	// [128..255|0..127]Y + 384 empty + 4*[128..255|0..127]U + 4*[128..255|0..127]V + (128+Safe*2)*n:SAT 

	int SatOfsR = 256+384+4*256*2+Safe;
	int SatOfsG = 256+384+4*256*2+Safe;
	int SatOfsB = 256+384+4*256*2+Safe;

	p->LookUp_Size = 256+384+4*256*2 + (128+Safe*2);
	if (p->DstSize[1] == p->DstSize[0])
		SatOfsG = SatOfsR;
	else
	{
		p->LookUp_Size += (128+Safe*2);
		SatOfsG = SatOfsR + (128+Safe*2);
	}

	if (p->DstSize[2] == p->DstSize[0])
		SatOfsB = SatOfsR;
	else
	if (p->DstSize[2] == p->DstSize[1])
		SatOfsB = SatOfsG;
	else
	{
		p->LookUp_Size += (128+Safe*2);
		SatOfsB = SatOfsG + (128+Safe*2);
	}
	p->LookUp_U = 128 + 384 + 4*128;
	p->LookUp_V = 128 + 384 + 4*256 + 4*128;
	p->LookUp_Data = malloc(p->LookUp_Size + Safe*2); //additional safe
	if (p->LookUp_Data)
	{
		int v;
		int8_t* YMul = (int8_t*)p->LookUp_Data;
		int16_t* UMul = (int16_t*)((uint8_t*)p->LookUp_Data + 256 + 384);
		int16_t* VMul = (int16_t*)((uint8_t*)p->LookUp_Data + 256 + 384 + 4*256);

		int8_t* SatR = (int8_t*)p->LookUp_Data + SatOfsR;
		int8_t* SatG = (int8_t*)p->LookUp_Data + SatOfsG;
		int8_t* SatB = (int8_t*)p->LookUp_Data + SatOfsB;

		memset(p->LookUp_Data,0,p->LookUp_Size + Safe*2);

		for (v=0;v<256;++v)
		{
			*YMul = (int8_t)(((p->_YMul * ((v+128)&255)) >> 17)-128);

			UMul[0] = (int16_t)(SatOfsB + ((p->_BUMul * ((v+128)&255) + p->_BAdd) >> 17));
			UMul[1] = (int16_t)((p->_GUMul * ((v+128)&255)) >> 17);

			VMul[0] = (int16_t)(SatOfsR + ((p->_RVMul * ((v+128)&255) + p->_RAdd) >> 17));
			VMul[1] = (int16_t)(SatOfsG + ((p->_GVMul * ((v+128)&255) + p->_GAdd) >> 17));

			++YMul;
			UMul+=2;
			VMul+=2;
		}

		for (v=-Safe;v<128+Safe;++v)
		{
			SatR[v] = (int8_t)(SAT(v*2) >> (8-p->DstSize[0]));
			SatG[v] = (int8_t)(SAT(v*2) >> (8-p->DstSize[1]));
			SatB[v] = (int8_t)(SAT(v*2) >> (8-p->DstSize[2]));
		}
	}
#endif
}

void CalcColor(blit_soft* p)
{
#ifdef ARM
	if (p->QAdd)
	{
		//saturation: signed 32bit / 3 (qdadd will triple)
		int64_t YMul;
		int64_t RVMul;
		int64_t RAdd;
		int64_t GUMul;
		int64_t GVMul;
		int64_t GAdd;
		int64_t BUMul;
		int64_t BAdd;

		CMul(p,&YMul,0x63C000,0);
		CMul(p,&RVMul,0x88B2AA,1);
		CAdd(p,&RAdd,(p->FX.Brightness+p->FX.RGBAdjust[0])*0x63C000 - (int64_t)0x75400000,0x2AAAAAAA,0x88B2AA);
		CMul(p,&GUMul,-0x218555,1);
		CMul(p,&GVMul,-0x45A555,1);
		CAdd(p,&GAdd,(p->FX.Brightness+p->FX.RGBAdjust[1])*0x63C000 + (int64_t)0x02AEAAAA,0x2AAAAAAA,-0x218555-0x45A555);
		CMul(p,&BUMul,0xACD2AA,1);
		CAdd(p,&BAdd,(p->FX.Brightness+p->FX.RGBAdjust[2])*0x63C000 - (int64_t)0x87500000,0x2AAAAAAA,0xACD2AA);

		if (InRange32(YMul*0xF0) && 
			InRange32(RVMul*0x10+RAdd) &&
			InRange32(RVMul*0xF0+RAdd) &&
			InRange32((GVMul+GUMul)*0x10+GAdd) &&
			InRange32((GVMul+GUMul)*0xF0+GAdd) &&
			InRange32(BUMul*0x10+BAdd) &&
			InRange32(BUMul*0xF0+BAdd))
		{
			p->_YMul = (int)YMul;
			p->_RVMul = (int)RVMul;
			p->_RAdd = (int)RAdd;
			p->_GUMul = (int)GUMul;
			p->_GVMul = (int)GVMul;
			p->_GAdd = (int)GAdd;
			p->_BUMul = (int)BUMul;
			p->_BAdd = (int)BAdd;
		}
		else
			p->QAdd = 0;
	}
	if (!p->QAdd)
#endif
	{
		//saturation: unsigned 24bit
		p->_YMul = CMul(p,NULL,0x12B40,0);
		p->_RVMul = CMul(p,NULL,0x19A18,1);
		p->_RAdd = CAdd(p,NULL,(p->FX.Brightness+p->FX.RGBAdjust[0])*0x12B40 - 0x0DFC000,0x800000,0x19A18);
		p->_GUMul = CMul(p,NULL,-0x06490,1);
		p->_GVMul = CMul(p,NULL,-0x0D0F0,1);
		p->_GAdd = CAdd(p,NULL,(p->FX.Brightness+p->FX.RGBAdjust[1])*0x12B40 + 0x0880C00,0x800000,-0x06490-0x0D0F0);
		p->_BUMul = CMul(p,NULL,0x20678,1);
		p->_BAdd = CAdd(p,NULL,(p->FX.Brightness+p->FX.RGBAdjust[2])*0x12B40 - 0x115F000,0x800000,0x20678);
	}
}


#if defined(_M_IX86) && !defined(TARGET_SYMBIAN)

static void ColMMX(blit_soft* p, int i, int ofs)
{
	int mul = 2048;
	int m;
	int o = 128;

	if (i>0)
	{
		m = p->FX.Saturation;
		if (m<0) m >>= 1; // adjust negtive interval: -128..0 -> -64..0
		m = 4*m+256;

		mul = (mul*m+128) >> 8;
		ofs += 128 - (m>>1);
		o = (o*m+128)>>8;
	}

	m = p->FX.Contrast;
	if (m<0) m >>= 1; // adjust negtive interval: -128..0 -> -64..0
	m = 4*m + 256;

	mul = (mul*m+128)>>8;
	ofs += o - ((m*o)>>8);

	if ((p->Src.Flags & PF_YUV_PC) && !(p->Dst.Flags & PF_YUV_PC))
	{
		if (i==0) //y [0..255]->[16..235]
		{
			mul = (mul*219+127)/255;
			ofs = (ofs*219+127)/255+16;
		}
		else //uv [0..255]->[16..240]
		{ 
			mul = (mul*224+127)/255;
			ofs = (ofs*224+127)/255+16;
		}
	}
	else
	if (!(p->Src.Flags & PF_YUV_PC) && (p->Dst.Flags & PF_YUV_PC))
	{
		if (i==0) //y [16..235]->[0..255]
		{
			mul = (mul*255+109)/219;
			ofs = ((ofs-16)*255+109)/219;
		}
		else //uv [16..240]->[0..255]
		{ 
			mul = (mul*255+112)/224;
			ofs = ((ofs-16)*255+112)/224;
		}
	}

	if (mul<-32768) mul=32768;
	if (mul>32767) mul=32767;

	p->Col[i][0][3] = p->Col[i][0][2] = p->Col[i][0][1] = p->Col[i][0][0] = (int16_t)mul;
	p->Col[i][1][3] = p->Col[i][1][2] = p->Col[i][1][1] = p->Col[i][1][0] = (int16_t)ofs;
}

static int16_t MulMMX(blit_soft* p, int i, int mul)
{
	int m;
	if (i>0)
	{
		m = p->FX.Saturation;
		if (m<0) m >>= 1; // adjust negtive interval: -128..0 -> -64..0
		m = 4*m+256;

		mul = (mul*m+128) >> 8;
	}

	m = p->FX.Contrast;
	if (m<0) m >>= 1; // adjust negtive interval: -128..0 -> -64..0
	m = 4*m + 256;

	mul = (mul*m+128)>>8;

	if (mul<-32768) mul=32768;
	if (mul>32767) mul=32767;
	return (int16_t)mul;
}

void CalcYUVMMX(blit_soft* p)
{
	if (AnyYUV(&p->Dst))
	{
		ColMMX(p,0,p->FX.Brightness+p->FX.RGBAdjust[1]);
		ColMMX(p,1,p->FX.RGBAdjust[2]-p->FX.RGBAdjust[1]);
		ColMMX(p,2,p->FX.RGBAdjust[0]-p->FX.RGBAdjust[1]);
	}
	else
	{
		const int16_t* m = GetYUVToRGB(&p->Src);
		p->Col[0][0][3] = p->Col[0][0][2] = p->Col[0][0][1] = p->Col[0][0][0] = MulMMX(p,0,m[0]);
		p->Col[0][1][3] = p->Col[0][1][2] = p->Col[0][1][1] = p->Col[0][1][0] = (int16_t)(m[1]+p->FX.Brightness+p->FX.RGBAdjust[1]);
		p->Col[1][0][1] = p->Col[1][0][0] = MulMMX(p,1,m[2]); //u_b
		p->Col[1][0][3] = p->Col[1][0][2] = MulMMX(p,1,m[3]); //u_g
		p->Col[1][1][3] = p->Col[1][1][2] = p->Col[1][1][1] = p->Col[1][1][0] = (int16_t)(m[4]+p->FX.RGBAdjust[2]-p->FX.RGBAdjust[1]);
		p->Col[2][0][1] = p->Col[2][0][0] = MulMMX(p,2,m[5]); //v_r
		p->Col[2][0][3] = p->Col[2][0][2] = MulMMX(p,2,m[6]); //v_g
		p->Col[2][1][3] = p->Col[2][1][2] = p->Col[2][1][1] = p->Col[2][1][0] = (int16_t)(m[7]+p->FX.RGBAdjust[0]-p->FX.RGBAdjust[1]);
	}
}

#endif
