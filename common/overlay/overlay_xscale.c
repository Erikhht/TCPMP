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
 * $Id: overlay_xscale.c 615 2006-01-26 16:57:51Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"

#define X50V // included in official version

#if defined(ARM)

#define WIN32_LEAN_AND_MEAN
#ifndef STRICT
#define STRICT
#endif
#include <windows.h>

#define MAXDESC			1024
#define MAXORIGBLOCK	8
#define MAXCOUNT		3
#define MAXDESCBLOCK	4

typedef struct xscaledesc
{
	uint32_t Next;
	uint32_t Base;
	uint32_t Id;
	uint32_t Cmd;

} xscaledesc;

typedef struct xscaledriver
{
	node Node;
	int Width;
	int Height;
	int BufferSize;
	int MaxBlock;
	bool_t AllocAtStart;
	bool_t SetupFlip;
	bool_t SetupStretch;
	bool_t InAlloc;
	bool_t AtStart;
	struct xscale* Used;

	int DescPhyCount[MAXCOUNT];
	phymemblock DescPhy[MAXCOUNT][MAXDESCBLOCK];
	xscaledesc* Desc[MAXCOUNT];
	xscaledesc* DescLast[MAXCOUNT];

	int* Map;
	int BufferPhyCount[MAXCOUNT];
	phymemblock* BufferPhy; // [MAXCOUNT][MaxBlock]
	void* Buffer[MAXCOUNT];
	int Count;

#ifdef X50V
	uint32_t* GPIO;
	uint32_t* Marathon;
	bool_t X50v;
#endif

	uint32_t* Regs;
	uint32_t OrigDescPhy[MAXORIGBLOCK];		
	phymemblock OrigBufferPhy[MAXORIGBLOCK];
	int OrigDescCount;
	void* OrigBuffer;
	int OrigBufferSize;
	xscaledesc* OrigDescEnd;
	bool_t OrigDescEndSet;
	bool_t NotOrig;

} xscaledriver;

typedef struct xscale
{
	overlay Overlay;
	xscaledriver* Driver;
	bool_t Flip;
	int Next;

} xscale;

#define MAGIC		0xBABE0000

#define GPIO_BASE	0x40E00000
#define GPIO_SIZE	0x1000
#define LCD_BASE	0x44000000
#define LCD_SIZE	0x1000

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

#define GAFR1_U		(0x060/4)
#define GAFR2_L		(0x064/4)

#define LCD_CONFIG	(0x03fe0060/4)

static void FreeOrig(xscaledriver* p)
{
	int n;

	// be sure that the old orig is not referenced by the buffers
	for (n=0;n<p->Count;++n)
		p->DescLast[n]->Next = p->DescPhy[n]->Addr;

	// restore and release end descriptor
	if (p->OrigDescEnd)
	{
		if (p->OrigDescEndSet)
		{
			p->OrigDescEnd->Next = p->OrigDescPhy[0];
			p->OrigDescEndSet = 0;
		}
		PhyMemEnd(p->OrigDescEnd);
		p->OrigDescEnd = NULL;
	}

	// release framebuffer
	if (p->OrigBuffer)
	{
		PhyMemEnd(p->OrigBuffer);
		p->OrigBuffer = NULL;
	}

	p->OrigDescCount = 0;
	p->OrigBufferSize = 0;
	p->NotOrig = 0;
}

