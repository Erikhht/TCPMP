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
 * $Id: softidct.c 543 2006-01-07 22:06:24Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/
 
#include "../common.h"
#include "softidct.h"

#ifndef MIPS64
#define MemCpy8 memcpy
#define MemSet8 memset
#else

static void MemCpy8(void* dst,void* src,int len)
{
	__asm(	"addi	$6,$6,-16;"
			"bltz	$6,cpyskip16;"

			"cpyloop16:"
			"ldr	$10,0($5);"
			"addi	$6,$6,-16;"
			"ldr	$11,8($5);"

#ifdef MIPSVR41XX
			".set noreorder;"
			"cache	13,0($4);" //cache without loading
			".set reorder;"
#endif
			"sdr	$10,0($4);"
			"addi	$5,$5,16;"
			"sdr	$11,8($4);"
			"addi	$4,$4,16;"
			"bgez	$6,cpyloop16;"
			"cpyskip16:"

			"andi	$6,$6,8;"
			"beq	$6,$0,cpyskip8;"
			"ldr	$10,0($5);"
			"sdr	$10,0($4);"
			"cpyskip8:");
}

static void MemSet8(void* dst,int v,int len) 
{
	__asm(	"dsll	$10,$5,56;"
			"dsrl	$11,$10,8;"
			"or		$10,$10,$11;"
			"dsrl	$11,$10,16;"
			"or		$10,$10,$11;"
			"dsrl	$11,$10,32;"
			"or		$10,$10,$11;"

			"addi	$6,$6,-16;"
			"bltz	$6,setskip16;"

			"setloop16:"

#ifdef MIPSVR41XX
			".set noreorder;"
			"cache	13,0($4);" //cache without loading
			".set reorder;"
#endif
			"sdr	$10,0($4);"
			"addi	$6,$6,-16;"
			"sdr	$10,8($4);"
			"addi	$4,$4,16;"
			"bgez	$6,setloop16;"
			"setskip16:"

			"andi	$6,$6,8;"
			"beq	$6,$0,setskip8;"
			"sdr	$10,0($4);"
			"setskip8:");
}
#endif

static void FillEdge(uint8_t *Ptr, int Width, int Height, int EdgeX, int EdgeY)
{
	int n;
	uint8_t *p;
	int InnerWidth = Width - EdgeX*2;
	int InnerHeight = Height - EdgeY*2;
		
	// left and right
	p = Ptr + EdgeX + EdgeY * Width;
	for (n=0;n<InnerHeight;++n)
	{
		MemSet8(p-EdgeX, p[0], EdgeX);
		MemSet8(p+InnerWidth,p[InnerWidth-1], EdgeX);
		p += Width;
	}

	// top
	p = Ptr;
	for (n=0;n<EdgeY;++n)
	{
		MemCpy8(p, Ptr+EdgeY * Width, Width);
		p += Width;
	}

	// bottom
	p = Ptr + (Height-EdgeY)*Width;
	for (n=0;n<EdgeY;++n)
	{
		MemCpy8(p, Ptr+(Height-EdgeY-1)*Width, Width);
		p += Width;
	}
}

static void FillEdgeYUV(softidct* p,uint8_t *Ptr)
{
	FillEdge(Ptr, p->BufWidth, p->BufHeight, EDGE, EDGE);
	Ptr += p->YToU;
	FillEdge(Ptr, p->BufWidth>>p->UVX2, p->BufHeight>>p->UVY2, EDGE>>p->UVX2, EDGE>>p->UVY2);
	Ptr += p->YToU >> (p->UVX2+p->UVY2);
	FillEdge(Ptr, p->BufWidth>>p->UVX2, p->BufHeight>>p->UVY2, EDGE>>p->UVX2, EDGE>>p->UVY2);
}

#if defined(MIPS64_WIN32) 
static int TouchPages(softidct* p)
{
	uint8_t* i;
	int Sum = 0;
	for (i=p->CodeBegin;i<p->CodeEnd;i+=p->CodePage)
		Sum += *i;
	return Sum;
}
#endif

static int Lock(softidct* p,int No,planes Planes,int* Brightness,video* Format)
{
	if (Brightness)
		*Brightness = 0;
	if (Format)
	{
		*Format = p->Out.Format.Format.Video;
		Format->Pitch = p->BufWidth;
		Format->Width = p->BufWidth - 2*EDGE;
		Format->Height = p->BufHeight - 2*EDGE;
	}

	if (No<0 || No>=p->BufCount)
	{
		Planes[0] = NULL;
		Planes[1] = NULL;
		Planes[2] = NULL;
		return ERR_INVALID_PARAM;
	}

	Planes[0] = p->Buffer[No] + EDGE + EDGE*p->BufWidth;
	Planes[1] = p->Buffer[No] + p->YToU + (EDGE >> p->UVX2) + (EDGE >> p->UVY2)*(p->BufWidth >> p->UVX2);
	Planes[2] = (uint8_t*)Planes[1] + (p->YToU >> (p->UVX2+p->UVY2));
	return ERR_NONE;
}

static void Unlock(softidct* p,int No)
{
}

#if defined(CONFIG_IDCT_LOWRES)

