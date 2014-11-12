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
 * $Id: mpeg_decode.c 603 2006-01-19 13:00:33Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common/common.h"
#include "mpeg1.h"
#include "mpeg_decode.h"
#include "mpeg_stream.h"

static _CONST uint8_t zigzag[64] = 
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

static int UpdateSize(mpeg_decode* dec)
{
	if (dec->Codec.IDCT.Width > (MB_X-1)*16 || dec->Codec.IDCT.Height > (MB_Y-1)*16)
	{
		if (!dec->ErrorShowed)
		{
			dec->ErrorShowed = 1;
			ShowError(dec->Codec.Node.Class,VOUT_CLASS,VOUT_ERROR_SIZE,(MB_X-1)*16,(MB_Y-1)*16);
		}
		return ERR_NOT_SUPPORTED;
	}

	dec->mb_xsize = (dec->Codec.IDCT.Width + 15) / 16; 
	dec->mb_ysize = (dec->Codec.IDCT.Height + 15) / 16;
	dec->pos_end = dec->mb_ysize * MB_X;
	return ERR_NONE;
}

static int Discontinuity( mpeg_decode* dec )
{
	dec->Frame = 0;
	return ERR_NONE;
}

static int Flush( mpeg_decode* dec )
{
	dec->StartState = -1;
	dec->SliceFound = 0;
	return ERR_NONE;
}

static int UpdateInput( mpeg_decode* dec )
{
	int Result = ERR_NONE;

	IDCT_BLOCK_PREPARE(dec,dec->blockptr);
	dec->mb_xsize = 0; 
	dec->mb_ysize = 0;
	dec->pos_end = 0;
	dec->refframe = 0;
	dec->Frame = 0;
	dec->ValidSeq = 0;
	dec->ErrorShowed = 0;
	dec->OnlyIVOP = 1;
	memcpy(dec->zigzag,zigzag,sizeof(zigzag));

	if (dec->Codec.In.Format.Type == PACKET_VIDEO)
		Result = CodecIDCTSetFormat(&dec->Codec,PF_YUV420,dec->Codec.In.Format.Format.Video.Width,dec->Codec.In.Format.Format.Video.Height,
									 		              dec->Codec.In.Format.Format.Video.Width,dec->Codec.In.Format.Format.Video.Height,
														  dec->Codec.In.Format.Format.Video.Aspect);
	return Result;
}

static bool_t FindNext( mpeg_decode* dec )
{
	const uint8_t* Ptr = dec->Codec.Buffer.Data;
	int Len = dec->Codec.Buffer.WritePos;
	int Pos = dec->Codec.FrameEnd;
	int32_t v = dec->StartState;

	for (;Pos<Len;++Pos)
	{
		v = (v<<8) | Ptr[Pos];
		if ((v & ~0xFF) == 0x100)
		{
            if (v < SLICE_MIN_START_CODE || v > SLICE_MAX_START_CODE)
			{
				if (dec->SliceFound)
				{
					dec->Codec.FrameEnd = Pos-3;
					dec->SliceFound = 0;
					dec->StartState = -1;
					return 1;
				}
			}
			else
				dec->SliceFound=1;
		}
	}

	dec->Codec.FrameEnd = Pos;
	dec->StartState = v;
	return 0;
}

static _CONST int FrameRate[16] = 
{
        0, 24000, 24024, 25025,
    30000, 30030, 50050, 60000,
    60060, 15015,  5005, 10010,
    12012, 15015, 25025, 25025,
};

static _CONST int8_t MPEG1_IntraMatrix[64] = 
{
	8,  16, 19, 22, 26, 27, 29, 34,
	16, 16, 22, 24, 27, 29, 34, 37,
	19, 22, 26, 27, 29, 34, 34, 38,
	22, 22, 26, 27, 29, 34, 37, 40,
	22, 26, 27, 29, 32, 35, 40, 48,
	26, 27, 29, 32, 35, 40, 48, 58,
	26, 27, 29, 34, 38, 46, 56, 69,
	27, 29, 35, 38, 46, 56, 69, 83
};

static void initbits(mpeg_decode* dec,const uint8_t *stream, int len)
{
	dec->bits = 0;
	dec->bitpos = 32;
	dec->bitptr = stream;
	dec->bitend = stream+len+4;
}

static void loadbits( mpeg_decode* dec )
{
	int bitpos = dec->bitpos;
	loadbits_pos(dec,bitpos);
	dec->bitpos = bitpos;
}

#ifndef NDEBUG
static int showbitslarge( mpeg_decode* dec, int n )
{
	int i = showbits(dec,n);
	i |= *dec->bitptr >> (40-n-dec->bitpos);
	return i;
}
#endif

static void Sequence( mpeg_decode* dec )
{
	int i;

	loadbits(dec);
	dec->width = getbits(dec,12);
	dec->height = getbits(dec,12);

	loadbits(dec);
    dec->aspect = getbits(dec,4);

	dec->Codec.In.Format.PacketRate.Den = 1001;
	dec->Codec.In.Format.PacketRate.Num = FrameRate[getbits(dec,4)];
	dec->Codec.FrameTime = Scale(TICKSPERSEC,dec->Codec.In.Format.PacketRate.Den,dec->Codec.In.Format.PacketRate.Num);

	loadbits(dec);
    dec->Codec.In.Format.ByteRate = getbits(dec,18) * 50;
	if (dec->Codec.In.Format.ByteRate > 8*1024*1024)
		dec->Codec.In.Format.ByteRate = 0;

    flushbits(dec,1); // marker
    flushbits(dec,10); // vbv_buffer_size
    flushbits(dec,1);

	loadbits(dec);
    if (getbits1(dec)) 
	{
        for (i=0;i<64;i++) 
		{
			loadbits(dec);
            dec->IntraMatrix[i] = (uint8_t)getbits(dec,8);
        }
    } 
	else 
	{
        for (i=0;i<64;i++) 
            dec->IntraMatrix[i] = MPEG1_IntraMatrix[dec->zigzag[i]];
    }

    if (getbits1(dec)) 
	{
        for (i=0;i<64;i++) 
		{
			loadbits(dec);
            dec->InterMatrix[i] = (uint8_t)getbits(dec, 8);
        }
    } 
	else 
	{
        for (i=0;i<64;i++) 
            dec->InterMatrix[i] = 16;
    }

	dec->ValidSeq = 1;
}

