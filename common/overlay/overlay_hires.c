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
 * $Id: overlay_hires.c 543 2006-01-07 22:06:24Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"

#if defined(TARGET_PALMOS)

#include "../palmos/pace.h"

#ifdef HAVE_PALMONE_SDK
#include <68K/System/PalmDisplayExtent.h>
static UInt16 DEX = sysInvalidRefNum;
#endif

#undef CPU_TYPE
#define CPU_TYPE CPU_68K
#include "RotationMgrLib.h"
static UInt16 RotM = sysInvalidRefNum;

typedef struct xscaledesc
{
	uint32_t Next;
	uint32_t Base;
	uint32_t Id;
	uint32_t Cmd;

} xscaledesc;

typedef struct hiresbuffer
{
	block Buffer;
	uint32_t DescPhy;
	xscaledesc* Desc;
	uint32_t SurfacePhy;
	uint8_t* Surface;

} hiresbuffer;

#define MAGIC		0xBABE0000

#define XSCALE_LCD_BASE	0x44000000

#define LCCR0		(0x000/4)
#define LCCR1		(0x004/4)
#define LCCR2		(0x008/4)
#define LCCR3		(0x00C/4)
#define LCCR4		(0x010/4)
#define LCCR5		(0x014/4)
#define LCSR0		(0x038/4)
#define LCSR1		(0x034/4)
#define FBR0		(0x020/4)
#define FDADR0		(0x200/4)
#define FSADR0		(0x204/4)
#define FIDR0		(0x208/4)

#define OMAP_LCD_BASE	0xfffec000
#define OMAP_DMA_BASE	0xfffedb00
#define OMAP_PAL_SIZE	32

#define OMAP_CONTROL 0
#define OMAP_TIMING0 1
#define OMAP_TIMING1 2
#define OMAP_TIMING2 3
#define OMAP_STATUS	 4

#define OMAP_DMA_CONTROL	0
#define OMAP_DMA_TOP_L		1
#define OMAP_DMA_TOP_U		2
#define OMAP_DMA_BOTTOM_L	3
#define OMAP_DMA_BOTTOM_U	4

typedef struct hires hires;
typedef	int (*hiresfunc)(hires* This);

struct hires
{
	overlay Overlay;
	bool_t UseLocking;
	bool_t Locked;
	char* Bits;
	int Offset;
	int BufferCount;
	hiresbuffer Buffer[3];
	int Next;
	int PhyWidth;
	int PhyHeight;
	int SurfaceSize;
	int SurfaceOffset;
	int OrigPitch;
	uint32_t OrigPhy;
	uint32_t OrigPhyEnd;
	int Border[4];
	volatile uint32_t* Regs;
	volatile uint16_t* Regs2;
	int Caps;
	int Model;
	bool_t UseBorder;
	bool_t TripleBuffer;
	bool_t LowLevelFailed;
	hiresfunc LowLevelAlloc;
	hiresfunc LowLevelGet;
	hiresfunc LowLevelGetOrig;
	hiresfunc LowLevelSetOrig;
	hiresfunc LowLevelFlip;
};

static char* GetBits()
{
	BitmapType* Bitmap;
	WinHandle Win = WinGetDisplayWindow();
	if (!Win)
		return NULL;

	Bitmap = WinGetBitmap(Win);
	if (!Bitmap)
		return NULL;

	return BmpGetBits(Bitmap);
}

static bool_t ProcessRawInfo(video* Video,char* Bits,char* RawBits,int Width,int Height,int Pitch,int* Offset,bool_t MovedSIP)
{
	int Dir = GetOrientation();

	Video->Pitch = Pitch;

	if (Width > Height) // native landscape device
		Dir = CombineDir(Dir,DIR_SWAPXY|DIR_MIRRORUPDOWN,0);

	// trust GetBits() if it's inside the Raw buffer
	if (Bits >= RawBits && Bits < RawBits + Height*Pitch) 
		return 0; 	

	if (!MovedSIP && Dir==0) // assuming working GetBits() with RotM in native mode
		return 0; 

	if (Dir & DIR_SWAPXY)
		SwapInt(&Video->Width,&Video->Height);

	if (Width > Height) // native landscape device
	{
		if ((Dir & DIR_SWAPXY)==0 ? GetHandedness() : (Dir & DIR_MIRRORLEFTRIGHT)!=0)
			*Offset = (Width-Video->Width)*(Video->Pixel.BitCount >> 3);
	}
	else 
	{
		// native portrait device
		if ((Dir & DIR_SWAPXY) && MovedSIP) // don't adjust for RotM (SIP stays on native bottom)
		{
			// assuming taskbar and SIP is in the upper part of the screen in landscape mode
			if (Height > 320+32 && Video->Height <= 320+32 && DIASupported() && !DIAGet(DIA_SIP))
				Height = 320+32; // closed slider on Tungsten T3
			*Offset = (Height-Video->Height)*Pitch;
		}
	}

	Video->Direction = Dir;
	return 1;
}