static void GetOrig(xscaledriver* p)
{
	uint32_t DescPhy[MAXORIGBLOCK]; // original descriptor head
	xscaledesc Desc[MAXORIGBLOCK];
	uint32_t Phy;
	
#ifdef X50V
	if (p->X50v)
	{
		// create a fake desc chain with SRAM usage (like Loox720)
		/*
		int Left = 640*480*2 - 256*1024;
		phymemblock Block2;
		phymemblock Block;
		int One = 1;
		xscaledesc* d = (xscaledesc*) PhyMemAlloc(sizeof(xscaledesc)*2,&Block,&One);
		if (!d)
			return;

		One = 1;
		if (!PhyMemAlloc(Left,&Block2,&One))
			return;

		d[0].Base = Block2.Addr;
		d[0].Cmd = Block2.Length;
		d[0].Id = 0;
		d[0].Next = Block.Addr + sizeof(xscaledesc);

		d[1].Base = 0x5C000000;
		d[1].Cmd = 256*1024 | (1<<21);
		d[1].Id = 0;
		d[1].Next = Block.Addr;

		Phy = Block.Addr;
		*/

		Phy = 0;
	}
	else
#endif
	{
		Phy = p->Regs[FDADR0] & ~15;
		p->Width = (p->Regs[LCCR1] & 1023)+1;
		p->Height = (p->Regs[LCCR2] & 1023)+1;
	}

	if (Phy)
	{
		int n,m;

		FreeOrig(p);

		for (n=0;n<MAXORIGBLOCK;)
		{
			xscaledesc* d = (xscaledesc*) PhyMemBegin(Phy,sizeof(xscaledesc),0);
			if (!d)
				break;

			DescPhy[n] = Phy;
			Desc[n] = *d;
			DEBUG_MSG4(-1,T("XScale %08x (%08x %08x %08x)"),Phy,Desc[n].Base,Desc[n].Cmd,Desc[n].Next);

			++n;
			
			Phy = d->Next & ~15;
			PhyMemEnd(d);

			for (m=0;m<n;++m)
				if (DescPhy[m] == Phy)
				{
					int i,j;

					if (n-m>1) // need to find end
					{
						for (i=m;i<n;++i)
							if (Desc[i].Cmd & (1<<21))
							{
								++i;
								if (i==n) i=m;
								break;
							}

						if (i==n)
							return; // failed
					}
					else
						i=m; // there is only one desc

					j=i;
					do
					{
						p->OrigDescPhy[p->OrigDescCount] = DescPhy[j];
						p->OrigBufferPhy[p->OrigDescCount].Addr = Desc[j].Base;
						p->OrigBufferPhy[p->OrigDescCount].Length = Desc[j].Cmd & 0x1FFFFF;
						p->OrigBufferSize += p->OrigBufferPhy[p->OrigDescCount].Length;
						p->OrigDescCount++;
						if (++j==n) j=m;
					}
					while (i!=j);

					// map end descriptor so we don't have to use disable/enable for flipping (with multiple desc displays)
					p->OrigDescEnd = (xscaledesc*) PhyMemBegin(p->OrigDescPhy[p->OrigDescCount-1],sizeof(xscaledesc),0);
					if (p->OrigDescEnd && p->OrigDescEnd->Next != p->OrigDescPhy[0])
					{
						PhyMemEnd(p->OrigDescEnd);
						p->OrigDescEnd = NULL;
					}

					// reuse original framebuffer
					if (p->OrigBufferSize >= p->Width*2*p->Height)
						p->OrigBuffer = PhyMemBeginEx(p->OrigBufferPhy,p->OrigDescCount,1);

					return;
				}
		}
	}
}

static bool_t FreeBuffer(xscaledriver* p)
{
	int n;

	if (!p->BufferPhy || p->InAlloc || p->Used)
		return 0;

	for (n=0;n<p->Count;++n)
	{
		PhyMemFree(p->Desc[n],p->DescPhy[n],p->DescPhyCount[n]);
		if (n>0 || !p->OrigDescCount)
			PhyMemFree(p->Buffer[n],p->BufferPhy+p->MaxBlock*n,p->BufferPhyCount[n]);
	}

	p->Count = 0;
	p->MaxBlock = 0;

	free(p->Map);
	p->Map = NULL;

	free(p->BufferPhy);
	p->BufferPhy = NULL;

	DEBUG_MSG(-1,T("XSCALE Freed"));
	return 1;
}