static void Picture( mpeg_decode* dec )
{
	static const int16_t aspect[16] = 
	{
		    0,10000, 6735, 7031,
		 7615, 8055, 8437, 8935,
		 9157, 9815,10255,10695,
		10950,11575,12015,    0,
	};

	if (!dec->Codec.In.Format.Format.Video.Aspect && aspect[dec->aspect])
		dec->Codec.In.Format.Format.Video.Aspect = Scale(ASPECT_ONE,10000,aspect[dec->aspect]);

	CodecIDCTSetFormat(&dec->Codec,PF_YUV420,dec->width,dec->height,dec->width,dec->height,dec->Codec.In.Format.Format.Video.Aspect);

    flushbits(dec,10); // temporal ref

    loadbits(dec);
	dec->prediction_type = getbits(dec,3);

    flushbits(dec,16); // non constant bit rate

	loadbits(dec);
    if (dec->prediction_type == P_VOP || dec->prediction_type == B_VOP)
	{
        dec->full_pixel[0] = getbits(dec,1);
        dec->fcode[0] = getbits(dec,3)-1;
    }

    if (dec->prediction_type == B_VOP) 
	{
        dec->full_pixel[1] = getbits(dec,1);
        dec->fcode[1] = getbits(dec,3)-1;
    }

    dec->frame_state = 1;

	DEBUG_MSG1(DEBUG_VCODEC,T("MPEG Picture:%d"),dec->prediction_type);
}

static int StartFrame( mpeg_decode* dec )
{
	int result = ERR_NONE;

	switch (dec->prediction_type) 
	{
	case P_VOP:
		if (dec->Frame < 1)
		{
			if (dec->Codec.Dropping)
				dec->Codec.IDCT.Ptr->Null(dec->Codec.IDCT.Ptr,NULL,0);
			return ERR_INVALID_DATA;
		}

		// no break

	case I_VOP:

		dec->refframe ^= 1;

		if (dec->OnlyIVOP && dec->Codec.State.DropLevel)
		{
			dec->Codec.IDCT.Ptr->Null(dec->Codec.IDCT.Ptr,NULL,0);
			return ERR_NONE;
		}

		if (dec->Codec.IDCT.Count>=3)
			dec->Codec.State.DropLevel = 0;

		if (dec->Codec.IDCT.Count<3 || dec->Frame==0)
			dec->Codec.Show = dec->refframe; // show this refframe
		else
			dec->Codec.Show = dec->refframe ^ 1; // show last refframe 

		if (dec->prediction_type == I_VOP)
		{
			result = dec->Codec.IDCT.Ptr->FrameStart(dec->Codec.IDCT.Ptr,dec->Frame,&dec->bufframe,dec->refframe,-1,-1,dec->Codec.Show,0);
		}
		else 
		{
			dec->OnlyIVOP = 0;
			result = dec->Codec.IDCT.Ptr->FrameStart(dec->Codec.IDCT.Ptr,dec->Frame,&dec->bufframe,dec->refframe,dec->refframe^1,-1,dec->Codec.Show,dec->Codec.State.DropLevel);
		}

		if (result != ERR_NONE)
			dec->Codec.Show = -1;
		break;

	case B_VOP:

		if (dec->Frame < 2)
		{
			if (dec->Codec.Dropping)
				dec->Codec.IDCT.Ptr->Null(dec->Codec.IDCT.Ptr,NULL,0);
			return ERR_INVALID_DATA;
		}

		if (dec->Codec.State.DropLevel)
		{
			dec->Codec.IDCT.Ptr->Null(dec->Codec.IDCT.Ptr,NULL,0);
			return ERR_NONE;
		}

		if (dec->Codec.IDCT.Count<3)
		{
			CodecIDCTSetCount(&dec->Codec,3);
			if (dec->Codec.IDCT.Count<3)
			{
				dec->Codec.IDCT.Ptr->Null(dec->Codec.IDCT.Ptr,NULL,0);
				return ERR_INVALID_DATA;
			}
		}

		dec->OnlyIVOP = 0;
		dec->Codec.Show = 2;
		result = dec->Codec.IDCT.Ptr->FrameStart(dec->Codec.IDCT.Ptr,-dec->Frame,&dec->bufframe,2,dec->refframe^1,dec->refframe,2,0);
		break;

	default:
		result = ERR_INVALID_DATA;
		break;
	}

	return result;
}

// max 9 bits
static INLINE int getDCsizeLum( mpeg_decode* dec )
{
	int i,code;
	
	if (!getbits1(dec))
		return getbits(dec,1)+1;

	if (!getbits1(dec))
		return getbits(dec,1)*3;
	
	code = showbits(dec,7);

	for (i=1;i<8;i++,code<<=1)
		if (!(code & 64))
		{
			flushbits(dec,i);
			return i+3;
		}

	flushbits(dec,7);
	return 11;
}

// max 10 bits
static INLINE int getDCsizeChr( mpeg_decode* dec )
{
	int i,code;
	
	if (!getbits1(dec))
		return getbits(dec,1);
	
	code = showbits(dec,9);

	for (i=1;i<10;i++,code<<=1)
		if (!(code & 256))
		{
			flushbits(dec,i);
			return i+1;
		}

	flushbits(dec,9);
	return 11;
}

// max 11bits
static INLINE int getDCdiff(int dct_dc_size, mpeg_decode* dec)
{
	int code = showbits(dec,32); //we need only dct_dc_size bits (but in the higher bits)
	int adj = 0;
	flushbits(dec,dct_dc_size);
	if (code >= 0)
		adj = (-1 << dct_dc_size) + 1;
	return adj + ((uint32_t)code >> (32-dct_dc_size));
}

