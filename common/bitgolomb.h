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
 * $Id: bitgolomb.h 292 2005-10-14 20:30:00Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __BITGOLOMB_H
#define __BITGOLOMB_H

#define GL2(n)	12-n+1+12-n

#define assertgolomb(p) //assert(p)

static const uint8_t golomb_log2[64] =
{
	GL2(0),
	GL2(1),
	GL2(2),GL2(2),
	GL2(3),GL2(3),GL2(3),GL2(3),
	GL2(4),GL2(4),GL2(4),GL2(4),GL2(4),GL2(4),GL2(4),GL2(4),
	GL2(5),GL2(5),GL2(5),GL2(5),GL2(5),GL2(5),GL2(5),GL2(5),
	GL2(5),GL2(5),GL2(5),GL2(5),GL2(5),GL2(5),GL2(5),GL2(5),
	GL2(6),GL2(6),GL2(6),GL2(6),GL2(6),GL2(6),GL2(6),GL2(6),
	GL2(6),GL2(6),GL2(6),GL2(6),GL2(6),GL2(6),GL2(6),GL2(6),
	GL2(6),GL2(6),GL2(6),GL2(6),GL2(6),GL2(6),GL2(6),GL2(6),
	GL2(6),GL2(6),GL2(6),GL2(6),GL2(6),GL2(6),GL2(6),GL2(6),
};

static void bitload(bitstream* p);

// result: [0..8190]
static NOINLINE int bitgolomb(bitstream* p) 
{
	int n,v;
	bitbegin_pos2(p);
	bitload_pos2(p);
	assertgolomb(bitshow_pos2(p,13)>0);
	if (bitshow_pos2(p,6))
		n = golomb_log2[bitshow_pos2(p,6)]-12;
	else
		n = golomb_log2[bitshow_pos2(p,12)];
	v = bitshow_pos2(p,n);
	bitflush_pos2(p,n);
	bitend_pos2(p);
	return --v;
}

// max: [2..7]
// result: [0..(1<<max)-2]+1
static INLINE int bitgolomb_max1(bitstream* p,int max) 
{
	int n;
	bitloadinline(p);
	assertgolomb(bitshow(p,max)>0);
	n = golomb_log2[bitshow(p,max-1)]-(13-max)*2;
	return bitget(p,n);
}

static NOINLINE int bitsgolomb(bitstream* p) 
{
	int n,v;
	bitbegin_pos2(p);
	bitload_pos2(p);
	assertgolomb(bitshow_pos2(p,13)>0);
	if (bitshow_pos2(p,6))
		n = golomb_log2[bitshow_pos2(p,6)]-12;
	else
		n = golomb_log2[bitshow_pos2(p,12)];
	v = bitshow_pos2(p,n);
	bitflush_pos2(p,n);
	bitend_pos2(p);
	if (v&1)
		v=-(v>>1);
	else
		v=(v>>1);
	return v;
}

#define DECLARE_BITSGOLOMB_LAGER \
static NOINLINE int bitsgolomb_large(bitstream* p) \
{ \
	int n; \
	uint32_t v; \
	bitloadinline(p); \
	if (bitshow(p,13)) \
		return bitsgolomb(p); \
	n = 14; \
	bitflush(p,13); \
	bitloadinline(p); \
	while (!bitshow(p,1)) \
	{ \
		bitflush(p,1); \
		++n; \
	} \
	v = bitshowlarge(p,n); \
	bitflush(p,n); \
	if (v&1) \
		return -(int)(v>>1); \
	else \
		return (v>>1); \
}

#define DECLARE_BITGOLOMB_LAGER \
static NOINLINE int bitgolomb_large(bitstream* p) \
{ \
	int n; \
	uint32_t v; \
	bitloadinline(p); \
	if (bitshow(p,13)) \
		return bitgolomb(p); \
	n = 14; \
	bitflush(p,13); \
	bitloadinline(p); \
	while (!bitshow(p,1)) \
	{ \
		bitflush(p,1); \
		++n; \
	} \
	v = bitshowlarge(p,n); \
	bitflush(p,n); \
	return --v; \
}

#endif
