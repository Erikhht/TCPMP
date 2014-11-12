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
 * $Id: flow.h 543 2006-01-07 22:06:24Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __FLOW_H
#define __FLOW_H

#define SHOWAHEAD	(TICKSPERSEC/25)

//--------------------------------------------
// flow objects have input/output packet pins 
// from which a graph can be built

// special flag for get/set format structure (packetformat)
#define PIN_FORMAT		0x10000
// special flag for get/set process function (packetprocess)
#define PIN_PROCESS		0x20000

// format not test
#define PACKET_NONE			0
// video (packet based)
#define PACKET_VIDEO		1
// audio (packet based)
#define PACKET_AUDIO		2
// subtitle (packet based)
#define PACKET_SUBTITLE		3

#define PACKET_MAX			4

typedef struct packetformat
{
	int Type;

	int ByteRate;
	fraction PacketRate;

	void* Extra;
	int ExtraLength;

	union
	{
		video Video;
		audio Audio;
		subtitle Subtitle;

	} Format;

} packetformat;

#define TIME_UNKNOWN	-1
#define TIME_RESEND		-2
#define TIME_BENCH		-3
#define TIME_SYNC		-4

typedef struct flowstate
{
	int DropLevel;
	tick_t CurrTime;

} flowstate;

typedef struct packet
{
	constplanes Data;
	constplanes LastData;	// optional (for video it may increase performance)
	size_t Length;			// length of data (in bytes, per planes, not filled for surfaces)
	tick_t RefTime;			// packet time (-1 if unknown)
	bool_t Key;			

} packet;

typedef int (*packetprocess)(void* This, const packet* Data, const flowstate* State);

DLL void Disconnect(node* Src,int SrcNo,node* Dst,int DstNo);
DLL int ConnectionUpdate(node* Src,int SrcNo,node* Dst,int DstNo);
DLL int DummyProcess(void*,const packet*,const flowstate*);

DLL void PacketFormatEnumClass(array* List,const packetformat* Format);
DLL bool_t PacketFormatMatch(int Class, const packetformat*);
DLL int PacketFormatCopy(packetformat* Dst,const packetformat* Src);
DLL void PacketFormatClear(packetformat*);
DLL void PacketFormatCombine(packetformat* Dst, const packetformat* Src);
DLL bool_t PacketFormatExtra(packetformat* p, int Length);
DLL void PacketFormatDefault(packetformat* p);
DLL bool_t PacketFormatRotatedVideo(const packetformat* Current, const packetformat* New,int Mask);
DLL bool_t PacketFormatSimilarAudio(const packetformat* Current, const packetformat* New);
DLL void PacketFormatPCM(packetformat* p, const packetformat* In, int Bit);
DLL bool_t PacketFormatName(packetformat* p, tchar_t* Name, int NameLen);
DLL bool_t EqPacketFormat(const packetformat* a, const packetformat* b);

//---------------------------------------------------------------
// packet flow (abstract)

#define FLOW_CLASS				FOURCC('F','L','O','W')
// buffered output (bool_t)
#define FLOW_BUFFERED			0x15
// resend buffered output (void)
#define FLOW_RESEND				0x16
// empty buffers (void)
#define FLOW_FLUSH				0x17
// event for not supported streams (pin)
#define FLOW_NOT_SUPPORTED		0x18
// release resources (bool)
#define FLOW_BACKGROUND			0x19

extern DLL int FlowEnum(void*, int* EnumNo, datadef* Out);

//---------------------------------------------------------------
// output flow (abstract)

#define OUT_CLASS				FOURCC('O','U','T','P')

#define OUT_INPUT				0x20
#define OUT_OUTPUT				0x21
#define OUT_TOTAL				0x22
#define OUT_DROPPED				0x23
#define OUT_KEEPALIVE			0x24

extern DLL int OutEnum(void*, int* EnumNo, datadef* Out);

extern void Flow_Init();
extern void Flow_Done();

#endif
