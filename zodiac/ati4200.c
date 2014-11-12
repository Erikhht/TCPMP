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
 * $Id: ati4200.c 545 2006-01-07 22:26:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common/common.h"

#if defined(TARGET_PALMOS)

#if defined(_M_IX86)
//#define ATIEMU
#endif

#if _MSC_VER > 1000
#pragma warning(disable : 4127)
#endif

#include "../common/palmos/pace.h"

#define ATI4200_ID			FOURCC('A','T','I','4')
#define ATI4200_IDCT_ID		VOUT_IDCT_CLASS(ATI4200_ID)

#define DCFACTOR 8

#define RING_NOP2			0
#define RING_NOP3			((1|0x80000)<<11)

#define BUFFER_COUNT		3
#define ALL_COUNT			5
#define SCALE_COUNT			3

#define REG_BASE			0x010000
#define INT_BASE			0x100000
#define EXT_BASE			0x800000

#define PWRMGT_CNTL			(0x0098/4)
#define GRAPHIC_CTRL        (0x0414/4)
#define GRAPHIC_OFFSET      (0x0418/4)
#define GRAPHIC_PITCH       (0x041C/4)
#define GRAPHIC_H_DISP      (0x042C/4)
#define GRAPHIC_V_DISP      (0x0430/4)
#define GEN_STATUS			(0x054C/4)
#define ACTIVE_H_DISP		(0x0424/4)
#define ACTIVE_V_DISP		(0x0428/4)
#define DISP_BUF			(0x04D8/4)
#define DISP_DEBUG			(0x04D4/4)
#define DISP_DEBUG2			(0x0538/4)
#define VIDEO_CTRL			(0x0434/4)
#define VIDEO_Y_OFFSET		(0x043C/4)
#define VIDEO_Y_PITCH		(0x0440/4)
#define VIDEO_U_OFFSET		(0x0444/4)
#define VIDEO_U_PITCH		(0x0448/4)
#define VIDEO_V_OFFSET		(0x044C/4)
#define VIDEO_V_PITCH		(0x0450/4)
#define VIDEO_H_POS			(0x0454/4)
#define VIDEO_V_POS			(0x0458/4)

#define RING_CNTL			(0x0210/4)
#define RING_BASE			(0x0214/4)
#define RING_RPTR_ADDR		(0x0218/4)
#define RING_RPTR			(0x021C/4)
#define RING_RPTR_WR		(0x02F8/4)
#define RING_WPTR			(0x0220/4)
#define RBBM_STATUS			(0x0140/4)
#define RBBM_CNTL			(0x0144/4)

#define REF1_PITCH_OFFSET	(0x10B8/4)
#define REF2_PITCH_OFFSET	(0x10BC/4)
#define REF3_PITCH_OFFSET	(0x10C0/4)
#define REF4_PITCH_OFFSET	(0x10C4/4)
#define REF5_PITCH_OFFSET	(0x10C8/4)
#define REF6_PITCH_OFFSET	(0x10CC/4)
#define SRC_SC_BOTTOM_RIGHT (0x11C4/4)
#define IDCT_CONTROL        (0x0C3C/4)

#define DP_GUI_MASTER_CNTL	0x41B 
#define DP_CNTL				0x472 	
#define DP_MIX				0x4B2 	
#define DP_DATATYPE			0x4B1 	
#define SC_TOP_LEFT			0x46F 	
#define SC_BOTTOM_RIGHT		0x470 	
#define DST_OFFSET			0x401 	
#define DST_PITCH			0x402 	
#define SRC_OFFSET			0x46B 	
#define SRC_PITCH			0x46C 	
#define E2_ARITHMETIC_CNTL	0x488 

#define SRC2_X_Y			0x416
#define SRC2_OFFSET			0x418 
#define SRC2_PITCH			0x419 
#define SRC_WIDTH			0x410 
#define SRC_HEIGHT			0x411 
#define SRC_INC				0x412 
#define SRC2_WIDTH			0x420 
#define SRC2_HEIGHT			0x421 
#define SRC2_INC			0x422 
#define SRC_X_Y				0x464 
#define DST_X_Y				0x465 
#define DST_WIDTH_HEIGHT	0x466 

typedef struct atisurf
{
	planes Planes;
	int YOffset;
	int YPitch;
	int UOffset;
	int UPitch;
	int VOffset;
	int VPitch;
	
} atisurf;

typedef struct idct_atidata
{
	int8_t skip3,skip2,skip1,skip0;
	int16_t data1,data0,data3,data2;

} idct_atidata;

typedef struct idct_ati
{
	union
	{
		idct_atidata d;
		int32_t v[3];
	};

} idct_ati;

#define BACKUP_LEN	16

static const int BackupRegs[BACKUP_LEN] = 
{
	DP_GUI_MASTER_CNTL,
	DP_CNTL,
	DP_MIX,
	DP_DATATYPE,
	DST_OFFSET,
	DST_PITCH,
	SRC_OFFSET,
	SRC_PITCH,
	E2_ARITHMETIC_CNTL,
	IDCT_CONTROL,
	REF1_PITCH_OFFSET,
	REF2_PITCH_OFFSET,
	REF3_PITCH_OFFSET,
	REF4_PITCH_OFFSET,
	REF5_PITCH_OFFSET,
	REF6_PITCH_OFFSET,
};

typedef struct ati4200
{
	overlay p;
	idct IdctVMT;

	int RingMask;
	int RingPos;
	int RingPos0;
	int RingRdPos;
	volatile int* Ring;

	int DCAdd;
	int SubNo;
	int MacroPos;
	int MacroCoord;
	int LastMode;
	int LastScanType;
	int ArithMode;
	int InterIDCT;
	int MVBack;
	int32_t MVBackTab[6];
	int MVFwd;
	int32_t MVFwdTab[6];
	atisurf* Dst;

	int AllCount;
	int BufCount;
	int ShowLast;
	int ShowCurr;
	int ShowNext;
	int IDCTWidth;
	int IDCTHeight;
	pin IDCTOutput;
	bool_t Rounding;
	bool_t IDCT;
	bool_t IDCTInit;

	int SaveScanType;
	int SavePos;
	int SaveData[64]; //data:16*3+close:1

	int BufferEnd;
	atisurf Buffer[ALL_COUNT];
	atisurf Scale[SCALE_COUNT];
	rect OverlayRect;
	video Overlay;
	blitfx SoftFX;
	point Offset;
	bool_t LowBitdepth;
	int Memory;
	uint8_t* Device;
	volatile int* Regs;
	int ShowScale;
	int Mirror;

	int BufFrameNo[MAXIDCTBUF];
	int Backup[BACKUP_LEN];

} ati4200;

extern char* QueryScreen(video* Video,int* Offset,int* Mode);

static bool_t Idle(ati4200* p)
{
	if (p->Regs[RING_RPTR] != p->Regs[RING_WPTR])
		return 0;

	if (p->Regs[RBBM_STATUS] & 0x80000000)
		return 0;

	return 1;
}

static NOINLINE bool_t Wait(ati4200* p)
{
	int n;
	for (n=0;n<100000;++n)
	{
		if (Idle(p))
			return 1;
		HALDelay(50);
	}
	return 0;
}

static INLINE void RingPush(ati4200* p, int Value)
{
	p->Ring[p->RingPos] = Value;
	p->RingPos = (p->RingPos+1) & p->RingMask;
}

static NOINLINE void RingReg(ati4200* p, int Reg, int Value)
{
	RingPush(p,Reg);
	RingPush(p,Value);
}

static INLINE void RingNOP2(ati4200* p)
{
	p->Ring[p->RingPos] = RING_NOP2;
	p->RingPos = (p->RingPos+2) & p->RingMask;
}

static INLINE void RingNOP3(ati4200* p)
{
	p->Ring[p->RingPos] = RING_NOP3;
	p->RingPos = (p->RingPos+3) & p->RingMask;
}

static NOINLINE void RingPrepair(ati4200* p, int Len)
{
	if (((p->RingRdPos - p->RingPos - 1) & p->RingMask) < Len)
	{
		p->RingRdPos = p->Regs[RING_RPTR];

		while (((p->RingRdPos - p->RingPos - 1) & p->RingMask) < Len)
		{
			HALDelay(50);
			p->RingRdPos = p->Regs[RING_RPTR];
		}
	}
}

static INLINE void RingFlush(ati4200* p)
{
	p->Regs[RING_WPTR] = p->RingPos;

#ifdef ATIEMU
	if (p->Regs[RING_RPTR] != p->RingPos)
	{
		if (p->Ring[p->Regs[RING_RPTR]] == RING_NOP3 ||
			p->Ring[p->Regs[RING_RPTR]] == RING_NOP2)
			p->Regs[RING_RPTR] = p->RingPos;
		else
		{
			//DebugMessage("ATI Command");
			while (p->Regs[RING_RPTR] != p->RingPos)
			{
				//DebugMessage("ATI %08x",p->Ring[p->Regs[RING_RPTR]]);
				p->Regs[RING_RPTR] = (p->Regs[RING_RPTR] + 1) & p->RingMask;
			}
		}
	}
#endif
}

