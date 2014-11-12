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
 * $Id: helper_video.c 548 2006-01-08 22:41:57Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "common.h"

bool_t EqSubtitle(const subtitle* a, const subtitle* b)
{
	return a->FourCC == b->FourCC;
}

bool_t EqAudio(const audio* a, const audio* b)
{
	return a->Bits == b->Bits &&
		   a->Channels == b->Channels &&
		   a->SampleRate == b->SampleRate &&
		   a->FracBits == b->FracBits &&
		   a->Flags == b->Flags &&
		   a->Format == b->Format;
}

bool_t EqFrac(const fraction* a, const fraction* b)
{
	if (a->Den == b->Den && a->Num == b->Num) 
		return 1;
	if (!a->Den) return b->Den==0;
	if (!b->Den) return 0;
	return (int64_t)b->Den * a->Num == (int64_t)a->Den * b->Num;
}

int BitMaskSize(uint32_t Mask)
{
	int i;
	for (i=0;Mask;++i)
		Mask &= Mask - 1;
	return i;
}

int BitMaskPos(uint32_t Mask)
{
	int i;
	for (i=0;Mask && !(Mask&1);++i) 
		Mask >>= 1;
	return i;
}

int ScaleRound(int v,int Num,int Den)
{
	int64_t i;
	if (!Den) 
		return 0;
	i = (int64_t)v * Num;
	if (i<0)
		i-=Den/2;
	else
		i+=Den/2;
	i/=Den;
	return (int)i;
}

void FillColor(uint8_t* Dst,int DstPitch,int x,int y,int Width,int Height,int BPP,int Value)
{
	if (Width>0 && Height>0)
	{
		uint16_t *p16,*p16e;
		uint32_t *p32,*p32e;
		uint8_t* End;

		Dst += y*DstPitch + (x*BPP)/8;
		End = Dst + Height * DstPitch;
		do
		{
			switch (BPP)
			{
			case 1:
				Value &= 1;
				memset(Dst,Value * 0xFF,Width >> 3);
				break;
			case 2:
				Value &= 3;
				memset(Dst,Value * 0x55,Width >> 2);
				break;
			case 4:
				Value &= 15;
				memset(Dst,Value * 0x11,Width >> 1);
				break;
			case 8:
				memset(Dst,Value,Width);
				break;
			case 16:
				p16 = (uint16_t*)Dst;
				p16e = p16+Width;
				for (;p16!=p16e;++p16)
					*p16 = (uint16_t)Value;
				break;
			case 32:
				p32 = (uint32_t*)Dst;
				p32e = p32+Width;
				for (;p32!=p32e;++p32)
					*p32 = Value;
				break;
			}
			Dst += DstPitch;
		}
		while (Dst != End);
	}
}

bool_t EqBlitFX(const blitfx* a, const blitfx* b)
{
	return a->Flags == b->Flags &&
		   a->Contrast == b->Contrast &&
		   a->Saturation == b->Saturation &&
		   a->Brightness == b->Brightness &&
		   a->Direction == b->Direction &&
		   a->RGBAdjust[0] == b->RGBAdjust[0] &&
		   a->RGBAdjust[1] == b->RGBAdjust[1] &&
		   a->RGBAdjust[2] == b->RGBAdjust[2] &&
		   a->ScaleX == b->ScaleX &&
		   a->ScaleY == b->ScaleY;
}

int CombineDir(int Src, int Blit, int Dst)
{
	//order of transformations 
	//  SrcMirror
	//  SrcSwap
	//  BlitSwap
	//  BlitMirror
	//  DstSwap
	//  DstMirror

	//should be combined to a single
	//  Swap
	//  Mirror

	if (Dst & DIR_SWAPXY)
	{
		if (Blit & DIR_MIRRORLEFTRIGHT)
			Dst ^= DIR_MIRRORUPDOWN;
		if (Blit & DIR_MIRRORUPDOWN)
			Dst ^= DIR_MIRRORLEFTRIGHT;
	}
	else
		Dst ^= Blit & (DIR_MIRRORUPDOWN|DIR_MIRRORLEFTRIGHT);
	Dst ^= Blit & DIR_SWAPXY;
	Dst ^= Src & DIR_SWAPXY;
	if (Dst & DIR_SWAPXY)
	{
		if (Src & DIR_MIRRORLEFTRIGHT)
			Dst ^= DIR_MIRRORUPDOWN;
		if (Src & DIR_MIRRORUPDOWN)
			Dst ^= DIR_MIRRORLEFTRIGHT;
	}
	else
		Dst ^= Src & (DIR_MIRRORUPDOWN|DIR_MIRRORLEFTRIGHT);

	return Dst;
}