#define TABLE_1			0
#define TABLE_2			252
#define TABLE_3			252+112
#define TABLE_END		252+112+112

static _CONST uint16_t vld_mpeg1[252+112+112] = 
{
0x403e,0x403e,0x403e,0x403e,0x5082,0x5082,0x5241,0x5241,
0x5004,0x5004,0x5201,0x5201,0x41c1,0x41c1,0x41c1,0x41c1,
0x4181,0x4181,0x4181,0x4181,0x4042,0x4042,0x4042,0x4042,
0x4141,0x4141,0x4141,0x4141,0x6341,0x6006,0x6301,0x62c1,
0x60c2,0x6043,0x6005,0x6281,0x3003,0x3003,0x3003,0x3003,
0x3003,0x3003,0x3003,0x3003,0x3101,0x3101,0x3101,0x3101,
0x3101,0x3101,0x3101,0x3101,0x30c1,0x30c1,0x30c1,0x30c1,
0x30c1,0x30c1,0x30c1,0x30c1,0x2002,0x2002,0x2002,0x2002,
0x2002,0x2002,0x2002,0x2002,0x2002,0x2002,0x2002,0x2002,
0x2002,0x2002,0x2002,0x2002,0x2081,0x2081,0x2081,0x2081,
0x2081,0x2081,0x2081,0x2081,0x2081,0x2081,0x2081,0x2081,
0x2081,0x2081,0x2081,0x2081,0x1041,0x1041,0x1041,0x1041,
0x1041,0x1041,0x1041,0x1041,0x1041,0x1041,0x1041,0x1041,
0x1041,0x1041,0x1041,0x1041,0x1041,0x1041,0x1041,0x1041,
0x1041,0x1041,0x1041,0x1041,0x1041,0x1041,0x1041,0x1041,
0x1041,0x1041,0x1041,0x1041,0x003f,0x003f,0x003f,0x003f,
0x003f,0x003f,0x003f,0x003f,0x003f,0x003f,0x003f,0x003f,
0x003f,0x003f,0x003f,0x003f,0x003f,0x003f,0x003f,0x003f,
0x003f,0x003f,0x003f,0x003f,0x003f,0x003f,0x003f,0x003f,
0x003f,0x003f,0x003f,0x003f,0x003f,0x003f,0x003f,0x003f,
0x003f,0x003f,0x003f,0x003f,0x003f,0x003f,0x003f,0x003f,
0x003f,0x003f,0x003f,0x003f,0x003f,0x003f,0x003f,0x003f,
0x003f,0x003f,0x003f,0x003f,0x003f,0x003f,0x003f,0x003f,
0x003f,0x003f,0x003f,0x003f,0x0001,0x0001,0x0001,0x0001,
0x0001,0x0001,0x0001,0x0001,0x0001,0x0001,0x0001,0x0001,
0x0001,0x0001,0x0001,0x0001,0x0001,0x0001,0x0001,0x0001,
0x0001,0x0001,0x0001,0x0001,0x0001,0x0001,0x0001,0x0001,
0x0001,0x0001,0x0001,0x0001,0x0001,0x0001,0x0001,0x0001,
0x0001,0x0001,0x0001,0x0001,0x0001,0x0001,0x0001,0x0001,
0x0001,0x0001,0x0001,0x0001,0x0001,0x0001,0x0001,0x0001,
0x0001,0x0001,0x0001,0x0001,0x0001,0x0001,0x0001,0x0001,
0x0001,0x0001,0x0001,0x0001,0xb282,0xb242,0xb143,0xb0c4,
0xb085,0xb047,0xb046,0xb00f,0xb00e,0xb00d,0xb00c,0xb681,
0xb641,0xb601,0xb5c1,0xb581,0xa00b,0xa00b,0xa202,0xa202,
0xa103,0xa103,0xa00a,0xa00a,0xa084,0xa084,0xa1c2,0xa1c2,
0xa541,0xa541,0xa501,0xa501,0xa009,0xa009,0xa4c1,0xa4c1,
0xa481,0xa481,0xa045,0xa045,0xa0c3,0xa0c3,0xa008,0xa008,
0xa182,0xa182,0xa441,0xa441,0x8401,0x8401,0x8401,0x8401,
0x8401,0x8401,0x8401,0x8401,0x8142,0x8142,0x8142,0x8142,
0x8142,0x8142,0x8142,0x8142,0x8007,0x8007,0x8007,0x8007,
0x8007,0x8007,0x8007,0x8007,0x8083,0x8083,0x8083,0x8083,
0x8083,0x8083,0x8083,0x8083,0x8044,0x8044,0x8044,0x8044,
0x8044,0x8044,0x8044,0x8044,0x83c1,0x83c1,0x83c1,0x83c1,
0x83c1,0x83c1,0x83c1,0x83c1,0x8381,0x8381,0x8381,0x8381,
0x8381,0x8381,0x8381,0x8381,0x8102,0x8102,0x8102,0x8102,
0x8102,0x8102,0x8102,0x8102,0xe052,0xe051,0xe050,0xe04f,
0xe183,0xe402,0xe3c2,0xe382,0xe342,0xe302,0xe2c2,0xe7c1,
0xe781,0xe741,0xe701,0xe6c1,0xd028,0xd028,0xd027,0xd027,
0xd026,0xd026,0xd025,0xd025,0xd024,0xd024,0xd023,0xd023,
0xd022,0xd022,0xd021,0xd021,0xd020,0xd020,0xd04e,0xd04e,
0xd04d,0xd04d,0xd04c,0xd04c,0xd04b,0xd04b,0xd04a,0xd04a,
0xd049,0xd049,0xd048,0xd048,0xc01f,0xc01f,0xc01f,0xc01f,
0xc01e,0xc01e,0xc01e,0xc01e,0xc01d,0xc01d,0xc01d,0xc01d,
0xc01c,0xc01c,0xc01c,0xc01c,0xc01b,0xc01b,0xc01b,0xc01b,
0xc01a,0xc01a,0xc01a,0xc01a,0xc019,0xc019,0xc019,0xc019,
0xc018,0xc018,0xc018,0xc018,0xc017,0xc017,0xc017,0xc017,
0xc016,0xc016,0xc016,0xc016,0xc015,0xc015,0xc015,0xc015,
0xc014,0xc014,0xc014,0xc014,0xc013,0xc013,0xc013,0xc013,
0xc012,0xc012,0xc012,0xc012,0xc011,0xc011,0xc011,0xc011,
0xc010,0xc010,0xc010,0xc010
};