static bool_t AllocBuffer(xscaledriver* p)
{
	if (!p->BufferPhy && p->OrigBuffer)
	{
		int n;

		p->BufferSize = (p->Width*2 * p->Height) & ~3;
		p->MaxBlock = (p->BufferSize / 4096)+1;

		p->BufferPhy = (phymemblock*) malloc(sizeof(phymemblock)*p->MaxBlock*MAXCOUNT);
		if (!p->BufferPhy)
			return 0;

		p->Map = (int*) malloc(sizeof(int)*p->Height);
		if (!p->Map)
		{
			free(p->BufferPhy);
			p->BufferPhy = NULL;
			return 0;
		}

		p->InAlloc = 1;
		for (n=0;n<(p->SetupFlip?MAXCOUNT:1);++n)
		{
			// allocate desc table

			p->DescPhyCount[n] = MAXDESCBLOCK;
			p->Desc[n] = p->DescLast[n] = PhyMemAlloc(MAXDESC*sizeof(xscaledesc),p->DescPhy[n],&p->DescPhyCount[n]);
			if (!p->Desc[n])
				break;

			if (n==0 && p->OrigBufferSize >= p->BufferSize)
			{
				// use original surface
				p->Buffer[n] = p->OrigBuffer;
				p->BufferPhyCount[n] = p->OrigDescCount;
				memcpy(p->BufferPhy+p->MaxBlock*n,p->OrigBufferPhy,sizeof(phymemblock)*p->OrigDescCount);
			}
			else
			{
				// allocate new surface
				p->BufferPhyCount[n] = p->MaxBlock;
				p->Buffer[n] = PhyMemAlloc(p->BufferSize,p->BufferPhy+p->MaxBlock*n,&p->BufferPhyCount[n]);
			}

			if (!p->Buffer[n])
			{
				PhyMemFree(p->Desc[n],p->DescPhy[n],p->DescPhyCount[n]);
				break;
			}
		}

		if (n==2)
		{
			// can't flip chain with 2 surfaces (minimum is 3)
			PhyMemFree(p->Desc[1],p->DescPhy[1],p->DescPhyCount[1]);
			PhyMemFree(p->Buffer[1],p->BufferPhy+p->MaxBlock*1,p->BufferPhyCount[1]);
			--n;
		}

		p->Count = n;
		p->InAlloc = 0;

		DEBUG_MSG5(-1,T("XSCALE %d %d %d %d %d"),p->Count,p->BufferPhyCount[0],p->BufferPhyCount[1],p->BufferPhyCount[2],AvailMemory());
	}
	return 1;
}

static void GetMode(xscale* p)
{
	p->Overlay.Output.Format.Video.Width = p->Driver->Width;
	p->Overlay.Output.Format.Video.Height = p->Driver->Height;
	p->Overlay.Output.Format.Video.Direction = 0;
	p->Overlay.Output.Format.Video.Aspect = ASPECT_ONE;
	p->Overlay.Output.Format.Video.Pitch = p->Overlay.Output.Format.Video.Width*2;
	DefaultRGB(&p->Overlay.Output.Format.Video.Pixel,16,5,6,5,0,0,0);
	AdjustOrientation(&p->Overlay.Output.Format.Video,1);

	if (p->Overlay.Output.Format.Video.Direction & DIR_SWAPXY)
	{
		int Model = QueryPlatform(PLATFORM_MODEL);
		if (Model == MODEL_IPAQ_H3800 || Model == MODEL_IPAQ_H3900 ||
			(Model == MODEL_NOVOGO && QueryPlatform(PLATFORM_VER)<500 && p->Driver->Width==240 && p->Driver->Height==320))
			p->Overlay.Output.Format.Video.Direction ^= DIR_MIRRORLEFTRIGHT|DIR_MIRRORUPDOWN;
	}

	if (!((uint32_t)p->Driver->OrigBuffer & 15))
		p->Overlay.Output.Format.Video.Pixel.Flags |= PF_16ALIGNED;
}

static int Init(xscale* p)
{
	if (!p->Driver->OrigBuffer)
	{
		GetOrig(p->Driver);
		if (!p->Driver->OrigBuffer)
			return ERR_NOT_SUPPORTED;
	}

	GetMode(p);
	p->Next = 0;
	p->Flip = 0;

	p->Overlay.SetFX = BLITFX_AVOIDTEARING;

	if (p->Driver->Used)
		return ERR_DEVICE_ERROR;
	p->Driver->Used = p;

	if (!AllocBuffer(p->Driver))
		return ERR_OUT_OF_MEMORY;

	if (p->Driver->Count >= 3)
		p->Overlay.ClearFX = BLITFX_ONLYDIFF;

	return ERR_NONE;
}

static void QuickDisable(xscaledriver* p)
{
	int n = p->Regs[LCCR0];
	n &= ~(1<<10); //DIS
	n &= ~1;	   //ENB
	p->Regs[LCCR0] = n;
	Sleep(1);
}

static void Disable(xscaledriver* p)
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
			Sleep(1);
		}

		if (i==30)
			QuickDisable(p);
	}
}

