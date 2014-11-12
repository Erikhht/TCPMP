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
 * $Id: tools.h 607 2006-01-22 20:58:29Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __TOOLS_H
#define __TOOLS_H

#define ALIGN64(x) (((x) + 63) & ~63)
#define ALIGN16(x) (((x) + 15) & ~15)
#define ALIGN8(x) (((x) + 7) & ~7)
#define ALIGN4(x) (((x) + 3) & ~3)

#define GET_R(x)   ((uint8_t)(((x) >> 0) & 255))
#define GET_G(x)   ((uint8_t)(((x) >> 8) & 255))
#define GET_B(x)   ((uint8_t)(((x) >> 16) & 255))

//some helper functions

static INLINE bool_t EqInt(const int* a, const int* b) { return *a == *b; }
static INLINE bool_t EqBool(const bool_t* a, const bool_t* b) { return *a == *b; }
static INLINE bool_t EqPtr(void** a, void** b) { return *a == *b; }

DLL void* Alloc16(size_t); //aligned to 16 bytes
DLL void Free16(void*);

DLL bool_t EqPoint(const point* a, const point* b);
DLL bool_t EqRect(const rect* a, const rect* b);
DLL bool_t EqPixel(const pixel* a, const pixel* b);
DLL bool_t EqVideo(const video* a, const video* b);
DLL bool_t EqFrac(const fraction* a, const fraction* b);
DLL bool_t EqAudio(const audio* a, const audio* b);
DLL bool_t EqSubtitle(const subtitle* a, const subtitle* b);
DLL bool_t EqBlitFX(const blitfx* a, const blitfx* b);

DLL void Simplify(fraction*, int MaxNum, int MaxDen);

DLL void SwapPChar(tchar_t**, tchar_t**);
DLL void SwapPByte(uint8_t**, uint8_t**);
DLL void SwapInt(int*, int*);
DLL void SwapLong(long*, long*);
DLL void SwapBool(bool_t*, bool_t*);
DLL void SwapPoint(point*);
DLL void SwapRect(rect*);

DLL void VirtToPhy(const rect* Virtual, rect* Physical, const video*);
DLL void PhyToVirt(const rect* Physical, rect* Virtual, const video*);
DLL void ClipRectPhy(rect* Physical, const video*);

DLL bool_t Compressed(const pixel*);
DLL bool_t AnyYUV(const pixel*);
DLL bool_t PlanarYUV(const pixel*, int* UVX2, int* UVY2,int* UVStride2);
DLL bool_t PlanarYUV444(const pixel*);
DLL bool_t PlanarYUV422(const pixel*);
DLL bool_t PlanarYUV410(const pixel*);
DLL bool_t PlanarYUV420(const pixel*);
DLL bool_t PackedYUV(const pixel*);
DLL uint32_t DefFourCC(const pixel*);
DLL void FillInfo(pixel*);
DLL int GetImageSize(const video*);
DLL int GetBPP(const pixel*);
DLL int BitMaskSize(uint32_t Mask);
DLL int BitMaskPos(uint32_t Mask);
DLL uint32_t RGBToFormat(rgbval_t RGB, const pixel*);

DLL int CombineDir(int SrcDir,int FXDir,int DstDir);
DLL void FillColor(uint8_t* Dst,int DstPitch,int x,int y,int Width,int Height,int BPP,int Value);
DLL int AnyAlign(rect* DstRect, rect* SrcRect, const blitfx* FX, 
				 int DstAlignSize, int DstAlignPos, 
				 int MinScale, int MaxScale);

DLL int SurfaceAlloc(planes, const video*);
DLL void SurfaceFree(planes);
DLL int SurfaceCopy(const video* SrcFormat, const video* DstFormat, 
					const planes Src, planes Dst, const blitfx* FX);
DLL int SurfaceRotate(const video* SrcFormat, const video* DstFormat, 
					  const planes Src, planes Dst, int Dir);

DLL int DefaultAspect(int,int);
DLL void DefaultPitch(video*);
DLL void DefaultRGB(pixel*, int BitCount, 
                     int RBits, int GBits, int BBits,
                     int RGaps, int GGaps, int BGaps);

DLL void BuildChapter(tchar_t* s,int slen,int No,int64_t Time,int Rate);

// variable names: Result, Data, Size

#define GETVALUE(Value,Type)								\
		{													\
			assert(Size == sizeof(Type));					\
			*(Type*)Data = Value;							\
			Result = ERR_NONE;								\
		}

#define GETVALUECOND(Value,Type,Cond)						\
		{													\
			assert(Size == sizeof(Type));					\
			if (Cond)										\
			{												\
				*(Type*)Data = Value;						\
				Result = ERR_NONE;							\
			}												\
		}

