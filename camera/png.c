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
 * $Id: png.c 313 2005-10-29 15:15:47Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/
 
#include "../common/common.h"
#include "../common/zlib/zlib.h"
#include "png.h"

typedef struct png
{
	codec Codec;
	buffer Buffer;
	bool_t ErrorShowed;

	const uint8_t* Ptr;

} png;

static int UpdateInput(png* p)
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

static NOINLINE int NotSupported(png* p)
{
	if (!p->ErrorShowed)
	{
		p->ErrorShowed = 1;
		ShowError(p->Codec.Node.Class,PNG_ID,PNG_NOT_SUPPORTED);
	}
	return ERR_NOT_SUPPORTED;
}

static NOINLINE int Read32(png* p)
{
	int v = LOAD32BE(p->Ptr);
	p->Ptr += 4;
	return v;

}

static int Predictor(int Left,int Up,int Corner)
{
	int i = Left+Up-Corner;
	int a = abs(i-Left);
	int b = abs(i-Up);
	int c = abs(i-Corner);
	return (a<=b && a<=c)?Left:((b<=c)?Up:Corner);
}

static void Filter(int Mode,const uint8_t* Src,const uint8_t* DstLast,uint8_t* Dst,int Ch,int BytesPerRow)
{
	const uint8_t* DstPrev;
	const uint8_t* DstLastPrev;
	int i;

	DstLastPrev = DstLast;
	DstPrev = Dst;

	if (!DstLast)
	{
		if (Mode==2) Mode=0;
		if (Mode==4) Mode=1;
	}

	switch (Mode)
	{
	case 1: // subtraction filter
		for (i=0;i<Ch;++i)
			*(Dst++) = *(Src++);

		for (BytesPerRow-=Ch;BytesPerRow>0;--BytesPerRow)
			*(Dst++) = (uint8_t)(*(Src++) + *(DstPrev++));

		break;

	case 2: // up filter
		for (;BytesPerRow>0;--BytesPerRow)
			*(Dst++) = (uint8_t)(*(Src++) + *(DstLast++));
		break;

	case 3: // average filter
		if (DstLast)
		{
			for (i=0;i<Ch;++i)
				*(Dst++) = (uint8_t)(*(Src++) + (*(DstLast++)>>1));

			for (BytesPerRow-=Ch;BytesPerRow>0;--BytesPerRow)
				*(Dst++) = (uint8_t)(*(Src++) + ((*(DstPrev++)+*(DstLast++))>>1));
		}
		else
		{
			for (i=0;i<Ch;++i)
				*(Dst++) = *(Src++);

			for (BytesPerRow-=Ch;BytesPerRow>0;--BytesPerRow)
				*(Dst++) = (uint8_t)(*(Src++) + (*(DstPrev++)>>1));
		}
		break
			;
	case 4: // paeth prediction
		for (i=0;i<Ch;++i)
			*(Dst++) = (uint8_t)(*(Src++) + Predictor(0,*(DstLast++),0));

		for (BytesPerRow-=Ch;BytesPerRow>0;--BytesPerRow)
			*(Dst++) = (uint8_t)(*(Src++) + Predictor(*(DstPrev++),*(DstLast++),*(DstLastPrev++)));

		break;

	default: // copy
		memmove(Dst,Src,BytesPerRow);
		break;
	}
}

