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
 * $Id: tiff.c 313 2005-10-29 15:15:47Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/
 
#include "../common/common.h"
#include "../common/zlib/zlib.h"
#include "tiff.h"

DECLARE_BITINIT
DECLARE_BITLOAD

typedef struct tiff
{
	codec Codec;
	buffer Buffer;

	bool_t ErrorShowed;
	bool_t BigEndian;
	const uint8_t* Ptr;

	int BitCount;
	int BytesPerRow;
	int BytesPerRow2;
	int Planar;
	int Channels;
	int Left;
	bool_t Reverse;

} tiff;

static int UpdateInput(tiff* p)
{
	p->ErrorShowed = 0;
	BufferClear(&p->Buffer);
	if (p->Codec.In.Format.Type == PACKET_VIDEO)
	{
		PacketFormatCopy(&p->Codec.Out.Format,&p->Codec.In.Format);
		p->Codec.Out.Format.Format.Video.Pixel.Flags = PF_RGB;
		p->Codec.Out.Format.Format.Video.Pixel.BitCount = 24;
		p->Codec.Out.Format.Format.Video.Pixel.BitMask[0] = 0xFF;
		p->Codec.Out.Format.Format.Video.Pixel.BitMask[1] = 0xFF00;
		p->Codec.Out.Format.Format.Video.Pixel.BitMask[2] = 0xFF0000;
		DefaultPitch(&p->Codec.Out.Format.Format.Video);
	}
	return ERR_NONE;
}

static NOINLINE int Read32(tiff* p)
{
	int v;
	if (p->BigEndian)
		v = LOAD32BE(p->Ptr);
	else
		v = LOAD32LE(p->Ptr);
	p->Ptr += 4;
	return v;
}

static NOINLINE int Read16(tiff* p)
{
	int v;
	if (p->BigEndian)
		v = LOAD16BE(p->Ptr);
	else
		v = LOAD16LE(p->Ptr);
	p->Ptr += 2;
	return v;
}

static NOINLINE int ReadVal(tiff* p,int Type)
{
	switch (Type)
	{
	case 1: return *(p->Ptr++);
	case 3: return Read16(p);
	case 4: return Read32(p);
	}
	return 0;
}

static NOINLINE int NotSupported(tiff* p)
{
	if (!p->ErrorShowed)
	{
		p->ErrorShowed = 1;
		ShowError(p->Codec.Node.Class,TIFF_ID,TIFF_NOT_SUPPORTED);
	}
	return ERR_NOT_SUPPORTED;
}

static NOINLINE void Write(tiff* p,const uint8_t* Data,int Size)
{
	int n,i,j;
	if (p->Planar == 1)
	{
		while (Size>0)
		{
			if (!p->Left)
				p->Left = p->BytesPerRow;
			n = min(p->Left,Size);
			Size -= n;
			p->Left -= n;
			switch (p->BitCount)
			{
			case 1:
				j = n*8;
				if (!p->Left)
					j -= 8*p->BytesPerRow - p->BytesPerRow2;
				for (i=0;i<j;++i)
					p->Buffer.Data[p->Buffer.WritePos++] = (uint8_t)((Data[i>>3] >> (7-(i&7))) & 1);
				break;
			case 2:
				j = n*4;
				if (!p->Left)
					j -= 4*p->BytesPerRow - p->BytesPerRow2;
				for (i=0;i<j;++i)
					p->Buffer.Data[p->Buffer.WritePos++] = (uint8_t)((Data[i>>2] >> (6-2*(i&3))) & 3);
				break;
			case 4:
				j = n*2;
				if (!p->Left)
					j -= 2*p->BytesPerRow - p->BytesPerRow2;
				for (i=0;i<j;++i)
					p->Buffer.Data[p->Buffer.WritePos++] = (uint8_t)((Data[i>>1] >> (4-4*(i&1))) & 15);
				break;
			default:
				memcpy(p->Buffer.Data + p->Buffer.WritePos,Data,n);
				p->Buffer.WritePos += n;
				break;
			}
			Data += n;
			if (!p->Left)
				p->Buffer.WritePos += p->Codec.Out.Format.Format.Video.Pitch - p->BytesPerRow2;
		}
	}
	else
	{
		while (Size>0)
		{
			if (!p->Left)
				p->Left = p->BytesPerRow;
			n = min(p->Left,Size);
			Size -= n;
			p->Left -= n;
			while (--n>=0)
			{
				p->Buffer.Data[p->Buffer.WritePos] = *(++Data);
				p->Buffer.WritePos += p->Channels;
			}
			if (!p->Left)
				p->Buffer.WritePos += p->Codec.Out.Format.Format.Video.Pitch - p->BytesPerRow*p->Channels;
		}
	}
}