static const copyblock AllCopyBlock4x4[2][16] = //[Rounding][x][y]
{
   { CopyBlock4x4_00, CopyBlock4x4_01, CopyBlock4x4_02, CopyBlock4x4_03,
	 CopyBlock4x4_10, CopyBlock4x4_11, CopyBlock4x4_12, CopyBlock4x4_13,
	 CopyBlock4x4_20, CopyBlock4x4_21, CopyBlock4x4_22, CopyBlock4x4_23,
	 CopyBlock4x4_30, CopyBlock4x4_31, CopyBlock4x4_32, CopyBlock4x4_33 },

   { CopyBlock4x4_00, CopyBlock4x4_01R, CopyBlock4x4_02R, CopyBlock4x4_03R,
     CopyBlock4x4_10R, CopyBlock4x4_11R, CopyBlock4x4_12R, CopyBlock4x4_13R,
     CopyBlock4x4_20R, CopyBlock4x4_21R, CopyBlock4x4_22R, CopyBlock4x4_23R,
     CopyBlock4x4_30R, CopyBlock4x4_31R, CopyBlock4x4_32R, CopyBlock4x4_33R },
};

#ifndef MIPS32
const addblock TableAddBlock4x4[16] = 
{
	AddBlock4x4_00,AddBlock4x4_01,AddBlock4x4_02,AddBlock4x4_03,
	AddBlock4x4_10,AddBlock4x4_11,AddBlock4x4_12,AddBlock4x4_13,
	AddBlock4x4_20,AddBlock4x4_21,AddBlock4x4_22,AddBlock4x4_23,
	AddBlock4x4_30,AddBlock4x4_31,AddBlock4x4_32,AddBlock4x4_33,
};
#endif
#endif

static const copyblock AllCopyBlock[2][4] = //[Rounding][x][y]
{
	{ CopyBlock, CopyBlockHor, CopyBlockVer, CopyBlockHorVer },
	{ CopyBlock, CopyBlockHorRound, CopyBlockVerRound, CopyBlockHorVerRound }
};

#ifdef MIPS64
static const copyblock AllCopyBlockM[2][4] = //[Rounding][x][y]
{
	{ CopyMBlock, CopyMBlockHor, CopyMBlockVer, CopyMBlockHorVer },
	{ CopyMBlock, CopyMBlockHorRound, CopyMBlockVerRound, CopyMBlockHorVerRound }
};
#endif

static int UpdateRounding(softidct* p)
{
#if defined(CONFIG_IDCT_LOWRES)
	if (p->Shift)
		p->CopyBlock4x4 = AllCopyBlock4x4[p->Rounding];
	else
#endif
		memcpy(p->CopyBlock,p->AllCopyBlock[p->Rounding],sizeof(copyblock)*4);
#ifdef MIPS64
	p->CopyMBlock = AllCopyBlockM[p->Rounding]; // no shift, instead 16x16
#endif
	return ERR_NONE;
}

static const idctprocess Process[3] =   // pixelformat
{
	(idctprocess)Process420,
	(idctprocess)Process422,
	(idctprocess)Process444,
};

static const idctcopy Copy16x16[3] =  // pixelformat
{
	(idctcopy)Copy420,
	NULL,
	NULL,
};

#ifdef CONFIG_IDCT_SWAP
static const idctprocess ProcessSwap[3] =   // pixelformat
{
	(idctprocess)Process420Swap,
	(idctprocess)Process422Swap,
	(idctprocess)Process444Swap
};

static const idctcopy Copy16x16Swap[3] =  // pixelformat
{
	(idctcopy)Copy420Swap,
	NULL,
	NULL,
};
#endif

#if defined(CONFIG_IDCT_LOWRES)
static const idctprocess ProcessHalf[3] =   // pixelformat
{
	(idctprocess)Process420Half,
	(idctprocess)Process422Half,
	(idctprocess)Process444Half
};

static const idctprocess ProcessQuarter[3] =   // pixelformat
{
	(idctprocess)Process420Quarter,
	(idctprocess)Process422Quarter,
	(idctprocess)Process444Quarter,
};

static const idctcopy Copy16x16Half[3] =  // pixelformat
{
	(idctcopy)Copy420Half,
	NULL,
	NULL,
};
#endif

static bool_t AllocBuffer(softidct* p,int i,block* Block,uint8_t** Ptr)
{
#ifndef MIPS
	int Align = 32;
	int Offset = 0;
#else
	int Align = 8192;
	int Offset = ((i>>1)+(i&1)*2)*2048;
#endif

	if (!AllocBlock(p->BufSize+Align+Offset,Block,0,HEAP_ANYWR))
		return 0;

	if (i>=2 && AvailMemory()<64*1024)
	{
		FreeBlock(Block);
		ShowOutOfMemory();
		return 0;
	}

	*Ptr = (uint8_t*)(((uintptr_t)Block->Ptr + Align-1) & ~(Align-1)) + Offset;
	return 1;
}

static int SetBufferCount(softidct* p, int n, int Temp)
{
	int i;
	n += Temp;
	if (n>p->MaxCount) return ERR_NOT_SUPPORTED;

	for (i=n;i<p->BufCount;++i)
	{
		FreeBlock(&p->_Buffer[i]);
		p->Buffer[i] = NULL;
		p->BufAlloc[i] = 0;
	}
	if (p->BufCount > n)
		p->BufCount = n;

	for (i=p->BufCount;i<n;++i)
	{
		if (!p->BufSize)
			p->Buffer[i] = NULL;
		else
		if (!AllocBuffer(p,i,&p->_Buffer[i],&p->Buffer[i]))
		{
			p->MaxCount = p->BufCount;
			break;
		}
		p->BufAlloc[i] = p->BufSize;
		p->BufBorder[i] = 0;
		p->BufFrameNo[i] = -1;
		p->BufCount = i+1;
	}

	if (p->ShowNext >= p->BufCount)
		p->ShowNext = -1;
	if (p->ShowCurr >= p->BufCount)
		p->ShowCurr = -1;
	p->LastTemp = Temp && p->BufCount == n;

	if (p->BufCount != n)
		return ERR_OUT_OF_MEMORY;

	return ERR_NONE;
}

