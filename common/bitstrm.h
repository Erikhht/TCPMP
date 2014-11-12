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
 * $Id: bitstrm.h 292 2005-10-14 20:30:00Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __BITSTRM_H
#define __BITSTRM_H

typedef struct bitstream
{
	int bits;
	int bitpos;
	const uint8_t *bitptr;
	const uint8_t *bitend;
	
} bitstream;

// n=1..24 (or ..32 after bytealign)
#define bitshow(p,n) ((uint32_t)((p)->bits << (p)->bitpos) >> (32-(n)))

// n=9..32
static INLINE int bitshowlarge(bitstream* p, int n) 
{
	int i = bitshow(p,n);
	i |= *p->bitptr >> (40-n-p->bitpos);
	return i;
}

static INLINE void bitrewind(bitstream* p,int n)
{
	p->bitpos += 32 - (n & 7);
	p->bitptr -= 4 + (n >> 3);
}

#define bitflush(p,n) (p)->bitpos += n;

// use this only in boolean expression. to get a 1 bit value use getbits(p,1)
#define bitget1(p) (((p)->bits << (p)->bitpos++) < 0)

static INLINE int bitget(bitstream* p,int n)
{
	int i = bitshow(p,n);
	bitflush(p,n);
	return i;
}

static INLINE const uint8_t* bitendptr(bitstream* p) { return p->bitend-4; }

static INLINE uintptr_t bitendbookmark(bitstream* p)
{
	return (uintptr_t)p->bitend*8;
}

static INLINE uintptr_t bitbookmark(bitstream* p)
{
	return (uintptr_t)p->bitptr*8 + p->bitpos;
}

// non-exact eof, just for error checking
static INLINE int biteof(bitstream* p)
{
	return p->bitptr >= p->bitend;
}

static INLINE void bitbytealign(bitstream* p)
{
	p->bitpos = ALIGN8(p->bitpos);
}

static INLINE int bittonextbyte(bitstream* p)
{
	return 8-(p->bitpos & 7);
}

static INLINE const uint8_t *bitbytepos(bitstream* p)
{
	return p->bitptr - 4 + ((p->bitpos+7) >> 3);
}

//************************************************************************
// optimized reader using one local variable

#define bitload_pos(p, bitpos)					\
	bitpos -= 8;								\
	if (bitpos >= 0) {							\
		int bits = (p)->bits;					\
		const uint8_t* bitptr = (p)->bitptr;	\
		do {									\
			bits = (bits << 8) | *bitptr++;		\
			bitpos -= 8;						\
		} while (bitpos>=0);					\
		(p)->bits = bits;						\
		(p)->bitptr = bitptr;					\
	}											\
	bitpos += 8;

#define bitbookmark_pos(p,bitpos) ((int)(p)->bitptr*8 + bitpos)
#define bitshow_pos(p,bitpos,n) ((uint32_t)((p)->bits << bitpos) >> (32-(n)))
#define bitflush_pos(p,bitpos,n) bitpos += n;
#define bitget1_pos(p,bitpos) (((p)->bits << bitpos++) < 0)
#define bitbytealign_pos(p,bitpos) bitpos = ALIGN8(bitpos);
#define bitrewind_pos(p,bitpos,n) bitpos += 32 - ((n) & 7); (p)->bitptr -= 4 + ((n) >> 3);

#define bitgetx_pos(p,bitpos,n,tmp)			\
{												\
	tmp = (p)->bits << bitpos;				\
	bitflush_pos(p,bitpos,n);					\
	tmp >>= 1;									\
	tmp ^= 0x80000000;							\
	tmp >>= 31-n;								\
	n = tmp - (tmp >> 31);						\
}

//************************************************************************
// optimized reader using two prenamed local variables

#define bitbegin_pos2(p)		\
	int bitpos = (p)->bitpos;	\
	int bits = (p)->bits;

#define bitend_pos2(p)		\
	(p)->bitpos = bitpos;

#define bitload_pos2(p)						\
	if (bitpos >= 8) {						\
		const uint8_t* bitptr = (p)->bitptr;\
		do {								\
			bits = (bits << 8) | *bitptr++;	\
			bitpos -= 8;					\
		} while (bitpos >= 8);				\
		(p)->bits = bits;					\
		(p)->bitptr = bitptr;				\
	}											

#define bitshow_pos2(p,n) ((uint32_t)(bits << bitpos) >> (32-(n)))
#define bitflush_pos2(p,n) bitpos += n
#define bitget1_pos2(p) ((bits << bitpos++) < 0)
#define bitsign_pos2(p) (((bits << bitpos++) >> 31)|1)

//************************************************************************

#define bitloadcond(p) if ((p)->bitpos >= 8) bitload(p);

#define DECLARE_BITLOAD										\
static NOINLINE void bitload(bitstream* p)					\
{															\
	bitbegin_pos2(p);										\
	bitload_pos2(p);										\
	bitend_pos2(p);											\
}

#define DECLARE_BITINIT														\
static void bitinit(bitstream* p,const uint8_t *stream, int len)			\
{																			\
	p->bits = 0;															\
	p->bitpos = 32;															\
	p->bitptr = stream;														\
	p->bitend = stream+len+4;												\
}

static INLINE void bitsetpos(bitstream* p,const uint8_t *stream)
{
	p->bitpos = 32;
	p->bitptr = stream;
}

#ifdef MIPS
static INLINE void bitloadinline(bitstream* p)
{
	int n = p->bitpos-8;
	if (n>=0)
	{
		const uint8_t* bitptr = p->bitptr;
		int bits = p->bits;

		do
		{
			bits = (bits << 8) | *bitptr++;
			n -= 8;
		}
		while (n>=0);

		p->bits = bits;
		p->bitptr = bitptr;
		p->bitpos = n+8;
	}
}
#else
#define bitloadinline(p) if ((p)->bitpos >= 8) bitload(p)
#endif

#endif