static NOINLINE void RingFill(ati4200* p,int TargetPos)
{
	int n = (TargetPos - p->RingPos) & p->RingMask;
	if (n)
	{
		if (n==1)
			n += p->RingMask+1; // bad luck

		while (n>1024*3+4)
		{
			int i;
			RingPrepair(p,1024*3);
			for (i=0;i<1024;++i)
				RingNOP3(p);
			RingFlush(p);
			n -= 1024*3;
		}

		RingPrepair(p,n);
		for (;n>4;n-=3) 
			RingNOP3(p);
		switch (n)
		{
		case 4:	RingNOP2(p); //no break
		case 2: RingNOP2(p); break;
		case 3: RingNOP3(p); break;
		}
		RingFlush(p);
	}

	assert(p->RingPos == TargetPos);
}

static void RingBegin(ati4200* p,bool_t Small)
{
	int v;
	p->RingRdPos = p->Regs[RING_RPTR];
	p->RingPos = p->Regs[RING_WPTR] & p->RingMask;
	v = p->RingPos == p->RingPos0 ? p->Ring[p->RingPos] : -1;
	p->RingPos0 = p->RingPos;

	if (v != RING_NOP3 && v != RING_NOP2)
	{
		// have to wait before doing the register backup
		int n;
		Wait(p); 
		for (n=0;n<BACKUP_LEN;++n) 
			p->Backup[n] = p->Regs[BackupRegs[n]];
	}

	if (Small)
	{
		int SmallPos = (p->RingPos - 256) & p->RingMask;

		if (v != RING_NOP3)
			RingFill(p,SmallPos);
		else
		{
			// assuming ring is already filled
			p->RingPos = SmallPos; 
			RingFlush(p);
		}
	}
}

static void RingEnd(ati4200* p)
{
	int n;

	// restore backup registers
	RingPrepair(p,2*BACKUP_LEN+2*2);
	RingReg(p,SC_TOP_LEFT,0);
	RingReg(p,SC_BOTTOM_RIGHT,0x1FFF1FFF);
	RingReg(p,SRC_SC_BOTTOM_RIGHT,0x1FFF1FFF);

	for (n=0;n<BACKUP_LEN;++n) 
		RingReg(p,BackupRegs[n],p->Backup[n]);
	RingFlush(p);

	// restore ring write position
	RingFill(p,p->RingPos0); 

	// let some free space for the OS driver
	RingPrepair(p,p->RingMask >> 1); 

	// signal for next iteration that no backup required
	if (p->Ring[p->RingPos0] != RING_NOP3)
		p->Ring[p->RingPos0] = RING_NOP2; 
}

static void AllocBuffer(ati4200* p,atisurf* s,int w,int h,int* Offset)
{
	int ofs = *Offset;
	int size;
	w = ALIGN16(w);
	h = ALIGN16(h);
	size = w*h;

	s->YOffset = ofs;
	ofs += size;
	s->YPitch = w;
	s->UOffset = ofs;
	ofs += size/4;
	s->UPitch = w/2;
	s->VOffset = ofs;
	ofs += size/4;
	s->VPitch = w/2;

	s->Planes[0] = p->Device + s->YOffset;
	s->Planes[1] = p->Device + s->UOffset;
	s->Planes[2] = p->Device + s->VOffset;

	memset(s->Planes[0],0,size);
	memset(s->Planes[1],128,size/4);
	memset(s->Planes[2],128,size/4);

	*Offset = ofs;
}

static void BeginUpdate(ati4200* p)
{
	p->Regs[DISP_BUF] = 0x78;
}

static void EndUpdate(ati4200* p)
{
	p->Regs[DISP_BUF] = 0x7B;
}

static void SetLowBitdepth(ati4200* p,bool_t Value)
{
/*	if (p->LowBitdepth != Value)
	{
		int Ctrl;

		p->LowBitdepth = Value;

		//if (Value)
		//	ClearPrimary(p);

		while ((p->Regs[GEN_STATUS] & 0x400)==0); // wait until not in vblank 
		while ((p->Regs[GEN_STATUS] & 0x400)!=0); // wait until in vblank

		Ctrl = p->Regs[GRAPHIC_CTRL];
		Ctrl &= ~7;
		Ctrl |= Value ? 2:6;
		p->Regs[GRAPHIC_CTRL] = Ctrl;
	}
*/
}

static void UpdateLowBitdepth(ati4200* p)
{
	SetLowBitdepth(p,p->p.Show && p->p.FullScreenViewport);
}

static int Hide(ati4200* p)
{
	SetLowBitdepth(p,0);
	BeginUpdate(p);
	p->Regs[VIDEO_CTRL] &= ~0x1C;
	EndUpdate(p);
#ifndef ATIEMU
	while (p->Regs[DISP_BUF] & 2);
#endif
	p->Regs[PWRMGT_CNTL] |= 1;
	return ERR_NONE;
}

static int Show(ati4200* p)
{
	UpdateLowBitdepth(p);
	BeginUpdate(p);
	p->Regs[VIDEO_CTRL] |= 0x14;
	p->Regs[PWRMGT_CNTL] &= ~1;
	EndUpdate(p);
	return ERR_NONE;
}

static uint8_t* GetBits()
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

static int IDCTUpdateFunc(ati4200* p);

static void GetMode(ati4200* p)
{
	QueryDesktop(&p->p.Output.Format.Video);

	p->p.Output.Format.Video.Direction = GetOrientation() ^ (DIR_MIRRORUPDOWN | DIR_MIRRORLEFTRIGHT);

	if (p->p.Output.Format.Video.Direction & DIR_SWAPXY)
	{
		int Ofs = GetBits() - (p->Device+INT_BASE);
		int h;
		h = p->Regs[ACTIVE_V_DISP];
		h = ((h >> 16) & 0x3FF) - (h & 0x3FF);

		SwapInt(&p->p.Output.Format.Video.Width,&p->p.Output.Format.Video.Height);

		if (Ofs>=0 && Ofs<h*2)
			h -= Ofs>>1;

		p->Offset.y = h - p->p.Output.Format.Video.Height;
	}
	else
		p->Offset.y = 0;
}

static int Init(ati4200* p)
{
	p->p.SetFX = BLITFX_AVOIDTEARING;
	p->p.Overlay = 1;

	p->IDCT = 0;
	p->ShowScale = -1;
	p->ShowCurr = 0;
	p->ShowNext = -1;
	p->ShowLast = -1;
	p->AllCount = 0;
	p->BufCount = 0;
	p->BufferEnd = 0;
	p->LowBitdepth = 0;

	if (p->IDCTInit)
	{
		if (!PlanarYUV420(&p->p.Input.Format.Video.Pixel))
			return ERR_NOT_SUPPORTED;

		RingBegin(p,1);
		RingPrepair(p,2);
		RingReg(p,0x30E,0);
		RingFlush(p);
		RingEnd(p);

		p->IDCT = 1;
		p->Rounding = 0;
		IDCTUpdateFunc(p);
	}

	GetMode(p);
	return ERR_NONE;
}

static void Done(ati4200* p)
{
	Wait(p);
	Hide(p);

	p->IDCT = 0;
	p->BufferEnd = 0;
	p->AllCount = 0;
}

static int Reset(ati4200* p)
{
	GetMode(p);
	return ERR_NONE;
}

static int UpdateShow(ati4200* p)
{
	if (p->p.Show)
		Show(p);
	else
		Hide(p);
	return ERR_NONE;
}

static int MirrorOffset(ati4200* p,atisurf* s,int Plane)
{
	int Ofs = 0;
	int Width = p->p.DstAlignedRect.Width;
	int Height = p->p.DstAlignedRect.Height;
	int Pitch = s->YPitch;
	int x = 0;
	int y = 0;

	if (p->ShowScale<0)
	{
		x = p->OverlayRect.x & ~7;
		y = p->OverlayRect.y;
	}

	if (Plane)
	{
		Pitch >>= 1;
		Width >>= 1;
		Height >>= 1;
		x >>= 1;
		y >>= 1;
	}

	Ofs += x;
	Ofs += y * Pitch;

	if (p->Mirror & DIR_MIRRORLEFTRIGHT)
		Ofs += Width-1;

	if (p->Mirror & DIR_MIRRORUPDOWN)
		Ofs += (Height-1)*Pitch;
	
	return Ofs;
}

static void SwapPtr(ati4200* p,int a,int b,int SwapShow)
{
	atisurf t;
	
	memcpy(&t,p->Buffer+a,sizeof(atisurf));
	memcpy(p->Buffer+a,p->Buffer+b,sizeof(atisurf));
	memcpy(p->Buffer+b,&t,sizeof(atisurf));

	if (SwapShow)
	{
		SwapInt(&p->BufFrameNo[a],&p->BufFrameNo[b]);
	
		if (p->ShowNext == a)
			p->ShowNext = b;
		else
		if (p->ShowNext == b)
			p->ShowNext = a;

		if (p->ShowLast == a)
			p->ShowLast = b;
		else
		if (p->ShowLast == b)
			p->ShowLast = a;

		if (p->ShowCurr == a)
			p->ShowCurr = b;
		else
		if (p->ShowCurr == b)
			p->ShowCurr = a;
	}
}

static void Stretch(ati4200* p,bool_t Flip);