char* QueryScreen(video* Video,int* Offset,int* Mode)
{
	char* Bits = GetBits(); 
	QueryDesktop(Video);
	*Offset = 0;
	if (Mode) *Mode = 0;

#ifdef HAVE_PALMONE_SDK
	if (DEX != sysInvalidRefNum)
	{
		UInt16 Pitch;
		Coord Width,Height;
		char* RawBits = DexGetDisplayAddress(DEX);
		if (RawBits)
		{
			DexGetDisplayDimensions(DEX,&Width,&Height,&Pitch);
			if (ProcessRawInfo(Video,Bits,RawBits,Width,Height,Pitch,Offset,1))
			{
				if (Mode) *Mode = 1;
				return RawBits;
			}
		}
	}
#endif

	if (RotM != sysInvalidRefNum)
	{
		UInt32 RawBits;
		UInt32 Width,Height,Pitch;

		if (RotationMgrAttributeGet(RotM,kRotationMgrAttrDisplayAddress,&RawBits)==errNone && RawBits &&
			RotationMgrAttributeGet(RotM,kRotationMgrAttrDisplayWidth,&Width)==errNone &&
			RotationMgrAttributeGet(RotM,kRotationMgrAttrDisplayHeight,&Height)==errNone &&
			RotationMgrAttributeGet(RotM,kRotationMgrAttrDisplayRowBytes,&Pitch)==errNone &&
			ProcessRawInfo(Video,Bits,(char*)RawBits,Width,Height,Pitch,Offset,0))
		{
			if (Mode) *Mode = 2;
			return (char*)RawBits;
		}
	}

	if (Video->Pitch < Video->Width*(Video->Pixel.BitCount/8))
		return NULL;

	return Bits;
}

static int BufferFree(hires* p)
{
	int n;
	for (n=0;n<p->BufferCount;++n)
		FreeBlock(&p->Buffer[n].Buffer);
	p->BufferCount = 0;
	memset(&p->Buffer,0,sizeof(p->Buffer));
	return ERR_NONE;
}

static int Init(hires* p)
{
	int Mode;
	p->Overlay.SetFX = BLITFX_AVOIDTEARING;
	p->UseLocking = 0;

	if (QueryPlatform(PLATFORM_MODEL)==MODEL_ZODIAC)
	{
		int Dir = GetOrientation();
		if (Dir & DIR_SWAPXY)
		{
			p->Overlay.SetFX |= BLITFX_VMEMROTATED;
			if (Dir & DIR_MIRRORLEFTRIGHT)
				p->Overlay.SetFX |= BLITFX_VMEMUPDOWN;
		}
	}

	p->Bits = QueryScreen(&p->Overlay.Output.Format.Video,&p->Offset,&Mode);
	if (!p->Bits)
		return ERR_INVALID_PARAM;

	if (!Mode)
		p->Bits = NULL; // use GetBits() during Lock()

	p->OrigPitch = p->Overlay.Output.Format.Video.Pitch;
	p->Next = 0;
	p->LowLevelFailed = 0;
	return ERR_NONE;
}

static void Hide(hires* p)
{
	if (p->Overlay.Overlay)
	{
		p->LowLevelSetOrig(p);
		p->Overlay.Overlay = 0;
		p->Overlay.Output.Format.Video.Pitch = p->OrigPitch;
	}
}

static void Done(hires* p)
{
	Hide(p);
	BufferFree(p);
}