int SurfaceRotate(const video* SrcFormat, const video* DstFormat, 
				  const planes Src, planes Dst, int Dir)
{
	blitfx FX;
	memset(&FX,0,sizeof(FX));
	FX.ScaleX = SCALE_ONE;
	FX.ScaleY = SCALE_ONE;
	FX.Direction = Dir;
	return SurfaceCopy(SrcFormat,DstFormat,Src,Dst,&FX);
}


int SurfaceCopy(const video* SrcFormat, const video* DstFormat, 
				const planes Src, planes Dst, const blitfx* FX)
{
	void* Blit;
	rect SrcRect;
	rect DstRect;

	VirtToPhy(NULL,&SrcRect,SrcFormat);
	VirtToPhy(NULL,&DstRect,DstFormat);

	Blit = BlitCreate(DstFormat,SrcFormat,FX,NULL);
	if (!Blit)
		return ERR_NOT_SUPPORTED;

	BlitAlign(Blit,&DstRect,&SrcRect);
	BlitImage(Blit,Dst,*(const constplanes*)Src,NULL,-1,-1);
	BlitRelease(Blit);

	return ERR_NONE;
}

int SurfaceAlloc(planes Ptr, const video* p)
{
	int i;
	for (i=0;i<MAXPLANES;++i)
		Ptr[i] = NULL;

	if (p->Pixel.Flags & (PF_YUV420|PF_YUV422|PF_YUV444|PF_YUV410))
	{
		int x,y,s;
		PlanarYUV(&p->Pixel,&x,&y,&s);
		Ptr[0] = Alloc16(p->Height * p->Pitch);
		Ptr[1] = Alloc16((p->Height>>y) * (p->Pitch>>s));
		Ptr[2] = Alloc16((p->Height>>y) * (p->Pitch>>s));
		if (!Ptr[0] || !Ptr[1] || !Ptr[2])
		{
			SurfaceFree(Ptr);
			return ERR_OUT_OF_MEMORY;
		}
		return ERR_NONE;
	}
	Ptr[0] = Alloc16(GetImageSize(p));
	return Ptr[0] ? ERR_NONE : ERR_OUT_OF_MEMORY;
}

void SurfaceFree(planes p)
{
	int i;
	for (i=0;i<MAXPLANES;++i)
	{
		Free16(p[i]);
		p[i] = NULL;
	}
}

int DefaultAspect(int Width,int Height)
{
	return ASPECT_ONE; //todo?
}

void DefaultPitch(video* p)
{
	p->Pitch = p->Width*GetBPP(&p->Pixel);
	if (p->Pixel.Flags & PF_RGB) 
		p->Pitch = ((p->Pitch+31)>>5)*4; // dword align
	else 
		p->Pitch = (p->Pitch+7)>>3; // byte align
}

void DefaultRGB(pixel* p, int BitCount, 
		         int RBits, int GBits, int BBits,
				 int RGaps, int GGaps, int BGaps)
{
	p->Flags = PF_RGB;
	p->BitCount = BitCount;
	p->BitMask[0] = ((1<<RBits)-1) << (RGaps+GBits+GGaps+BBits+BGaps);
	p->BitMask[1] = ((1<<GBits)-1) << (GGaps+BBits+BGaps);
	p->BitMask[2] = ((1<<BBits)-1) << (BGaps);
}

bool_t Compressed(const pixel* Fmt)
{
	return (Fmt->Flags & PF_FOURCC) && !AnyYUV(Fmt);
}

bool_t PlanarYUV(const pixel* Fmt, int* x, int* y,int *s)
{
	if (PlanarYUV420(Fmt))
	{
		if (x) *x = 1;
		if (y) *y = 1;
		if (s)
		{
			if (Fmt->Flags & PF_FOURCC &&
				((Fmt->FourCC == FOURCC_IMC2) || (Fmt->FourCC == FOURCC_IMC4)))
				*s = 0; // interleaved uv scanlines
			else
				*s = 1;
		}
		return 1;

	}
	if (PlanarYUV422(Fmt))
	{
		if (x) *x = 1;
		if (s) *s = 1;
		if (y) *y = 0;
		return 1;

	}
	if (PlanarYUV444(Fmt))
	{
		if (x) *x = 0;
		if (y) *y = 0;
		if (s) *s = 0;
		return 1;
	}
	if (PlanarYUV410(Fmt))
	{
		if (x) *x = 2;
		if (s) *s = 2;
		if (y) *y = 2;
		return 1;

	}

	if (x) *x = 0;
	if (y) *y = 0;
	if (s) *s = 0;
	return 0;
}