static int Update(ati4200* p)
{
	video OldOverlay;
	blitfx OvlFX = p->p.FX;
	rect BufferRect;
	int Total;
	int Ctrl,n,x0,y0,x1,y1;
	int Offset = p->Memory;
	atisurf* s; 

	Wait(p);
	Hide(p);

	memcpy(&OldOverlay,&p->Overlay,sizeof(OldOverlay));
	OvlFX.Direction = 0;
	p->Mirror = 0;

	p->SoftFX = p->p.FX;
	p->SoftFX.ScaleX = SCALE_ONE; // scale handled by hardware
	p->SoftFX.ScaleY = SCALE_ONE;
	p->SoftFX.Flags |= BLITFX_ENLARGEIFNEEDED;

	if (p->IDCT)
	{
		// use overlay mirroring, because hardware idct can only swapxy
		p->SoftFX.Direction &= DIR_SWAPXY;
		p->Mirror = p->p.FX.Direction & ~DIR_SWAPXY;
		p->AllCount = ALL_COUNT;
	}
	else
		p->AllCount = BUFFER_COUNT;

	//align to 100%
	if (OvlFX.ScaleX > SCALE_ONE-(SCALE_ONE/16) && OvlFX.ScaleX < SCALE_ONE+(SCALE_ONE/16) &&
		OvlFX.ScaleY > SCALE_ONE-(SCALE_ONE/16) && OvlFX.ScaleY < SCALE_ONE+(SCALE_ONE/16))
	{
		OvlFX.ScaleX = SCALE_ONE;
		OvlFX.ScaleY = SCALE_ONE;
	}

	if (p->SoftFX.Direction & DIR_SWAPXY)
		SwapInt(&OvlFX.ScaleX,&OvlFX.ScaleY);

	memset(&p->Overlay,0,sizeof(p->Overlay));
	p->Overlay.Pixel.Flags = PF_YUV420;
	p->Overlay.Width = ALIGN16(p->IDCT?p->IDCTWidth:p->p.Input.Format.Video.Width);
	p->Overlay.Height = ALIGN16(p->IDCT?p->IDCTHeight:p->p.Input.Format.Video.Height);
	p->Overlay.Direction = (p->p.FX.Direction ^ p->p.Input.Format.Video.Direction) & DIR_SWAPXY;
	if ((p->IDCT?p->Overlay.Direction:p->p.FX.Direction) & DIR_SWAPXY)
		SwapInt(&p->Overlay.Width,&p->Overlay.Height);
	p->Overlay.Pitch = p->Overlay.Width;

	VirtToPhy(NULL,&p->OverlayRect,&p->p.Input.Format.Video);
	if (p->p.FX.Direction & DIR_SWAPXY)
		SwapInt(&p->OverlayRect.Width,&p->OverlayRect.Height);

	if (p->IDCT)
		IDCTUpdateFunc(p);

	VirtToPhy(&p->p.Viewport,&p->p.DstAlignedRect,&p->p.Output.Format.Video);

	if (OvlFX.ScaleX != SCALE_ONE || OvlFX.ScaleY != SCALE_ONE)
	{
		// avoid bilinear scale overrun (shrink source)
		if (p->OverlayRect.Width>2) 
		{
			OvlFX.ScaleX = Scale(OvlFX.ScaleX,p->OverlayRect.Width,p->OverlayRect.Width-2);
			p->OverlayRect.Width -= 2;
		}
		if (p->OverlayRect.Height>2) 
		{
			OvlFX.ScaleY = Scale(OvlFX.ScaleY,p->OverlayRect.Height,p->OverlayRect.Height-2);
			p->OverlayRect.Height -= 2;
		}
	}

	AnyAlign(&p->p.DstAlignedRect, &p->OverlayRect, &OvlFX, 2, 1, SCALE_ONE>>1, SCALE_ONE*16);

	VirtToPhy(NULL,&p->p.SrcAlignedRect,&p->p.Input.Format.Video);

	BufferRect.x = p->OverlayRect.x & ~15;
	BufferRect.y = p->OverlayRect.y & ~15;
	BufferRect.Width = ALIGN16(p->OverlayRect.Width + (p->OverlayRect.x & 15));
	BufferRect.Height = ALIGN16(p->OverlayRect.Height + (p->OverlayRect.y & 15));

	BlitRelease(p->p.Soft);
	p->p.Soft = BlitCreate(&p->Overlay,&p->p.Input.Format.Video,&p->SoftFX,&p->p.Caps);
	BlitAlign(p->p.Soft,&BufferRect,&p->p.SrcAlignedRect);

	if (!p->IDCT)
	{
		// adjust Overlay size if not all part of the input is needed
		p->BufferEnd = 0;
		p->Overlay.Width = ALIGN16(BufferRect.Width);
		p->Overlay.Height = ALIGN16(BufferRect.Height);
		p->Overlay.Pitch = p->Overlay.Width;

		// adjust OverlayRect
		BufferRect.x = p->p.SrcAlignedRect.x;
		BufferRect.y = p->p.SrcAlignedRect.y;
		if (p->SoftFX.Direction & DIR_SWAPXY)
			SwapInt(&BufferRect.x,&BufferRect.y);
		p->OverlayRect.x -= BufferRect.x;
		p->OverlayRect.y -= BufferRect.y;
		ClipRectPhy(&p->OverlayRect,&p->Overlay);

		BufferRect.x = 0;
		BufferRect.y = 0;

		BlitRelease(p->p.Soft);
		p->p.Soft = BlitCreate(&p->Overlay,&p->p.Input.Format.Video,&p->SoftFX,&p->p.Caps);
		BlitAlign(p->p.Soft,&BufferRect,&p->p.SrcAlignedRect);
	}
	else
	{
		int DCAdd;

		p->p.Caps = VC_BRIGHTNESS;

		// swapxy changed?
		if (p->BufferEnd && ((OldOverlay.Direction ^ p->Overlay.Direction) & DIR_SWAPXY))
		{
			for (n=0;n<p->AllCount;++n)
			{
				p->Buffer[n].YPitch = p->Overlay.Pitch;
				p->Buffer[n].UPitch = p->Overlay.Pitch >> 1;
				p->Buffer[n].VPitch = p->Overlay.Pitch >> 1;
			}

			if (p->AllCount > p->BufCount)
				for (n=0;n<p->BufCount;++n)
				{
					SurfaceRotate(&OldOverlay,&p->Overlay,p->Buffer[n].Planes,
					              p->Buffer[p->BufCount].Planes,DIR_SWAPXY);
					SwapPtr(p,n,p->BufCount,0);
				}
		}

		// brightness changed?
		DCAdd = p->p.FX.Brightness * DCFACTOR;
		if (p->DCAdd != DCAdd)
		{
			int Delta = (DCAdd - p->DCAdd) / DCFACTOR;
			p->DCAdd = DCAdd;

			if (p->BufferEnd)
				for (n=0;n<p->BufCount;++n)
				{
					uint8_t* v = p->Buffer[n].Planes[0];
					uint8_t* ve = v + p->Overlay.Width * p->Overlay.Height;
					for (;v!=ve;v+=4)
					{
						int a = v[0] + Delta;
						int b = v[1] + Delta;
						int c = v[2] + Delta;
						int d = v[3] + Delta;
						if (a>255) a=255; else if (a<0) a=0;
						if (b>255) b=255; else if (b<0) b=0;
						if (c>255) c=255; else if (c<0) c=0;
						if (d>255) d=255; else if (d<0) d=0;
						*(uint32_t*)v = a | (b<<8) | (c<<16) | (d<<24);
					}
				}
		}
	}

	if (!p->BufferEnd)
	{
		for (n=0;n<p->AllCount;++n)
		{
			AllocBuffer(p,&p->Buffer[n],p->Overlay.Width,p->Overlay.Height,&Offset);
			p->BufFrameNo[n] = -1;
		}
		p->BufferEnd = Offset;
	}
	else
		Offset = p->BufferEnd;

	if (OvlFX.ScaleX != SCALE_ONE || OvlFX.ScaleY != SCALE_ONE)
	{
		for (n=0;n<SCALE_COUNT;++n)
			AllocBuffer(p,&p->Scale[n],p->p.DstAlignedRect.Width,p->p.DstAlignedRect.Height,&Offset);
		p->ShowScale = 0;
		Stretch(p,0);
		s = p->Scale + p->ShowScale;
	}
	else
	{
		// no scaling -> adjust dst rectangle with soft yuv blitting alignement
		memset(p->Scale,0,sizeof(p->Scale));
		p->ShowScale = -1;

		p->p.DstAlignedRect.x += (p->p.DstAlignedRect.Width - p->OverlayRect.Width) >> 1;
		p->p.DstAlignedRect.y += (p->p.DstAlignedRect.Height - p->OverlayRect.Height) >> 1;
		p->p.DstAlignedRect.Width = p->OverlayRect.Width;
		p->p.DstAlignedRect.Height = p->OverlayRect.Height;
		s = p->Buffer + p->ShowCurr;
	}

	PhyToVirt(&p->p.DstAlignedRect,&p->p.GUIAlignedRect,&p->p.Output.Format.Video);

	BeginUpdate(p);
	p->Regs[DISP_DEBUG] &= ~0x1F0;

	Total = (p->p.DstAlignedRect.Width + 3) >> 2;
	Ctrl = 
		(1 << 0)|		// yuv420 video mode
		((Total & 511) << 9) |
		(1 << 22)|		// yuv
		(1 << 23);		// yuv option

	if (p->Mirror & DIR_MIRRORLEFTRIGHT)
		Ctrl |= 1 << 24;

	if (p->Mirror & DIR_MIRRORUPDOWN)
		Ctrl |= 1 << 25;
	
	p->Regs[VIDEO_CTRL] = Ctrl;

	x0 = p->Regs[ACTIVE_H_DISP] & 0x3FF;
	y0 = p->Regs[ACTIVE_V_DISP] & 0x3FF;

	x0 += p->Offset.x;
	y0 += p->Offset.y;

	x0 += p->p.DstAlignedRect.x;
	y0 += p->p.DstAlignedRect.y;

	x1 = x0 + p->p.DstAlignedRect.Width;
	y1 = y0 + p->p.DstAlignedRect.Height;

	p->Regs[VIDEO_H_POS] = x0 | (x1 << 16);
	p->Regs[VIDEO_V_POS] = y0 | (y1 << 16);
	p->Regs[VIDEO_Y_OFFSET] = s->YOffset+MirrorOffset(p,s,0);
	p->Regs[VIDEO_Y_PITCH] = s->YPitch;
	p->Regs[VIDEO_U_OFFSET] = s->UOffset+MirrorOffset(p,s,1);
	p->Regs[VIDEO_U_PITCH] = s->UPitch;
	p->Regs[VIDEO_V_OFFSET] = s->VOffset+MirrorOffset(p,s,2);
	p->Regs[VIDEO_V_PITCH] = s->VPitch;
	p->Regs[DISP_DEBUG2] |= 0x40000000;

	EndUpdate(p);

	return UpdateShow(p);
}