static int UpdateAlloc(hires* p)
{
	int MaxCount = 1;
	if (p->TripleBuffer)
		MaxCount = 3;

	if (p->UseBorder)
	{
		p->SurfaceSize = (p->PhyWidth+p->Border[0]+p->Border[1])*(p->PhyHeight+p->Border[2]+p->Border[3])*2;
		p->SurfaceOffset = ((p->PhyWidth+p->Border[0]+p->Border[1])*p->Border[2]+p->Border[0])*2;
	}
	else
	{
		p->SurfaceSize = p->PhyWidth*p->PhyHeight*2;
		p->SurfaceOffset = 0;
	}

	while (p->BufferCount<MaxCount)
		if (p->LowLevelAlloc(p) == ERR_NONE)
			++p->BufferCount;
		else
			break;

	if (p->BufferCount==2)
	{
		FreeBlock(&p->Buffer[1].Buffer);
		--p->BufferCount;
	}

	if (!p->BufferCount)
		return ERR_OUT_OF_MEMORY;
	return ERR_NONE;
}

static int Update(hires* p)
{
	if (p->Overlay.FullScreenViewport && (p->UseBorder || p->TripleBuffer) && 
		!p->LowLevelFailed && (p->Overlay.Overlay || (p->LowLevelGetOrig && p->LowLevelGetOrig(p)==ERR_NONE)))
	{
		p->Overlay.Overlay = 1;
		if (UpdateAlloc(p)!=ERR_NONE || (p->BufferCount<3 && !p->UseBorder))
		{
			Hide(p);
			p->LowLevelFailed = 1;
			BufferFree(p);
		}
		else
		{
			int n;
			for (n=0;n<p->BufferCount;++n)
				memset(p->Buffer[n].Surface,0,p->SurfaceSize);
		}
	}	
	else
		Hide(p);

	p->Overlay.DoPowerOff = p->Overlay.Overlay;
	return OverlayUpdateAlign(&p->Overlay);
}

static int Show(hires* p)
{
	if (!p->Overlay.Show && p->Overlay.Overlay)
	{
		p->LowLevelSetOrig(p);
		p->Next = 0;
	}
	return ERR_NONE;
}

static int Reset(hires* p)
{
	int Pitch = p->Overlay.Output.Format.Video.Pitch;
	Init(p);
	if (p->Overlay.Overlay)
		p->Overlay.Output.Format.Video.Pitch = Pitch;
	return ERR_NONE;
}

static int Lock(hires* p, planes Planes, bool_t OnlyAligned)
{
	if (p->Overlay.Overlay)
	{
		int Next = p->Next+1;
		if (Next == p->BufferCount) 
			Next = 0;

		// use new buffer only if it's not the current
		if (p->LowLevelGet(p) != Next) 
			p->Next = Next;

		Planes[0] = p->Buffer[p->Next].Surface + p->SurfaceOffset;
	}
	else
	if (p->UseLocking && p->Overlay.CurrTime>=0 &&  // don't use locking with benchmark
		(Planes[0] = WinScreenLock(p->Overlay.Dirty?winLockCopy:winLockDontCare)) != NULL)
		p->Locked = 1;
	else
	if (p->Bits)
		Planes[0] = p->Bits + p->Offset;
	else
	{
		char* Bits = GetBits();
		if (!Bits)
			return ERR_DEVICE_ERROR;
		Planes[0] = Bits;
	}
	return ERR_NONE;
}

static int Unlock(hires* p)
{
	if (p->Overlay.Overlay)
		return p->LowLevelFlip(p);
	else
	if (p->Locked)
		WinScreenUnlock();
	return ERR_NONE;
}

//-------------------------------------------------------------------------------

static int XScaleGetOrig(hires* p)
{
	int ReTry = 0;
	do
	{
		ThreadSleep(2);
		if (++ReTry == 5) return ERR_NOT_SUPPORTED;
	}
	while (!(p->Regs[LCCR0] & 1)); // again if lcd disabled

	p->PhyWidth = (p->Regs[LCCR1] & 0x3FF)+1;
	p->PhyHeight = (p->Regs[LCCR2] & 0x3FF)+1;

	if (p->Overlay.Output.Format.Video.Width > p->PhyWidth ||
		p->Overlay.Output.Format.Video.Height > p->PhyHeight)
		return ERR_NOT_SUPPORTED; // this shouldn't happen

	p->OrigPhy = p->Regs[FDADR0];
	if (!p->OrigPhy)
		return ERR_NOT_SUPPORTED; // this shouldn't happen

	// assuming physical direction and size is already correct
	p->Overlay.Output.Format.Video.Pitch = p->PhyWidth*2;
	if (p->UseBorder)
		p->Overlay.Output.Format.Video.Pitch += (p->Border[0]+p->Border[1])*2;
	
	return ERR_NONE;
}