static int UpdateFormat(softidct* p)
{
	if (p->Out.Format.Type == PACKET_VIDEO)
	{
		int AlignX,AlignY,Mode;
		int BlockSize;

		PlanarYUV(&p->Out.Format.Format.Video.Pixel,&p->UVX2,&p->UVY2,NULL);

		AlignX = (8 << p->UVX2) - 1;
		AlignY = (8 << p->UVY2) - 1;

		if (!p->BufferWidth || !p->BufferHeight)
			p->BufWidth = p->BufHeight = 0;
		else
		{
			p->BufWidth = (((p->BufferWidth+AlignX)&~AlignX) >> p->Shift)+2*EDGE;
			p->BufHeight = (((p->BufferHeight+AlignY)&~AlignY) >> p->Shift)+2*EDGE;
		}
		if (p->Out.Format.Format.Video.Direction & DIR_SWAPXY)
			SwapInt(&p->BufWidth,&p->BufHeight);

		p->YToU = p->BufHeight * p->BufWidth;
		p->BufSize = p->YToU + 2*(p->YToU >> (p->UVX2+p->UVY2));
		p->BufWidthUV = p->BufWidth >> p->UVX2;

		p->Out.Format.Format.Video.Pitch = p->BufWidth;

		BlockSize = 8 >> p->Shift;
		if (p->Out.Format.Format.Video.Pixel.Flags & PF_YUV420)
		{
			Mode = 0;
			p->Tab[0] = 2*(BlockSize);							//Y[0;0] -> Y[0;1]
			p->Tab[1] = 2*(BlockSize*p->BufWidth-BlockSize);	//Y[0;1] -> Y[1;0]
			p->Tab[2] = 2*(BlockSize);							//Y[1;0] -> Y[1;1]
			p->Tab[4] = 2*(p->YToU >> 2);						//U->V
			p->Tab[5] = 2*(0);	
		}
		else 
		if (p->Out.Format.Format.Video.Pixel.Flags & PF_YUV422)
		{
			Mode = 1;
			p->Tab[0] = 2*(BlockSize);			//Y[0;0] -> Y[0;1]
			p->Tab[2] = 2*(p->YToU >> 1);		//U->V
			p->Tab[3] = 2*(0);	
		}
		else
		if (p->Out.Format.Format.Video.Pixel.Flags & PF_YUV444)
		{
			Mode = 2;
			p->Tab[0] = 2*(p->YToU);			//Y->U
			p->Tab[1] = 2*(p->YToU);			//U->V
			p->Tab[2] = 2*(0);					
		}
		else
			return ERR_NOT_SUPPORTED;

		if (Mode==0 && (p->Mode & IDCTMODE_QPEL))
		{
			p->IDCT.MComp8x8 = p->QPELMComp8x8;
			p->IDCT.MComp16x16 = p->QPELMComp16x16;
		}
		else
		{
			p->IDCT.MComp8x8 = (idctmcomp) SoftMComp8x8;
			p->IDCT.MComp16x16 = (idctmcomp) SoftMComp16x16;
		}

#if defined(CONFIG_IDCT_LOWRES)
		if (p->Shift == 1)
		{
			p->IDCT.Process = ProcessHalf[Mode];
			p->IDCT.Copy16x16 = Copy16x16Half[Mode];
			p->IDCT.Intra8x8 = (idctintra)Intra8x8Half;
		}
		else
		if (p->Shift == 2)
		{
			p->IDCT.Process = ProcessQuarter[Mode];
			p->IDCT.Intra8x8 = (idctintra)Intra8x8Quarter;
			p->IDCT.Copy16x16 = NULL;
		}
		else
#endif
#ifdef CONFIG_IDCT_SWAP
		if (p->Out.Format.Format.Video.Direction & DIR_SWAPXY)
		{
			p->IDCT.Process = ProcessSwap[Mode];
			p->IDCT.Copy16x16 = Copy16x16Swap[Mode];
			p->IDCT.Intra8x8 = p->Intra8x8Swap;
			p->IDCT.MComp8x8 = (idctmcomp) SoftMComp8x8Swap;
			p->IDCT.MComp16x16 = (idctmcomp) SoftMComp16x16Swap;

			if (Mode == 0)
			{
				p->Tab[0] = 2*(8*p->BufWidth);		//Y[0;0] -> Y[1;0]
				p->Tab[1] = 2*(8-8*p->BufWidth);	//Y[1;0] -> Y[0;1]
				p->Tab[2] = 2*(8*p->BufWidth);		//Y[0;1] -> Y[1;1]
			}
		}
		else
#endif
		{
			p->IDCT.Process = Process[Mode];
			p->IDCT.Copy16x16 = Copy16x16[Mode];
			p->IDCT.Intra8x8 = p->Intra8x8;
		}

#if defined(MIPS64)
		p->IDCT.Inter8x8 = Inter8x8Add;
#endif

	}
	return ConnectionUpdate((node*)p,IDCT_OUTPUT,p->Out.Pin.Node,p->Out.Pin.No);
}

