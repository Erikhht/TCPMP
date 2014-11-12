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
 * $Id: flow.c 543 2006-01-07 22:06:24Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "common.h"

static const datatable FlowParams[] =
{
	{ FLOW_BUFFERED,TYPE_BOOL, DF_HIDDEN },

	DATATABLE_END(FLOW_CLASS)
};

int FlowEnum(void* p, int* No, datadef* Param)
{
	return NodeEnumTable(No,Param,FlowParams);
}

static const nodedef Flow =
{
	CF_ABSTRACT,
	FLOW_CLASS,
	NODE_CLASS,
	PRI_DEFAULT,
};

static const datatable OutParams[] =
{
	{ OUT_INPUT,	TYPE_PACKET, DF_INPUT },
	{ OUT_OUTPUT,	TYPE_PACKET, DF_OUTPUT|DF_RDONLY },
	{ OUT_TOTAL,	TYPE_INT,	 DF_HIDDEN },
	{ OUT_DROPPED,	TYPE_INT,	 DF_HIDDEN },
	{ OUT_KEEPALIVE,TYPE_BOOL,	 DF_HIDDEN },

	DATATABLE_END(OUT_CLASS)
};

int OutEnum(void* p, int* No, datadef* Param)
{
	return NodeEnumTable(No,Param,OutParams);
}

static const nodedef Out =
{
	CF_ABSTRACT,
	OUT_CLASS,
	FLOW_CLASS,
	PRI_DEFAULT,
};

void Flow_Init()
{
	NodeRegisterClass(&Flow);
	NodeRegisterClass(&Out);
}

void Flow_Done()
{
	NodeUnRegisterClass(OUT_CLASS);
	NodeUnRegisterClass(FLOW_CLASS);
}

void Disconnect(node* Src,int SrcNo,node* Dst,int DstNo)
{
	ConnectionUpdate(NULL,0,Dst,DstNo);
	ConnectionUpdate(Src,SrcNo,NULL,0);
}

int ConnectionUpdate(node* Src,int SrcNo,node* Dst,int DstNo)
{
	int Result;
	pin Pin;
	packetformat Format;
	packetprocess Process;

	memset(&Format,0,sizeof(Format));

	// first setup connection via Pin, so when PIN_FORMAT is set the nodes can negotiate
	// ConnectionUpdate should not disconnect nodes even is something fails

	if (Dst && Dst->Get(Dst,DstNo,&Pin,sizeof(Pin))==ERR_NONE && (Pin.Node != Src || Pin.No != SrcNo))
	{
		Pin.Node = Src;
		Pin.No = SrcNo;
		Result = Dst->Set(Dst,DstNo,&Pin,sizeof(Pin));
		if (Result != ERR_NONE)
			return Result;
	}

	if (Src && Src->Get(Src,SrcNo,&Pin,sizeof(Pin))==ERR_NONE && (Pin.Node != Dst || Pin.No != DstNo))
	{
		Pin.Node = Dst;
		Pin.No = DstNo;
		Result = Src->Set(Src,SrcNo,&Pin,sizeof(Pin));
		if (Result != ERR_NONE)
			return Result;
	}

	if (Src && Dst)
	{
		Src->Get(Src,SrcNo|PIN_FORMAT,&Format,sizeof(Format));
		Result = Dst->Set(Dst,DstNo|PIN_FORMAT,&Format,sizeof(Format));
		if (Result != ERR_NONE)
			Dst->Set(Dst,DstNo|PIN_FORMAT,NULL,0);

		Dst->Get(Dst,DstNo|PIN_PROCESS,&Process,sizeof(packetprocess));
		Src->Set(Src,SrcNo|PIN_PROCESS,&Process,sizeof(packetprocess));

		if (Result != ERR_NONE)
			return Result;
	}
	else
	if (Dst)
		Dst->Set(Dst,DstNo|PIN_FORMAT,NULL,0);

	return ERR_NONE;
}

int DummyProcess(void* p, const packet* Packet, const flowstate* State)
{
	if (State->CurrTime >= 0 && Packet && Packet->RefTime > State->CurrTime + SHOWAHEAD)
		return ERR_BUFFER_FULL;
	return ERR_NONE;
}

static bool_t ContentType(const packetformat* Format,tchar_t* Out,int OutLen)
{
	tchar_t Id[16];
	switch (Format->Type)
	{
	case PACKET_VIDEO:
		FourCCToString(Id,TSIZEOF(Id),Format->Format.Video.Pixel.FourCC);
		stprintf_s(Out,OutLen,T("vcodec/%s"),Id);
		break;
	case PACKET_AUDIO:
		stprintf_s(Out,OutLen,T("acodec/0x%04x"),Format->Format.Audio.Format);
		break;
	case PACKET_SUBTITLE:
		FourCCToString(Id,TSIZEOF(Id),Format->Format.Subtitle.FourCC);
		stprintf_s(Out,OutLen,T("subtitle/%s"),Id);
		break;
	default:
		return 0;
	}
	return 1;
}