static int XScaleAlloc(hires* p)
{
	hiresbuffer* Buffer = p->Buffer+p->BufferCount;
	if (!AllocBlock(16+sizeof(xscaledesc)+256+p->SurfaceSize,&Buffer->Buffer,0,HEAP_ANYWR))
		return ERR_OUT_OF_MEMORY;
	Buffer->Desc = (xscaledesc*)ALIGN16((uintptr_t)Buffer->Buffer.Ptr);
	Buffer->DescPhy = MemVirtToPhy(Buffer->Desc);
	if (!Buffer->DescPhy)
	{
		FreeBlock(&Buffer->Buffer);
		return ERR_NOT_SUPPORTED;
	}
	Buffer->Surface = (uint8_t*)Buffer->Desc+sizeof(xscaledesc)+256;
	Buffer->SurfacePhy = Buffer->DescPhy+sizeof(xscaledesc)+256; 
	Buffer->Desc->Base = Buffer->SurfacePhy;
	Buffer->Desc->Id = MAGIC|(p->BufferCount << 4); 
	Buffer->Desc->Next = Buffer->DescPhy; 
	Buffer->Desc->Cmd = p->SurfaceSize | (1<<21);
	return ERR_NONE;
}
	
static int XScaleGet(hires* p)
{
	int Id = p->Regs[FIDR0] & ~15;
	uint32_t Phy = p->Regs[FDADR0] & ~15;

	if ((Id & 0xFFFF0000) == MAGIC)
		return (Id >> 4) & 0xFF;

	if (Phy == p->OrigPhy || Phy == p->OrigPhy+sizeof(xscaledesc))
		return -1;

	p->OrigPhy = Phy; // changed...
	return -1; 
}

static void XScaleQuickDisable(hires* p)
{
	int n = p->Regs[LCCR0];
	n &= ~(1<<10); //DIS
	n &= ~1;	   //ENB
	p->Regs[LCCR0] = n;
	ThreadSleep(1);
}

static void XScaleDisable(hires* p)
{
	int i;
	int n = p->Regs[LCCR0];
	if (n & 1)
	{
		n |= 1<<10; //DIS
		p->Regs[LCCR0] = n;

		for (i=0;i<30;++i)
		{
			if (!(p->Regs[LCCR0] & 1)) 
				break;
			ThreadSleep(1);
		}

		if (i==30)
			XScaleQuickDisable(p);
	}
}

static void XScaleEnable(hires* p,uint32_t Phy)
{
	int n = p->Regs[LCCR0];
	n &= ~(1<<10); //DIS
	n |= 1; //EN;
	p->Regs[FBR0] = 0;
	p->Regs[FDADR0] = Phy;
	p->Regs[LCCR0] = n;
	ThreadSleep(1);
}

static NOINLINE bool_t TimeAdjust(volatile uint32_t* p,int x,int left,int right,bool_t UseBorder)
{
	int o1,o2;
	uint32_t n;
	uint32_t v = *p;

	if (UseBorder)
		x += left+right;
	--x;
	n = v & 0x3FF;
	if ((int)n == x)
		return 0;

	v &= ~0x3FF;
	v |= x;

	x -= n;
	if (x>=left)
		o1 = left;
	else
	if (x<=-left)
		o1 = -left;
	else
		o1 = x >> 1;
	o2 = x - o1;

	n = (v >> 16) & 0xFF;
	v &= ~0xFF0000;
	v |= (n-o2) << 16; 

	n = (v >> 24) & 0xFF;
	v &= ~0xFF000000;
	v |= (n-o1) << 24; 

	*p = v;
	return 1;
}

static NOINLINE bool_t Adjust(hires* p,volatile uint32_t* p1,volatile uint32_t* p2,bool_t UseBorder)
{
	bool_t Changed;
	Changed = TimeAdjust(p1,p->PhyWidth,p->Border[0],p->Border[1],UseBorder);
	Changed = TimeAdjust(p2,p->PhyHeight,p->Border[2],p->Border[3],UseBorder) || Changed;
	return Changed;
}

static void XScaleChange(hires* p,uint32_t Phy,bool_t UseBorder)
{
	XScaleDisable(p);
	if (Adjust(p,&p->Regs[LCCR1],&p->Regs[LCCR2],UseBorder))
	{
		// PXA270 bug. need to enable/disable twice
		XScaleEnable(p,Phy);
		XScaleDisable(p);
	}
	XScaleEnable(p,Phy);
}

