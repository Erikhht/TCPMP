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
 * $Id: vlc.h 287 2005-10-07 17:49:26Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __VLC_H
#define __VLC_H

typedef uint16_t vlc;

// item: code[32] bits[16] value[16]
DLL bool_t vlcinit(vlc**, int* size, const void* code, bool_t code32, const uint16_t* data, int num, int escape, int bits);
DLL void vlcdone(vlc**,int* size);

#define vlcget2_pos(code, table, stream, bitpos, tmp, mask, bits)	\
{													\
	bitload_pos(stream,bitpos);						\
	code = table[bitshow_pos(stream,bitpos,bits)];	\
	tmp = code >> 12;								\
	if (tmp >= 12)									\
	{												\
		bitflush_pos(stream,bitpos,bits);			\
		bitload_pos(stream,bitpos);					\
		tmp = (code >> 10) & 15;					\
		code = (code & 1023) << 3;					\
													\
		code = table[code+bitshow_pos(stream,bitpos,tmp)];\
		assert((code >> 12) < 12);					\
		tmp = code >> 12;							\
	}												\
													\
	bitflush_pos(stream,bitpos,tmp);				\
	code &= mask;									\
}

#define vlcget3_pos(code, table, stream, bitpos, tmp, mask, bits)	\
{													\
	bitload_pos(stream,bitpos);						\
	code = table[bitshow_pos(stream,bitpos,bits)];	\
	tmp = code >> 12;								\
	if (tmp >= 12)									\
	{												\
		const vlc *_table;							\
		bitflush_pos(stream,bitpos,bits);			\
		bitload_pos(stream,bitpos);					\
		tmp = (code >> 10) & 15;					\
		_table = table;								\
		_table += (code & 1023) << 3;				\
													\
		code = _table[bitshow_pos(stream,bitpos,tmp)];\
		if ((code >> 12) >= 12)						\
		{											\
			bitflush_pos(stream,bitpos,tmp);		\
			bitload_pos(stream,bitpos);				\
			tmp = (code >> 10) & 15;				\
			_table += (code & 1023) << 3;			\
													\
			code = _table[bitshow_pos(stream,bitpos,tmp)];\
			assert((code >> 12) < 12);				\
		}											\
		tmp = code >> 12;							\
	}												\
													\
	bitflush_pos(stream,bitpos,tmp);				\
	code &= mask;									\
}

#define DECLARE_VLCGET3(BITS)							\
static int vlcget3(const vlc* p, bitstream* stream)		\
{														\
	int n,code;											\
	int bits = BITS;									\
														\
	code = p[bitshow(stream,bits)];						\
	n = code >> 12;										\
	if (n >= 12)										\
	{													\
		bitflush(stream,bits);							\
		bits = (code >> 10) & 15;						\
		p += (code & 1023) << 3;						\
														\
		code = p[bitshow(stream,bits)];					\
		n = code >> 12;									\
		if (n >= 12)									\
		{												\
			bitflush(stream,bits);						\
			bits = (code >> 10) & 15;					\
			p += (code & 1023) << 3;					\
														\
			bitload(stream);							\
														\
			code = p[bitshow(stream,bits)];				\
			n = code >> 12;								\
			assert(n < 12);								\
		}												\
	}													\
														\
	bitflush(stream,n);									\
	return code & 0xFFF;								\
}

#define DECLARE_VLCGET2(BITS)							\
static int vlcget2(const vlc* p, bitstream* stream)		\
{														\
	int n,code;											\
	int bits = BITS;									\
														\
	code = p[bitshow(stream,bits)];						\
	n = code >> 12;										\
	if (n >= 12)										\
	{													\
		bitflush(stream,bits);							\
		bits = (code >> 10) & 15;						\
		p += (code & 1023) << 3;						\
														\
		code = p[bitshow(stream,bits)];					\
		n = code >> 12;									\
		assert(n < 12);									\
	}													\
														\
	bitflush(stream,n);									\
	return code & 0xFFF;								\
}

#endif
