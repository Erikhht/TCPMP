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
 * $Id: mjpeg.c 603 2006-01-19 13:00:33Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/
 
#include "../common/common.h"
#include "mjpeg.h"

#define WEBMJEG_READSIZE	2048
#define VLC_BITS 9

typedef struct blockindex
{
	uint8_t quant;
	uint8_t comp;
	uint8_t ac;
	uint8_t dc;
} blockindex;

typedef struct mjpeg 
{
	codecidct Codec;

	bool_t error;
	int sos_no;
	int restart_interval;
	int restart_count;
	idct_block_t* blockptr;

	// bitstream
	bitstream s;
	vlc* v[2][4];
	int vsize[2][4];
    uint8_t quant[4][64];

	int hblock[4];
	int vblock[4];
	int compid[4];
	int quantidx[4];

	int Ss;
	int Se;
	int Al;
	int Ah;

	int blocks;
	int mb_width;
	int mb_height;
	int last_dc[4];
	int comp;
	bool_t progressive;
	int startstate;

	blockindex *indexend;
	blockindex index[4*2*2];
	uint8_t zigzag[64];
	IDCT_BLOCK_DECLARE

} mjpeg;

static const uint8_t zigzag[64] = 
{
	 0,  1,  8, 16,  9,  2,  3, 10, 
	17,	24, 32, 25, 18, 11,  4,  5,
	12, 19, 26, 33, 40, 48, 41, 34, 
	27,	20, 13,  6,  7, 14, 21, 28, 
	35, 42,	49, 56, 57, 50, 43, 36, 
	29, 22,	15, 23, 30, 37, 44, 51, 
	58, 59, 52, 45, 38, 31, 39, 46, 
	53, 60, 61,	54, 47, 55, 62, 63
};

static const uint8_t dc_luminance[] =
{ 
	0, 0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
};
static const uint8_t dc_chrominance[] =
{ 
	0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 
};
static const uint8_t ac_luminance[] =
{ 
	0, 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 125, 
	0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
	0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
	0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
	0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
	0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
	0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
	0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
	0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
	0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
	0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
	0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
	0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
	0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
	0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
	0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
	0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
	0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
	0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
	0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
	0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
	0xf9, 0xfa 
};

static const uint8_t ac_chrominance[] =
{ 
	0, 0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 119,
	0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
	0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
	0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
	0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
	0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
	0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
	0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
	0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
	0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
	0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
	0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
	0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
	0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
	0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
	0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
	0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
	0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
	0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
	0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
	0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
	0xf9, 0xfa 
};

static const uint8_t* const default_dht[4] =
{
	dc_luminance,
	dc_chrominance,
	ac_luminance,
	ac_chrominance
};

DECLARE_BITINIT
DECLARE_BITLOAD

#undef bitload_pos
#define bitload_pos(p, bitpos)					\
	bitpos -= 8;								\
	if (bitpos >= 0) {							\
		int bits = (p)->bits;					\
		const uint8_t* bitptr = (p)->bitptr;	\
		do {									\
			bits = (bits << 8) | *bitptr;		\
			if (*bitptr++ == 0xFF)				\
			{									\
				while (*bitptr==0xFF &&	bitptr<(p)->bitend) \
					bitptr++;					\
				if (*bitptr==0)					\
					++bitptr;					\
			}									\
			bitpos -= 8;						\
		} while (bitpos>=0);					\
		(p)->bits = bits;						\
		(p)->bitptr = bitptr;					\
	}											\
	bitpos += 8;


static NOINLINE int NotSupported(mjpeg* p)
{
	if (!p->error)
	{
		p->error = 1;
		ShowError(p->Codec.Node.Class,MJPEG_ID,MJPEG_NOT_SUPPORTED);
	}
	return ERR_NOT_SUPPORTED;
}

static NOINLINE bool_t VLC(vlc** tab, int* size, const uint8_t *data) 
{
	uint32_t huffcode[256];
	uint16_t huffdata[256];
	int i,j,k,n,code;

	code = 0;
	k = 0;
	for (i=1;i<=16;i++) 
	{
		n = data[i];
		for (j=0;j<n;j++) 
		{
			huffcode[k] = ((code++)<<5)|i;
			huffdata[k] = data[k+17];
			++k;
		}
		code <<= 1;
	}

    return vlcinit(tab,size,huffcode,4,huffdata,k,0,VLC_BITS);
}