static void Enable(xscaledriver* p)
{
	int n = p->Regs[LCCR0];

	n &= ~(1<<10); //DIS
	n |= 1; //EN;

	p->Regs[LCCR0] = n;
	Sleep(1);
}

static int GetCurrent2(xscaledriver* p)
{
	int n;
	int Id = p->Regs[FIDR0] & ~15;
	uint32_t Phy = p->Regs[FDADR0] & ~15;

	if ((Id & 0xFFFF0000) == MAGIC)
		return (Id >> 4) & 0xFF;

	for (n=0;n<p->OrigDescCount;++n)
		if (Phy == p->OrigDescPhy[n] || Phy == p->OrigDescPhy[n]+sizeof(xscaledesc))
			return -1;

	return -2; // unknown
}

static int GetCurrent(xscaledriver* p)
{
	int Current;

	if (!(p->Regs[LCCR0] & 1))
		return -2; // turned off lcd

	Current = GetCurrent2(p);
	if (Current >= -1)
		return Current;

	Sleep(50); // original desc chain has changed. maybe it's just temporary

	Current = GetCurrent2(p);
	if (Current >= -1)
		return Current;

	GetOrig(p); // reread original desc chain 

	return GetCurrent2(p);
}

#ifdef X50V

static void X50vDisable(xscaledriver* p)
{
	if (p->Marathon[LCD_CONFIG] & (1<<16))
	{
		uint32_t v;

		while ((v = p->Marathon[LCD_CONFIG]) & (1<<16))
			p->Marathon[LCD_CONFIG] = v & ~0x1000A;

		Disable(p);

		v = p->GPIO[GAFR1_U];
		if (v & 0xFFF00000)
			p->GPIO[GAFR1_U] = v & 0xFFFFF;

		v = p->GPIO[GAFR2_L];
		if (v & 0xFFFFFFF)
			p->GPIO[GAFR2_L] = v & 0xF0000000;
	}
}

static bool_t X50vEnable(xscaledriver* p,uint32_t Desc)
{
	bool_t Changed = 0;
	if (!(p->Marathon[LCD_CONFIG] & (1<<16)))
	{
		uint32_t v;

		Changed = 1;
		Disable(p);

		p->Regs[LCCR0] = 0x1B008F8;	
		p->Regs[LCCR1] = 0x1056FDDF; 
		p->Regs[LCCR2] = 0x104067F;  
		p->Regs[LCCR3] = 0x4f00001;  
		p->Regs[LCCR4] = 0;		
		p->Regs[LCCR5] = 0x3F3F3F3F;

		p->Regs[FDADR0] = 0;
		p->Regs[LCSR0] = 0;

		v = p->GPIO[GAFR1_U];
		if ((v & 0xFFF00000) != 0xAAA00000)
			p->GPIO[GAFR1_U] = (v & 0xFFFFF) | 0xAAA00000;
		
		v = p->GPIO[GAFR2_L];
		if ((v & 0xFFFFFFF) != 0xAAAAAAA)
			p->GPIO[GAFR2_L] = (v & 0xF0000000) | 0xAAAAAAA;

		p->Regs[FDADR0] = Desc;
		Enable(p);
		QuickDisable(p);
		p->Regs[FDADR0] = Desc;
		Enable(p);

		while (!((v = p->Marathon[LCD_CONFIG]) & (1<<16)))
		{
			p->Marathon[LCD_CONFIG] = v | 0x1000A;
			Sleep(1);
		}
	}
	return Changed;
}

#endif

static void SetOrig(xscaledriver* p)
{
	int n;

#ifdef X50V
	if (p->X50v)
		X50vDisable(p);
	else
#endif
	{
		if (p->NotOrig)
		{
			if (p->OrigDescEnd && p->OrigDescEndSet)
			{
				p->OrigDescEnd->Next = p->OrigDescPhy[0];
				p->OrigDescEndSet = 0;
			}

			for (n=0;n<p->Count;++n)
				p->DescLast[n]->Next = p->OrigDescPhy[0];

			// wait for primary to be restored
			for (n=0;n<100;++n)
			{
				if (GetCurrent(p)<0)
					break;
				Sleep(1);
			}

			p->NotOrig = 0;
		}
	}
}

static void Done(xscale* p)
{
	SetOrig(p->Driver);
	if (p->Driver->Used == p)
		p->Driver->Used = NULL;
}

