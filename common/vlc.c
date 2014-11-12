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
 * $Id: vlc.c 287 2005-10-07 17:49:26Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "common.h"

// 16bit vlc table
// 
// data = 4bit bits | 12bit data
// subtable = 1|1|4bit tabbits|10bit offset*8

typedef struct buildstate
{
	vlc* table;
	int sizealign;
	int size;
	int pos;
	int escape;
	const uint8_t* code;
	bool_t code32;
	const uint16_t* data;
	int count;

} buildstate;

static bool_t buildvlc(buildstate* v, int tabbits, int prefix, int prefixbits)
{
	int data,code,bits,i,j,n;
	uint16_t* p;
	int size = 1 << tabbits;
	int pos = v->pos;
	v->pos += size;

	if (v->pos > v->size)
	{
		// realloc table
		int size = (v->pos + v->sizealign) & ~v->sizealign; 
		uint16_t* table;
		table = (uint16_t*) realloc(v->table,size*sizeof(uint16_t));
		if (!table)
			return 0;
		//memset(table+v->pos,0xFE,(size-v->pos)*sizeof(uint16_t)); //mark unused
		v->size = size;
		v->table = table;
	}

	p = v->table + pos;
	for (i=0;i<size;++i)
		p[i] = (uint16_t)v->escape;

	for (i=0;i<v->count;++i)
	{
		if (v->code32)
		{
			code = ((uint32_t*)v->code)[i];
			bits = code & 31;
			code >>= 5;
		}
		else
		{
			code = ((uint16_t*)v->code)[i];
			bits = v->data[i] >> 12;
		}

		if (v->data)
			data = v->data[i] & 0xFFF;
		else
			data = i;

		bits -= prefixbits;
        if (bits > 0 && (code >> bits) == prefix) 
		{
            if (bits <= tabbits) 
			{
				// store data and bits

                j = (code << (tabbits - bits)) & (size - 1);
                n = j+(1 << (tabbits - bits));
                for (;j<n;j++) 
					p[j] = (uint16_t)((bits << 12) | data);
            } 
			else 
			{
				// subtable

                bits -= tabbits;
                j = (code >> bits) & (size - 1);
				if (p[j] == v->escape)
					n = bits;
				else
				{
					n = p[j] & 31;
					if (bits > n)
						n = bits;
				}
                p[j] = (uint16_t)(0xC000 | n);
            }
        }
	}
	
	// create subtables

	for (i=0;i<size;++i)
		if ((p[i] >> 12) >= 12)
		{
			bits = p[i] & 31;
			if (bits > tabbits)
				bits = tabbits;

			j = v->pos - pos;

			// align to 8*offset
			if (j & 7)
			{
				n = 8 - (j & 7);
				v->pos += n;
				j += n;
			}

			assert(j<1024*8);

			p[i] = (uint16_t)(0xC000 | (bits << 10) | (j >> 3));

			if (!buildvlc(v,bits,(prefix << tabbits)|i, prefixbits+tabbits))
				return 0;

			p = v->table + pos;
		}

	return 1;
}

bool_t vlcinit(vlc** table, int* size, const void* code, bool_t code32, const uint16_t* data, int num, int escape, int bits)
{
	bool_t Result;
	buildstate v;
	v.table = *table;
	v.sizealign = (1<<bits)-1;
	v.size = size?*size:0;
	v.pos = 0;
	v.code = (const uint8_t*)code;
	v.data = data;
	v.code32 = code32;
	v.count = num;
	v.escape = escape & 0xFFF;

	Result = buildvlc(&v,bits,0,0);
	*table = v.table;
	if (size) *size = v.size;
	return Result;
}

void vlcdone(vlc** v,int *size)
{
	free(*v); 
	*v = NULL;
	if (size) 
		*size = 0;
}