void PacketFormatEnumClass(array* List, const packetformat* Format)
{
	tchar_t s[16];
	if (ContentType(Format,s,TSIZEOF(s)))
		NodeEnumClassEx(List,FLOW_CLASS,s,NULL,NULL,0);
	else
		memset(List,0,sizeof(array));
}

bool_t PacketFormatMatch(int Class, const packetformat* Format)
{
	tchar_t s[16];
	const tchar_t* Supported = LangStr(Class,NODE_CONTENTTYPE);
	assert(Supported[0]!=0);
	if (Supported[0]==0)
		return 0;
	if (!ContentType(Format,s,TSIZEOF(s)))
		return 0;
	return CheckContentType(s,Supported);
}

bool_t PacketFormatSimilarAudio(const packetformat* Current, const packetformat* New)
{
	return Current && Current->Type == PACKET_AUDIO && New && New->Type == PACKET_AUDIO &&
		Current->Format.Audio.Format == New->Format.Audio.Format &&
		Current->Format.Audio.Channels == New->Format.Audio.Channels &&
		Current->Format.Audio.SampleRate == New->Format.Audio.SampleRate;
}

bool_t PacketFormatRotatedVideo(const packetformat* Current, const packetformat* New,int Mask)
{
	if (Current && Current->Type == PACKET_VIDEO && New && New->Type == PACKET_VIDEO &&
		(Current->Format.Video.Direction ^ New->Format.Video.Direction) & Mask)
	{
		video Tmp = New->Format.Video;
		Tmp.Pitch = Current->Format.Video.Pitch;
		if ((Current->Format.Video.Direction ^ Tmp.Direction) & DIR_SWAPXY)
			SwapInt(&Tmp.Width,&Tmp.Height);
		Tmp.Direction = Current->Format.Video.Direction;
		return EqVideo(&Current->Format.Video,&Tmp);
	}
	return 0;
}

bool_t EqPacketFormat(const packetformat* a, const packetformat* b)
{
	if (!a || !b || a->Type != b->Type)
		return 0;

	switch (a->Type)
	{
	case PACKET_VIDEO: return EqVideo(&a->Format.Video,&b->Format.Video);
	case PACKET_AUDIO: return EqAudio(&a->Format.Audio,&b->Format.Audio);
	case PACKET_SUBTITLE: return EqSubtitle(&a->Format.Subtitle,&b->Format.Subtitle);
	}
	return 1;
}

int PacketFormatCopy(packetformat* Dst, const packetformat* Src)
{
	PacketFormatClear(Dst);
	if (Src)
	{
		*Dst = *Src;
		if (Src->ExtraLength >= 0)
		{
			Dst->Extra = NULL;
			Dst->ExtraLength = 0;
			if (Src->ExtraLength && PacketFormatExtra(Dst,Src->ExtraLength))
				memcpy(Dst->Extra,Src->Extra,Dst->ExtraLength);
			if (Src->Type==PACKET_VIDEO && Src->Format.Video.Pixel.Palette == (rgb*)Src->Extra)
				Dst->Format.Video.Pixel.Palette = (rgb*)Dst->Extra;
		}
	}
	return ERR_NONE;
}

void PacketFormatCombine(packetformat* Dst, const packetformat* Src)
{
	if (Dst->Type == Src->Type)
	{
		if (!Dst->ByteRate)
			Dst->ByteRate = Src->ByteRate;
		if (!Dst->PacketRate.Num)
			Dst->PacketRate = Src->PacketRate;

		switch (Dst->Type)
		{
		case PACKET_VIDEO:
			if (!Dst->Format.Video.Width && !Dst->Format.Video.Height)
			{
				Dst->Format.Video.Width = Src->Format.Video.Width;
				Dst->Format.Video.Height = Src->Format.Video.Height;
				Dst->Format.Video.Direction = Src->Format.Video.Direction;
			}
			if (!Dst->Format.Video.Aspect)
				Dst->Format.Video.Aspect = Src->Format.Video.Aspect;
			break;

		case PACKET_AUDIO:
			// force update
			Dst->Format.Audio.Channels = Src->Format.Audio.Channels;
			Dst->Format.Audio.SampleRate = Src->Format.Audio.SampleRate; 
			if (!Dst->Format.Audio.Bits)
			{
				Dst->Format.Audio.Bits = Src->Format.Audio.Bits;
				Dst->Format.Audio.FracBits = Src->Format.Audio.FracBits;
			}
			break;
		}
	}
}

void PacketFormatClear(packetformat* p)
{
	if (p->ExtraLength>=0)
		free(p->Extra);
	memset(p,0,sizeof(packetformat));
}

bool_t PacketFormatExtra(packetformat* p, int Length)
{
	if (Length<=0)
	{
		if (p->ExtraLength>=0)
			free(p->Extra);
		p->Extra = NULL;
		p->ExtraLength = 0;
		return 0;
	}
	else
	{
		void* Extra = realloc(p->Extra,Length);
		if (!Extra && Length)
			return 0;
		p->Extra = Extra;
		p->ExtraLength = Length;
		return 1;
	}
}