static int Reset(xscale* p)
{
	Done(p);
	Init(p);
	return ERR_NONE;
}

static xscaledesc* BuildDesc(xscaledesc* Desc,phymemblock* DescPhy,int DescPhyCount,phymemblock* BufferPhy,int BufferPhyCount,
							 int* Map,int Pitch,int Height,int Id)
{
	uint32_t First = DescPhy->Addr;
	uint32_t Last = 0;
	uint32_t DescAddr = 0;
	int n,m;

	for (n=0;n<Height;++n)
	{
		uint32_t Pos = Pitch * Map[n];
		int Length = Pitch;

		for (m=0;Length>0 && m<BufferPhyCount;++m)
		{
			if (Pos < BufferPhy[m].Length)
			{
				uint32_t Addr = BufferPhy[m].Addr + Pos;
				int Size = BufferPhy[m].Length - Pos;
				if (Size > Length)
					Size = Length;

				if (Size & 3) return NULL; // this shouldn't happen

				// add desc 
				if (Addr != Last)
				{
					DescAddr += sizeof(xscaledesc);
					if (DescAddr > DescPhy->Length)
					{
						// next desc block
						if (--DescPhyCount==0) return NULL; // not enough desc...
						++DescPhy;
						DescAddr = 0;
					}

					Desc->Next = DescPhy->Addr + DescAddr;
					Desc->Base = Addr;
					Desc->Id = Id;
					Desc->Cmd = Size;
					++Desc;
				}
				else
					Desc[-1].Cmd += Size;
				Last = Addr + Size;

				Length -= Size;
				Pos += Size;
			}

			Pos -= BufferPhy[m].Length;
		}
	}

	--Desc;
	Desc->Next = First;
	Desc->Cmd |= (1<<21);
	return Desc;
}

static bool_t Build(xscaledriver* p)
{
	int n;
	for (n=0;n<p->Count;++n)
	{
		p->DescLast[n] = BuildDesc(p->Desc[n],p->DescPhy[n],p->DescPhyCount[n],p->BufferPhy+p->MaxBlock*n,
			p->BufferPhyCount[n],p->Map,p->Width*2,p->Height,MAGIC | (n<<4));

		if (!p->DescLast[n])
			break;
	}

	if (n==p->Count)
	{
		for (n=p->OrigDescCount?1:0;n<p->Count;++n)
			memset(p->Buffer[n],0,p->BufferSize);
		return 1;
	}

	return 0;
}

static int UpdateAlign(xscale* p)
{
	rect FullScreen;
	int* ScaleY;
	blitfx FX = p->Overlay.FX;
	bool_t FullView;

	if (FX.Direction & DIR_SWAPXY)
		ScaleY = &FX.ScaleX;
	else
		ScaleY = &FX.ScaleY;

	PhyToVirt(NULL,&FullScreen,&p->Overlay.Output.Format.Video);
	FullView = p->Driver->Count && EqRect(&FullScreen,&p->Overlay.Viewport);

	if (FullView && p->Driver->Count>=3)
		p->Overlay.FX.Flags &= ~BLITFX_AVOIDTEARING; //flipping used

	p->Flip = 0;
	p->Next = 0;
	SetOrig(p->Driver);

	OverlayUpdateAlign(&p->Overlay);

	if (FullView && p->Driver->Count)
	{
		int n;
		for (n=0;n<p->Overlay.Output.Format.Video.Height;++n)
			p->Driver->Map[n]=n;

		if (p->Driver->SetupStretch && 
			((p->Overlay.Node.Class == XSCALE_LOW_ID && *ScaleY > SCALE_ONE/2) || *ScaleY > SCALE_ONE) && 
			!(p->Overlay.Output.Format.Video.Width & 1) && p->Overlay.Output.Format.Video.Height>0)
		{
			int Num,Den;
			int Adj;
			int LowSrc,HighSrc,CenterSrc;
			rect DstAlignedRect;
			rect SrcAlignedRect;

			*ScaleY = p->Overlay.Node.Class == XSCALE_LOW_ID ? SCALE_ONE/2 : SCALE_ONE;

			VirtToPhy(&p->Overlay.Viewport,&DstAlignedRect,&p->Overlay.Output.Format.Video);
			VirtToPhy(NULL,&SrcAlignedRect,&p->Overlay.Input.Format.Video);

			BlitRelease(p->Overlay.Soft);
			p->Overlay.Soft = BlitCreate(&p->Overlay.Output.Format.Video,&p->Overlay.Input.Format.Video,&FX,&p->Overlay.Caps);

			BlitAlign(p->Overlay.Soft, &DstAlignedRect, &SrcAlignedRect );

			LowSrc = -(DstAlignedRect.Height/2);
			HighSrc = LowSrc + DstAlignedRect.Height;
			CenterSrc = DstAlignedRect.y - LowSrc;

			Num = DstAlignedRect.Height;
			Den = p->Overlay.DstAlignedRect.Height;
			if (FX.Direction & DIR_SWAPXY)
			{
				Num *= p->Overlay.SrcAlignedRect.Width;
				Den *= SrcAlignedRect.Width;
			}
			else
			{
				Num *= p->Overlay.SrcAlignedRect.Height;
				Den *= SrcAlignedRect.Height;
			}
			Adj = Scale(p->Overlay.DstAlignedRect.Height,Num,2*Den);

			for (n=0;n<p->Overlay.DstAlignedRect.Height;++n)
			{	
				int i = Scale(n,Num,Den) - Adj;
				if (i >= HighSrc) i=HighSrc-1;
				if (i < LowSrc) i=LowSrc;
				p->Driver->Map[p->Overlay.DstAlignedRect.y+n] = CenterSrc + i;
			}
		}

		if (Build(p->Driver))
		{
			p->Overlay.FX.Flags |= BLITFX_AVOIDTEARING; //restore
			p->Flip = 1;
		}
	}

	return ERR_NONE;
}