#ifdef CONFIG_IDCT_SWAP
static int ChangeSwap(softidct* p)
{
	if (p->BufCount > 0)
	{
		int No;
		planes TmpPlanes;
		video TmpFormat;
		int Brightness;
		video Format[2];
		planes Planes[2];

		TmpFormat = p->Out.Format.Format.Video;
		TmpFormat.Width = p->BufWidth - 2*EDGE;
		TmpFormat.Height = p->BufHeight - 2*EDGE;

		if (SurfaceAlloc(TmpPlanes,&TmpFormat) == ERR_NONE)
		{
			if (Lock(p,0,Planes[0],&Brightness,&Format[0])==ERR_NONE)
			{
				SurfaceCopy(&Format[0],&TmpFormat,Planes[0],TmpPlanes,NULL);
				Unlock(p,0);
			}

			for (No=1;No<p->BufCount;++No)
				if (Lock(p,No,Planes[0],&Brightness,&Format[0])==ERR_NONE)
				{
					SwapInt(&p->BufWidth,&p->BufHeight);
					if (Lock(p,0,Planes[1],&Brightness,&Format[1])==ERR_NONE)
					{
						block Tmp;
						SurfaceRotate(&Format[0],&Format[1],Planes[0],Planes[1],DIR_SWAPXY);
						Unlock(p,0);

						SwapPByte(&p->Buffer[No],&p->Buffer[0]);
						Tmp = p->_Buffer[0];
						p->_Buffer[0] = p->_Buffer[No];
						p->_Buffer[No] = Tmp;
						p->BufBorder[No] = 0;
						SwapInt(&p->BufAlloc[No],&p->BufAlloc[0]);
					}
					SwapInt(&p->BufWidth,&p->BufHeight);
					Unlock(p,No);
				}

			SwapInt(&p->BufWidth,&p->BufHeight);
			if (Lock(p,0,Planes[0],&Brightness,&Format[0])==ERR_NONE)
			{
				SurfaceRotate(&TmpFormat,&Format[0],TmpPlanes,Planes[0],DIR_SWAPXY);
				Unlock(p,0);
				p->BufBorder[0] = 0;
			}
			SwapInt(&p->BufWidth,&p->BufHeight);

			SurfaceFree(TmpPlanes);
		}
	}

	p->Out.Format.Format.Video.Direction ^= DIR_SWAPXY;
	SwapInt(&p->Out.Format.Format.Video.Width,&p->Out.Format.Format.Video.Height);
	SwapInt(&p->OutWidth,&p->OutHeight);
	return UpdateFormat(p);
}
#endif

static int Send(softidct* p,tick_t RefTime,const flowstate* State);

static int VerifyShift(softidct* p,int Shift)
{
	// have to keep dword pitch alignment
	if (p->BufferWidth && Shift>0)
	{
		int AlignX = (8 << p->UVX2) - 1;
		int BufWidth = ((p->BufferWidth+AlignX)&~AlignX) >> (Shift+p->UVX2);
		if (BufWidth & 1)
		{
			Shift -= 2;
		}
		else
		if (BufWidth & 3)
		{
			--Shift;
		}
		if (Shift<0)
			Shift=0;
	}
	return Shift;
}

static int SetShift(softidct* p,int Shift,int Count)
{
#if !defined(CONFIG_IDCT_LOWRES)
	return ERR_NOT_SUPPORTED;
#else
	if (Shift<0 || Shift>2) 
		return ERR_NOT_SUPPORTED;

#ifdef CONFIG_IDCT_SWAP
	if (Shift>0 && (p->Out.Format.Format.Video.Direction & DIR_SWAPXY))
		ChangeSwap(p);
#endif

	Shift = VerifyShift(p,Shift);
	if (p->Shift != Shift)
	{
		flowstate State;
		blitfx FX;
		planes Src[MAXIDCTBUF];
		planes Dst;
		video SrcFormat;
		video DstFormat;
		uint8_t *TmpPtr = NULL;
		planes Tmp;
		block TmpBlock;
		int Diff = Shift - p->Shift;
		int i;

		if (AllocBlock(p->BufSize+16,&TmpBlock,0,HEAP_ANYWR))
		{
			TmpPtr = (uint8_t*)ALIGN16((uintptr_t)TmpBlock.Ptr);
			Tmp[0] = TmpPtr + EDGE + EDGE*p->BufWidth;
			Tmp[1] = TmpPtr + p->YToU + (EDGE >> p->UVX2) + (EDGE >> p->UVY2)*(p->BufWidth >> p->UVX2);
			Tmp[2] = (uint8_t*)Tmp[1] + (p->YToU >> (p->UVX2+p->UVY2));
		}

		for (i=0;i<Count;++i)
			Lock(p,i,Src[i],NULL,&SrcFormat);

		p->Out.Format.Format.Video.Width = p->OutWidth >> Shift;
		p->Out.Format.Format.Video.Height = p->OutHeight >> Shift;

		memset(&FX,0,sizeof(FX));
		if (Diff>0)
		{
			FX.ScaleX = SCALE_ONE >> Diff;
			FX.ScaleY = SCALE_ONE >> Diff;
		}
		else
		{
			FX.ScaleX = SCALE_ONE << -Diff;
			FX.ScaleY = SCALE_ONE << -Diff;
		}

		p->Shift = Shift;
		UpdateRounding(p);
		UpdateFormat(p);

		for (i=0;i<Count;++i)
		{
			if (TmpPtr)
				SurfaceCopy(&SrcFormat,&SrcFormat,Src[i],Tmp,NULL);

			if (p->BufAlloc[i] < p->BufSize)
			{
				uint8_t* Ptr;
				block Block;
				if (!AllocBuffer(p,i,&Block,&Ptr)) // realloc buffer if neccessary)
				{
					if (TmpPtr)
						FreeBlock(&TmpBlock);
					SetShift(p,Shift - Diff,i); // restore (shrinking won't need more memory)
					return ERR_OUT_OF_MEMORY;
				}
				FreeBlock(&p->_Buffer[i]);
				p->Buffer[i] = Ptr;
				p->_Buffer[i] = Block;
				p->BufAlloc[i] = p->BufSize;
			}

			if (TmpPtr)
			{
				Lock(p,i,Dst,NULL,&DstFormat);
				SurfaceCopy(&SrcFormat,&DstFormat,Tmp,Dst,&FX);
			}
			p->BufBorder[i] = 0;
		}

		if (TmpPtr)
			FreeBlock(&TmpBlock);

		State.CurrTime = TIME_RESEND;
		State.DropLevel = 0;
		Send(p,TIME_UNKNOWN,&State);
	}
	return ERR_NONE;
#endif
}