static void Inflate(tiff *p,const uint8_t* Data,int Size,int Total)
{
	int i,n;
	z_stream Inflate;
	uint8_t Buffer[4096];

	memset(&Inflate,0,sizeof(Inflate));
    if (inflateInit2(&Inflate, MAX_WBITS) == Z_OK)
	{
		Inflate.next_in = (Bytef*)Data;
		Inflate.avail_in = Size;
		
		do
		{
			Inflate.next_out = Buffer;
			Inflate.avail_out = sizeof(Buffer);

			i = inflate(&Inflate, Z_SYNC_FLUSH);

			n = sizeof(Buffer) - Inflate.avail_out;
			if (n>Total)
				n=Total;
			Write(p,Buffer,n);
			Total -= n;

		} while (i>=0 && i!=Z_STREAM_END && Total>0);

		inflateEnd(&Inflate);
	}
}

/*
static void Fax3(tiff *p,const uint8_t* Data,int Size,int Total)
{
	//todo
}
*/

static void LZW(tiff *p,const uint8_t* Data,int Size)
{
	bitstream bs;
	uint16_t Prefix[4096];
	uint8_t Suffix[4096];
	uint8_t Stack[4096];
	uint8_t FirstChar;
	int ClearCode = 256;
	int EOICode = ClearCode+1;
	int FreeCode = ClearCode+2;
	int OldCode = -1;
	int CodeSize = 9;
	int i;

	for (i=0;i<ClearCode;++i)
		Suffix[i] = (uint8_t)i;

	FirstChar = 0;
	bitinit(&bs,Data,Size);

	while (!biteof(&bs))
	{
		uint8_t* StackPtr;
		int Code;
		bitload(&bs);
		Code = bitget(&bs,CodeSize);

		if (Code == EOICode)
			break;

		if (Code == ClearCode)
		{
			CodeSize = 9;
			FreeCode = ClearCode + 2;
			OldCode = -1;
			continue;
		}

		if (Code > FreeCode)
			break;

		if (OldCode<0)
		{
			FirstChar = Suffix[Code];
			Write(p,&FirstChar,1);
			OldCode = Code;
			continue;
		}

		StackPtr = Stack+sizeof(Stack);	
		i = Code;

		if (i == FreeCode)
		{
			*(--StackPtr) = FirstChar;
			i = OldCode;
		}

		while (i>ClearCode)
		{
			*(--StackPtr) = Suffix[i];
			i = Prefix[i];
		}

		FirstChar = Suffix[i];
		*(--StackPtr) = FirstChar;

		Prefix[FreeCode] = (uint16_t)OldCode;
		Suffix[FreeCode] = FirstChar;
		if (FreeCode < 4096) ++FreeCode;

		if (CodeSize<12 && FreeCode == (1<<CodeSize)-1)
			++CodeSize;

		OldCode = Code;
		Write(p,StackPtr,Stack+sizeof(Stack)-StackPtr);
	}
}