static int XScaleSetOrig(hires* p)
{
	if (XScaleGet(p)>=0)
		XScaleChange(p,p->OrigPhy,0);
	return ERR_NONE;
}

static int XScaleFlip(hires* p)
{
	uint32_t DescPhy = p->Buffer[p->Next].DescPhy;
	p->Buffer[p->Next].Desc->Next = DescPhy; // close chain

	if (!(p->Regs[LCCR0] & 1)) // is lcd enabled?
		return ERR_DEVICE_ERROR;

	if (XScaleGet(p)<0)
		XScaleChange(p,DescPhy,p->UseBorder);
	else
	{
		// modify last frames tail to new frame
		int Current = p->Next-1;
		if (Current<0)
			Current = p->BufferCount-1;
		p->Buffer[Current].Desc->Next = DescPhy;
	}
	return ERR_NONE;
}

//-------------------------------------------------------------------------------

static int OMAPGetOrig(hires* p)
{
	int ReTry = 0;
	do
	{
		ThreadSleep(1);
		if (++ReTry == 5) return ERR_NOT_SUPPORTED;
	}
	while (!(p->Regs[OMAP_CONTROL] & 1)); // again if lcd disabled

	p->PhyWidth = (p->Regs[OMAP_TIMING0] & 0x3FF)+1;
	p->PhyHeight = (p->Regs[OMAP_TIMING1] & 0x3FF)+1;

	if (p->Overlay.Output.Format.Video.Width > p->PhyWidth ||
		p->Overlay.Output.Format.Video.Height > p->PhyHeight)
		return ERR_NOT_SUPPORTED; // this shouldn't happen

	p->OrigPhy = (p->Regs2[OMAP_DMA_TOP_U] << 16) | p->Regs2[OMAP_DMA_TOP_L];
	p->OrigPhyEnd = (p->Regs2[OMAP_DMA_BOTTOM_U] << 16) | p->Regs2[OMAP_DMA_BOTTOM_L];

	if (!p->OrigPhy)
		return ERR_NOT_SUPPORTED; // this shouldn't happen

	// assuming physical direction and size is already correct
	p->Overlay.Output.Format.Video.Pitch = p->PhyWidth*2;
	if (p->UseBorder)
		p->Overlay.Output.Format.Video.Pitch += (p->Border[0]+p->Border[1])*2;
	return ERR_NONE;
}

static int OMAPAlloc(hires* p)
{
	hiresbuffer* Buffer = p->Buffer+p->BufferCount;
	if (!AllocBlock(p->SurfaceSize+16+OMAP_PAL_SIZE,&Buffer->Buffer,0,HEAP_ANYWR))
		return ERR_OUT_OF_MEMORY;
	Buffer->Surface = (uint8_t*)ALIGN16((uintptr_t)Buffer->Buffer.Ptr)+OMAP_PAL_SIZE;

	memset(Buffer->Surface-OMAP_PAL_SIZE,0,OMAP_PAL_SIZE); //clear palette
	Buffer->Surface[-OMAP_PAL_SIZE+1] = 0x40; //set 16bit mode framebuffer

	Buffer->SurfacePhy = MemVirtToPhy(Buffer->Surface);
	if (!Buffer->SurfacePhy)
	{
		FreeBlock(&Buffer->Buffer);
		return ERR_NOT_SUPPORTED;
	}
	return ERR_NONE;
}

static int OMAPGet(hires* p)
{
	uint32_t Phy = p->Regs2[OMAP_DMA_TOP_L] | (p->Regs2[OMAP_DMA_TOP_U] << 16);
	int n;

	for (n=0;n<p->BufferCount;++n)
		if (Phy == p->Buffer[n].SurfacePhy-OMAP_PAL_SIZE)
			return 0;

	p->OrigPhy = Phy;
	return -1; 
}

static void OMAPDisable(hires* p)
{
	int i;
	int n = p->Regs[OMAP_CONTROL];
	if (n & 1)
	{
		n &= ~1;
		p->Regs[OMAP_CONTROL] = n;

		for (i=0;i<50;++i)
		{
			if (p->Regs[OMAP_STATUS] & 1) 
				break;
			ThreadSleep(1);
		}
	}
}