// size:4 run:6		level:6 
// size:4 escape    62
// size:4 end		63

#define vld_code								\
	code = showbits_pos(dec,bitpos,16);			\
	if (code >> 10)								\
		code = (code >> 8) - 4 + TABLE_1;		\
	else if (code >> 7)							\
		code = (code >> 3) - 16 + TABLE_2;		\
	else /* if (code >= 16)  but we don't care about invalid huffman codes */ \
		code = code - 16 + TABLE_3;				\
	code = table[code];							\
	flushbits_pos(dec,bitpos,2+(code >> 12));

static void blockIntra( mpeg_decode* dec, int pos )
{
	int j;
	idct_block_t *block;
	const uint16_t *table = vld_mpeg1;
	int qscale;

	dec->Codec.IDCT.Ptr->Process(dec->Codec.IDCT.Ptr,POSX(pos),POSY(pos));

	block = dec->blockptr;
	qscale = dec->qscale;

	for (j=0;j<6;++j)
	{
		int bitpos;
		int len;

		ClearBlock(block);	
		loadbits(dec);

		{
			int dct_dc_size, dct_dc_diff;

			dct_dc_diff = 0;
			dct_dc_size = j<4 ? getDCsizeLum(dec) : getDCsizeChr(dec); //max11bit

			if (dct_dc_size)
				dct_dc_diff = getDCdiff(dct_dc_size,dec);

			dct_dc_size = j<4 ? 0 : j-4+1;
			DEBUG_MSG2(DEBUG_VCODEC2,T("dc=%d diff=%d"), dec->last_dc[dct_dc_size]+dct_dc_diff, dct_dc_diff );
			dct_dc_diff += dec->last_dc[dct_dc_size];
			dec->last_dc[dct_dc_size] = dct_dc_diff;

			*block = (idct_block_t)(dct_dc_diff << 3);
			len = 1;
		}

		bitpos = dec->bitpos;
		
		for (;;) // event vld
		{
			int code,level;

			loadbits_pos(dec,bitpos);
			code = showbits_pos(dec,bitpos,16);	

			vld_code;
			level = code & 63;
			if (level < 62) 
			{
				level *= qscale;
				code >>= 6;
				code &= 63;
				len += code; // run
				if (len >= 64)
					break;

				code = dec->zigzag[len];
				level *= dec->IntraMatrix[len];
				level >>= 3;
				level = (level-1)|1;

				if (getbits1_pos(dec,bitpos)) 
					level = -level;

				block[code] = (idct_block_t)level;
				++len;
			} 
			else 
			{
				if (level==63)
					break;

				// this value is escaped
				loadbits_pos(dec,bitpos);

				len += showbits_pos(dec,bitpos,6); flushbits_pos(dec,bitpos,6);
				if (len >= 64)
					break;

				code = showbits_pos(dec,bitpos,8); flushbits_pos(dec,bitpos,8);
				level = (code << 24) >> 24; //sign extend the lower 8 bits
				code = dec->zigzag[len];

				if (level == -128)
				{
					level = showbits_pos(dec,bitpos,8)-256; flushbits_pos(dec,bitpos,8);
				}
				else 
				if (level == 0)
				{
					level = showbits_pos(dec,bitpos,8); flushbits_pos(dec,bitpos,8);
				}

				if (level<0)
				{
					level= -level;
					level *= qscale * dec->IntraMatrix[len];
					level >>= 3;
					level= (level-1)|1;
					level= -level;

					block[code] = (idct_block_t)level;
					++len;
				}
				else
				{
					level *= qscale * dec->IntraMatrix[len];
					level >>= 3;
					level = (level-1)|1;

					block[code] = (idct_block_t)level;
					++len;
				}
			}

			DEBUG_MSG2(DEBUG_VCODEC2,T("intra_block[%i] %i"), code, level );
		}

		dec->bitpos = bitpos;
	
		dec->Codec.IDCT.Ptr->Intra8x8(dec->Codec.IDCT.Ptr,block,len,IDCTSCAN_ZIGZAG);
	}

	dec->fmv[0] = dec->bmv[0] = 0;
}

static INLINE int readqscale( mpeg_decode* dec )
{
    return getbits(dec,5);
}