static int Process(png* p, const packet* Packet, const flowstate* State)
{
	static const uint8_t Magic[8] = {0x89,0x50,0x4E,0x47,0x0D,0x0A,0x1A,0x0A};
	rgb Pal[256];
	const uint8_t* PtrEnd;
	uint8_t* LastRow = NULL;
	int Ch = 0;
	video Video;
	int BytesPerRow=0,Left=0,Row=0;
	z_stream Inflate;

	if (!Packet)
		return ERR_NEED_MORE_DATA;

	p->Ptr = Packet->Data[0];
	PtrEnd = p->Ptr + Packet->Length;

	if (Packet->Length<16 || memcmp(p->Ptr,Magic,8)!=0)
		return ERR_INVALID_DATA;
	p->Ptr += 8;

	memset(&Video,0,sizeof(Video));
	BufferDrop(&p->Buffer);

	while (p->Ptr+8<=PtrEnd)
	{
		int i;
		int Len = Read32(p);
		int Id = Read32(p);
		const uint8_t* Next = p->Ptr+Len+4; //dword CRC
		if (Next > PtrEnd)
			break;

		switch (Id)
		{
		case FOURCCBE('I','H','D','R'):
			if (Len<13)
				return ERR_INVALID_DATA;

			Video.Width = Read32(p);
			Video.Height = Read32(p);
			Video.Pixel.BitCount = p->Ptr[0];
			Video.Aspect = ASPECT_ONE; //todo

			if (p->Ptr[2] != 0 || // compression
				p->Ptr[3] != 0 || // filter mode
				p->Ptr[4] != 0)   // interlaced
				return NotSupported(p);

			if (Video.Pixel.BitCount != 8)
				return NotSupported(p);

			if (p->Ptr[1] & 1)
			{
				if (p->Ptr[1] & 4)
					return NotSupported(p); //todo

				Video.Pixel.Flags |= PF_PALETTE;
				Ch = 1;
			}
			else
			if (p->Ptr[1] & 2)
			{
				Video.Pixel.Flags |= PF_RGB;
				Video.Pixel.BitMask[0] = 0xFF;
				Video.Pixel.BitMask[1] = 0xFF00;
				Video.Pixel.BitMask[2] = 0xFF0000;

				Ch = 3;
				if (p->Ptr[1] & 4)
					++Ch; // alpha
			}
			else
			{
				if (p->Ptr[1] & 4)
					return NotSupported(p); //todo

				Video.Pixel.Flags |= PF_PALETTE|PF_GRAYSCALE;
				Ch = 1;
			}

			for (i=0;i<256;++i)
				Pal[i].v = CRGB(i,i,i);
			
			BytesPerRow = Ch * ((Video.Width * Video.Pixel.BitCount)>>3)+1;
			Left = BytesPerRow;
			Row = Video.Height;
			LastRow = NULL;

			Video.Pixel.BitCount *= Ch;
			DefaultPitch(&Video);

			if (!BufferAlloc(&p->Buffer,Video.Pitch*Video.Height,4096))
				return ERR_OUT_OF_MEMORY;

			memset(&Inflate,0,sizeof(Inflate));
		    if (inflateInit2(&Inflate, MAX_WBITS) != Z_OK)
				return ERR_OUT_OF_MEMORY;

			break;

		case FOURCCBE('P','L','T','E'):
			for (i=0;i<256 && Len>=3;++i,Len-=3,p->Ptr+=3)
			{
				Pal[i].c.r = p->Ptr[0];
				Pal[i].c.g = p->Ptr[1];
				Pal[i].c.b = p->Ptr[2];
			}
			break;

		case FOURCCBE('I','D','A','T'):

			if (!BytesPerRow || Row<=0)
				break;

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

			Inflate.next_in = (Bytef*)p->Ptr;
			Inflate.avail_in = Len;
			Inflate.next_out = p->Buffer.Data + p->Buffer.WritePos;
			Inflate.avail_out = Left;
			
			do
			{
				if (Inflate.avail_out<=0)
				{
					uint8_t* CurrRow = p->Buffer.Data + p->Buffer.WritePos - BytesPerRow;
					Filter(CurrRow[0],CurrRow+1,LastRow,CurrRow,Ch,BytesPerRow-1);
					LastRow = CurrRow;

					p->Buffer.WritePos += Video.Pitch - BytesPerRow;
					Inflate.next_out += Video.Pitch - BytesPerRow;
					Inflate.avail_out = Left = BytesPerRow;
					if (--Row<=0)
						break;
				}

				if (Inflate.avail_in<=0)
					break;
				i = inflate(&Inflate, Z_SYNC_FLUSH);

				p->Buffer.WritePos += Left - Inflate.avail_out;
				Left = Inflate.avail_out;

				if (i==Z_STREAM_END)
					Inflate.avail_in = 0;

			} while (i>=0);

			break;

		case FOURCCBE('I','E','N','D'):
			Next = PtrEnd;
			break;
		}

		p->Ptr = Next;
	}

	if (BytesPerRow)
		inflateEnd(&Inflate);

	if (p->Buffer.WritePos<=0)
		return ERR_INVALID_DATA;

	p->Codec.Packet.RefTime = Packet->RefTime;
	p->Codec.Packet.Length = p->Buffer.WritePos;
	p->Codec.Packet.Data[0] = p->Buffer.Data;
	return ERR_NONE;
}

static int Resend(png* p)
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

static int Flush(png* p)
{
	BufferDrop(&p->Buffer);
	return ERR_NONE;
}

static int Create(png* p)
{
	p->Codec.Process = (packetprocess)Process;
	p->Codec.UpdateInput = (nodefunc)UpdateInput;
	p->Codec.ReSend = (nodefunc)Resend;
	p->Codec.Flush = (nodefunc)Flush;
	return ERR_NONE;
}

static const nodedef PNG =
{
	sizeof(png),
	PNG_ID,
	CODEC_CLASS,
	PRI_DEFAULT,
	(nodecreate)Create,
	NULL,
};

//-------------------------------------------------------------------------------------------

static const nodedef PNGFile =
{
	0, // parent size
	PNG_FILE_ID,
	RAWIMAGE_CLASS,
	PRI_DEFAULT,
};
//---------------------------------------------------------------------------------------------

void PNG_Init()
{
	NodeRegisterClass(&PNG);
	NodeRegisterClass(&PNGFile);
}

void PNG_Done()
{
	NodeUnRegisterClass(PNG_ID);
	NodeUnRegisterClass(PNG_FILE_ID);
}