typedef struct rgbfourcc
{
	uint32_t FourCC;
	int BitCount;
	uint32_t BitMask[3];

} rgbfourcc;

static const rgbfourcc RGBFourCC[] =
{
	{ FOURCC_RGB32, 32, { 0xFF0000, 0xFF00, 0xFF }},
	{ FOURCC_RGB24, 24, { 0xFF0000, 0xFF00, 0xFF }},
	{ FOURCC_RGB16, 16, { 0xF800, 0x07E0, 0x001F }},
	{ FOURCC_RGB15, 16, { 0x7C00, 0x03E0, 0x001F }},
	{ FOURCC_BGR32, 32, { 0xFF, 0xFF00, 0xFF0000 }},
	{ FOURCC_BGR24, 24, { 0xFF, 0xFF00, 0xFF0000 }},
	{ FOURCC_BGR16, 16, { 0x001F, 0x07E0, 0xF800 }},
	{ FOURCC_BGR15, 16, { 0x001F, 0x03E0, 0x7C00 }},
	{0},
};

uint32_t DefFourCC(const pixel* Fmt)
{
	uint32_t FourCC=0;
	if (Fmt->Flags & PF_YUV420)
		return FOURCC_I420;
	if (Fmt->Flags & PF_YUV422)
		return FOURCC_YV16;
	if (Fmt->Flags & PF_YUV410)
		return FOURCC_YUV9;

	if (Fmt->Flags & PF_FOURCC)
	{
		FourCC = Fmt->FourCC;
		if (FourCC == FOURCC_YVU9)
			FourCC = FOURCC_YUV9;
		if (FourCC == FOURCC_IYUV || FourCC == FOURCC_YV12)
			FourCC = FOURCC_I420;
		if (FourCC == FOURCC_YUNV || FourCC == FOURCC_V422 || FourCC == FOURCC_YUYV)
			FourCC = FOURCC_YUY2;
		if (FourCC == FOURCC_Y422 || FourCC == FOURCC_UYNV)
			FourCC = FOURCC_UYVY;
	}
	else
	if (Fmt->Flags & PF_RGB)
	{
		const rgbfourcc *i;
		for (i=RGBFourCC;i->FourCC;++i)
			if (i->BitCount == Fmt->BitCount &&
				i->BitMask[0] == Fmt->BitMask[0] &&
				i->BitMask[1] == Fmt->BitMask[1] &&
				i->BitMask[2] == Fmt->BitMask[2])
			{
				FourCC = i->FourCC;
				break;
			}
	}
	return FourCC;
}

bool_t PlanarYUV420(const pixel* Fmt)
{
	if (Fmt->Flags & PF_YUV420)
		return 1;

	return (Fmt->Flags & PF_FOURCC) && 
	  	  ((Fmt->FourCC == FOURCC_YV12) ||
		   (Fmt->FourCC == FOURCC_IYUV) ||
		   (Fmt->FourCC == FOURCC_I420) ||
		   (Fmt->FourCC == FOURCC_IMC2) ||
		   (Fmt->FourCC == FOURCC_IMC4));
}

bool_t PlanarYUV410(const pixel* Fmt)
{
	if (Fmt->Flags & PF_YUV410)
		return 1;

	return (Fmt->Flags & PF_FOURCC) && 
	  	  ((Fmt->FourCC == FOURCC_YVU9) ||
		   (Fmt->FourCC == FOURCC_YUV9));
}

bool_t PlanarYUV422(const pixel* Fmt)
{
	if (Fmt->Flags & PF_YUV422)
		return 1;

	return (Fmt->Flags & PF_FOURCC) && (Fmt->FourCC == FOURCC_YV16);
}

bool_t PlanarYUV444(const pixel* Fmt)
{
	return (Fmt->Flags & PF_YUV444) != 0;
}

bool_t PackedYUV(const pixel* Fmt)
{
	return (Fmt->Flags & PF_FOURCC) && 
	
		  ((Fmt->FourCC == FOURCC_YUY2) ||
		   (Fmt->FourCC == FOURCC_YUNV) ||
		   (Fmt->FourCC == FOURCC_V422) ||
  		   (Fmt->FourCC == FOURCC_YUYV) ||
		   (Fmt->FourCC == FOURCC_VYUY) ||
		   (Fmt->FourCC == FOURCC_UYVY) ||
		   (Fmt->FourCC == FOURCC_Y422) ||
		   (Fmt->FourCC == FOURCC_YVYU) ||
		   (Fmt->FourCC == FOURCC_UYNV));
}

