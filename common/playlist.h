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
 * $Id: playlist.h 346 2005-11-21 22:20:40Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __PLAYLIST_H
#define __PLAYLIST_H

//---------------------------------------------------------------
// playlist params

#define PLAYLIST_CLASS			FOURCC('F','M','T','L')

// input/output stream (stream*)
#define PLAYLIST_STREAM			0x100
#define PLAYLIST_DATAFEED		0x101

// read playlist item
typedef int (*playreadlist)(void* This,tchar_t* Path,int PathLen,tchar_t* DispName,int DispNameLen,tick_t* Length);
// write playlist item
typedef int (*playwritelist)(void* This,const tchar_t* Path,const tchar_t* DispName,tick_t Length);

typedef struct playlist
{
	VMT_NODE
	playreadlist	ReadList;
	playwritelist	WriteList;

	//---------------------------
	//private

	nodefunc		UpdateStream;
	stream*			Stream;
	parser			Parser;
	int				No;

} playlist;

void Playlist_Init();
void Playlist_Done();

#endif