#define GETSTRING(Text)										\
		{													\
			tcscpy_s((tchar_t*)Data,Size/sizeof(tchar_t),Text);\
			Result = ERR_NONE;								\
		}

#define SETVALUE(Value,Type,Update)							\
		{													\
			assert(Size == sizeof(Type));					\
			Value = *(Type*)Data;							\
			Result = Update;								\
		}

#define SETVALUENULL(Value,Type,Update,Null)				\
		{													\
			if (Data)										\
			{												\
				assert(Size == sizeof(Type));				\
				Value = *(Type*)Data;						\
			}												\
			else											\
				Null;										\
			Result = Update;								\
		}

#define SETVALUECOND(Value,Type,Update,Cond)				\
		{													\
			assert(Size == sizeof(Type));					\
			if (Cond)										\
			{												\
				Value = *(Type*)Data;						\
				Result = Update;							\
			}												\
		}

#define SETVALUECMP(Value,Type,Update,EqFunc)				\
		{													\
			assert(Size == sizeof(Type));					\
			Result = ERR_NONE;								\
			if (!EqFunc(&Value,(Type*)Data))				\
			{												\
				Value = *(Type*)Data;						\
				Result = Update;							\
			}												\
		}

#define SETSTRING(Text)										\
		{													\
			tcscpy_s(Text,TSIZEOF(Text),(tchar_t*)Data);	\
			Result = ERR_NONE;								\
		}

#define SETPACKETFORMAT(Value,Type,Update)					\
		{													\
			assert(Size == sizeof(Type) || !Data);			\
			Result = PacketFormatCopy(&Value,(Type*)Data);	\
			if (Result == ERR_NONE)							\
				Result = Update;							\
		}

#define SETPACKETFORMATCMP(Value,Type,Update)				\
		{													\
			assert(Size == sizeof(Type) || !Data);			\
			Result = ERR_NONE;								\
			if (!EqPacketFormat(&Value,(Type*)Data))		\
			{												\
				Result = PacketFormatCopy(&Value,(Type*)Data);\
				if (Result == ERR_NONE)						\
					Result = Update;						\
			}												\
		}

DLL void SplitURL(const tchar_t* URL, tchar_t* Mime, int MimeLen, tchar_t* Dir, int DirLen, tchar_t* Name, int NameLen, tchar_t* Ext, int ExtLen);
DLL bool_t SetFileExt(tchar_t* URL, int URLLen, const tchar_t* Ext);
DLL int CheckExts(const tchar_t* URL, const tchar_t* Exts);
DLL bool_t CheckContentType(const tchar_t* s, const tchar_t* List);
DLL void AbsPath(tchar_t* Abs, int AbsLen, const tchar_t* Any, const tchar_t* Base);
DLL void RelPath(tchar_t* Rel, int RelLen, const tchar_t* Any, const tchar_t* Base);
DLL bool_t UpperPath(tchar_t* Path, tchar_t* Last, int LastLen);

DLL bool_t UniqueExts(const int* Begin,const int* Pos);

DLL void GetAsciiToken(tchar_t* Out,size_t OutLen,const char* In,size_t InLen);

#define SWAP32(a) ((((uint32_t)(a) >> 24) & 0x000000FF) | (((uint32_t)(a) >> 8)  & 0x0000FF00)|\
                  (((uint32_t)(a) << 8)  & 0x00FF0000) | (((uint32_t)(a) << 24) & 0xFF000000))

#define SWAP16(a) ((uint16_t)((((uint32_t)(a) >> 8) & 0xFF) | (((uint32_t)(a) << 8) & 0xFF00)))
#define SWAP64(a) (((uint64_t)SWAP32(a) << 32) | SWAP32((uint64_t)(a)>>32))

#define RSHIFT_ROUND(v,n)	(((v)+(1<<(n-1)))>>(n))

#ifdef IS_BIG_ENDIAN
#define INT64BE(a) (a)
#define INT64LE(a) SWAP64(a)
#define INT32BE(a) (a)
#define INT32LE(a) SWAP32(a)
#define INT16BE(a) (a)
#define INT16LE(a) SWAP16(a)
#else
#define INT64LE(a) (a)
#define INT64BE(a) SWAP64(a)
#define INT32LE(a) (a)
#define INT32BE(a) SWAP32(a)
#define INT16LE(a) (a)
#define INT16BE(a) SWAP16(a)
#endif