static void StretchPlane(ati4200* p, int SrcOffset, int SrcPitch, rect* SrcRect, int DstOffset, int DstPitch, rect* DstRect)
{
	int DWidth,DHeight;
	int RScaleX = min(0x1FF,max(0x10,(SrcRect->Width << 8) / DstRect->Width));
	int RScaleY = min(0x1FF,max(0x10,(SrcRect->Height << 8) / DstRect->Height));

	DWidth = ((SrcRect->Width << 8)+RScaleX-1) / RScaleX;
	DHeight = ((SrcRect->Height << 8)+RScaleY-1) / RScaleY;

	RingReg(p,DP_GUI_MASTER_CNTL, 0xD598B2DF);
	RingReg(p,DP_CNTL			, 0x0000003F);
	RingReg(p,DP_MIX			, 0x01CC1200);
	RingReg(p,DP_DATATYPE		, 0x40037D02);
	RingReg(p,E2_ARITHMETIC_CNTL, 0x02020008);

	RingReg(p,SC_TOP_LEFT,(DstRect->x & 0xFFFF) | (DstRect->y << 16));
	RingReg(p,SC_BOTTOM_RIGHT,((DstRect->x + DstRect->Width) & 0xFFFF) | ((DstRect->y + DstRect->Height) << 16));
	RingReg(p,DST_OFFSET,DstOffset);
	RingReg(p,DST_PITCH,DstPitch);
	RingReg(p,SRC_OFFSET,SrcOffset);
	RingReg(p,SRC_PITCH,SrcPitch);

	RingReg(p,SRC2_OFFSET,SrcOffset);
	RingReg(p,SRC2_PITCH,SrcPitch);

	RingReg(p,SRC_WIDTH,DWidth+1);
	RingReg(p,SRC_HEIGHT,DHeight);
	RingReg(p,SRC_INC,(RScaleX & 0xFFFF) | (RScaleY << 16));

	RingReg(p,SRC2_WIDTH,DWidth+1);
	RingReg(p,SRC2_HEIGHT,DHeight);
	RingReg(p,SRC2_INC,(RScaleX & 0xFFFF) | (RScaleY << 16));

	RingReg(p,SRC_X_Y,((SrcRect->y*4) & 0xFFFF) + ((SrcRect->x*4) << 16));
	RingReg(p,SRC2_X_Y,((SrcRect->y*4 + 4) & 0xFFFF) + ((SrcRect->x*4) << 16));
	RingReg(p,DST_X_Y,(DstRect->y & 0xFFFF) | (DstRect->x << 16));
	RingReg(p,DST_WIDTH_HEIGHT,(DHeight & 0xFFFF) | (DWidth << 16));
}

static void Stretch(ati4200* p,bool_t Flip)
{
	rect SrcRect2;
	rect DstRect2;
	rect* SrcRect = &p->OverlayRect;
	rect DstRect;
	atisurf* Dst;
	atisurf* Src = p->Buffer+p->ShowCurr;

	if (++p->ShowScale >= SCALE_COUNT)
		p->ShowScale = 0;
	Dst = p->Scale + p->ShowScale;

	DstRect.x = 0;
	DstRect.y = 0;
	DstRect.Width = p->p.DstAlignedRect.Width;
	DstRect.Height = p->p.DstAlignedRect.Height;

	SrcRect2.x = SrcRect->x >> 1;
	SrcRect2.y = SrcRect->y >> 1;
	SrcRect2.Width = SrcRect->Width >> 1;
	SrcRect2.Height = SrcRect->Height >> 1;

	DstRect2.x = DstRect.x >> 1;
	DstRect2.y = DstRect.y >> 1;
	DstRect2.Width = DstRect.Width >> 1;
	DstRect2.Height = DstRect.Height >> 1;

	if (DstRect2.Width && DstRect2.Height)
	{
		bool_t PossibleBug = SrcRect2.Width*7 > DstRect2.Width*10;

		RingBegin(p,1);

		RingPrepair(p,128*2); // just an approx.

		StretchPlane(p,Src->UOffset,Src->UPitch,&SrcRect2,Dst->UOffset,Dst->UPitch,&DstRect2);
		if (PossibleBug)
		{
			RingFlush(p);
			Wait(p);
		}

		StretchPlane(p,Src->VOffset,Src->VPitch,&SrcRect2,Dst->VOffset,Dst->VPitch,&DstRect2);
		if (PossibleBug)
		{
			RingFlush(p);
			Wait(p);
		}
		
		StretchPlane(p,Src->YOffset,Src->YPitch,SrcRect,Dst->YOffset,Dst->YPitch,&DstRect);
		if ((p->Mirror & DIR_MIRRORUPDOWN) || !Flip)
		{
			RingFlush(p);
			Wait(p);
		}

		if (Flip)
		{
			RingReg(p,DISP_BUF,0x78);
			RingReg(p,VIDEO_Y_OFFSET,Dst->YOffset+MirrorOffset(p,Dst,0));
			RingReg(p,VIDEO_Y_PITCH,Dst->YPitch);
			RingReg(p,VIDEO_U_OFFSET,Dst->UOffset+MirrorOffset(p,Dst,1));
			RingReg(p,VIDEO_U_PITCH,Dst->UPitch);
			RingReg(p,VIDEO_V_OFFSET,Dst->VOffset+MirrorOffset(p,Dst,2));
			RingReg(p,VIDEO_V_PITCH,Dst->VPitch);
			RingReg(p,DISP_BUF,0x7B);
			RingFlush(p);
		}

		RingEnd(p);
	}
}

static void SetOverlay(ati4200* p)
{
	atisurf* s = p->Buffer + p->ShowCurr;
	BeginUpdate(p);
	p->Regs[VIDEO_Y_OFFSET] = s->YOffset+MirrorOffset(p,s,0);
	p->Regs[VIDEO_Y_PITCH] = s->YPitch;
	p->Regs[VIDEO_U_OFFSET] = s->UOffset+MirrorOffset(p,s,1);
	p->Regs[VIDEO_U_PITCH] = s->UPitch;
	p->Regs[VIDEO_V_OFFSET] = s->VOffset+MirrorOffset(p,s,2);
	p->Regs[VIDEO_V_PITCH] = s->VPitch;
	EndUpdate(p);
}

static int Blit(ati4200* p, const constplanes Data, const constplanes DataLast )
{
	if (++p->ShowCurr>=BUFFER_COUNT) 
		p->ShowCurr=0; 

	BlitImage(p->p.Soft,p->Buffer[p->ShowCurr].Planes,Data,DataLast,-1,-1);

	if (p->ShowScale<0)
		SetOverlay(p);
	else
		Stretch(p,1);
	return ERR_NONE;
}

//---------------------------------------------------------------------------------------------

static const uint8_t ScanTable[3][64] = {
{
	 0,  1,  8, 16,  9,  2,  3, 10, 
	17,	24, 32, 25, 18, 11,  4,  5,
	12, 19, 26, 33, 40, 48, 41, 34, 
	27,	20, 13,  6,  7, 14, 21, 28, 
	35, 42,	49, 56, 57, 50, 43, 36, 
	29, 22,	15, 23, 30, 37, 44, 51, 
	58, 59, 52, 45, 38, 31, 39, 46, 
	53, 60, 61,	54, 47, 55, 62, 63
},
{
 	 0,  1,  2,  3,  8,  9, 16, 17, 
	10, 11,  4,  5,  6,  7, 15, 14,
	13, 12, 19, 18, 24, 25, 32, 33, 
	26, 27, 20, 21, 22, 23, 28, 29,
	30, 31, 34, 35, 40, 41, 48, 49, 
	42, 43, 36, 37, 38, 39, 44, 45,
	46, 47, 50, 51, 56, 57, 58, 59, 
	52, 53, 54, 55, 60, 61, 62, 63
},
{
	 0,  8, 16, 24,  1,  9,  2, 10, 
	17, 25, 32, 40, 48, 56, 57, 49,
	41, 33, 26, 18,  3, 11,  4, 12, 
	19, 27, 34, 42, 50, 58, 35, 43,
	51, 59, 20, 28,  5, 13,  6, 14, 
	21, 29, 36, 44, 52, 60, 37, 45,
	53, 61, 22, 30,  7, 15, 23, 31, 
	38, 46, 54, 62, 39, 47, 55, 63
}};