static int UpdateMode(softidct* p)
{
	if (p->Mode & ~p->ModeSupported)
		return ERR_NOT_SUPPORTED;

#ifdef CONFIG_IDCT_SWAP
	if (p->Mode && (p->Out.Format.Format.Video.Direction & DIR_SWAPXY))
		ChangeSwap(p);
#endif

#if defined(CONFIG_IDCT_LOWRES)
	if (p->Mode && p->Shift)
		return SetShift(p,0,p->BufCount);
#endif

	UpdateRounding(p);
	UpdateFormat(p);
	return ERR_NONE;
}

static int SetFormat(softidct* p, const video* Format)
{
	SetBufferCount(p,0,0);
	PacketFormatClear(&p->Out.Format);

	if (Format)
	{
		size_t Area;

		if (!(Format->Pixel.Flags & (PF_YUV420|PF_YUV422|PF_YUV444)))
			return ERR_NOT_SUPPORTED;

		p->Out.Format.Type = PACKET_VIDEO;
		p->Out.Format.Format.Video = *Format;
		if (p->Out.Format.Format.Video.Direction & DIR_SWAPXY)
			SwapInt(&p->Out.Format.Format.Video.Width,&p->Out.Format.Format.Video.Height);
		p->Out.Format.Format.Video.Direction = 0;
		p->Out.Format.Format.Video.Pixel.Flags |= PF_16ALIGNED | PF_SAFEBORDER;
		p->MaxCount = MAXBUF;
		p->Rounding = 0;

		Area = p->Out.Format.Format.Video.Width * p->Out.Format.Format.Video.Height;
		if (Area >= 1024*1024 && AvailMemory() < Area*2)
		{
			video Desktop;
			size_t DesktopArea;
			QueryDesktop(&Desktop);
			DesktopArea = Desktop.Width * Desktop.Height;
			if (Area >= DesktopArea*4 && p->Shift<1)
				p->Shift = 1;
			if (Area >= DesktopArea*16 && p->Shift<2)
				p->Shift = 2;
		}

		p->Shift = VerifyShift(p,p->Shift);

#if defined(MIPS64)
		if (p->Shift>0)
			return ERR_NOT_SUPPORTED; // fallback to mips32
#endif

		p->OutWidth = p->Out.Format.Format.Video.Width;
		p->OutHeight = p->Out.Format.Format.Video.Height;
		p->Out.Format.Format.Video.Width >>= p->Shift;
		p->Out.Format.Format.Video.Height >>= p->Shift;

		UpdateRounding(p);
	}

#ifdef FREESCALE_MX1
	if (p->MX1)
	{
		volatile int* Cmd = p->MX1-64; //0x22400
		volatile int* GCCR = p->MX1-6972; //0x1B810
		if (Format)
		{
			*GCCR |= 2;
			ThreadSleep(1);
			*Cmd = 0x284C+0x20;
			ThreadSleep(1);
			*Cmd = 0x284D;
		}
		else
		{
			*Cmd = 0x00;
			*GCCR &= ~2;
		}
	}
#endif

	return UpdateFormat(p);
}

static int Set(softidct* p, int No, const void* Data, int Size)
{
	flowstate State;
	int Result = ERR_INVALID_PARAM;

	switch (No)
	{
	case NODE_SETTINGSCHANGED:
		p->NeedLast = QueryAdvanced(ADVANCED_SLOW_VIDEO);
		break;

	case IDCT_SHOW: 
		SETVALUE(p->ShowNext,int,ERR_NONE); 
		if (p->ShowNext >= p->BufCount) p->ShowNext = -1;
		break;

	case FLOW_FLUSH:
		p->ShowCurr = -1;
		p->ShowNext = -1;
		return ERR_NONE;

	case IDCT_BACKUP: 
		assert(Size == sizeof(idctbackup));
		Result = IDCTRestore(&p->IDCT,(idctbackup*)Data);
		break;

	case IDCT_MODE: SETVALUECMP(p->Mode,int,UpdateMode(p),EqInt); break;
	case IDCT_BUFFERWIDTH: SETVALUE(p->BufferWidth,int,ERR_NONE); break;
	case IDCT_BUFFERHEIGHT: SETVALUE(p->BufferHeight,int,ERR_NONE); break;
	case IDCT_FORMAT:
		assert(Size == sizeof(video) || !Data);
		Result = SetFormat(p,(const video*)Data);
		break;

	case IDCT_OUTPUT|PIN_PROCESS: SETVALUE(p->Out.Process,packetprocess,ERR_NONE); break;

#ifdef CONFIG_IDCT_SWAP
	case IDCT_OUTPUT|PIN_FORMAT:
		assert(Size == sizeof(packetformat) || !Data);
		if (Data && QueryAdvanced(ADVANCED_IDCTSWAP) && !p->Shift && !p->Mode &&
			!(p->Out.Format.Format.Video.Pixel.Flags & PF_YUV422) &&
			PacketFormatRotatedVideo(&p->Out.Format,(packetformat*)Data,DIR_SWAPXY))
			Result = ChangeSwap(p);
		break;
#endif

	case IDCT_OUTPUT: SETVALUE(p->Out.Pin,pin,ERR_NONE); break;
	case IDCT_ROUNDING: SETVALUE(p->Rounding,bool_t,UpdateRounding(p)); break;
	case IDCT_SHIFT: 
		assert(Size == sizeof(int));
		Result = SetShift(p,*(const int*)Data,p->BufCount);
		break;

	case IDCT_BUFFERCOUNT:
		assert(Size == sizeof(int));
		Result = ERR_NONE;
		if (p->BufCount < *(const int*)Data)
			Result = SetBufferCount(p,*(const int*)Data,0);
		break;

	case FLOW_RESEND:
		State.CurrTime = TIME_RESEND;
		State.DropLevel = 0;
		Result = Send(p,TIME_UNKNOWN,&State);
		break;
	}

	if (No>=IDCT_FRAMENO && No<IDCT_FRAMENO+p->BufCount)
		SETVALUE(p->BufFrameNo[No-IDCT_FRAMENO],int,ERR_NONE);

	return Result;
}