static int DQT(mjpeg* p)
{
    int len = bitget(&p->s,16) - 2;
    while (len >= 65 && !biteof(&p->s)) 
	{
		int comp,i;

		bitload(&p->s);
        if (bitget(&p->s,4) != 0)
			return NotSupported(p); // 16bit prec

        comp = bitget(&p->s, 4);
        if (comp >= 4)
            return ERR_INVALID_DATA;

        for (i=0;i<64;i++) 
		{
			bitload(&p->s);
			p->quant[comp][i] = (uint8_t)bitget(&p->s,8);
		}

        len -= 65;
    }
    
    return ERR_NONE;
}

static int DHT(mjpeg* p)
{
    uint8_t data[17+256];
    int len = bitget(&p->s,16)-2;

    while (len>=17 && !biteof(&p->s)) 
	{
		int n,i;
		int comp,idx;

		bitload(&p->s);
        comp = bitget(&p->s,4);
        if (comp >= 2)
			break;

        idx = bitget(&p->s,4);
        if (idx >= 4)
			break;

        n = 0;
        for (i=1;i<=16;i++) 
		{
			bitload(&p->s);
            data[i] = (uint8_t) bitget(&p->s,8);
            n += data[i];
        }

        len -= 17+n;
        if (len<0 || n>256)
			break;

        for (i=0;i<n;i++) 
		{
			bitload(&p->s);
            data[17+i] = (uint8_t) bitget(&p->s,8);
		}

		if (!VLC(&p->v[comp][idx],&p->vsize[comp][idx],data))
			return ERR_NONE;
    }
	return ERR_NONE;
}

static int DRI(mjpeg* p)
{
    if (bitget(&p->s,16) == 4)
	{
		bitload(&p->s);
	    p->restart_interval = bitget(&p->s,16);
	}
	return ERR_NONE;
}

static int SOF(mjpeg* p,bool_t progressive)
{
    int len,i,width,height,mode;

	p->progressive = progressive;

    len = bitget(&p->s,16);

	bitload(&p->s);
    if (bitget(&p->s,8) != 8) // 8bit/component
		return NotSupported(p);
    
    height = bitget(&p->s,16);
	bitload(&p->s);
    width = bitget(&p->s,16);
    p->comp = bitget(&p->s,8);

    if (p->comp != 3 && p->comp != 1) // 3 or 1 components
		return NotSupported(p);

	if (len != 8+3*p->comp)
		return ERR_INVALID_DATA;

	p->blocks = 0;
    for (i=0;i<p->comp;i++) 
	{
		bitload(&p->s);

        p->compid[i] = bitget(&p->s,8);
        p->hblock[i] = bitget(&p->s,4);
        p->vblock[i] = bitget(&p->s,4);
        p->quantidx[i] = bitget(&p->s,8);
        if (p->quantidx[i] >= 4)
            return ERR_INVALID_DATA;
		if (i>0 && (p->hblock[i]!=1 || p->vblock[i]!=1))
			return NotSupported(p);
		p->blocks += p->hblock[i] * p->vblock[i];
    }

	p->indexend = p->index + p->blocks;

	switch (16*p->vblock[0]+p->hblock[0])
	{
	case 0x11:
		mode = PF_YUV444;
		p->mb_width  = (width + 7) >> 3;
		p->mb_height = (height + 7) >> 3;
		break;
	case 0x12:
		mode = PF_YUV422;
		p->mb_width  = (width + 15) >> 4;
		p->mb_height = (height + 7) >> 3;
		break;
	case 0x22:
		mode = PF_YUV420;
		p->mb_width  = (width + 15) >> 4;
		p->mb_height = (height + 15) >> 4;
		break;
	default:
		return NotSupported(p);
	}

	return CodecIDCTSetFormat(&p->Codec,mode|PF_YUV_PC,width,height,width,height,p->Codec.In.Format.Format.Video.Aspect);
}