static _CONST uint8_t mcbp_p[32*2+128*2] = 
{
0x00,0x00,0x08,0x39,0x08,0x2b,0x08,0x29,0x07,0x22,0x07,0x21,0x06,0x3f,0x06,0x24,
0x05,0x3e,0x05,0x02,0x05,0x3d,0x05,0x01,0x05,0x38,0x05,0x34,0x05,0x2c,0x05,0x1c,
0x05,0x28,0x05,0x14,0x05,0x30,0x05,0x0c,0x04,0x20,0x04,0x20,0x04,0x10,0x04,0x10,
0x04,0x08,0x04,0x08,0x04,0x04,0x04,0x04,0x03,0x3c,0x03,0x3c,0x03,0x3c,0x03,0x3c,
0x00,0x00,0x00,0x00,0x09,0x27,0x09,0x1b,0x09,0x3b,0x09,0x37,0x09,0x2f,0x09,0x1f,
0x08,0x3a,0x08,0x3a,0x08,0x36,0x08,0x36,0x08,0x2e,0x08,0x2e,0x08,0x1e,0x08,0x1e,
0x08,0x39,0x08,0x39,0x08,0x35,0x08,0x35,0x08,0x2d,0x08,0x2d,0x08,0x1d,0x08,0x1d,
0x08,0x26,0x08,0x26,0x08,0x1a,0x08,0x1a,0x08,0x25,0x08,0x25,0x08,0x19,0x08,0x19,
0x08,0x2b,0x08,0x2b,0x08,0x17,0x08,0x17,0x08,0x33,0x08,0x33,0x08,0x0f,0x08,0x0f,
0x08,0x2a,0x08,0x2a,0x08,0x16,0x08,0x16,0x08,0x32,0x08,0x32,0x08,0x0e,0x08,0x0e,
0x08,0x29,0x08,0x29,0x08,0x15,0x08,0x15,0x08,0x31,0x08,0x31,0x08,0x0d,0x08,0x0d,
0x08,0x23,0x08,0x23,0x08,0x13,0x08,0x13,0x08,0x0b,0x08,0x0b,0x08,0x07,0x08,0x07,
0x07,0x22,0x07,0x22,0x07,0x22,0x07,0x22,0x07,0x12,0x07,0x12,0x07,0x12,0x07,0x12,
0x07,0x0a,0x07,0x0a,0x07,0x0a,0x07,0x0a,0x07,0x06,0x07,0x06,0x07,0x06,0x07,0x06,
0x07,0x21,0x07,0x21,0x07,0x21,0x07,0x21,0x07,0x11,0x07,0x11,0x07,0x11,0x07,0x11,
0x07,0x09,0x07,0x09,0x07,0x09,0x07,0x09,0x07,0x05,0x07,0x05,0x07,0x05,0x07,0x05,
0x06,0x3f,0x06,0x3f,0x06,0x3f,0x06,0x3f,0x06,0x3f,0x06,0x3f,0x06,0x3f,0x06,0x3f,
0x06,0x03,0x06,0x03,0x06,0x03,0x06,0x03,0x06,0x03,0x06,0x03,0x06,0x03,0x06,0x03,
0x06,0x24,0x06,0x24,0x06,0x24,0x06,0x24,0x06,0x24,0x06,0x24,0x06,0x24,0x06,0x24,
0x06,0x18,0x06,0x18,0x06,0x18,0x06,0x18,0x06,0x18,0x06,0x18,0x06,0x18,0x06,0x18
};

static _CONST uint8_t mb_type_p[64] =
{
0x00,0xd1,0xa9,0xa9,0xab,0xab,0xb0,0xb0,0x62,0x62,0x62,0x62,0x62,0x62,0x62,0x62,
0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,
0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,
0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a,0x2a
};

static _CONST uint8_t mb_type_b[64] =
{
0x00,0xd1,0xcd,0xcb,0xaf,0xaf,0xb0,0xb0,0x82,0x82,0x82,0x82,0x8a,0x8a,0x8a,0x8a,
0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x64,0x6c,0x6c,0x6c,0x6c,0x6c,0x6c,0x6c,0x6c,
0x46,0x46,0x46,0x46,0x46,0x46,0x46,0x46,0x46,0x46,0x46,0x46,0x46,0x46,0x46,0x46,
0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,0x4e,0x4e
};

#define SKIP_1	 0
#define SKIP_2	 16
#define SKIP_3	 (16+8)
#define SKIP_END (16+8+64)

static _CONST uint8_t skip[SKIP_END*2] = 
{
0x0a,0x22,0x07,0x0c,0x04,0x06,0x04,0x05,0x03,0x04,0x03,0x04,0x03,0x03,0x03,0x03,
0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x02,0x01,0x02,0x01,0x02,0x01,0x02,0x01,
0x07,0x0c,0x07,0x0b,0x07,0x0a,0x07,0x09,0x06,0x08,0x06,0x08,0x06,0x07,0x06,0x07,
0x0a,0x22,0x0a,0x22,0x0a,0x22,0x0a,0x22,0x0a,0x22,0x0a,0x22,0x0a,0x22,0x0a,0x22,
0x0a,0x21,0x0a,0x22,0x0a,0x22,0x0a,0x22,0x0a,0x22,0x0a,0x22,0x0a,0x22,0x0a,0x22,
0x0a,0x22,0x0a,0x22,0x0a,0x22,0x0a,0x22,0x0a,0x22,0x0a,0x22,0x0a,0x22,0x0a,0x22,
0x0a,0x20,0x0a,0x1f,0x0a,0x1e,0x0a,0x1d,0x0a,0x1c,0x0a,0x1b,0x0a,0x1a,0x0a,0x19,
0x0a,0x18,0x0a,0x17,0x0a,0x16,0x0a,0x15,0x09,0x14,0x09,0x14,0x09,0x13,0x09,0x13,
0x09,0x12,0x09,0x12,0x09,0x11,0x09,0x11,0x09,0x10,0x09,0x10,0x09,0x0f,0x09,0x0f,
0x07,0x0e,0x07,0x0e,0x07,0x0e,0x07,0x0e,0x07,0x0e,0x07,0x0e,0x07,0x0e,0x07,0x0e,
0x07,0x0d,0x07,0x0d,0x07,0x0d,0x07,0x0d,0x07,0x0d,0x07,0x0d,0x07,0x0d,0x07,0x0d
};

static INLINE void readskip( mpeg_decode* dec )
{
	int i;
	dec->skip = 0;

	while (!getbits1(dec))
	{
		inlineloadbits(dec);
		i = showbits(dec,10);

		if (i >> 7)
			i = (i >> 6) + SKIP_1;
		else
		if (i >> 6)
			i = (i >> 3)-8 + SKIP_2;
		else
			i = i + SKIP_3;

		i <<= 1;
		flushbits(dec,skip[i]);
		i = skip[i+1];

		if (i<33)
		{
			dec->skip += i;
			break;
		}
		if (i==33)
			dec->skip += 33;

		if (eofbits(dec))
			break;
	}
}

