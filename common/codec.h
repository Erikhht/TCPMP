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
 * $Id: codec.h 384 2005-12-12 07:28:12Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __CODEC_H
#define __CODEC_H

//-----------------------------------------------------------
// simple codec template to make life easier

#define CODEC_CLASS			FOURCC('C','O','D','C')

// input pin
#define CODEC_INPUT			0x100
// output pin
#define CODEC_OUTPUT		0x101
// output comments
#define CODEC_COMMENT		0x102

// process return code 
#define CODEC_NO_DROPPING	0x200

typedef struct codec
{
	node Node;
	nodefunc UpdateInput;
	nodefunc UpdateOutput;	// optional
	nodefunc Flush;			// optional
	packetprocess Process;	// optional
	nodefunc ReSend;		// optional

	struct
	{
		pin Pin;
		packetprocess Process;
		packetformat Format;

	} In;

	struct
	{
		pin Pin;
		packetprocess Process;
		packetformat Format;

	} Out;

	packet Packet;
	bool_t Pending;
	bool_t UseComment;
	bool_t NoHardDropping;
	pin Comment;
	pin NotSupported;

} codec;

void Codec_Init();
void Codec_Done();

DLL int CodecEnum(codec* p, int* No, datadef* Param);
DLL int CodecGet(codec* p, int No, void* Data, int Size);
DLL int CodecSet(codec* p, int No, const void* Data, int Size);

#endif