static void OMAPEnable(hires* p)
{
	int v = p->Regs[OMAP_CONTROL];
	v |= 1;
	p->Regs[OMAP_CONTROL] = v;
	ThreadSleep(1);
}

static void OMAPChange(hires* p,uint32_t Phy,uint32_t PhyEnd,bool_t UseBorder)
{
	bool_t Old = (p->Regs[OMAP_CONTROL] & 1);
	OMAPDisable(p);
	Adjust(p,&p->Regs[OMAP_TIMING0],&p->Regs[OMAP_TIMING1],UseBorder);
	p->Regs2[OMAP_DMA_TOP_U] = (uint16_t)(Phy >> 16);
	p->Regs2[OMAP_DMA_TOP_L] = (uint16_t)(Phy);
	p->Regs2[OMAP_DMA_BOTTOM_U] = (uint16_t)(PhyEnd >> 16);
	p->Regs2[OMAP_DMA_BOTTOM_L] = (uint16_t)(PhyEnd);
	if (Old) OMAPEnable(p);
}

static int OMAPSetOrig(hires* p)
{
	if (OMAPGet(p)>=0)
		OMAPChange(p,p->OrigPhy,p->OrigPhyEnd,0);
	return ERR_NONE;
}

static int OMAPFlip(hires* p)
{
	if (!(p->Regs[OMAP_CONTROL] & 1)) // is lcd enabled?
		return ERR_DEVICE_ERROR;

	if (OMAPGet(p)<0)
		OMAPChange(p,p->Buffer[0].SurfacePhy-OMAP_PAL_SIZE,p->Buffer[0].SurfacePhy+p->SurfaceSize-2,p->UseBorder);
	return ERR_NONE;
}

//-------------------------------------------------------------------------------

static const datatable Params[] = 
{
	{ HIRES_TRIPLEBUFFER,			TYPE_BOOL, DF_SETUP|DF_CHECKLIST },
	{ HIRES_USEBORDER,				TYPE_BOOL, DF_SETUP|DF_CHECKLIST },
	{ HIRES_USEBORDER_INFO,			TYPE_LABEL, DF_SETUP },

	DATATABLE_END(HIRES_ID)
};

static int Enum(hires* p, int* No, datadef* Param)
{
	int Result;
	if (OverlayEnum(&p->Overlay,No,Param)==ERR_NONE)
		return ERR_NONE;
	Result = NodeEnumTable(No,Param,Params);
	if (Result==ERR_NONE)
	{
		if (Param->No == HIRES_TRIPLEBUFFER && (!p->Regs || !(p->Caps & CAPS_ARM_XSCALE)))
			Param->Flags |= DF_HIDDEN;
		if (Param->No == HIRES_USEBORDER && !p->Regs)
			Param->Flags |= DF_HIDDEN;
	}
	return Result;
}

static int Get(hires* p,int No,void* Data,int Size)
{
	int Result = OverlayGet(&p->Overlay,No,Data,Size);
	switch (No) 
	{
	case HIRES_TRIPLEBUFFER: GETVALUE(p->TripleBuffer,bool_t); break;
	case HIRES_USEBORDER: GETVALUE(p->UseBorder,bool_t); break;
	}
	return Result;
}

static int UpdateLowLevel(hires* p)
{
	p->LowLevelFailed = 0;
	Hide(p);
	BufferFree(p);
	OverlayUpdateFX(&p->Overlay,1);
	return ERR_NONE;
}

static int Set(hires* p,int No,const void* Data,int Size)
{
	int Result = OverlaySet(&p->Overlay,No,Data,Size);
	switch (No)
	{
	case HIRES_TRIPLEBUFFER: SETVALUECMP(p->TripleBuffer,bool_t,UpdateLowLevel(p),EqBool); break;
	case HIRES_USEBORDER: SETVALUECMP(p->UseBorder,bool_t,UpdateLowLevel(p),EqBool); break;
	}
	return Result;
}

extern bool_t SwapLandscape;

