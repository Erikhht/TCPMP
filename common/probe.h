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
 * $Id: probe.h 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __PORBE_H
#define __PORBE_H

// le32(ofs)
// be32(ofs)
// le16(ofs)
// be16(ofs)
// and(a,b,...)
// or(a,b,...)
// fwd(ofs)
// scan(limit,success,fail,step)
// eq(a,b)
// ne(a,b)
// 'abcd' fourccle
// 1234
// 0x1234

DLL bool_t DataProbe(const void* Data,int Length,const tchar_t* Code);

#endif
