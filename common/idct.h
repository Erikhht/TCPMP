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
 * $Id: idct.h 548 2006-01-08 22:41:57Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/
 
#ifndef __IDCT_H
#define __IDCT_H

#define MAXIDCTBUF		6

//---------------------------------------------------------------
// motion vector macros
#define NOMV		0x80000000

#define MVX(a)		(((a)<<16)>>16)
#define MVY(a)		((a)>>16)
#define MAKEMV(x,y)	(((y)<<16)|((x)&0xFFFF))

#ifdef IS_BIG_ENDIAN
#define MVXIDX	1
#define MVYIDX	0
#else
#define MVXIDX	0
#define MVYIDX	1
#endif

//---------------------------------------------------------------
// idct (abstract)

#define IDCT_CLASS			FOURCC('I','D','C','T')

// motion compesation rounding
#define IDCT_ROUNDING		0x50
// video format
#define IDCT_FORMAT			0x51
// output packet
#define IDCT_OUTPUT			0x52
// number of buffers (int)
#define IDCT_BUFFERCOUNT	0x53
// manual setting of buffer width, -1 for auto (int)
#define IDCT_BUFFERWIDTH	0x57
// manual setting of buffer height, -1 for auto (int)
#define IDCT_BUFFERHEIGHT	0x58
// showed buffer (int)
#define IDCT_SHOW			0x56
// backup/restore data (data is allocated/freed, idct is closed/inited)
#define IDCT_BACKUP			0x59
// low-res modes (1/2,1/4)
#define IDCT_SHIFT			0x5A
// mode flags
#define IDCT_MODE			0x5B

// frame no associated with a buffer (int) (array)
#define IDCT_FRAMENO		0x1000

extern DLL int IDCTEnum(void*, int* EnumNo, datadef* Out);

//---------------------------------------------------------------
#define IDCTMODE_QPEL		1
#define IDCTMODE_INTERLACE	2
#define IDCTMODE_GMC		4

//---------------------------------------------------------------
// scan orders (not every data format requires)
#define IDCTSCAN_ZIGZAG		0
#define IDCTSCAN_ALT_HORI	1
#define IDCTSCAN_ALT_VERT	2

//---------------------------------------------------------------
// block format
//
// input is a idct_block_t[8][8] matrix
// which is directly passed to 8x8 idct
// attention: input matrix is overwritten(trashed) after the call
//
// length/scantype is hint how much data is in the matrix
// block[scan[length..63]] souhld be zero
// pass length=64 if you can't provide this information

typedef short int idct_block_t;

#define IDCT_BLOCK_DECLARE \
	idct_block_t _block[2*64+16/sizeof(idct_block_t)];

#define IDCT_BLOCK_PREPARE(x,out) \
	out = (idct_block_t*)ALIGN16((uintptr_t)x->_block);		

//---------------------------------------------------------------
// gmc

#define GMC_FRACBITS	16

typedef struct idct_gmc
{
    int points;
	int accuracy;
    int offset[2][2];
    int delta[2][2];

} idct_gmc;

//---------------------------------------------------------------
// idct virtual method table

// set all buffer's frameno to -1
typedef void (*idctdrop)(void* This);
// send packet to output
typedef	int (*idctsend)(void* This,tick_t RefTime,const flowstate* State);
// send null packet to output (State == NULL means dropping)
typedef	int (*idctnull)(void* This,const flowstate* State,bool_t Empty);
// lock one of the buffers
typedef	int (*idctlock)(void* This,int No,planes Planes,int* Brightness,video* Format);
// unlock buffer
typedef	void (*idctunlock)(void* This,int No);
// start decoding a frame (oldframeno is -1 if content is lost)
typedef	int (*idctframestart)(void* This,int FrameNo,int* OldFrameNo,int Dst,int Back,int Fwd,int Show,bool_t Drop);
// end decoding a frame
typedef	void (*idctframeend)(void* This);
// setup gmc params
typedef	void (*idctgmc)(void* This,const idct_gmc*);