static int Get(softidct* p, int No, void* Data, int Size)
{
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case FLOW_BUFFERED: GETVALUE(1,bool_t); break;
	case IDCT_OUTPUT|PIN_PROCESS: GETVALUE(p->Out.Process,packetprocess); break;
	case IDCT_OUTPUT|PIN_FORMAT: GETVALUE(p->Out.Format,packetformat); break;
	case IDCT_OUTPUT: GETVALUE(p->Out.Pin,pin); break;
	case IDCT_FORMAT: GETVALUE(p->Out.Format.Format.Video,video); break;
	case IDCT_ROUNDING: GETVALUE(p->Rounding,bool_t); break;
	case IDCT_SHIFT: GETVALUE(p->Shift,int); break;
	case IDCT_MODE: GETVALUE(p->Mode,int); break;
	case IDCT_BUFFERCOUNT: GETVALUE(p->BufCount - p->LastTemp,int); break;
	case IDCT_BUFFERWIDTH: GETVALUE(p->BufferWidth,int); break;
	case IDCT_BUFFERHEIGHT: GETVALUE(p->BufferHeight,int); break;
	case IDCT_SHOW: GETVALUE(p->ShowNext,int); break;
	case IDCT_BACKUP: 
		assert(Size == sizeof(idctbackup));
		Result = IDCTBackup(&p->IDCT,(idctbackup*)Data);
		break;
	}
	if (No>=IDCT_FRAMENO && No<IDCT_FRAMENO+p->BufCount)
		GETVALUE(p->BufFrameNo[No-IDCT_FRAMENO],int);
	return Result;
}

static void Drop(softidct* p)
{
	int i;
	for (i=0;i<p->BufCount;++i)
		p->BufFrameNo[i] = -1;
}

static int Null(softidct* p,const flowstate* State,bool_t Empty)
{
	packet* Packet = NULL;
	packet EmptyPacket;
	flowstate DropState;
	if (!State)
	{
		DropState.CurrTime = TIME_UNKNOWN;
		DropState.DropLevel = 1;
		State = &DropState;
	}
	if (Empty)
	{
		memset(&EmptyPacket,0,sizeof(EmptyPacket));
		EmptyPacket.RefTime = TIME_UNKNOWN;
		Packet = &EmptyPacket;
	}
	return p->Out.Process(p->Out.Pin.Node,Packet,State);
}

static int Send(softidct* p,tick_t RefTime,const flowstate* State)
{
	packet Packet;
	int Result = ERR_INVALID_DATA;

	if (State->DropLevel)
		return p->Out.Process(p->Out.Pin.Node,NULL,State);

	if (p->ShowNext < 0)
		return ERR_NEED_MORE_DATA;

	if (p->ShowCurr == p->ShowNext)
		p->ShowCurr = -1;

	if (Lock(p,p->ShowNext,*(planes*)&Packet.Data,NULL,NULL) == ERR_NONE)
	{
		Lock(p,p->ShowCurr,*(planes*)&Packet.LastData,NULL,NULL);

		Packet.RefTime = RefTime;
		Result = p->Out.Process(p->Out.Pin.Node,&Packet,State);

		Unlock(p,p->ShowCurr);
		Unlock(p,p->ShowNext);
	}

	if (Result != ERR_BUFFER_FULL)
		p->ShowCurr = p->ShowNext;

	return Result;
}