static NOINLINE void scan(mjpeg* p)
{
	idct* IDCT = p->Codec.IDCT.Ptr;
    int x,y,bitpos;
	blockindex* i; 

	p->restart_count = p->restart_interval;
	bitpos = p->s.bitpos;

    for (y=0;y<p->mb_height;++y)
        for (x=0;x<p->mb_width;++x)
		{
			IDCT->Process(IDCT,x,y);

            for (i=p->index;i!=p->indexend;i++)
			{
				int len,val,n;
				idct_block_t* block = p->blockptr; 
				uint8_t* quant = p->quant[i->quant];
				vlc* tab = p->v[0][i->dc]; 

				if (biteof(&p->s))
					break;

				ClearBlock(block);

				vlcget2_pos(val,tab,&p->s,bitpos,n,255,VLC_BITS);
				if (val)
				{
					bitload_pos(&p->s,bitpos);
					bitgetx_pos(&p->s,bitpos,val,n);
				}

				val = val * quant[0] + p->last_dc[i->comp];
				p->last_dc[i->comp] = val;
				block[0] = (idct_block_t)val;

				tab = p->v[1][i->ac];
				len = 1;
				for (;;)
				{
					vlcget2_pos(val,tab,&p->s,bitpos,n,255,VLC_BITS);
					if (!val)
						break;
					if (val == 0xF0) 
					{
						len += 16;
						if (len>=64)
							break;
					}
					else
					{
						n = val >> 4;
						val &= 15;
						len += n;
						bitload_pos(&p->s,bitpos);
						bitgetx_pos(&p->s,bitpos,val,n);
						n = len & 63;
						val *= quant[n];
						n = p->zigzag[n];
						block[n] = (idct_block_t)val;
						if (++len>=64)
							break;
					}
				}

				IDCT->Intra8x8(IDCT,block,len,IDCTSCAN_ZIGZAG);
            }

			if (p->comp == 1)
			{
				idct_block_t* block = p->blockptr; 

				ClearBlock(block);
				block[0] = 1024;
				IDCT->Intra8x8(IDCT,block,1,IDCTSCAN_ZIGZAG);

				ClearBlock(block);
				block[0] = 1024;
				IDCT->Intra8x8(IDCT,block,1,IDCTSCAN_ZIGZAG);
			}

            if (p->restart_interval && !--p->restart_count) 
			{
                p->restart_count = p->restart_interval;
				p->last_dc[0] = p->last_dc[1] = p->last_dc[2] = 1024;

                bitbytealign_pos(&p->s,bitpos);
				bitflush_pos(&p->s,bitpos,16);
				bitload_pos(&p->s,bitpos);
            }
        }

	p->s.bitpos = bitpos;
}

static int SOS(mjpeg* p)
{
	const uint8_t *pos;
	int i,j,comp,len,result;

    len = bitget(&p->s,16);
    comp = bitget(&p->s,8);
    if (len != 6+2*comp)
		return ERR_INVALID_DATA;

	if (p->comp != comp)
		return NotSupported(p);

	j=0;
    for (i=0;i<comp;i++) 
	{
		int n,dc,ac;
		bitload(&p->s);

        if (p->compid[i] != bitget(&p->s,8))
			return NotSupported(p);

        dc = bitget(&p->s,4);
        ac = bitget(&p->s,4);

		if (dc >= 4 || ac >= 4)
			return ERR_INVALID_DATA;

		for (n=p->hblock[i]*p->vblock[i];n;--n,++j)
		{
			p->index[j].quant=(uint8_t)p->quantidx[i];
			p->index[j].comp=(uint8_t)i;
			p->index[j].ac=(uint8_t)ac;
			p->index[j].dc=(uint8_t)dc;
		}
    }

	bitload(&p->s);
    p->Ss = bitget(&p->s,8); // Ss
    p->Se = bitget(&p->s,8); // Se

	bitload(&p->s);
    p->Ah = bitget(&p->s,4); // Ah
    p->Al = bitget(&p->s,4); // Al

	p->last_dc[0] = p->last_dc[1] = p->last_dc[2] = 1024;

	if (!p->sos_no)
	{
		result = p->Codec.IDCT.Ptr->FrameStart(p->Codec.IDCT.Ptr,0,NULL,0,-1,-1,0,0);
		if (result != ERR_NONE)
			return result;
		p->Codec.Show = 0;
	}

	pos = bitbytepos(&p->s);
	bitinit(&p->s,pos,bitendptr(&p->s)-pos);

	++p->sos_no;
    scan(p);

	return ERR_NONE;
}

static int APP(mjpeg* p)
{
    int n = bitget(&p->s,16);
    if (n>6)
	    bitflush(&p->s,n*8-16);
	return ERR_NONE;
}