static void Restore(xscaledriver* p,int Next)
{
	int n;
	char *s,*d;
	char *Src = (char*)p->Buffer[Next];
	char *Dst = (char*)p->OrigBuffer;
	int Middle = p->Height/2;
	int Pitch = p->Width*2;

	for (n=0;n<Middle;++n)
	{
		s = Src + p->Map[n] * Pitch;
		d = Dst + n*Pitch;
		if (s != d)
			memcpy(d,s,Pitch);
	}

	for (n=p->Height-1;n>=Middle;--n)
	{
		s = Src + p->Map[n] * Pitch;
		d = Dst + n*Pitch;
		if (s != d)
			memcpy(d,s,Pitch);
	}
}

static int UpdateShow(xscale* p)
{
	if (!p->Overlay.Show)
	{
		SetOrig(p->Driver);

		if (p->Flip)
		{
			// restore after SetOrig because it's vsynced and restore starts for upper part anyway
			Restore(p->Driver,p->Next); 
		}
		p->Next = 0;
	}
	return ERR_NONE;
}

static int Lock(xscale* p, planes Planes, bool_t OnlyAligned )
{
	if (p->Flip)
	{
		int Next = p->Next+1;
		if (Next == p->Driver->Count) 
			Next = 0;

		// use new buffer only if it's not the current
		if (GetCurrent(p->Driver) != Next) 
			p->Next = Next;

		Planes[0] = p->Driver->Buffer[p->Next];
	}
	else
		Planes[0] = p->Driver->OrigBuffer;

	return ERR_NONE;
}

static int Unlock(xscale* p )
{
	if (p->Flip)
	{
		int Current = GetCurrent(p->Driver);
		uint32_t DescPhy = p->Driver->DescPhy[p->Next]->Addr;

		// close chain
		p->Driver->DescLast[p->Next]->Next = DescPhy;

#ifdef X50V
		if (p->Driver->X50v && X50vEnable(p->Driver,DescPhy))
			Current = p->Next;
#endif

		DEBUG_MSG4(-1,T("%08x %08x %08x %d"),p->Driver->Regs[LCCR0],p->Driver->Regs[LCCR3],p->Driver->Regs[FDADR0],Current);

		if (Current<-1)
			return ERR_DEVICE_ERROR;

		if (Current<0)
		{
			if (p->Driver->OrigDescEnd)
			{
				p->Driver->OrigDescEnd->Next = DescPhy;
				p->Driver->OrigDescEndSet = 1;
			}
			else
			if (p->Driver->OrigDescCount>1) 
			{
				// multi desc original
				bool_t Old = (p->Driver->Regs[LCCR0] & 1);
				Disable(p->Driver);
				p->Driver->Regs[FBR0] = 0;
				p->Driver->Regs[FDADR0] = DescPhy;
				if (Old) Enable(p->Driver);
			}
			else // simply branch
				p->Driver->Regs[FBR0] = DescPhy | 1; 
		}
		else
		{
			// modify last frames tail to new frame
			Current = p->Next-1;
			if (Current<0)
				Current = p->Driver->Count-1;
			p->Driver->DescLast[Current]->Next = DescPhy;
		}
		p->Driver->NotOrig = 1;
	}

	return ERR_NONE;
}