static _CONST uint8_t mv_tab[64] = 
{
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x84,0x7c,0x74,0x6c,
	0x64,0x5c,0x53,0x53,0x4b,0x4b,0x43,0x43,0x39,0x39,0x39,0x39,0x39,0x39,0x39,0x39,
	0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x31,0x29,0x29,0x29,0x29,0x29,0x29,0x29,0x29,
	0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20
};

#define GetMVData( dec, table, shift, shift2, last, bitpos )\
{														\
	int code,v;											\
	loadbits_pos(dec,bitpos);							\
	code = showbits_pos(dec,bitpos,10);					\
	if (code >= 512)									\
	{													\
		flushbits_pos(dec,bitpos,1);					\
	}													\
	else												\
	{													\
		if (code >= 64)									\
		{												\
			v = 4;										\
			if (code >= 128) --v;						\
			if (code >= 256) --v;						\
			flushbits_pos(dec,bitpos,v);				\
			code = v-1;									\
		}												\
		else											\
		{												\
			code = table[code];							\
			v = (code & 7)+6;							\
			flushbits_pos(dec,bitpos,v);				\
			code >>= 3;									\
		}												\
		/* 14bits left	*/								\
		v = showbits_pos(dec,bitpos,1);					\
		flushbits_pos(dec,bitpos,1);					\
		if (shift) /* max 6bits */						\
		{												\
			code = (code - 1) << shift;					\
			code |= showbits_pos(dec,bitpos,shift);		\
			flushbits_pos(dec,bitpos,shift);			\
			code++;										\
		}												\
		if (v)											\
			code = -code;								\
		last += code;									\
		last = ((last + shift2) & (shift2*2-1)) - shift2;\
	}													\
}

static void GetMV(int fcode, int* mv, int full, mpeg_decode* dec )
{
	int mv_x = ((int16_t*)mv)[MVXIDX];
	int mv_y = ((int16_t*)mv)[MVYIDX];
	int bitpos = dec->bitpos;
	int fcode2 = 1 << (fcode+4);
	_CONST uint8_t* tab = mv_tab;

	GetMVData(dec,tab,fcode,fcode2,mv_x,bitpos);
	GetMVData(dec,tab,fcode,fcode2,mv_y,bitpos);

	dec->bitpos = bitpos;

	((int16_t*)mv)[MVXIDX] = (int16_t)mv_x;
	((int16_t*)mv)[MVYIDX] = (int16_t)mv_y;

	if (full)
	{
		mv_x <<= 1;
		mv_y <<= 1;
	}

	mv_x &= 0xFFFF;
	mv_y <<= 16;
	mv_y |= mv_x;

	mv[1] = mv_y;
	mv[2] = mv_y;
	mv[3] = mv_y;
	mv[4] = mv_y;

	mv_y >>= 1; //shift
	mv_y &= ~0x8000; //replace dx sign bit with old signed bit
	mv_x &= 0x8000;
	mv_y |= mv_x;

	mv[5] = mv_y;
	mv[6] = mv_y;
}

static int decodeInter( mpeg_decode* dec )
{
	idct_block_t *block = dec->blockptr;
	const uint16_t *table = vld_mpeg1;
	int qscale;

	int bitpos;
	int len;
	int code,level;

	ClearBlock(block);	

	bitpos = dec->bitpos;
	loadbits_pos(dec,bitpos);

	len = 0;
	qscale = dec->qscale;

	// special case for first
	code = showbits_pos(dec,bitpos,2);
    if (code & 2) 
	{
		flushbits_pos(dec,bitpos,2);

        level= (3 * qscale * dec->InterMatrix[0]) >> 4;
        level= (level-1)|1;
        if (code & 1)
			level= -level;
        block[0] = (idct_block_t)level;
		DEBUG_MSG2(DEBUG_VCODEC2,T("inter_block[%i] %i"), 0, level );
        ++len;
    }

	for (;;) // event vld
	{
		loadbits_pos(dec,bitpos);
		code = showbits_pos(dec,bitpos,16);	

		vld_code;
		level = code & 63;
		if (level < 62) 
		{
			level = level*2+1;
			level *= qscale;

			code >>= 6;
			code &= 63;
			len += code; // run
			if (len >= 64)
				break;

			code = dec->zigzag[len];
			level *= dec->InterMatrix[len];
			level >>= 4;
			level = (level-1)|1;

			if (getbits1_pos(dec,bitpos)) 
				level = -level;

			block[code] = (idct_block_t)level;
			++len;
		} 
		else 
		{
			if (level==63)
				break;

			// this value is escaped
			loadbits_pos(dec,bitpos);

			len += showbits_pos(dec,bitpos,6); flushbits_pos(dec,bitpos,6);
			if (len >= 64)
				break;

			code = showbits_pos(dec,bitpos,8); flushbits_pos(dec,bitpos,8);
			level = (code << 24) >> 24; //sign extend the lower 8 bits
			code = dec->zigzag[len];

			if (level == -128)
			{
				level = showbits_pos(dec,bitpos,8)-256; flushbits_pos(dec,bitpos,8);
			}
			else 
			if (level == 0)
			{
				level = showbits_pos(dec,bitpos,8); flushbits_pos(dec,bitpos,8);
			}

			if (level<0)
			{
				level= -level;
				level = level*2+1;
				level *= qscale * dec->InterMatrix[len];
				level >>= 4;
				level= (level-1)|1;
				level= -level;

				block[code] = (idct_block_t)level;
				++len;
			}
			else
			{
				level = level*2+1;
				level *= qscale * dec->InterMatrix[len];
				level >>= 4;
				level = (level-1)|1;

				block[code] = (idct_block_t)level;
				++len;
			}
		}

		DEBUG_MSG2(DEBUG_VCODEC2,T("inter_block[%i] %i"), code, level );
	}

	dec->bitpos = bitpos;
	return len;
}