static int FrameStart(softidct* p,int FrameNo,int* OldFrameNo,int DstNo,int BackNo,int FwdNo,int ShowNo,bool_t Drop)
{
#if defined(MIPS64_WIN32) 
	// win32 doesn't support 64bit natively. thread and process
	// changes won't save upper part of 64bit registers.
	// we have to disable interrupt and make sure all 1KB
	// code pages are in memory so no page loading is called
	// during code execution 
	p->KMode = KernelMode(1);
	TouchPages(p);	
	DisableInterrupts();
	p->NextIRQ = 0;
#endif

	if (!p->CopyBlock && (BackNo>=0 || FwdNo>=0))
		return ERR_NOT_SUPPORTED;

#ifdef ARM
	// ShowCurr will be the next "LastData" in output packet
	// we don't want to lose it's content
	if (p->NeedLast && p->ShowCurr == DstNo)
	{
		int SwapNo;
		for (SwapNo=0;SwapNo<p->BufCount;++SwapNo)
			if (SwapNo != DstNo && SwapNo != BackNo && SwapNo != FwdNo && SwapNo != ShowNo)
				break;

		if (SwapNo < p->BufCount || SetBufferCount(p,p->BufCount,1)==ERR_NONE)
		{
			//DEBUG_MSG2("IDCT Swap %d,%d",SwapNo,DstNo);
			block Tmp;
			
			Tmp = p->_Buffer[SwapNo];
			p->_Buffer[SwapNo] = p->_Buffer[DstNo];
			p->_Buffer[DstNo] = Tmp;

			SwapPByte(&p->Buffer[SwapNo],&p->Buffer[DstNo]);
			SwapInt(&p->BufFrameNo[SwapNo],&p->BufFrameNo[DstNo]);
			SwapBool(&p->BufBorder[SwapNo],&p->BufBorder[DstNo]);
			SwapInt(&p->BufAlloc[SwapNo],&p->BufAlloc[DstNo]);

			p->ShowCurr = SwapNo;
		}
	}
#endif

	p->ShowNext = ShowNo;

	if (OldFrameNo)
		*OldFrameNo = p->BufFrameNo[DstNo];
	p->BufFrameNo[DstNo] = FrameNo;

	p->Dst = p->Buffer[DstNo];
	p->Ref[0] = NULL;
	p->Ref[1] = NULL;
	p->RefMin[0] = NULL;
	p->RefMin[1] = NULL;
	p->RefMax[0] = NULL;
	p->RefMax[1] = NULL;

	if (BackNo>=0)
	{
		p->Ref[0] = p->Buffer[BackNo];
		if (!p->BufBorder[BackNo])
		{
			p->BufBorder[BackNo] = 1;
			FillEdgeYUV(p,p->Ref[0]);
		}
		p->RefMin[0] = p->Ref[0];
		p->RefMax[0] = p->Ref[0] + p->BufSize - ((8+8*p->BufWidthUV)>>p->Shift);
	}

	if (FwdNo>=0)
	{
		p->Ref[1] = p->Buffer[FwdNo];
		if (!p->BufBorder[FwdNo])
		{
			p->BufBorder[FwdNo] = 1;
			FillEdgeYUV(p,p->Ref[1]);
		}
		p->RefMin[1] = p->Ref[1];
		p->RefMax[1] = p->Ref[1] + p->BufSize - ((8+8*p->BufWidthUV)>>p->Shift);
	}

	p->BufBorder[DstNo] = 0; //invalidate border for dst

#if !defined(MIPS64)

#if defined(CONFIG_IDCT_LOWRES)
	if (p->Shift == 1)
		p->IDCT.Inter8x8 = p->Ref[1]!=NULL?(idctinter)Inter8x8BackFwdHalf:(idctinter)Inter8x8BackHalf;
	else
#endif
#ifdef CONFIG_IDCT_SWAP
	if (p->Out.Format.Format.Video.Direction & DIR_SWAPXY)
		p->IDCT.Inter8x8 = p->Inter8x8Swap[p->Ref[1]!=NULL];
	else
#endif
		p->IDCT.Inter8x8 = p->Inter8x8[p->Ref[1]!=NULL];
#endif

	p->inter8x8uv = p->IDCT.Inter8x8; // for qpel

#ifdef FREESCALE_MX1
	if (p->MX1)
		p->MX1Pop = MX1PopNone;
#endif

	return ERR_NONE;
} 

static void FrameEnd(softidct* p)
{
#ifdef MIPS64_WIN32
	EnableInterrupts();
	KernelMode(p->KMode);
#endif
#ifdef FREESCALE_MX1
	if (p->MX1)
		p->MX1Pop(p);
#endif
}