static int UpdateStretch(xscaledriver* Driver)
{
	if (Driver->Used)
		UpdateAlign(Driver->Used);
	return ERR_NONE;
}

static int UpdateFlip(xscaledriver* Driver)
{
	if (Driver->Used)
	{
		xscale* p = Driver->Used;
		Done(p);
		FreeBuffer(Driver);
		Init(p);
		UpdateAlign(p);
		p->Overlay.Dirty = 1;
		p->Overlay.LastTime = -1;
	}
	return ERR_NONE;
}

static const datatable Params[] = 
{
	{ XSCALEDRIVER_STRETCH,			TYPE_BOOL, DF_SETUP|DF_CHECKLIST },
	{ XSCALEDRIVER_FLIP,			TYPE_BOOL, DF_SETUP|DF_CHECKLIST },
	{ XSCALEDRIVER_ALLOC_AT_START,	TYPE_BOOL, DF_SETUP|DF_CHECKLIST },

	DATATABLE_END(XSCALEDRIVER_ID)
};

static int Enum(xscaledriver* p, int* No, datadef* Param)
{
	return NodeEnumTable(No,Param,Params);
}

static int Get(xscaledriver* p,int No,void* Data,int Size)
{
	int Result = ERR_INVALID_PARAM;
	switch (No) 
	{
	case XSCALEDRIVER_STRETCH: GETVALUE(p->SetupStretch,bool_t); break;
	case XSCALEDRIVER_FLIP: GETVALUE(p->SetupFlip,bool_t); break;
	case XSCALEDRIVER_ALLOC_AT_START: GETVALUE(p->AllocAtStart,bool_t); break;
	}
	return Result;
}

static bool_t Selected()
{
	int Id;
	return NodeRegLoadValue(PLAYER_ID,PLAYER_VOUTPUTID,&Id,sizeof(Id),TYPE_INT) && (Id == XSCALE_ID || Id == XSCALE_LOW_ID);
}

static int Set(xscaledriver* p,int No,const void* Data,int Size)
{
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case XSCALEDRIVER_STRETCH: SETVALUECMP(p->SetupStretch,bool_t,UpdateStretch(p),EqBool); break;
	case XSCALEDRIVER_FLIP: SETVALUECMP(p->SetupFlip,bool_t,UpdateFlip(p),EqBool); break;
	case XSCALEDRIVER_ALLOC_AT_START: SETVALUE(p->AllocAtStart,bool_t,ERR_NONE); break;
	case NODE_SETTINGSCHANGED:
		if (p->AllocAtStart && p->AtStart && Selected())
		{
			if (!p->OrigBuffer)
				GetOrig(p);
			AllocBuffer(p);
		}
		p->AtStart = 0;
		break;
	case NODE_HIBERNATE:
		if (!p->Used && FreeBuffer(p))
			Result = ERR_NONE;
		break;
	}
	return Result;
}

static int Create(xscale* p)
{
	p->Driver = (xscaledriver*)NodeEnumObject(NULL,XSCALEDRIVER_ID);
	if (!p->Driver)
		return ERR_NOT_SUPPORTED;

	if (p->Overlay.Node.Class == XSCALE_LOW_ID && (p->Driver->Width*p->Driver->Height <= 320*240 || 
		(QueryPlatform(PLATFORM_CAPS) & CAPS_ARM_WMMX)!=0))
		return ERR_NOT_SUPPORTED;

	GetMode(p);
	p->Overlay.Init = Init;
	p->Overlay.Done= Done;
	p->Overlay.Reset = Reset;
	p->Overlay.Update = UpdateAlign;
	p->Overlay.UpdateShow = UpdateShow;
	p->Overlay.Lock = Lock;
	p->Overlay.Unlock = Unlock;
	return ERR_NONE;
}