static int Frame(mjpeg* p, const uint8_t* ptr, int num)
{
	int Result = ERR_NONE;
	int code,v;

	if (p->Codec.State.DropLevel)
		return p->Codec.IDCT.Ptr->Null(p->Codec.IDCT.Ptr,NULL,0);

	p->sos_no = 0;
	p->progressive = 0;

	do
	{
		for (;;)
		{
			if (num < 2) 
			{
				code = 0xD9;
				break;
			}

			v = *ptr++;
			--num;
			if ((v == 0xFF) && (*ptr >= 0xC0) && (*ptr <= 0xFE)) 
			{
				code = *ptr++;
				--num;
				break;
			}
		}

		if (code == 0xD9)  // end of image
			break;

		bitinit(&p->s,ptr,num);
		bitload(&p->s);
        switch(code) 
		{
        case 0xD8:	// start of image
		    p->restart_interval = 0;
			break;
		case 0xDD:	// define restart interval
		    Result = DRI(p);
		    break;
        case 0xDB:	// define quantization tables
			Result = DQT(p);
			break;
		case 0xC4:	// define huffman tables
			Result = DHT(p);
			break;
		case 0xC0:	// baseline, huffman
            Result = SOF(p,0);
			break;
		case 0xC2: // progressive, huffman
			if (!p->error)
			{
				p->error = 1;
				ShowError(p->Codec.Node.Class,MJPEG_ID,MJPEG_PROGRESSIVE);
			}
			Result = ERR_NOT_SUPPORTED;
			//todo... Result = SOF(p,1);
			break;
		case 0xDA:  // start of scan
			if (!p->sos_no || p->progressive)
	            Result = SOS(p);
			continue;
			
		case 0xE0:
		case 0xE1:
		case 0xE2:
		case 0xE3:
		case 0xE4:
		case 0xE5:
		case 0xE6:
		case 0xE7:
		case 0xE8:
		case 0xE9:
		case 0xEA:
		case 0xEB:
		case 0xEC:
		case 0xED:
		case 0xEE:
		case 0xEF:
			Result = APP(p);
			break;

		case 0xC1: // extended sequential, huffman
		case 0xC3: // lossless, huffman
		case 0xC5: // differential sequential, huffman
		case 0xC6: // differential progressive, huffman
		case 0xC7: // differential lossless, huffman
		case 0xC9: // extended sequential, arithmetic
		case 0xCA: // progressive, arithmetic
		case 0xCB: // lossless, arithmetic
		case 0xCD: // differential sequential, arithmetic
		case 0xCE: // differential progressive, arithmetic
		case 0xCF: // differential lossless, arithmetic
		case 0xC8: // reserved
			Result = NotSupported(p);
			break;
        }

		v = bitbytepos(&p->s) - ptr;
		ptr += v;
		num -= v;
    }
	while (Result == ERR_NONE);

	if (p->sos_no)
		p->Codec.IDCT.Ptr->FrameEnd(p->Codec.IDCT.Ptr);

	return Result;
}

static int Flush(mjpeg* p)
{
	p->startstate = 0;
	return ERR_NONE;
}

static bool_t FindNext(mjpeg* p)
{
	const uint8_t* Ptr = p->Codec.Buffer.Data;
	int Len = p->Codec.Buffer.WritePos;
	int Pos = p->Codec.FrameEnd;

	for (;Pos<Len;++Pos)
	{
		uint8_t ch = Ptr[Pos];
		bool_t Valid;

		switch (p->startstate)
		{
		case -1:
			Valid = 0;
			break;
		case 1:
			Valid = ch == 0xD8;
			break;
		case 3:
			Valid = (ch >= 0xC0) && (ch <= 0xFE);
			break;
		default:
			Valid = ch == 0xFF;
			break;
		}
		if (!Valid)
			p->startstate = 0;
		else
		if (++p->startstate == 4)
		{
			p->Codec.FrameEnd = Pos-3;
			p->startstate = -1;
			return 1;
		}
	}

	p->Codec.FrameEnd = Pos;
	return 0;
}