bool_t AnyYUV(const pixel* Fmt)
{
	return PlanarYUV420(Fmt) || 
		PlanarYUV410(Fmt) || 
		PlanarYUV422(Fmt) || 
		PlanarYUV444(Fmt) || 
		PackedYUV(Fmt);
}

uint32_t RGBToFormat(rgbval_t RGB, const pixel* Fmt)
{
	uint32_t v;
	int R,G,B;
	int Y,U,V;
	int Pos[3];

	R = (INT32LE(RGB) >> 0) & 255;
	G = (INT32LE(RGB) >> 8) & 255;
	B = (INT32LE(RGB) >> 16) & 255;

	if (AnyYUV(Fmt))
	{
		Y = ((2105 * R) + (4128 * G) + (802 * B))/0x2000 + 16;
		V = ((3596 * R) - (3015 * G) - (582 * B))/0x2000 + 128;
		U = (-(1212 * R) - (2384 * G) + (3596 * B))/0x2000 + 128;

		if (Fmt->Flags & PF_INVERTED)
		{
			Y ^= 255;
			U ^= 255;
			V ^= 255;
		}
		v =  (Fmt->BitMask[0] / 255) * Y;
		v += (Fmt->BitMask[1] / 255) * U;
		v += (Fmt->BitMask[2] / 255) * V;
	}
	else
	{
		if (Fmt->Flags & PF_INVERTED)
		{
			R ^= 255;
			G ^= 255;
			B ^= 255;
		}

		Pos[0] = BitMaskPos(Fmt->BitMask[0]) + BitMaskSize(Fmt->BitMask[0]);
		Pos[1] = BitMaskPos(Fmt->BitMask[1]) + BitMaskSize(Fmt->BitMask[1]);
		Pos[2] = BitMaskPos(Fmt->BitMask[2]) + BitMaskSize(Fmt->BitMask[2]);

		v = ((R << Pos[0]) & (Fmt->BitMask[0] << 8)) |
			((G << Pos[1]) & (Fmt->BitMask[1] << 8)) |
			((B << Pos[2]) & (Fmt->BitMask[2] << 8));
		v >>= 8;
	}

	return v;
}

void FillInfo(pixel* Fmt)
{
	Fmt->BitCount = GetBPP(Fmt);
	if (PlanarYUV(Fmt,NULL,NULL,NULL))
	{
		if (Fmt->Flags & (PF_YUV420|PF_YUV422|PF_YUV444|PF_YUV410))
		{
			Fmt->BitMask[0] = 0x000000FF;
			Fmt->BitMask[1] = 0x0000FF00;
			Fmt->BitMask[2] = 0x00FF0000;
		}
		else
		switch (Fmt->FourCC)
		{
		case FOURCC_IMC4:
		case FOURCC_I420: 
		case FOURCC_IYUV:
		case FOURCC_YUV9:
			Fmt->BitMask[0] = 0x000000FF;
			Fmt->BitMask[1] = 0x0000FF00;
			Fmt->BitMask[2] = 0x00FF0000;
			break;
		case FOURCC_IMC2:
		case FOURCC_YV16:
		case FOURCC_YV12: 
		case FOURCC_YVU9:
			Fmt->BitMask[0] = 0x000000FF;
			Fmt->BitMask[1] = 0x00FF0000;
			Fmt->BitMask[2] = 0x0000FF00;
			break;
		}
	}
	else
	if (PackedYUV(Fmt))
		switch (Fmt->FourCC)
		{
		case FOURCC_YUY2:
		case FOURCC_YUNV:
		case FOURCC_V422:
		case FOURCC_YUYV:
			Fmt->BitMask[0] = 0x00FF00FF;
			Fmt->BitMask[1] = 0x0000FF00;
			Fmt->BitMask[2] = 0xFF000000;
			break;
		case FOURCC_YVYU:
			Fmt->BitMask[0] = 0x00FF00FF;
			Fmt->BitMask[1] = 0xFF000000;
			Fmt->BitMask[2] = 0x0000FF00;
			break;
		case FOURCC_UYVY:
		case FOURCC_Y422:
		case FOURCC_UYNV:
			Fmt->BitMask[0] = 0xFF00FF00;
			Fmt->BitMask[1] = 0x000000FF;
			Fmt->BitMask[2] = 0x00FF0000;
			break;
		}	
}