static const uint8_t ZigZagSwap[64] = {
	 0,  8,  1,  2,  9, 16, 24, 17, 
	10,	 3,  4, 11, 18, 25, 32, 40,
	33, 26, 19, 12,  5,  6, 13, 20, 
	27,	34, 41, 48, 56, 49, 42, 35, 
	28, 21,	14,  7, 15, 22, 29, 36, 
	43, 50,	57, 58, 51, 44, 37, 30, 
	23, 31, 38, 45, 52, 59, 60, 53, 
	46, 39, 47,	54, 61, 62, 55, 63
};

static const uint8_t ZigZagSwapLength[64] = {
	1,   3,  3,  6,  6,  6, 10,	10,	
	10, 10, 15, 15, 15, 15, 15,	21, 
	21, 21, 21, 21, 21, 28, 28, 28,	
	28, 28, 28, 28, 36, 36, 36, 36, 
	36,	36, 36, 36, 43, 43, 43, 43, 
	43,	43, 43, 49, 49, 49, 49, 49,
	49, 54, 54, 54, 54, 54, 58, 58, 
	58, 58,	61, 61, 61, 63, 63, 64
};

#define AHI(p) ((ati4200*)((char*)(p)-(int)&(((ati4200*)0)->IdctVMT)))

// input: Block, Length
#define AHIDCBLOCK(Offset)					\
{											\
	if (Length < 1)							\
	{										\
		Block[0] = (idct_block_t)(Offset);	\
		Length = 1;							\
	}										\
	else									\
		Block[0] = (idct_block_t)(Block[0] + (Offset));\
}

// ADJUST: don't care about MVFwd (only in b-frames which are not accumulated)
#define AHIMVADJBLOCK(SubNo)														\
	if (AHI(p)->MVBack && (AHI(p)->MVBackTab[SubNo-1] & 0x00020002)==0x00020002)	\
		AHIDCBLOCK(2);

#define AHIPUSH(v)											\
{															\
	Dst[DstPos] = v;										\
	DstPos = (DstPos+1) & DstMask;							\
}

// Block, Length, Scan, ScanEnd
#define AHIPACK(ScanType,Inter,SkipLabel,Backup)			\
	ScanEnd = Scan + Length;								\
															\
	Last = Scan;											\
	while (Scan != ScanEnd && !Block[*Scan]) ++Scan;		\
	if (Scan == ScanEnd)									\
	{														\
		if (!Backup && (!Inter || AHI(p)->InterIDCT))		\
			AHIPUSH(0);										\
	}														\
	else													\
	{														\
		int v;												\
		if (!Backup)										\
		{													\
			if (Inter && !AHI(p)->InterIDCT)				\
			{												\
				AHI(p)->InterIDCT = 1;						\
				for (v=SubNo;v>1;--v)						\
					AHIPUSH(0);								\
			}												\
															\
			if (AHI(p)->LastScanType != ScanType)			\
			{												\
				AHI(p)->LastScanType = ScanType;			\
				AHIPUSH(7);									\
				v = ScanType?(ScanType==2 ? 0x10: 0x800):0;	\
				AHIPUSH(v);									\
			}												\
			else											\
				AHIPUSH(2);									\
		}													\
															\
		LenPtr = Dst+DstPos;								\
		DstPos = (DstPos+1) & DstMask;						\
															\
		for (Length=0;;)									\
		{													\
			Data.v[0] = -1;									\
															\
			v = *Scan;										\
			Data.d.skip0 = (char)(Scan++ - Last);			\
			Data.d.data0 = Block[v];						\
															\
			Last = Scan;									\
			while (Scan != ScanEnd && !Block[*Scan]) ++Scan;\
			if (Scan == ScanEnd) break;						\
			v = *Scan;										\
			Data.d.skip1 = (char)(Scan++ - Last);			\
			Data.d.data1 = Block[v];						\
															\
			Last = Scan;									\
			while (Scan != ScanEnd && !Block[*Scan]) ++Scan;\
			if (Scan == ScanEnd) break;						\
			v = *Scan;										\
			Data.d.skip2 = (char)(Scan++ - Last);			\
			Data.d.data2 = Block[v];						\
															\
			Last = Scan;									\
			while (Scan != ScanEnd && !Block[*Scan]) ++Scan;\
			if (Scan == ScanEnd) break;						\
			v = *Scan;										\
			Data.d.skip3 = (char)(Scan++ - Last);			\
			Data.d.data3 = Block[v];						\
															\
			AHIPUSH(Data.v[0]);								\
			AHIPUSH(Data.v[1]);								\
			AHIPUSH(Data.v[2]);								\
			++Length;										\
															\
			Last = Scan;									\
			while (Scan != ScanEnd && !Block[*Scan]) ++Scan;\
			if (Scan == ScanEnd)							\
			{												\
				AHIPUSH(0);									\
				goto SkipLabel;								\
			}												\
		}													\
															\
		AHIPUSH(Data.v[0]);									\
		AHIPUSH(Data.v[1]);									\
		AHIPUSH(Data.v[2]);									\
		++Length;											\
															\
		SkipLabel:											\
		*LenPtr = Length;									\
	}														

#define AHIPACKNORMAL(ScanType,Inter)						\
{															\
	const unsigned char *Scan = ScanTable[ScanType];		\
	const unsigned char *ScanEnd;							\
	const unsigned char *Last;								\
	int* Dst = (int*)AHI(p)->Ring;							\
	int DstPos = AHI(p)->RingPos;							\
	int DstMask = AHI(p)->RingMask;							\
	int* LenPtr;											\
	idct_ati Data;											\
	AHIPACK(ScanType,Inter,SkipLabel,0)						\
	AHI(p)->RingPos = DstPos;								\
}

#define AHIPACKSWAP(ScanType,Inter,SkipLabel,_Dst,_DstPos,_DstMask,Backup) \
{															\
	const unsigned char *Scan;								\
	const unsigned char *ScanEnd;							\
	const unsigned char *Last;								\
	int* LenPtr;											\
	int* Dst = (int*)_Dst;									\
	int DstPos = _DstPos;									\
	int DstMask = _DstMask;									\
	idct_ati Data;											\
	if (ScanType == IDCTSCAN_ZIGZAG)						\
	{														\
		Scan = ZigZagSwap;									\
		Length = ZigZagSwapLength[Length-1];				\
	}														\
	else													\
	{														\
		Scan = ScanTable[ScanType];							\
		ScanType = 3 - ScanType;							\
	}														\
	AHIPACK(ScanType,Inter,SkipLabel,Backup)				\
	_DstPos = DstPos;										\
}

#define AHIPACKSWAPZIGZAG(Inter,SkipLabel,_Dst,_DstPos,_DstMask,Backup)	\
{															\
	const unsigned char *Scan;								\
	const unsigned char *ScanEnd;							\
	const unsigned char *Last;								\
	int* LenPtr;											\
	int* Dst = (int*)_Dst;									\
	int DstPos = _DstPos;									\
	int DstMask = _DstMask;									\
	idct_ati Data;											\
	Scan = ZigZagSwap;										\
	Length = ZigZagSwapLength[Length-1];					\
	AHIPACK(IDCTSCAN_ZIGZAG,Inter,SkipLabel,Backup)			\
	_DstPos = DstPos;										\
}

#define AHIQPEL(MVPtr)			\
		MV = *(MVPtr++);		\
		MV <<= 1;				\
		MV &= ~0x00010000;		

#define AHIQPELSWAP(MVPtr)		\
		MV = *(MVPtr++);		\
		MV = (MV << 17) |		\
		 ((uint32_t)MV >> 15);	\
		MV &= ~0x00010001;

static void IDCTMComp(void* p,const int* MVBack,const int* MVFwd)
{
	int32_t* Tab;
	int MV;

	AHI(p)->MVBack = 0;
	AHI(p)->MVFwd = 0;

	if (MVBack)
	{
		AHI(p)->MVBack = 1;
		Tab = AHI(p)->MVBackTab;
		AHIQPELSWAP(MVBack); Tab[0] = MV;
		AHIQPELSWAP(MVBack); Tab[1] = MV;
		AHIQPELSWAP(MVBack); Tab[2] = MV;
		AHIQPELSWAP(MVBack); Tab[3] = MV;
		AHIQPELSWAP(MVBack); Tab[4] = MV;
		AHIQPELSWAP(MVBack); Tab[5] = MV;
	}

	if (MVFwd)
	{
		AHI(p)->MVFwd = 1;
		Tab = AHI(p)->MVFwdTab;
		AHIQPELSWAP(MVFwd); Tab[0] = MV;
		AHIQPELSWAP(MVFwd); Tab[1] = MV;
		AHIQPELSWAP(MVFwd); Tab[2] = MV;
		AHIQPELSWAP(MVFwd); Tab[3] = MV;
		AHIQPELSWAP(MVFwd); Tab[4] = MV;
		AHIQPELSWAP(MVFwd); Tab[5] = MV;
	}
}

static void IDCTMCompSwap(void* p,const int* MVBack,const int* MVFwd)
{
	int32_t* Tab;
	int MV;

	AHI(p)->MVBack = 0;
	AHI(p)->MVFwd = 0;

	if (MVBack)
	{
		AHI(p)->MVBack = 1;
		Tab = AHI(p)->MVBackTab;
		AHIQPEL(MVBack); Tab[0] = MV;
		AHIQPEL(MVBack); Tab[2] = MV;
		AHIQPEL(MVBack); Tab[1] = MV;
		AHIQPEL(MVBack); Tab[3] = MV;
		AHIQPEL(MVBack); Tab[4] = MV;
		AHIQPEL(MVBack); Tab[5] = MV;
	}

	if (MVFwd)
	{
		AHI(p)->MVFwd = 1;
		Tab = AHI(p)->MVFwdTab;
		AHIQPEL(MVFwd); Tab[0] = MV;
		AHIQPEL(MVFwd); Tab[2] = MV;
		AHIQPEL(MVFwd); Tab[1] = MV;
		AHIQPEL(MVFwd); Tab[3] = MV;
		AHIQPEL(MVFwd); Tab[4] = MV;
		AHIQPEL(MVFwd); Tab[5] = MV;
	}
}