static int Create(softidct* p)
{
#if defined(MMX)
	if (!(QueryPlatform(PLATFORM_CAPS) & CAPS_X86_MMX))
		return ERR_NOT_SUPPORTED;
#endif

#if defined(MIPS64)
	if (!QueryAdvanced(ADVANCED_VR41XX))
		return ERR_NOT_SUPPORTED;

#if defined(MIPSVR41XX)
	if (!(QueryPlatform(PLATFORM_CAPS) & CAPS_MIPS_VR4120))
		return ERR_NOT_SUPPORTED;
#endif
#endif

	p->IDCT.Get = (nodeget)Get;
	p->IDCT.Set = (nodeset)Set;
	p->IDCT.Send = (idctsend)Send;
	p->IDCT.Null = (idctnull)Null;
	p->IDCT.Drop = (idctdrop)Drop;
	p->IDCT.Lock = (idctlock)Lock;
	p->IDCT.Unlock = (idctunlock)Unlock;
	p->IDCT.FrameStart = (idctframestart)FrameStart;
	p->IDCT.FrameEnd = (idctframeend)FrameEnd;

#if !defined(MIPS64) && !defined(MIPS32)
	p->AddBlock[0] = AddBlock;
	p->AddBlock[1] = AddBlockHor;
	p->AddBlock[2] = AddBlockVer;
	p->AddBlock[3] = AddBlockHorVer;
#endif

	memcpy(p->AllCopyBlock,AllCopyBlock,sizeof(AllCopyBlock));

	p->Intra8x8 = (idctintra)Intra8x8;

#ifndef MIPS64
	p->Inter8x8[0] = (idctinter)Inter8x8Back;
	p->Inter8x8[1] = (idctinter)Inter8x8BackFwd;
	p->QPELInter = (idctinter)Inter8x8QPEL;
	p->QPELCopyBlockM = (copyblock)CopyBlockM;
	p->Inter8x8GMC = (idctinter)Inter8x8GMC;
#endif

#ifdef CONFIG_IDCT_SWAP
	p->Intra8x8Swap = (idctintra)Intra8x8Swap;

#ifndef MIPS64
	p->Inter8x8Swap[0] = (idctinter)Inter8x8BackSwap;
	p->Inter8x8Swap[1] = (idctinter)Inter8x8BackFwdSwap;
#endif
#endif 

#if defined(MIPS64_WIN32) 
	CodeFindPages(*(void**)&p->IDCT.Enum,&p->CodeBegin,&p->CodeEnd,&p->CodePage);
#endif

#if defined(ARM)

#if defined(FREESCALE_MX1)
	if (QueryPlatform(PLATFORM_MODEL)==MODEL_ZODIAC)
	{
		p->MX1 = (int*)0x80022500;

		p->Intra8x8 = (idctintra)MX1Intra8x8;
		p->Inter8x8[0] = (idctinter)MX1Inter8x8Back;
		p->Inter8x8[1] = (idctinter)MX1Inter8x8BackFwd;

#ifdef CONFIG_IDCT_SWAP
		p->Intra8x8Swap = (idctintra)MX1Intra8x8Swap;
		p->Inter8x8Swap[0] = (idctinter)MX1Inter8x8BackSwap;
		p->Inter8x8Swap[1] = (idctinter)MX1Inter8x8BackFwdSwap;
#endif
	}
	else
#endif

#if (!defined(TARGET_PALMOS) && !defined(TARGET_SYMBIAN)) || (__GNUC__>=4) || (__GNUC__==3 && __GNUC_MINOR__>=4)
	if (!QueryAdvanced(ADVANCED_NOWMMX) && 
		(QueryPlatform(PLATFORM_CAPS) & CAPS_ARM_WMMX))
	{
		p->Intra8x8 = (idctintra)WMMXIntra8x8;
		p->Inter8x8[0] = (idctinter)WMMXInter8x8Back;
		p->Inter8x8[1] = (idctinter)WMMXInter8x8BackFwd;
		p->QPELInter = (idctinter)WMMXInter8x8QPEL;
		p->Inter8x8GMC = (idctinter)WMMXInter8x8GMC;
		p->QPELCopyBlockM = (copyblock)WMMXCopyBlockM;

#ifdef CONFIG_IDCT_SWAP
		p->Intra8x8Swap = (idctintra)WMMXIntra8x8Swap;
		p->Inter8x8Swap[0] = (idctinter)WMMXInter8x8BackSwap;
		p->Inter8x8Swap[1] = (idctinter)WMMXInter8x8BackFwdSwap;
#endif

		p->AllCopyBlock[0][0] = WMMXCopyBlock;
		p->AllCopyBlock[0][1] = WMMXCopyBlockHor;
		p->AllCopyBlock[0][2] = WMMXCopyBlockVer;
		p->AllCopyBlock[0][3] = WMMXCopyBlockHorVer;
		p->AllCopyBlock[1][0] = WMMXCopyBlock;
		p->AllCopyBlock[1][1] = WMMXCopyBlockHorRound;
		p->AllCopyBlock[1][2] = WMMXCopyBlockVerRound;
		p->AllCopyBlock[1][3] = WMMXCopyBlockHorVerRound;
		p->AddBlock[0] = WMMXAddBlock;
		p->AddBlock[1] = WMMXAddBlockHor;
		p->AddBlock[2] = WMMXAddBlockVer;
		p->AddBlock[3] = WMMXAddBlockHorVer;
	}
	else
#endif // TARGET_PALMOS
	{}
/*	if (QueryPlatform(PLATFORM_CAPS) & CAPS_ARM_5E)
	{
		if (QueryPlatform(PLATFORM_ICACHE) >= 32768)
		{
			p->AllCopyBlock[0][0] = FastCopyBlock;
			p->AllCopyBlock[0][1] = FastCopyBlockHor;
			p->AllCopyBlock[0][2] = FastCopyBlockVer;
			p->AllCopyBlock[0][3] = FastCopyBlockHorVer;
			p->AllCopyBlock[0][0] = FastCopyBlock;
			p->AllCopyBlock[1][1] = FastCopyBlockHorRound;
			p->AllCopyBlock[1][2] = FastCopyBlockVerRound;
			p->AllCopyBlock[1][3] = FastCopyBlockHorVerRound;
			p->AddBlock[0] = FastAddBlock;
			p->AddBlock[1] = FastAddBlockHor;
			p->AddBlock[2] = FastAddBlockVer;
			p->AddBlock[3] = FastAddBlockHorVer;
		}
		else
		{
			p->AllCopyBlock[0][0] = PreLoadCopyBlock;
			p->AllCopyBlock[0][1] = PreLoadCopyBlockHor;
			p->AllCopyBlock[0][2] = PreLoadCopyBlockVer;
			p->AllCopyBlock[0][3] = PreLoadCopyBlockHorVer;
			p->AllCopyBlock[0][0] = PreLoadCopyBlock;
			p->AllCopyBlock[1][1] = PreLoadCopyBlockHorRound;
			p->AllCopyBlock[1][2] = PreLoadCopyBlockVerRound;
			p->AllCopyBlock[1][3] = PreLoadCopyBlockHorVerRound;
			p->AddBlock[0] = PreLoadAddBlock;
			p->AddBlock[1] = PreLoadAddBlockHor;
			p->AddBlock[2] = PreLoadAddBlockVer;
			p->AddBlock[3] = PreLoadAddBlockHorVer;
		}
	}
*/

#endif //ARM

	p->Tmp = (uint8_t*)ALIGN16((uintptr_t)p->_Tmp);
	p->MaxCount = MAXBUF;
	p->BufCount = 0;
	p->LastTemp = 0;
	p->ShowCurr = -1;
	p->ShowNext = -1;

	return ERR_NONE;
}

static const nodedef SoftIDCT =
{
	sizeof(softidct),
#if defined(MPEG4_EXPORTS)
	MPEG4IDCT_ID,
	IDCT_CLASS,
	PRI_DEFAULT+20,
#elif defined(MSMPEG4_EXPORTS)
	MSMPEG4IDCT_ID,
	IDCT_CLASS,
	PRI_DEFAULT+10,
#else
	SOFTIDCT_ID,
	IDCT_CLASS,
	PRI_DEFAULT,
#endif
	(nodecreate)Create,
};

void SoftIDCT_Init()
{
	NodeRegisterClass(&SoftIDCT);
}

void SoftIDCT_Done()
{
	NodeUnRegisterClass(SoftIDCT.Class);
}