int GetImageSize(const video* p)
{
	int Size = p->Pitch * p->Height;
	if (PlanarYUV420(&p->Pixel))
		Size = (Size*3)/2; //1:0.25:0.25
	else
	if (PlanarYUV422(&p->Pixel))
		Size *= 2; //1:0.5:0.5
	else
	if (PlanarYUV444(&p->Pixel))
		Size *= 3; //1:1:1
	return Size;
}

int GetBPP(const pixel* Fmt)
{
	if (Fmt->Flags & (PF_RGB | PF_PALETTE)) 
		return Fmt->BitCount;
	if (PlanarYUV(Fmt,NULL,NULL,NULL))
		return 8;
	if (PackedYUV(Fmt))
		return 16;
	return 0;
}

bool_t EqPoint(const point* a, const point* b)
{
	return a->x==b->x && a->y==b->y;
}

bool_t EqRect(const rect* a, const rect* b)
{
	return a->x==b->x && a->y==b->y && a->Width==b->Width && a->Height==b->Height;
}

bool_t EqPixel(const pixel* a, const pixel* b)
{
	if (a->Flags != b->Flags) 
		return 0;

	if ((a->Flags & PF_PALETTE) &&
		a->BitCount != b->BitCount)
		return 0;

	if ((a->Flags & PF_RGB) &&
		(a->BitCount != b->BitCount ||
		 a->BitMask[0] != b->BitMask[0] ||
		 a->BitMask[1] != b->BitMask[1] ||
		 a->BitMask[2] != b->BitMask[2]))
		return 0;

	if ((a->Flags & PF_FOURCC) && a->FourCC != b->FourCC)
		return 0;

	return 1;
}

bool_t EqVideo(const video* a, const video* b)
{
	// no direction check here!
	return a->Width == b->Width &&
		   a->Height == b->Height &&
		   a->Pitch == b->Pitch &&
		   EqPixel(&a->Pixel,&b->Pixel);
}

void ClipRectPhy(rect* Physical, const video* p)
{
	if (Physical->x < 0)
	{
		Physical->Width += Physical->x;
		Physical->x = 0;
	}
	if (Physical->y < 0)
	{
		Physical->Height += Physical->y;
		Physical->y = 0;
	}
	if (Physical->x + Physical->Width > p->Width)
	{
		Physical->Width = p->Width - Physical->x;
		if (Physical->Width < 0)
		{
			Physical->Width = 0;
			Physical->x = 0;
		}
	}
	if (Physical->y + Physical->Height > p->Height)
	{
		Physical->Height = p->Height - Physical->y;
		if (Physical->Height < 0)
		{
			Physical->Height = 0;
			Physical->y = 0;
		}
	}
}

void VirtToPhy(const rect* Virtual, rect* Physical, const video* p)
{
	if (Virtual)
	{
		*Physical = *Virtual;

		if (p->Pixel.Flags & PF_PIXELDOUBLE)
		{
			Physical->x >>= 1;
			Physical->y >>= 1;
			Physical->Width >>= 1;
			Physical->Height >>= 1;
		}

		if (p->Direction & DIR_SWAPXY)
			SwapRect(Physical);

		if (p->Direction & DIR_MIRRORLEFTRIGHT)
			Physical->x = p->Width - Physical->x - Physical->Width;

		if (p->Direction & DIR_MIRRORUPDOWN)
			Physical->y = p->Height - Physical->y - Physical->Height;

		ClipRectPhy(Physical,p);
	}
	else
	{
		Physical->x = 0;
		Physical->y = 0;
		Physical->Width = p->Width;
		Physical->Height = p->Height;
	}
}

void PhyToVirt(const rect* Physical, rect* Virtual, const video* p)
{
	if (Physical)
		*Virtual = *Physical;
	else
	{
		Virtual->x = 0;
		Virtual->y = 0;
		Virtual->Width = p->Width;
		Virtual->Height = p->Height;
	}

	if (p->Direction & DIR_MIRRORLEFTRIGHT)
		Virtual->x = p->Width - Virtual->x - Virtual->Width;

	if (p->Direction & DIR_MIRRORUPDOWN)
		Virtual->y = p->Height - Virtual->y - Virtual->Height;

	if (p->Direction & DIR_SWAPXY)
		SwapRect(Virtual);

	if (p->Pixel.Flags & PF_PIXELDOUBLE)
	{
		Virtual->x <<= 1;
		Virtual->y <<= 1;
		Virtual->Width <<= 1;
		Virtual->Height <<= 1;
	}
}