static int Create(hires* p)
{
	p->Overlay.Node.Enum = (nodeenum)Enum;
	p->Overlay.Node.Get = (nodeget)Get;
	p->Overlay.Node.Set = (nodeset)Set;
	p->Overlay.Init = (ovlfunc)Init;
	p->Overlay.Done = (ovldone)Done;
	p->Overlay.Reset = (ovlfunc)Reset;
	p->Overlay.Lock = (ovllock)Lock;
	p->Overlay.Unlock = (ovlfunc)Unlock;
	p->Overlay.UpdateShow = (ovlfunc)Show;
	p->Overlay.Update = (ovlfunc)Update;

	p->Caps = QueryPlatform(PLATFORM_CAPS);
	if (p->Caps & CAPS_ARM_XSCALE)
	{
		p->Regs = (volatile uint32_t*)MemPhyToVirt(XSCALE_LCD_BASE);
		if (p->Regs)
		{
			p->LowLevelGet = XScaleGet;
			p->LowLevelGetOrig = XScaleGetOrig;
			p->LowLevelSetOrig = XScaleSetOrig;
			p->LowLevelFlip = XScaleFlip;
			p->LowLevelAlloc = XScaleAlloc;
			p->Border[0] = 2;
			p->Border[1] = 2;
			p->Border[2] = p->Border[3] = 2;
		}
	}
	else
	{
		p->Regs = (volatile uint32_t*)MemPhyToVirt(OMAP_LCD_BASE);
		p->Regs2 = (volatile uint16_t*)MemPhyToVirt(OMAP_DMA_BASE);
		if (!p->Regs || !p->Regs2)
		{
			p->Regs = NULL;
			p->Regs2 = NULL;
		}
		else
		{
			p->LowLevelGet = OMAPGet;
			p->LowLevelGetOrig = OMAPGetOrig;
			p->LowLevelSetOrig = OMAPSetOrig;
			p->LowLevelFlip = OMAPFlip;
			p->LowLevelAlloc = OMAPAlloc;
			p->Border[0] = 14; // OMAP width has to be multiple of 16
			p->Border[1] = 2;
			p->Border[2] = p->Border[3] = 2;
		}
	}

	p->Model = QueryPlatform(PLATFORM_MODEL);
	// don't use triple buffering on 320x320 devices by default
	p->TripleBuffer = (Context()->StartUpMemory > 3*1024*1024) &&
		(p->Model==MODEL_TUNGSTEN_T3 ||
		p->Model==MODEL_TUNGSTEN_T5 ||
		p->Model==MODEL_PALM_TX ||
		p->Model==MODEL_LIFEDRIVE);

	// only device always supporting borderless mode is LifeDrive (?)
	p->UseBorder = 
		p->Model==MODEL_LIFEDRIVE ||
		p->Model==MODEL_PALM_TX;
	return ERR_NONE;
}

static const nodedef HIRES = 
{
	sizeof(hires)|CF_GLOBAL|CF_SETTINGS,
	HIRES_ID,
	OVERLAY_CLASS,
	PRI_DEFAULT+10,
	(nodecreate)Create,
};

void OverlayHIRES_Init() 
{
#ifdef HAVE_PALMONE_SDK
	if (SysLibFind(dexLibName, &DEX) == sysErrLibNotFound)
		SysLibLoad(dexLibType, dexLibCreator, &DEX);
	if (DEX != sysInvalidRefNum && SysLibOpen(DEX)!=errNone)
		DEX = sysInvalidRefNum;
#endif

	if (SysLibFind(kRotationMgrLibName, &RotM) == sysErrLibNotFound)
		SysLibLoad(kRotationMgrLibType, kRotationMgrLibCreator, &RotM);
	if (RotM != sysInvalidRefNum && SysLibOpen(RotM)!=errNone)
		RotM = sysInvalidRefNum;

	if (RotM != sysInvalidRefNum)
	{
		UInt32 Width,Height;
		if (RotationMgrAttributeGet(RotM,kRotationMgrAttrDisplayWidth,&Width)==errNone &&
			RotationMgrAttributeGet(RotM,kRotationMgrAttrDisplayHeight,&Height)==errNone &&
			Width < Height) // native portrait -> landscape modes are swapped by Mobile Stream
			SwapLandscape = 1;
	}

	NodeRegisterClass(&HIRES);
}

void OverlayHIRES_Done() 
{
	NodeUnRegisterClass(HIRES_ID);

#ifdef HAVE_PALMONE_SDK
	if (DEX != sysInvalidRefNum)
	{
		SysLibClose(DEX);
		DEX = sysInvalidRefNum;
	}
#endif
	if (RotM != sysInvalidRefNum)
	{
		SysLibClose(RotM);
		RotM = sysInvalidRefNum;
	}
}

#endif