static void IDCTProcess(void* p,int x,int y)
{
	AHI(p)->SubNo = 0;
	AHI(p)->MacroCoord = (y << 4) | (x << 20);
}

static void IDCTProcessSwap(void* p,int x,int y)
{
	AHI(p)->SubNo = 0;
	AHI(p)->MacroCoord = (x << 4) | (y << 20);
}

static NOINLINE void IDCTIntraStart(ati4200* p)
{
	RingPrepair(p,512);

	if (p->LastMode != 0x0F)
	{
		p->LastMode = 0x0F;
		RingPush(p,0x4A1);
	}
	else
		RingPush(p,0xC0001000);
	RingPush(p,0);

	p->MacroPos = p->RingPos;
	p->RingPos = (p->RingPos+1) & p->RingMask;
}

static NOINLINE void IDCTIntraEnd(ati4200* p)
{
	atisurf* Dst = p->Dst;
	int Pos;

	RingPush(p,5);
	RingPush(p,0xF|p->ArithMode); 

	RingPush(p,0x2D000003 | Dst->YOffset);

	Pos = p->MacroCoord;
	RingPush(p,Pos);
	RingPush(p,Pos*4);
	Pos += 8 << 16;
	RingPush(p,Pos);
	RingPush(p,Pos*4);
	Pos += 8 - (8 << 16);
	RingPush(p,Pos);
	RingPush(p,Pos*4);
	Pos += 8 << 16;
	RingPush(p,Pos);
	RingPush(p,Pos*4);
	Pos -= 8 + (8 << 16);
	Pos >>= 1;
	RingPush(p,Pos);
	RingPush(p,Pos*4);
	RingPush(p,0x2C000003 | Dst->UOffset);
	RingPush(p,Pos);
	RingPush(p,Pos*4);
	RingPush(p,0x2C000003 | Dst->VOffset);

	p->Ring[p->MacroPos] = 0xC0003200 | (((p->RingPos - (p->MacroPos+2)) & p->RingMask) << 16);

	RingFlush(p);
}

static INLINE void IDCTInterStart(ati4200* p)
{
	RingPrepair(p,512);

	p->MacroPos = p->RingPos;
	p->RingPos = (p->RingPos+3) & p->RingMask;
	p->InterIDCT = 0;
}

static INLINE void PushMV(ati4200* p,int Pos,int n)
{
	if (p->MVBack)
		RingPush(p,(p->MVBackTab[n] + Pos) & ~0x00010000);

	if (p->MVFwd)
		RingPush(p,(p->MVFwdTab[n] + Pos) & ~0x00010000);
}

static NOINLINE void IDCTInterEnd(ati4200* p)
{
	atisurf* Dst = p->Dst;
	int Mode,Pos;

	assert(p->MVBack || p->MVFwd);

	if (p->MVBack && p->MVFwd)
	{
		Mode = 0x0E;
		RingPush(p,6);
	}
	else
	{
		Mode = 0x0D;
		RingPush(p,5);
	}

	if (!p->InterIDCT)
		Mode -= 2;

	RingPush(p,Mode|p->ArithMode);

	RingPush(p,0x2D000000 | Dst->YOffset | (p->MVBack?0:3));

	Pos = p->MacroCoord;
	RingPush(p,Pos);
	PushMV(p,Pos*4,0);

	Pos += 8 << 16;
	RingPush(p,Pos);
	PushMV(p,Pos*4,1);

	Pos += 8 - (8 << 16);
	RingPush(p,Pos);
	PushMV(p,Pos*4,2);

	Pos += 8 << 16;
	RingPush(p,Pos);
	PushMV(p,Pos*4,3);

	Pos -= 8 + (8 << 16);
	Pos >>= 1;
	RingPush(p,Pos);
	PushMV(p,Pos*4,4);

	RingPush(p,0x20000000 | (4<<26) | Dst->UOffset | (p->MVBack?1:4));
	
	RingPush(p,Pos);
	PushMV(p,Pos*4,5);

	RingPush(p,0x20000000 | (5<<26) | Dst->VOffset | (p->MVBack?2:5));

	Pos = p->MacroPos;
	if (p->LastMode != Mode)
	{
		p->LastMode = Mode;
		p->Ring[Pos] = 0x4A1;
	}
	else
		p->Ring[Pos] = 0xC0001000;
	Pos = (Pos+1) & p->RingMask;
	p->Ring[Pos] = 0;
	Pos = (Pos+1) & p->RingMask;
	p->Ring[Pos] = 0xC0003200 | (((p->RingPos - (Pos+2)) & p->RingMask) << 16);

	RingFlush(p);
}

void IDCTIntra(void* p,idct_block_t *Block,int Length,int ScanType)
{
	int SubNo = ++AHI(p)->SubNo;
	if (SubNo <= 4)
		AHIDCBLOCK(AHI(p)->DCAdd)

	if (SubNo == 1)
		IDCTIntraStart(AHI(p));
	
	if (Length>0 && Length<=64)
		AHIPACKNORMAL(ScanType,0)
	else
		RingPush(AHI(p),0);

	if (SubNo == 6)
		IDCTIntraEnd(AHI(p));
}

void IDCTInter(void* p,idct_block_t *Block,int Length)
{
	int SubNo = ++AHI(p)->SubNo;

	AHIMVADJBLOCK(SubNo)

	if (SubNo == 1)
		IDCTInterStart(AHI(p));

	if (Length>0 && Length<=64)
		AHIPACKNORMAL(IDCTSCAN_ZIGZAG,1)
	else
	if (AHI(p)->InterIDCT)
		RingPush(AHI(p),0);

	if (SubNo == 6)
		IDCTInterEnd(AHI(p));
}

void IDCTIntraSwap(void* p,idct_block_t *Block,int Length,int ScanType)
{
	int SubNo = ++AHI(p)->SubNo;
	if (SubNo <= 4)
		AHIDCBLOCK(AHI(p)->DCAdd)

	if (SubNo == 1)
		IDCTIntraStart(AHI(p));
	
	if (SubNo == 2)	
	{				
		AHI(p)->SavePos = 0;
		if (Length>0 && Length<=64)
		{
			AHIPACKSWAP(ScanType,0,SkipLabel1,AHI(p)->SaveData,AHI(p)->SavePos,-1,1)
			AHI(p)->SaveScanType = ScanType;
		}
	}												
	else											
	{				
		if (Length>0 && Length<=64)					
			AHIPACKSWAP(ScanType,0,SkipLabel2,AHI(p)->Ring,AHI(p)->RingPos,AHI(p)->RingMask,0)
		else
			RingPush(AHI(p),0);
			
		if (SubNo == 3)							
		{
			int j,i = AHI(p)->RingPos;
			if (AHI(p)->SavePos)
			{
				j = AHI(p)->SaveScanType;
				if (AHI(p)->LastScanType != j)
				{
					AHI(p)->LastScanType = j;
					AHI(p)->Ring[i] = 7;
					i = (i+1) & AHI(p)->RingMask;
					if (j==2) j=0x10;
					else if (j==1) j=0x800;
					AHI(p)->Ring[i] = j;
					i = (i+1) & AHI(p)->RingMask;
				}
				else
				{
					AHI(p)->Ring[i] = 2;
					i = (i+1) & AHI(p)->RingMask;
				}

				for (j=0;j<AHI(p)->SavePos;++j)
				{
					AHI(p)->Ring[i] = AHI(p)->SaveData[j];
					i = (i+1) & AHI(p)->RingMask;
				}
			}
			else
			{
				AHI(p)->Ring[i] = 0;
				i = (i+1) & AHI(p)->RingMask;
			}
			AHI(p)->RingPos = i;
		}
		else
		if (SubNo == 6)
			IDCTIntraEnd(AHI(p));
	}
}