static int Process(tiff* p, const packet* Packet, const flowstate* State)
{
	int n,Compress;
	size_t IFD;
	int RowPerStrip;
	video Video;
	const uint8_t *Strips;
	rgb Pal[256];
	int StripsType,StripsLen;
	int Bits[3];
	int BitCount;
	int h,ch;
	int i,j,k;

	if (!Packet)
		return ERR_NEED_MORE_DATA;

	p->Ptr = Packet->Data[0];

	if (Packet->Length<8)
		return ERR_INVALID_DATA;

	if (p->Ptr[0]=='M' && p->Ptr[1]=='M')
		p->BigEndian = 1;
	else
	if (p->Ptr[0]=='I' && p->Ptr[1]=='I')
		p->BigEndian = 0;
	else
		return ERR_INVALID_DATA;
	p->Ptr += 2;

	if (Read16(p) != 42)
		return ERR_INVALID_DATA;

	IFD = Read32(p);
	if (IFD<8 || IFD+2+4 >= Packet->Length)
		return ERR_INVALID_DATA;

	p->Reverse = 0;
	RowPerStrip = MAX_INT;
	memset(&Video,0,sizeof(Video));
	p->Planar = 1;
	p->Channels = 0;
	Compress = 1;
	Strips = NULL;
	StripsType = 0;
	StripsLen = 0;
	BitCount = 0;
	Video.Pixel.Flags = PF_RGB;
	Video.Aspect = ASPECT_ONE; //todo

	for (i=0;i<256;++i)
		Pal[i].v = CRGB(i,i,i);

	p->Ptr = (const uint8_t*)Packet->Data[0]+IFD;
	n = Read16(p);
	while (--n>=0)
	{
		const uint8_t* Save = p->Ptr+12;
		int Tag = Read16(p);
		int Type = Read16(p);
		int Len = Read32(p);
		size_t Pos = Read32(p);

		switch (Type)
		{
		case 1: 
		case 2: i=1; break;
		case 3: i=2; break;
		case 4: i=4; break;
		case 5: i=8; break;
		default: i=0; break;
		}

		i *= Len;
		if (i<=4)
			p->Ptr -= 4;
		else
		{
			if (Pos > Packet->Length-i)
				continue;
			p->Ptr = (const uint8_t*)Packet->Data[0]+Pos;
		}

		// read integer
		i = 0;
		if (Len==1) 
			switch (Type)
			{
			case 1: i=p->Ptr[0]; break;
			case 3: i=Read16(p); p->Ptr-=2; break;
			case 4: i=Read32(p); p->Ptr-=4; break;
			}


		switch (Tag)
		{
		case 0x100:
			Video.Width = i;
			break;

		case 0x101:
			Video.Height = i;
			break;

		case 0x102: 
			if (Len>3)
				return NotSupported(p);

			p->Channels = Len;
			BitCount = 0;
			for (i=0;i<Len;++i)
			{
				Bits[i] = ReadVal(p,Type);
				Video.Pixel.BitMask[i] = ((1 << Bits[i])-1) << BitCount;
				BitCount += Bits[i];
			}
			p->BitCount = BitCount;
			Video.Pixel.BitCount = BitCount;
			if (Video.Pixel.BitCount==1 ||
				Video.Pixel.BitCount==2 ||
				Video.Pixel.BitCount==4)
			{
				int n = 1<<BitCount;
				for (i=0;i<n;++i)
				{
					int j = (255*i)/(n-1);
					Pal[i].v = CRGB(j,j,j);
				}
				Video.Pixel.BitCount=8;
			}

			if (Video.Pixel.BitCount & 7) // only 1,2,4,8,16,24,32 bitdepth supported
				return NotSupported(p);
			break;

		case 0x103:
			Compress = i;
			break;

		case 0x106:
			switch (i)
			{
			case 0: // invert grayscale
				Video.Pixel.Flags = PF_GRAYSCALE|PF_PALETTE;
				for (i=0;i<256;++i)
					Pal[i].v ^= CRGB(255,255,255);
				break;
			case 1: // grayscale
				Video.Pixel.Flags = PF_GRAYSCALE|PF_PALETTE;
				break;
			case 2: // rgb
				Video.Pixel.Flags = PF_RGB;
				break;
			case 3: // palette
				Video.Pixel.Flags = PF_PALETTE;
				break;
			}
			break;

		case 0x10A: // fill order
			p->Reverse = i==2;
			break;

		case 0x111: // strip offsets
			Strips = p->Ptr;
			StripsType = Type;
			StripsLen = Len;
			break;

		case 0x116: 
			RowPerStrip = i; 
			break;

		case 0x11C:
			p->Planar = i;
			if (i != 1 && BitCount != p->Channels*8) // only 24,32 supported
				return NotSupported(p);
			break;

		case 0x15B:
			// jpeg tables
			break;

		case 0x140:
			Video.Pixel.Flags = PF_PALETTE;
			k = 1<<BitCount;
			if (Len >= 3*k)
				for (j=0;j<3;++j)
					for (i=0;i<k;++i)
						Pal[i].ch[j] = (uint8_t)(ReadVal(p,Type)>>8);
			break;
		}

		p->Ptr = Save;
	}

	if (!Strips || !Video.Width || !Video.Height || !p->Channels) 
		return ERR_INVALID_DATA; 

	if ((Video.Pixel.Flags & PF_PALETTE) && (p->Channels!=1 || Video.Pixel.BitCount!=8))
		return NotSupported(p);

	DefaultPitch(&Video);

	if (!EqVideo(&Video,&p->Codec.Out.Format.Format.Video))
	{
		if ((Video.Pixel.Flags & PF_PALETTE) && PacketFormatExtra(&p->Codec.Out.Format,sizeof(Pal)))
		{
			memcpy(p->Codec.Out.Format.Extra,Pal,sizeof(Pal));
			Video.Pixel.Palette = p->Codec.Out.Format.Extra;
		}
		p->Codec.In.Format.Format.Video.Width = Video.Width;
		p->Codec.In.Format.Format.Video.Height = Video.Height;
		p->Codec.Out.Format.Format.Video = Video;
		ConnectionUpdate(&p->Codec.Node,CODEC_OUTPUT,p->Codec.Out.Pin.Node,p->Codec.Out.Pin.No);
	}

	BufferDrop(&p->Buffer);
	if (!BufferAlloc(&p->Buffer,Video.Pitch*Video.Height,4096))
		return ERR_OUT_OF_MEMORY;

	p->Ptr = Strips;
	p->Left = 0;

	ch = 0;
	h = Video.Height;

	for (n=0;n<StripsLen && h>0;++n)
	{
		int Len,Pos,y;
		const uint8_t* Src;

		Pos = ReadVal(p,StripsType);
		Len = Packet->Length - Pos;
		if (Len <= 0)
			break;

		Src = (const uint8_t*)Packet->Data[0]+Pos;

		y = min(RowPerStrip,h);
		h -= y;

		if (p->Planar == 1)
		{
			p->BytesPerRow = ((BitCount*Video.Width + 7) >> 3);
			p->BytesPerRow2 = ((Video.Pixel.BitCount*Video.Width + 7) >> 3);
		}
		else
			p->BytesPerRow2 = p->BytesPerRow = ((Bits[ch] * Video.Width + 7) >> 3);

		switch (Compress)
		{
		case 1:
			Write(p,Src,p->BytesPerRow*y);
			break;

//		case 2: //rle
//			break;

//		case 3: //fax3
//			Fax3(p,Src,Len,p->BytesPerRow*y);
//			break;

//		case 4: //fax4
//			break;

		case 5:
			LZW(p,Src,Len);
			break; 

		case 8:
		case 32946: //zlib
			Inflate(p,Src,Len,p->BytesPerRow*y);
			break;

		default:
			return NotSupported(p);
		}

		if (p->Planar != 1 && h==0)
		{
			// next plane
			if (ch == p->Channels) break;
			h = Video.Height;
			p->Left = 0;
			p->Buffer.WritePos = ++ch;
		}
	}

	p->Codec.Packet.RefTime = Packet->RefTime;
	p->Codec.Packet.Length = p->Buffer.WritePos;
	p->Codec.Packet.Data[0] = p->Buffer.Data;
	return ERR_NONE;
}

