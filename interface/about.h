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
 * $Id: about.h 325 2005-11-03 21:22:28Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __ABOUT_H
#define __ABOUT_H

extern void About_Init();
extern void About_Done();

#define ABOUT_ID			FOURCC('A','B','O','U')

//strings
#define ABOUT_TITLE			0xC8
#define ABOUT_HEAD			0xC9
#define ABOUT_LICENSE		0xCB
#define ABOUT_LIBSUSED		0xCD
#define ABOUT_LIBS			0xCE
#define ABOUT_DUMP			0xCC
#define ABOUT_THANKS		0xCF
#define ABOUT_THANKSLIBS	0xD0
#define ABOUT_FORUM			0xD1
#define ABOUT_TRANSLATION	0xD2
#define ABOUT_AUTHORS		0xD3
#define ABOUT_COPYRIGHT		0xD4
#define ABOUT_OPTIONS		0xD5
#define ABOUT_LICENSE2		0xD6

#endif