static int UpdateInput(mjpeg* p)
{
	int i,j;
	int Result = ERR_NONE;

	p->error = 0;
	IDCT_BLOCK_PREPARE(p,p->blockptr);
	memcpy(p->zigzag,zigzag,sizeof(zigzag));

	for (i=0;i<2;++i)
		for (j=0;j<4;++j)
			vlcdone(&p->v[i][j],&p->vsize[i][j]);

	if (p->Codec.In.Format.Type == PACKET_VIDEO)
	{
		Result = CodecIDCTSetFormat(&p->Codec,PF_YUV422|PF_YUV_PC,p->Codec.In.Format.Format.Video.Width,p->Codec.In.Format.Format.Video.Height,
									 		            p->Codec.In.Format.Format.Video.Width,p->Codec.In.Format.Format.Video.Height,
														p->Codec.In.Format.Format.Video.Aspect);
		if (Result == ERR_NONE)
		{
			for (i=0;i<2;++i)
				for (j=0;j<2;++j)
					if (!VLC(&p->v[i][j],&p->vsize[i][j],default_dht[i*2+j]))
						Result = ERR_OUT_OF_MEMORY;
	
			if (p->Codec.In.Format.PacketRate.Num>0)
				p->Codec.FrameTime = Scale(TICKSPERSEC,p->Codec.In.Format.PacketRate.Den,p->Codec.In.Format.PacketRate.Num);
			else
				p->Codec.FrameTime = 0;
		}
	}

	return Result;
}

static int Create(mjpeg* p)
{
	p->Codec.MinCount = 1;
	p->Codec.Frame = (codecidctframe)Frame;
	p->Codec.UpdateInput = (nodefunc)UpdateInput;
	p->Codec.FindNext = (codecidctnext)FindNext;
	p->Codec.Flush = (nodefunc)Flush;
	return ERR_NONE;
}

static const nodedef MJPEG =
{
	sizeof(mjpeg),
	MJPEG_ID,
	CODECIDCT_CLASS,
	PRI_DEFAULT,
	(nodecreate)Create,
	NULL,
};

//-------------------------------------------------------------------------------------------

static const nodedef JPEG =
{
	0, // parent size
	JPEG_ID,
	RAWIMAGE_CLASS,
	PRI_DEFAULT,
};

//---------------------------------------------------------------------------------------------

typedef struct webmjpeg
{
	format_base Format;

} webmjpeg;

static int WEBMJPEGInit(rawaudio* p)
{
	format_stream* s = Format_AddStream(&p->Format,sizeof(format_stream));
	if (s)
	{
		PacketFormatClear(&s->Format);
		s->Format.Type = PACKET_VIDEO;
		s->Format.Format.Video.Pixel.Flags = PF_FOURCC|PF_FRAGMENTED;
		s->Format.Format.Video.Pixel.FourCC = FOURCC('J','P','E','G');
		s->PacketBurst = 1;
		s->Fragmented = 1;
		s->DisableDrop = 1;
		Format_PrepairStream(&p->Format,s);
	}
	p->Format.HeaderLoaded = 1;
	p->Format.ProcessMinBuffer = 0;
	p->Format.ReadSize = WEBMJEG_READSIZE;
	return ERR_NONE;
}

static int WEBMJPEGPacket(rawaudio* p, format_reader* Reader, format_packet* Packet)
{
	format_stream* Stream = p->Format.Streams[0];
	
	if (Reader->FilePos<=0)
		Packet->RefTime = 0;
	else
		Packet->RefTime = TIME_UNKNOWN;
	Packet->Data = Reader->ReadAsRef(Reader,-WEBMJEG_READSIZE);
	Packet->Stream = Stream;

	if (Stream->LastTime < TIME_UNKNOWN)
		Stream->LastTime = TIME_UNKNOWN;

	return ERR_NONE;
}

static int WEBMJPEGSeek(rawaudio* p, tick_t Time, filepos_t FilePos,bool_t PrevKey)
{
	if (Time==0 || FilePos==0)
		p->Format.Reader->Seek(p->Format.Reader,0,SEEK_SET);
	return ERR_NONE;
}

static int WEBMJPEGCreate(rawaudio* p)
{
	p->Format.Init = (fmtfunc)WEBMJPEGInit;
	p->Format.Seek = (fmtseek)WEBMJPEGSeek;
	p->Format.ReadPacket = (fmtreadpacket)WEBMJPEGPacket;
	return ERR_NONE;
}

static const nodedef WEBMJPEG =
{
	sizeof(webmjpeg),
	WEB_MJPEG_ID,
	FORMATBASE_CLASS,
	PRI_DEFAULT,
	(nodecreate)WEBMJPEGCreate,
	NULL,
};

//---------------------------------------------------------------------------------------------

void MJPEG_Init()
{
	NodeRegisterClass(&MJPEG);
	NodeRegisterClass(&JPEG);
	NodeRegisterClass(&WEBMJPEG);
}

void MJPEG_Done()
{
	NodeUnRegisterClass(MJPEG_ID);
	NodeUnRegisterClass(JPEG_ID);
	NodeUnRegisterClass(WEB_MJPEG_ID);
}
