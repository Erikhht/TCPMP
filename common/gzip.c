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
 * $Id: gzip.c 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "common.h"
#undef ZLIB_DLL
#include "gzip.h"

#define ASCII_FLAG		0x01
#define HEAD_CRC		0x02
#define EXTRA_FIELD		0x04
#define ORIG_NAME		0x08
#define COMMENT			0x10
#define RESERVED		0xE0

void GZDone(gzreader* p)
{
	inflateEnd(&p->Inflate);
	BufferClear(&p->Buffer);
}

static bool_t GZLoad(gzreader* p)
{
	BufferStream(&p->Buffer,p->Stream);
	p->Inflate.next_in = p->Buffer.Data + p->Buffer.ReadPos;
	p->Inflate.avail_in = p->Buffer.WritePos - p->Buffer.ReadPos;
	return p->Inflate.avail_in > 0;
}

static int GZGetByte(gzreader* p)
{
	if (!p->Inflate.avail_in && !GZLoad(p))
		return -1;
	p->Buffer.ReadPos++;
	p->Inflate.avail_in--;
	return *(p->Inflate.next_in++);
}

bool_t GZInit(gzreader* p,stream* Stream)
{
	int Method,Flags,i;
	memset(p,0,sizeof(gzreader));
	p->Stream = Stream;
	BufferAlloc(&p->Buffer,4096,1);

	if (GZGetByte(p) != 0x1F || GZGetByte(p) != 0x8B)
	{
		BufferClear(&p->Buffer);
		return 0;
	}

	Method = GZGetByte(p);
	Flags = GZGetByte(p);
    if (Method != Z_DEFLATED || (Flags & RESERVED) != 0)
	{
		BufferClear(&p->Buffer);
		return 0;
	}

    for (i=0;i<6;++i)
		GZGetByte(p);

    if (Flags & EXTRA_FIELD) 
	{
        i = GZGetByte(p);
        i += GZGetByte(p) << 8;
		while (i-- && GZGetByte(p)!=-1);
    }

    if (Flags & ORIG_NAME)
		while ((i = GZGetByte(p)) != -1 && i != 0);

    if (Flags & COMMENT)
		while ((i = GZGetByte(p)) != -1 && i != 0);

    if (Flags & HEAD_CRC)
	{
		GZGetByte(p);
		GZGetByte(p);
	}

    return inflateInit2(&p->Inflate, -MAX_WBITS) == Z_OK;
}

int GZRead(gzreader* p,void* Data,int Length)
{
    p->Inflate.next_out = (Bytef*)Data;
    p->Inflate.avail_out = Length;

    while (p->Inflate.avail_out>0)
	{
		int i = inflate(&p->Inflate, Z_NO_FLUSH);
		p->Buffer.ReadPos = p->Buffer.WritePos - p->Inflate.avail_in;
		if (i==Z_STREAM_END)
		{
			Length -= p->Inflate.avail_out;
			break;
		}
		if (i<0)
			return -1;
		GZLoad(p);
	}

	return Length;
}