#if defined(CONFIG_UNALIGNED_ACCESS)
#define LOAD16LE(ptr)		INT16LE(*(uint16_t*)(ptr))
#define LOAD16BE(ptr)		INT16BE(*(uint16_t*)(ptr))
#define LOAD32LE(ptr)		INT32LE(*(uint32_t*)(ptr))
#define LOAD32BE(ptr)		INT32BE(*(uint32_t*)(ptr))
#define LOAD64LE(ptr)		INT64LE(*(uint64_t*)(ptr))
#define LOAD64BE(ptr)		INT64BE(*(uint64_t*)(ptr))
#else
#define LOAD8(ptr,ofs)		(((uint8_t*)(ptr))[ofs])
#define LOAD16LE(ptr)		((uint16_t)((LOAD8(ptr,1)<<8)|LOAD8(ptr,0)))
#define LOAD16BE(ptr)		((uint16_t)((LOAD8(ptr,0)<<8)|LOAD8(ptr,1)))
#define LOAD32LE(ptr)		((LOAD8(ptr,3)<<24)|(LOAD8(ptr,2)<<16)|(LOAD8(ptr,1)<<8)|LOAD8(ptr,0))
#define LOAD32BE(ptr)		((LOAD8(ptr,0)<<24)|(LOAD8(ptr,1)<<16)|(LOAD8(ptr,2)<<8)|LOAD8(ptr,3))
#define LOAD64LE(ptr)		(((uint64_t)(LOAD8(ptr,0))    )|((uint64_t)(LOAD8(ptr,1))<< 8)|((uint64_t)(LOAD8(ptr,2))<<16)|((uint64_t)(LOAD8(ptr,3))<<24)| \
							 ((uint64_t)(LOAD8(ptr,4))<<32)|((uint64_t)(LOAD8(ptr,5))<<40)|((uint64_t)(LOAD8(ptr,6))<<48)|((uint64_t)(LOAD8(ptr,0))<<56)|)
#define LOAD64BE(ptr)		(((uint64_t)(LOAD8(ptr,0))<<56)|((uint64_t)(LOAD8(ptr,1))<<48)|((uint64_t)(LOAD8(ptr,2))<<40)|((uint64_t)(LOAD8(ptr,3))<<32)| \
							 ((uint64_t)(LOAD8(ptr,4))<<24)|((uint64_t)(LOAD8(ptr,5))<<16)|((uint64_t)(LOAD8(ptr,6))<< 8)|((uint64_t)(LOAD8(ptr,0))    )|)
#endif

#ifdef IS_BIG_ENDIAN
#define LOAD16(ptr) LOAD16BE(ptr)
#define LOAD32(ptr) LOAD32BE(ptr)
#define LOAD64(ptr) LOAD64BE(ptr)
#define LOAD16SW(ptr) LOAD16LE(ptr)
#define LOAD32SW(ptr) LOAD32LE(ptr)
#define LOAD64SW(ptr) LOAD64LE(ptr)
#else
#define LOAD16(ptr) LOAD16LE(ptr)
#define LOAD32(ptr) LOAD32LE(ptr)
#define LOAD64(ptr) LOAD64LE(ptr)
#define LOAD16SW(ptr) LOAD16BE(ptr)
#define LOAD32SW(ptr) LOAD32BE(ptr)
#define LOAD64SW(ptr) LOAD64BE(ptr)
#endif

// a=(a+c+1)/2
// b=(b+d+1)/2
#define AVG32R(a,b,c,d) \
	c^=a; \
	d^=b; \
	a|=c; \
	b|=d; \
	c &= 0xFEFEFEFE; \
	d &= 0xFEFEFEFE; \
	a-=c>>1; \
	b-=d>>1; 

// a=(a+c)/2
// b=(b+d)/2
#ifdef ARM
#define AVG32NR(a,b,c,d) \
	c^=a; \
	d^=b; \
	a &= ~c; \
	b &= ~d; \
	c &= 0xFEFEFEFE; \
	d &= 0xFEFEFEFE; \
	a+=c>>1; \
	b+=d>>1; 
#else
#define AVG32NR(a,b,c,d) \
	c^=a; \
	d^=b; \
	a|=c; \
	b|=d; \
	a-=c & 0x01010101; \
	b-=d & 0x01010101; \
	c &= 0xFEFEFEFE; \
	d &= 0xFEFEFEFE; \
	a-=c>>1; \
	b-=d>>1; 
#endif

static INLINE int _log2(uint32_t data)
{
	int i;
	if (!data) ++data;
	for (i=0;data;++i)
		data >>= 1;
    return i;
}

DLL int ScaleRound(int v,int Num,int Den);

static INLINE int Scale(int v,int Num,int Den)
{
	if (Den) 
		return (int)((int64_t)v * Num / Den);
	return 0;
}

static INLINE int Scale64(int v,int64_t Num,int64_t Den)
{
	if (Den) 
		return (int)((int64_t)v * Num / Den);
	return 0;
}

#define OFS(name,item) ((int)&(((name*)NULL)->item))

#define WMVF_ID		FOURCC('W','M','V','F')
#define WMAF_ID		FOURCC('W','M','A','F')

extern DLL const nodedef WMVF;
extern DLL const nodedef WMAF;

#endif