static void blockInter( mpeg_decode* dec, int mb_type, int pos )
{
	int j;
	idct* IDCT = dec->Codec.IDCT.Ptr;

 	IDCT->Process(IDCT,POSX(pos),POSY(pos));
	IDCT->MComp16x16(IDCT,
		dec->fmv[1]==NOMV?NULL:dec->fmv+1,
		dec->bmv[1]==NOMV?NULL:dec->bmv+1);

	if (mb_type & MB_PAT)
	{
		int len,cbp;

		loadbits(dec);
		len = showbits(dec,9);

		if (len >= 128)
			len >>= 4;
		else
			len += 32;

		len <<= 1;
		flushbits(dec,mcbp_p[len]);
		cbp = mcbp_p[len+1];

		for (j = 0; j < 6; j++, cbp+=cbp)
		{
			len = 0;
			if (cbp & 32)
				len = decodeInter(dec);
			IDCT->Inter8x8(IDCT,dec->blockptr,len);
		}
	}
	else
	{
		for (j = 0; j < 6; j++)
			IDCT->Inter8x8(IDCT,dec->blockptr,0);
	}

	dec->last_dc[2] =
	dec->last_dc[1] =
	dec->last_dc[0] = 128;
}

static int IVOP_Slice( mpeg_decode* dec, int pos )
{
	dec->lastrefframe = dec->Frame;
	dec->mapofs = dec->Frame;
	memset(dec->framemap,0,dec->pos_end); // set all block to current frame

	for (;pos<dec->pos_end;pos+=MB_X-dec->mb_xsize) 
	{
		for (;POSX(pos)<dec->mb_xsize;++pos) 
		{
			if (!getbits1(dec))
			{
				flushbits(dec,1); // should be 1
	            dec->qscale = readqscale(dec);
			}

			blockIntra(dec, pos);

			inlineloadbits(dec);
			if (showbits(dec,8)==0) // eof slice
				return ERR_NONE;

			if (!getbits1(dec))
				return ERR_NONE; // skip invalid with IVOP
		}
	}

	return ERR_NONE;
}

static int PVOP_Slice( mpeg_decode* dec, int pos )
{
	dec->currframemap = (dec->Frame - dec->mapofs) << 1;
	dec->lastrefframe = dec->Frame;
	dec->bmv[1] = NOMV;

	for (;pos<dec->pos_end;pos+=MB_X-dec->mb_xsize) 
	{
		for (;POSX(pos)<dec->mb_xsize;++pos) 
		{
			if (!dec->skip)
			{
				int mb_type;

				DEBUG_MSG3(DEBUG_VCODEC2,T("macro %d,%d %08x"),POSX(pos),POSY(pos),showbitslarge((loadbits(dec),dec),32));

				dec->framemap[pos] = (uint8_t)dec->currframemap;

				mb_type = mb_type_p[showbits(dec,6)];
				flushbits(dec,mb_type >> 5); // after this 6bits left

				if (mb_type & MB_QUANT)
		            dec->qscale = readqscale(dec);

				if (mb_type & MB_INTRA)
					blockIntra(dec, pos);
				else
				{
					if (mb_type & MB_FOR)
						GetMV(dec->fcode[0],dec->fmv,dec->full_pixel[0],dec);
					else
						dec->fmv[6] = dec->fmv[5] = dec->fmv[4] = dec->fmv[3] = 
						dec->fmv[2] = dec->fmv[1] = dec->fmv[0] = 0;

					DEBUG_MSG3(DEBUG_VCODEC2,T("mv%d %d:%d"),0,MVX(dec->fmv[1]),MVY(dec->fmv[1]));
						
					blockInter( dec, mb_type, pos );
				}				

				inlineloadbits(dec);
				if (showbits(dec,8)==0) // eof slice
					return ERR_NONE;

				readskip(dec); // after this 12bits left
			}
			else
			{
				int n;
				dec->skip--;
				// not coded macroblock
				n = dec->framemap[pos];
				// copy needed or the buffer already has this block?
				if (dec->bufframe < dec->mapofs+(n>>1))
					dec->Codec.IDCT.Ptr->Copy16x16(dec->Codec.IDCT.Ptr,POSX(pos),POSY(pos),0);

				dec->fmv[0] = 0;
				dec->last_dc[2] =
				dec->last_dc[1] =
				dec->last_dc[0] = 128;
			}
		}
	}

	return ERR_NONE;
}

static int BVOP_Slice( mpeg_decode* dec, int pos )
{
	for (;pos<dec->pos_end;pos+=MB_X-dec->mb_xsize) 
	{
		for (;POSX(pos)<dec->mb_xsize;++pos) 
		{
			if (!dec->skip)
			{
				int mb_type;

				DEBUG_MSG3(DEBUG_VCODEC2,T("macro %d,%d %08x"),POSX(pos),POSY(pos),showbitslarge((loadbits(dec),dec),32));

				//dec->framemap[pos] |= 1;

				mb_type = mb_type_b[showbits(dec,6)];
				flushbits(dec,mb_type >> 5); // after this 6bits left

				if (mb_type & MB_QUANT)
		            dec->qscale = readqscale(dec);

				if (mb_type & MB_INTRA)
					blockIntra(dec, pos);
				else
				{
					if (mb_type & MB_FOR)
					{
						GetMV(dec->fcode[0],dec->fmv,dec->full_pixel[0],dec);
						DEBUG_MSG3(DEBUG_VCODEC2,T("mv%d %d:%d"),0,MVX(dec->fmv[1]),MVY(dec->fmv[1]));
					}
					else
						dec->fmv[1] = NOMV;

					if (mb_type & MB_BACK)
					{
						GetMV(dec->fcode[1],dec->bmv,dec->full_pixel[1],dec);
						DEBUG_MSG3(DEBUG_VCODEC2,T("mv%d %d:%d"),1,MVX(dec->bmv[1]),MVY(dec->bmv[1]));
					}
					else
						dec->bmv[1] = NOMV;

					blockInter( dec, mb_type, pos );
				}				

				inlineloadbits(dec);
				if (showbits(dec,8)==0) // eof slice
					return ERR_NONE;

				readskip(dec); // after this 12bits left
			}
			else
			{
				// not coded macroblock, use last motion compensation vectors
				dec->skip--;

				if (dec->fmv[1]==0 && dec->bmv[1]==NOMV)
				{
					dec->Codec.IDCT.Ptr->Copy16x16(dec->Codec.IDCT.Ptr,POSX(pos),POSY(pos),0);
					dec->last_dc[2] =
					dec->last_dc[1] =
					dec->last_dc[0] = 128;
				}
				else
				if (dec->bmv[1]==0 && dec->fmv[1]==NOMV)
				{
					dec->Codec.IDCT.Ptr->Copy16x16(dec->Codec.IDCT.Ptr,POSX(pos),POSY(pos),1);
					dec->last_dc[2] =
					dec->last_dc[1] =
					dec->last_dc[0] = 128;
				}
				else
					blockInter(dec, 0, pos );
			}
		}
	}

	return ERR_NONE;
}