// start processing a macroblock
typedef	void (*idctprocess)(void* This,int x,int y);
// copy a macroblock from the backward or forward buffer
typedef void (*idctcopy)(void* This,int x,int y,int Forward);
// motion compensation data (MVBack and MVFwd has to be valid during Inter8x8 are called)
typedef void (*idctmcomp)(void* This,const int *MVBack,const int *MVFwd);
// passing idct data 
typedef void (*idctintra)(void* This,void* Data,int Length,int ScanType);
// passing idct data (always has to be called even if there is no data)
typedef void (*idctinter)(void* This,void* Data,int Length);

typedef struct idct
{
	VMT_NODE

	idctsend		Send;
	idctnull		Null;
	idctdrop		Drop;
	idctlock		Lock;
	idctunlock		Unlock;
	idctframestart	FrameStart;
	idctframeend	FrameEnd;
	idctgmc			GMC;

	idctprocess		Process;
	idctcopy		Copy16x16;
	idctmcomp		MComp16x16;
	idctmcomp		MComp8x8;
	idctintra		Intra8x8;
	idctinter		Inter8x8; // data is optional (length=0)
	idctprocess		MCompGMC;
	idctinter		Inter8x8GMC; // data is optional (length=0)

} idct;

typedef struct idctbufferbackup
{
	int FrameNo;
	planes Buffer;
	int Brightness;
	video Format;

} idctbufferbackup;

typedef struct idctbackup
{
	video Format;
	int Width;
	int Height;
	int Count;
	int Show;
	int Shift;
	int Mode;
	idctbufferbackup Buffer[MAXIDCTBUF];

} idctbackup;

DLL int IDCTBackup(idct*,idctbackup*);
DLL int IDCTRestore(idct*,idctbackup*);
DLL int IDCTSwitch(idct* Src, idct* Dst, int MinBufferCount);

static INLINE void ClearBlock(idct_block_t *p) 
{
	int *i=(int*)p,*ie=(int*)(p+64);
	do
	{
		i[3] = i[2] = i[1] = i[0] = 0;
		i[7] = i[6] = i[5] = i[4] = 0;
		i+=8;
	}
	while (i!=ie);
}

//---------------------------------------------------------------
// decoder using idct (abstract)

#define CODECIDCT_CLASS		FOURCC('C','C','I','D')

#define CODECIDCT_INPUT		0x100
#define CODECIDCT_IDCT		0x101

typedef	int (*codecidctframe)(void* This,const uint8_t* Ptr,int Len);
typedef	bool_t (*codecidctnext)(void* This);

typedef struct codecidct
{
	node Node;

	nodefunc UpdateInput;
	nodefunc UpdateCount;
	nodefunc UpdateSize;
	nodefunc Discontinuity;
	nodefunc Flush;
	codecidctframe Frame;
	codecidctnext FindNext;

	int MinCount;
	int DefCount;
	tick_t DropTolerance;

	struct
	{
		idct *Ptr;
		video Format;
		int Width;
		int Height;
		int Count;
		int Mode;

	} IDCT;

	struct
	{
		pin Pin; 
		packetformat Format;
	} In;

	int Show;
	flowstate State;
	tick_t RefTime;
	bool_t RefUpdated;
	bool_t Dropping; // indicate that we are not after a seek, but a hard dropping period
	pin NotSupported;

	// packed stream
	tick_t FrameTime;
	int FrameEnd;
	buffer Buffer;

	int Reserved[8];

} codecidct;

DLL int CodecIDCTEnum(void*, int* EnumNo, datadef* Out);
DLL int CodecIDCTGet(codecidct* p, int No, void* Data, int Size);
DLL int CodecIDCTSet(codecidct* p, int No, const void* Data, int Size);
DLL int CodecIDCTSetFormat(codecidct* p, int Flags, int Width, int Height, int IDCTWidth, int IDCTHeight,int Aspect);
DLL int CodecIDCTSetCount(codecidct* p, int Count);
DLL int CodecIDCTSetMode(codecidct* p, int Mode, bool_t Value);

void IDCT_Init();
void IDCT_Done();

#endif