void IDCTInterSwap(void* p,idct_block_t *Block,int Length)
{
	int SubNo = ++AHI(p)->SubNo;

	AHIMVADJBLOCK(SubNo)

	if (SubNo == 1)
		IDCTInterStart(AHI(p));

	if (SubNo == 2)				
	{							
		AHI(p)->SavePos = 0;
		if (Length>0 && Length<=64)	
			AHIPACKSWAPZIGZAG(1,SkipLabel1,AHI(p)->SaveData,AHI(p)->SavePos,-1,1)
	}										
	else									
	if (SubNo == 3)							
	{
		if (Length>0 && Length<=64)			
		{
			--SubNo;
			AHIPACKSWAPZIGZAG(1,SkipLabel2,AHI(p)->Ring,AHI(p)->RingPos,AHI(p)->RingMask,0)
		}
		else
		if (AHI(p)->InterIDCT)
			RingPush(AHI(p),0);

		if (AHI(p)->SavePos>0 || AHI(p)->InterIDCT)
		{
			int j,i = AHI(p)->RingPos;

			if (!AHI(p)->InterIDCT)
			{					
				AHI(p)->InterIDCT = 1;
				AHI(p)->Ring[i] = 0; 
				i = (i+1) & AHI(p)->RingMask;
				AHI(p)->Ring[i] = 0; 
				i = (i+1) & AHI(p)->RingMask;
			}

			if (AHI(p)->SavePos>0)
			{
				if (AHI(p)->LastScanType != IDCTSCAN_ZIGZAG)
				{
					AHI(p)->LastScanType = IDCTSCAN_ZIGZAG;
					AHI(p)->Ring[i] = 7;
					i = (i+1) & AHI(p)->RingMask;
					AHI(p)->Ring[i] = 0;
					i = (i+1) & AHI(p)->RingMask;
				}
				else
				{
					AHI(p)->Ring[i] = 2;
					i = (i+1) & AHI(p)->RingMask;
				}

				for (j=0;j<AHI(p)->SavePos;++j)
				{
					AHI(p)->Ring[i] = AHI(p)->SaveData[j];
					i = (i+1) & AHI(p)->RingMask;
				}
			}
			else
			{
				AHI(p)->Ring[i] = 0;
				i = (i+1) & AHI(p)->RingMask;
			}
			AHI(p)->RingPos = i;
		}
	}				
	else
	{
		if (Length>0 && Length<=64)			
			AHIPACKSWAPZIGZAG(1,SkipLabel3,AHI(p)->Ring,AHI(p)->RingPos,AHI(p)->RingMask,0)
		else
		if (AHI(p)->InterIDCT)
			RingPush(AHI(p),0);

		if (SubNo == 6)
			IDCTInterEnd(AHI(p));
	}
}

static void IDCTCopy16x16(void* p,int x,int y,int Forward)
{
	int32_t* Tab;
	AHI(p)->MacroCoord = (y << 4) | (x << 20);

	if (Forward)
	{
		AHI(p)->MVBack = 0;
		AHI(p)->MVFwd = 1;
		Tab = AHI(p)->MVFwdTab;
	}
	else
	{
		AHI(p)->MVBack = 1;
		AHI(p)->MVFwd = 0;
		Tab = AHI(p)->MVBackTab;
	}

	Tab[0] = 0;
	Tab[1] = 0;
	Tab[2] = 0;
	Tab[3] = 0;
	Tab[4] = 0;
	Tab[5] = 0;

	IDCTInterStart(AHI(p));
	IDCTInterEnd(AHI(p));
}

static void IDCTCopy16x16Swap(void* p,int x,int y,int Forward)
{
	int32_t* Tab;
	AHI(p)->MacroCoord = (x << 4) | (y << 20);
	AHI(p)->InterIDCT = 0;

	if (Forward)
	{
		AHI(p)->MVBack = 0;
		AHI(p)->MVFwd = 1;
		Tab = AHI(p)->MVFwdTab;
	}
	else
	{
		AHI(p)->MVBack = 1;
		AHI(p)->MVFwd = 0;
		Tab = AHI(p)->MVBackTab;
	}

	Tab[0] = 0;
	Tab[1] = 0;
	Tab[2] = 0;
	Tab[3] = 0;
	Tab[4] = 0;
	Tab[5] = 0;
	
	IDCTInterStart(AHI(p));
	IDCTInterEnd(AHI(p));
}

static int IDCTUpdateFunc(ati4200* p)
{
	if (p->Overlay.Direction)
	{
		p->IdctVMT.Copy16x16 = IDCTCopy16x16Swap;
		p->IdctVMT.Process = IDCTProcessSwap;
		p->IdctVMT.MComp8x8 = IDCTMCompSwap;
		p->IdctVMT.MComp16x16 = IDCTMCompSwap;
		p->IdctVMT.Intra8x8 = (idctintra)IDCTIntraSwap;
		p->IdctVMT.Inter8x8 = (idctinter)IDCTInterSwap;
	}
	else 
	{
		p->IdctVMT.Copy16x16 = IDCTCopy16x16;
		p->IdctVMT.Process = IDCTProcess;
		p->IdctVMT.MComp8x8 = IDCTMComp;
		p->IdctVMT.MComp16x16 = IDCTMComp;
		p->IdctVMT.Intra8x8 = (idctintra)IDCTIntra;
		p->IdctVMT.Inter8x8 = (idctinter)IDCTInter;
	}
	return ERR_NONE;
}

static int IDCTOutputFormat(ati4200* p, const void* Data, int Size)
{
	packetformat Format;
	int Result;
	p->IDCTInit = 1;

	if (Size == sizeof(video))
	{
		memset(&Format,0,sizeof(Format));
		Format.Type = PACKET_VIDEO;
		Format.Format.Video = *(const video*)Data;
		Data = &Format;
		Size = sizeof(packetformat);

		//IDCTWidth or IDCTHeight could have changes so invalidate current input format
		memset(&p->p.Input.Format.Video,0,sizeof(video));
	}

	Result = p->p.Node.Set((node*)p,OUT_INPUT|PIN_FORMAT,Data,Size);

	if (Size && Result != ERR_NONE)
		p->p.Node.Set((node*)p,OUT_INPUT|PIN_FORMAT,NULL,0);

	p->IDCTInit = 0;
	return Result;
}

static int IDCTSend(void* p,tick_t RefTime,const flowstate* State);

static int IDCTSet(void* p, int No, const void* Data, int Size)
{
	flowstate State;
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case IDCT_MODE: if (*(int*)Data==0) Result = ERR_NONE; break;
	case IDCT_BUFFERWIDTH: SETVALUE(AHI(p)->IDCTWidth,int,ERR_NONE); break;
	case IDCT_BUFFERHEIGHT: SETVALUE(AHI(p)->IDCTHeight,int,ERR_NONE); break;
	case IDCT_FORMAT:
		assert(Size == sizeof(video) || !Data);
		Result = IDCTOutputFormat(AHI(p),Data,Size);
		break;

	case IDCT_BACKUP:
		if (Size == sizeof(idctbackup))
			Result = IDCTRestore(p,(idctbackup*)Data);
		break;

	case IDCT_ROUNDING: SETVALUE(AHI(p)->Rounding,bool_t,ERR_NONE); break;

	case IDCT_SHOW: 
		SETVALUE(AHI(p)->ShowNext,int,ERR_NONE); 
		if (AHI(p)->ShowNext >= AHI(p)->BufCount) AHI(p)->ShowNext = -1;
		break;

	case IDCT_BUFFERCOUNT:
		assert(Size == sizeof(int));
		if (*(const int*)Data > ALL_COUNT)
			Result = ERR_OUT_OF_MEMORY;
		else
		{	
			AHI(p)->BufCount = *(const int*)Data;
			Result = ERR_NONE;
		}
		break;

	case FLOW_FLUSH:
		AHI(p)->ShowNext = -1;
		Result = ERR_NONE;
		break;

	case FLOW_RESEND:
		State.CurrTime = TIME_RESEND;
		State.DropLevel = 0;
		Result = IDCTSend(p,-1,&State);
		break;
	}
	if (No>=IDCT_FRAMENO && No<IDCT_FRAMENO+AHI(p)->AllCount)
		SETVALUE(AHI(p)->BufFrameNo[No-IDCT_FRAMENO],int,ERR_NONE);
	return Result;
}

static int IDCTEnumAHI(void* p, int* No, datadef* Param)
{
	int Result = IDCTEnum(p,No,Param);
	if (Result == ERR_NONE && Param->No == IDCT_OUTPUT)
		Param->Flags |= DF_RDONLY;
	return Result;
}

static int IDCTGet(void* p, int No, void* Data, int Size)
{
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case FLOW_BUFFERED: GETVALUE(1,bool_t); break;
	case NODE_PARTOF: GETVALUE(AHI(p),ati4200*); break;
	case IDCT_OUTPUT|PIN_FORMAT: GETVALUECOND(AHI(p)->p.Input,packetformat,AHI(p)->IDCT); break;
	case IDCT_OUTPUT|PIN_PROCESS: GETVALUE(NULL,packetprocess); break;
	case IDCT_OUTPUT: GETVALUE(AHI(p)->IDCTOutput,pin); break;
	case IDCT_FORMAT: 
		assert(Size==sizeof(video));
		if (AHI(p)->IDCT)
		{
			video* i = (video*)Data;
			*i = AHI(p)->p.Input.Format.Video;
			if ((AHI(p)->Overlay.Direction ^ i->Direction) & DIR_SWAPXY)
			{
				i->Direction ^= DIR_SWAPXY;
				SwapInt(&i->Width,&i->Height);
			}
			Result = ERR_NONE;
		}
		break;
	case IDCT_MODE: GETVALUE(0,int); break;
	case IDCT_ROUNDING: GETVALUE(AHI(p)->Rounding,bool_t); break;
	case IDCT_BUFFERCOUNT: GETVALUE(AHI(p)->BufCount,int); break;
	case IDCT_BUFFERWIDTH: GETVALUE(AHI(p)->IDCTWidth,int); break;
	case IDCT_BUFFERHEIGHT: GETVALUE(AHI(p)->IDCTHeight,int); break;
	case IDCT_SHOW: GETVALUE(AHI(p)->ShowNext,int); break;
	case IDCT_BACKUP: 
		assert(Size == sizeof(idctbackup));
		Result = IDCTBackup(p,(idctbackup*)Data);
		break;
	}
	if (No>=IDCT_FRAMENO && No<IDCT_FRAMENO+AHI(p)->AllCount)
		GETVALUE(AHI(p)->BufFrameNo[No-IDCT_FRAMENO],int);
	return Result;
}