static int CreateDriver(xscaledriver* p)
{
	bool_t Detected;
	int Caps = QueryPlatform(PLATFORM_CAPS);
	if (!(Caps & CAPS_ARM_XSCALE))
		return ERR_NOT_SUPPORTED;

	p->Node.Enum = Enum;
	p->Node.Get = Get;
	p->Node.Set = Set;

	p->AtStart = 1;
	p->OrigBuffer = NULL;
	p->OrigDescCount = 0;
	p->Regs = (uint32_t*)PhyMemBegin(LCD_BASE,LCD_SIZE,0);
	if (!p->Regs)
		return ERR_NOT_SUPPORTED;

#ifdef X50V
	p->X50v = QueryPlatform(PLATFORM_MODEL)==MODEL_AXIM_X50 && CheckModule(T("pvrvadd.dll"));
	if (p->X50v)
	{
		p->Width = 480;
		p->Height = 640;
		p->Marathon = (uint32_t*) 0xBC000000;
		p->GPIO = (uint32_t*) PhyMemBegin(GPIO_BASE,GPIO_SIZE,0);
		if (p->GPIO)
		{
			RawFrameBufferInfo Info;
			HDC DC= GetDC(NULL);
			memset(&Info,0,sizeof(Info));
			ExtEscape(DC, GETRAWFRAMEBUFFER, 0, NULL, sizeof(RawFrameBufferInfo), (char*)&Info);
			ReleaseDC(NULL,DC);

			p->OrigBuffer = Info.pFramePointer;
		}
	}
	else
#endif
	if (!(p->Regs[LCCR0] & 1) || ((p->Regs[LCCR3] >> 24) & 7)!=4) // LCD enabled, RGB 16bit
		return ERR_NOT_SUPPORTED;

	if (!NodeRegLoadValue(XSCALEDRIVER_ID, XSCALEDRIVER_DETECTED, &Detected, sizeof(Detected), TYPE_BOOL) || !Detected)
	{
		GetOrig(p);
		if (!p->OrigBuffer)
		{
			PhyMemEnd(p->Regs);
			p->Regs = NULL;
			return ERR_NOT_SUPPORTED;
		}
		Detected = 1;
		NodeRegSaveValue(XSCALEDRIVER_ID, XSCALEDRIVER_DETECTED, &Detected, sizeof(Detected), TYPE_BOOL);
	}

	p->AllocAtStart = 1;
	p->SetupFlip = 1;
	p->SetupStretch = (QueryPlatform(PLATFORM_CAPS) & CAPS_ARM_WMMX)==0;
	return ERR_NONE;
}

static void DeleteDriver(xscaledriver* p)
{
	if (p->Regs)
		SetOrig(p);

	FreeBuffer(p);
	FreeOrig(p);

#ifdef X50V
	if (p->GPIO)
	{
		PhyMemEnd(p->GPIO);
		p->GPIO = NULL;
	}	
#endif

	if (p->Regs)
	{
		PhyMemEnd(p->Regs);
		p->Regs = NULL;
	}	
}

static const nodedef XScaleDriver = 
{
	sizeof(xscaledriver)|CF_GLOBAL|CF_SETTINGS,
	XSCALEDRIVER_ID,
	NODE_CLASS,
	PRI_DEFAULT,
	(nodecreate)CreateDriver,
	(nodedelete)DeleteDriver,
};

static const nodedef XScale = 
{
	sizeof(xscale)|CF_GLOBAL,
	XSCALE_ID,
	OVERLAY_CLASS,
	PRI_DEFAULT+97,
	(nodecreate)Create,
};

static const nodedef XScaleLow = 
{
	sizeof(xscale)|CF_GLOBAL,
	XSCALE_LOW_ID,
	XSCALE_ID,
	PRI_DEFAULT+96,
};

void OverlayXScale_Init() 
{ 
	NodeRegisterClass(&XScaleDriver);
	NodeRegisterClass(&XScale);
	NodeRegisterClass(&XScaleLow);
}

void OverlayXScale_Done()
{
	NodeUnRegisterClass(XSCALE_LOW_ID);
	NodeUnRegisterClass(XSCALE_ID);
	NodeUnRegisterClass(XSCALEDRIVER_ID);
}

#else
void OverlayXScale_Init() {}
void OverlayXScale_Done() {}
#endif