static int Resend(tiff* p)
{
	int Result = ERR_INVALID_DATA;
	if (p->Buffer.WritePos)
	{
		packet Packet;
		flowstate State;

		State.CurrTime = TIME_RESEND;
		State.DropLevel = 0;

		memset(&Packet,0,sizeof(Packet));
		Packet.RefTime = TIME_UNKNOWN;
		Packet.Length = p->Buffer.WritePos;
		Packet.Data[0] = p->Buffer.Data;
		
		Result = p->Codec.Out.Process(p->Codec.Out.Pin.Node,&Packet,&State);
	}
	return Result;
}

static int Flush(tiff* p)
{
	BufferDrop(&p->Buffer);
	return ERR_NONE;
}

static int Create(tiff* p)
{
	p->Codec.Process = (packetprocess)Process;
	p->Codec.UpdateInput = (nodefunc)UpdateInput;
	p->Codec.ReSend = (nodefunc)Resend;
	p->Codec.Flush = (nodefunc)Flush;
	return ERR_NONE;
}

static const nodedef TIFF =
{
	sizeof(tiff),
	TIFF_ID,
	CODEC_CLASS,
	PRI_DEFAULT,
	(nodecreate)Create,
	NULL,
};

//-------------------------------------------------------------------------------------------

static const nodedef TIFFFile =
{
	0, // parent size
	TIFF_FILE_ID,
	RAWIMAGE_CLASS,
	PRI_DEFAULT,
};
//---------------------------------------------------------------------------------------------

void TIFF_Init()
{
	NodeRegisterClass(&TIFF);
	NodeRegisterClass(&TIFFFile);
}

void TIFF_Done()
{
	NodeUnRegisterClass(TIFF_ID);
	NodeUnRegisterClass(TIFF_FILE_ID);
}