static int IDCTLock(void* p,int No,planes Planes,int* Brightness,video* Format)
{
	if (Brightness)
		*Brightness = AHI(p)->p.FX.Brightness;
	if (Format)
		*Format = AHI(p)->Overlay;

	if (No<0 || No>=AHI(p)->BufCount || No>=AHI(p)->AllCount)
	{
		Planes[0] = NULL;
		Planes[1] = NULL;
		Planes[2] = NULL;
		return ERR_INVALID_PARAM;
	}

	Wait(AHI(p));
	memcpy(Planes,AHI(p)->Buffer[No].Planes,sizeof(planes));
	return ERR_NONE;
}

static void IDCTUnlock(void* p,int No)
{
}

static int IDCTFrameStart(void* p,int FrameNo,int *OldFrameNo,int DstNo,int BackNo,int FwdNo,int ShowNo,bool_t Drop)
{
	if (!AHI(p)->IDCT)
		return ERR_DEVICE_ERROR;

	// we will try to avoid to overwrite currently displayed buffers:
	//   ShowCurr currently displayed
	//   ShowLast may be still on screen (because flip occurs only during vblank)

	if (AHI(p)->ShowScale<0 && (AHI(p)->ShowLast == DstNo || AHI(p)->ShowCurr == DstNo))
	{
		// try to find a free buffer
		int SwapNo;
		for (SwapNo=0;SwapNo<AHI(p)->AllCount;++SwapNo)
			if (SwapNo != AHI(p)->ShowLast && SwapNo != AHI(p)->ShowCurr && 
				SwapNo != BackNo && SwapNo != FwdNo && SwapNo != ShowNo)
				break; 

		if (SwapNo < AHI(p)->AllCount)
			SwapPtr(AHI(p),SwapNo,DstNo,1);
	}

	AHI(p)->ShowNext = ShowNo;

	if (OldFrameNo)
		*OldFrameNo = AHI(p)->BufFrameNo[DstNo];
	AHI(p)->BufFrameNo[DstNo] = FrameNo;

	AHI(p)->Dst = AHI(p)->Buffer+DstNo;

	RingBegin(AHI(p),0);

	RingPrepair(AHI(p),32);

	RingReg(AHI(p),DP_MIX,0x01CC1200);
	RingReg(AHI(p),DP_GUI_MASTER_CNTL,0xD598B2DF);
	RingReg(AHI(p),DP_DATATYPE,0x40037D02);
	RingReg(AHI(p),DST_PITCH,AHI(p)->Dst->UPitch);
	RingReg(AHI(p),SC_TOP_LEFT,0);
	RingReg(AHI(p),SC_BOTTOM_RIGHT,(AHI(p)->Overlay.Width & 0xFFFF) | (AHI(p)->Overlay.Height << 16));
	RingReg(AHI(p),SRC_SC_BOTTOM_RIGHT,(((AHI(p)->Overlay.Width>>1)-1) & 0xFFFF) | (((AHI(p)->Overlay.Height>>1)-1) << 16));

	if (BackNo >= 0)
	{
		atisurf* s = AHI(p)->Buffer+BackNo;
		RingReg(AHI(p),REF1_PITCH_OFFSET,(s->YPitch << 19) | (s->YOffset >> 4));
		RingReg(AHI(p),REF2_PITCH_OFFSET,(s->UPitch << 19) | (s->UOffset >> 4));
		RingReg(AHI(p),REF3_PITCH_OFFSET,(s->VPitch << 19) | (s->VOffset >> 4));
	}

	if (FwdNo >= 0)
	{
		atisurf* s = AHI(p)->Buffer+FwdNo;
		RingReg(AHI(p),REF4_PITCH_OFFSET,(s->YPitch << 19) | (s->YOffset >> 4));
		RingReg(AHI(p),REF5_PITCH_OFFSET,(s->UPitch << 19) | (s->UOffset >> 4));
		RingReg(AHI(p),REF6_PITCH_OFFSET,(s->VPitch << 19) | (s->VOffset >> 4));
	}

	AHI(p)->LastScanType = -1;
	AHI(p)->LastMode = -1;
	AHI(p)->ArithMode = (AHI(p)->Rounding?0x80000000:0)|0x200;

	RingFlush(AHI(p));

	return ERR_NONE;
}

static void IDCTFrameEnd(void* p)
{
	if (AHI(p)->IDCT)
	{
		Wait(AHI(p)); // register restoring would mess up running IDCT
		RingEnd(AHI(p));
	}
}

static void IDCTDrop(void* p)
{
	int No;
	for (No=0;No<AHI(p)->AllCount;++No)
		AHI(p)->BufFrameNo[No] = -1;
}

static int IDCTNull(void* p,const flowstate* State,bool_t Empty)
{
	if (Empty)
		++AHI(p)->p.Total;
	if (!State || State->DropLevel)
		++AHI(p)->p.Dropped;
	return ERR_NONE;
}

static int IDCTSend(void* p,tick_t RefTime,const flowstate* State)
{
	if (AHI(p)->ShowNext < 0)
		return ERR_NEED_MORE_DATA;

	if (State->CurrTime >= 0)
	{
		if (!AHI(p)->p.Play && State->CurrTime == AHI(p)->p.LastTime)
			return ERR_BUFFER_FULL;

		if (RefTime - (State->CurrTime + SHOWAHEAD) >= 0)
			return ERR_BUFFER_FULL;

		AHI(p)->p.LastTime = State->CurrTime;
	}
	else
		AHI(p)->p.LastTime = RefTime;

	if (State->CurrTime != TIME_RESEND)
		++AHI(p)->p.Total;

	AHI(p)->ShowLast = AHI(p)->ShowCurr;
	AHI(p)->ShowCurr = AHI(p)->ShowNext;

	if (AHI(p)->ShowScale>=0)
	{
		if (State->DropLevel)
			++AHI(p)->p.Dropped;
		else
			Stretch(AHI(p),1);
	}
	else
		SetOverlay(AHI(p));

	return ERR_NONE;
}

static const nodedef ATI4200_IDCT =
{
	CF_ABSTRACT,
	ATI4200_IDCT_ID,
	IDCT_CLASS,
	PRI_DEFAULT-10, // don't use acceleration by default...
};

//---------------------------------------------------------------------------------------------

#if defined(_M_IX86)
extern void* __stdcall VirtualAlloc(void*,int,int,int);
#define MEM_COMMIT        0x1000     
#define PAGE_READWRITE    0x04     
#endif

static int Create(ati4200* p)
{
	if (QueryPlatform(PLATFORM_MODEL)!=MODEL_ZODIAC)
		return ERR_NOT_SUPPORTED;

	p->p.Init = (ovlfunc)Init;
	p->p.Done = (ovldone)Done;
	p->p.Reset = (ovlfunc)Reset;
	p->p.Blit = (ovlblit)Blit;
	p->p.Update = (ovlfunc)Update;
	p->p.UpdateShow = (ovlfunc)UpdateShow;
	p->p.DoPowerOff = 1;

#if defined(_M_IX86)
#if !defined(ATIEMU)
	return ERR_NOT_SUPPORTED;
#else
	p->Device = VirtualAlloc(NULL,EXT_BASE+8*1024*1024,MEM_COMMIT,PAGE_READWRITE);
	if (!p->Device)
		return ERR_NOT_SUPPORTED;
	memset(p->Device,0,EXT_BASE+8*1024*1024);
	p->Regs = (int*)(p->Device + REG_BASE);
	p->Regs[RING_CNTL] = 0x0800000D;
	p->Regs[RING_BASE] = 0x00150000;
	p->Regs[ACTIVE_V_DISP] = 480 << 16;

#endif
#else
	p->Device = (uint8_t*)0x90000000;
	p->Regs = (int*)(p->Device + REG_BASE);
#endif

	p->Memory = EXT_BASE + 512*1024;

	p->RingMask = (2 << (p->Regs[RING_CNTL] & 0x3F))-1;
	p->Ring = (int*)(p->Device + p->Regs[RING_BASE]);

	p->IdctVMT.Class = ATI4200_IDCT_ID;
	p->IdctVMT.Enum = IDCTEnumAHI;
	p->IdctVMT.Get = IDCTGet;
	p->IdctVMT.Set = IDCTSet;
	p->IdctVMT.Send = IDCTSend;
	p->IdctVMT.Null = IDCTNull;
	p->IdctVMT.Drop = IDCTDrop;
	p->IdctVMT.Lock = IDCTLock;
	p->IdctVMT.Unlock = IDCTUnlock;
	p->IdctVMT.FrameStart = IDCTFrameStart;
	p->IdctVMT.FrameEnd = IDCTFrameEnd;

	p->IDCTOutput.Node = (node*)p;
	p->IDCTOutput.No = OUT_INPUT;

	p->p.AccelIDCT = &p->IdctVMT;
	NodeRegisterClass(&ATI4200_IDCT);

	GetMode(p);

	return ERR_NONE;
}

static const nodedef ATI4200 = 
{
	sizeof(ati4200)|CF_GLOBAL,
	ATI4200_ID,
	OVERLAY_CLASS,
	PRI_DEFAULT+50,
	(nodecreate)Create,
};

void ATI4200_Init() 
{
	NodeRegisterClass(&ATI4200);
}

void ATI4200_Done() 
{
	NodeUnRegisterClass(ATI4200_ID);
}

#else
void ATI4200_Init() {}
void ATI4200_Done() {}
#endif