void PacketFormatPCM(packetformat* p, const packetformat* In, int Bits)
{
	PacketFormatClear(p);
	p->Type = PACKET_AUDIO;
	p->Format.Audio.Format = AUDIOFMT_PCM;
	p->Format.Audio.Bits = Bits;
	p->Format.Audio.SampleRate = In->Format.Audio.SampleRate;
	p->Format.Audio.Channels = In->Format.Audio.Channels;
	if (p->Format.Audio.Channels > 2)
		p->Format.Audio.Channels = 2;
	PacketFormatDefault(p);
}

void PacketFormatDefault(packetformat* p)
{
	switch (p->Type)
	{
	case PACKET_VIDEO:
		if (p->Format.Video.Pixel.FourCC==0) // DIB?
		{
			if (p->Format.Video.Pixel.BitCount <= 8)
				p->Format.Video.Pixel.Flags = PF_PALETTE;
			else
			switch (p->Format.Video.Pixel.BitCount)
			{
			case 16:
				DefaultRGB(&p->Format.Video.Pixel,p->Format.Video.Pixel.BitCount,5,5,5,0,0,0);
				break;
			case 24:
			case 32:
				DefaultRGB(&p->Format.Video.Pixel,p->Format.Video.Pixel.BitCount,8,8,8,0,0,0);
				break;
			}
			p->Format.Video.Direction = DIR_MIRRORUPDOWN;
		}
		else
		if (p->Format.Video.Pixel.FourCC==3)
		{
			p->Format.Video.Pixel.Flags = PF_RGB;
			p->Format.Video.Direction = DIR_MIRRORUPDOWN;
		}
		else
		{
			p->Format.Video.Pixel.Flags = PF_FOURCC;
			p->Format.Video.Pixel.FourCC = UpperFourCC(p->Format.Video.Pixel.FourCC);
			p->Format.Video.Direction = 0;
		}

		DefaultPitch(&p->Format.Video);

		if (p->Format.Video.Height<0)
		{
			p->Format.Video.Height = -p->Format.Video.Height;
			p->Format.Video.Direction ^= DIR_MIRRORUPDOWN;
		}
		break;

	case PACKET_AUDIO:
		// detect fake PCM
		if (p->Format.Audio.Format > 8192 && 
			p->Format.Audio.BlockAlign > 0 &&
			p->ByteRate > 0 &&
			p->Format.Audio.BlockAlign == ((p->Format.Audio.Channels * p->Format.Audio.Bits) >> 3) &&
			p->ByteRate == p->Format.Audio.SampleRate * p->Format.Audio.BlockAlign)
			p->Format.Audio.Format = AUDIOFMT_PCM;

		if (p->Format.Audio.Format == AUDIOFMT_PCM)
		{
			p->Format.Audio.FracBits = p->Format.Audio.Bits - 1;
			if (p->Format.Audio.Bits <= 8)
				p->Format.Audio.Flags |= PCM_UNSIGNED;
			p->Format.Audio.Bits = ALIGN8(p->Format.Audio.Bits);
			p->Format.Audio.BlockAlign = (p->Format.Audio.Channels * p->Format.Audio.Bits) >> 3;
			p->ByteRate = p->Format.Audio.SampleRate * p->Format.Audio.BlockAlign;
		}
		break;
	}
}	

bool_t PacketFormatName(packetformat* p, tchar_t* Name, int NameLen)
{
	tchar_t Id[8];
	switch (p->Type)
	{
	case PACKET_SUBTITLE:
		tcscpy_s(Name,NameLen,LangStr(p->Format.Subtitle.FourCC,0x4400));
		if (!Name[0])
			FourCCToString(Name,NameLen,p->Format.Subtitle.FourCC);
		return 1;
	case PACKET_AUDIO:
		if (p->Format.Audio.Format != AUDIOFMT_PCM)
		{
			stprintf_s(Id,TSIZEOF(Id),T("%04X"),p->Format.Audio.Format);
			tcscpy_s(Name,NameLen,LangStr(FOURCC(Id[0],Id[1],Id[2],Id[3]),0x4400));
			if (!Name[0])
				tcscpy_s(Name,NameLen,Id);
		}
		else
			tcscpy_s(Name,NameLen,T("PCM"));
		return 1;
	case PACKET_VIDEO:
		if (Compressed(&p->Format.Video.Pixel))
		{
			tcscpy_s(Name,NameLen,LangStr(p->Format.Video.Pixel.FourCC,0x4400));
			if (!Name[0])
				FourCCToString(Name,NameLen,p->Format.Video.Pixel.FourCC);
		}
		else
		if (AnyYUV(&p->Format.Video.Pixel))
			tcscpy_s(Name,NameLen,T("YUV"));
		else
			stprintf_s(Name,NameLen,T("RGB %d bits"),p->Format.Video.Pixel.BitCount);
		return 1;
	}
	return 0;
}

