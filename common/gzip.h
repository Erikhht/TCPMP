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
 * $Id: gzip.h 543 2006-01-07 22:06:24Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __GZIP_H
#define __GZIP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "zlib/zlib.h"

typedef struct gzreader
{
	stream* Stream;
	buffer Buffer;
	z_stream Inflate;

} gzreader;

typedef struct tarhead
{
	char Name[100];
	char mode[8];
	char uid[8];
	char gid[8];
	char size[12];
	char mtime[12];
	char chksum[8];
	char link;
	char linkname[100];
	char unused[512-100-8-8-8-12-12-8-1-100];

} tarhead;

bool_t GZInit(gzreader* p,stream*);
int GZRead(gzreader* p,void* Data,int Length);
void GZDone(gzreader* p);

#ifdef __cplusplus
}
#endif

#endif