static int Frame( mpeg_decode* dec, const uint8_t* Ptr, int Len )
{
	int Result;
	int Code;
	
	if (Len == 0)
		return ERR_INVALID_DATA;

	initbits(dec,Ptr,Len);

	Result = ERR_NONE;

	DEBUG_MSG1(DEBUG_VCODEC,T("MPEG Frame Length:%d"),Len);

	dec->frame_state = -1;
	do
	{
		bytealign(dec);
		loadbits(dec);
		if (eofbits(dec))
			break;

		Code = showbits(dec,32);
		if ((Code & ~0xFF)==0x100)
		{
			flushbits(dec,32);
			if (Code >= SLICE_MIN_START_CODE && Code <= SLICE_MAX_START_CODE)
			{
				// found a slice

				Code = (Code-1) & 0xFF;
				if (Code >= dec->mb_ysize) 
				{
					Result = ERR_INVALID_DATA;
					break;
				}

				if (dec->frame_state)
				{
					if (dec->frame_state<0 || dec->Codec.IDCT.Count<2)
					{
						Result = ERR_INVALID_DATA; // no picture header
						break;
					}

					Result = StartFrame(dec);
					if (Result != ERR_NONE || dec->Codec.Show < 0)
						break;

					dec->frame_state = 0;
				}

				loadbits(dec);
				dec->qscale = readqscale(dec);

				while (getbits1(dec)) 
				{
					flushbits(dec,8);
					loadbits(dec);
				}

				readskip(dec);
				Code = MB_X*Code + dec->skip;
				dec->skip = 0;

				dec->last_dc[2] =
				dec->last_dc[1] =
				dec->last_dc[0] = 128;
				dec->fmv[0] = dec->bmv[0] = 0;

				loadbits(dec);

				switch (dec->prediction_type) 
				{
				case P_VOP:
					Result = PVOP_Slice(dec,Code);
					break;
				case I_VOP:
					Result = IVOP_Slice(dec,Code);
					break;
				case B_VOP:
					Result = BVOP_Slice(dec,Code);
					break;
				}
			}
			else
			if (Code == PICTURE_START_CODE)
			{
				if (dec->ValidSeq)
					Picture(dec);
				else
					Result = ERR_INVALID_DATA;
			}
			else
			if (Code == SEQ_START_CODE)
				Sequence(dec);
			else
			if (Code == EXT_START_CODE)
			{
				if (!dec->ErrorShowed)
				{
					pin Pin;
					Pin.No = CODECIDCT_INPUT;
					Pin.Node = &dec->Codec.Node;

					dec->ErrorShowed = 1;
					if (!dec->Codec.NotSupported.Node || 
						dec->Codec.NotSupported.Node->Set(dec->Codec.NotSupported.Node,dec->Codec.NotSupported.No,
						&Pin,sizeof(pin))!=ERR_NONE)
						ShowError(dec->Codec.Node.Class,MPEG1_ID,MPEG2_NOT_SUPPORTED);
					else
						Result = ERR_INVALID_DATA;
				}
			}
		}
		else
			flushbits(dec,8);
	}
	while (Result == ERR_NONE);

	if (dec->frame_state==0)
	{
		dec->Codec.IDCT.Ptr->FrameEnd(dec->Codec.IDCT.Ptr);
		dec->Frame++;

		// possible (uint8) framemap overflow?
		if ((dec->Frame - dec->mapofs) >= 128) 
		{
			int pos;
			for (pos=0;pos<dec->pos_end;pos+=MB_X-dec->mb_xsize)
				for (;POSX(pos)<dec->mb_xsize;++pos)
				{
					int i = dec->framemap[pos];
					if (i >= (120<<1))
						i -= 120<<1;
					else
						i &= 1;
					dec->framemap[pos] = (uint8_t)i;
				}

			dec->mapofs += 120;
		}
	}

	return Result;
}

static int Create( mpeg_decode* p )
{
	p->Codec.MinCount = 2;
	p->Codec.DefCount = 3; // default assuming B-frames (avoid reinit when reaching first B-frame)
	p->Codec.FindNext = (codecidctnext)FindNext;
	p->Codec.Frame = (codecidctframe)Frame;
	p->Codec.UpdateSize = (nodefunc)UpdateSize;
	p->Codec.UpdateInput = (nodefunc)UpdateInput;
	p->Codec.Discontinuity = (nodefunc)Discontinuity;
	p->Codec.Flush = (nodefunc)Flush;
	return ERR_NONE;
}

static const nodedef MPEG1 =
{
	sizeof(mpeg_decode),
	MPEG1_ID,
	CODECIDCT_CLASS,
	PRI_DEFAULT,
	(nodecreate)Create,
	NULL,
};

void MPEG1_Init()
{
	NodeRegisterClass(&MPEG1);
}

void MPEG1_Done()
{
	NodeUnRegisterClass(MPEG1_ID);
}
