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
 * $Id: mpeg_decode.h 603 2006-01-19 13:00:33Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __MPEG_DECODE_H
#define __MPEG_DECODE_H

// maximun picture size in macroblocks (16x16)
// example MB_X2=6 -> 2^6=64 macroblocks -> 1024 pixels

#define MB_X2	6
#define MB_Y2	6
#define MB_X	(1<<MB_X2)
#define MB_Y	(1<<MB_Y2)

#define SEQ_END_CODE			0x1B7
#define SEQ_START_CODE			0x1B3
#define GOP_START_CODE			0x1B8
#define PICTURE_START_CODE		0x100
#define SLICE_MIN_START_CODE	0x101
#define SLICE_MAX_START_CODE	0x1AF
#define EXT_START_CODE			0x1B5
#define USER_START_CODE			0x1B2

#define I_VOP	1
#define P_VOP	2
#define B_VOP	3

#define POSX(pos) ((pos) & (MB_X-1))
#define POSY(pos) ((pos) >> MB_X2)

#define MB_QUANT	0x01
#define MB_FOR		0x02
#define MB_BACK		0x04
#define MB_PAT		0x08
#define MB_INTRA	0x10

#define MPEG1_FRAME_RATE_BASE 1001

typedef struct mpeg_decode
{
	codecidct Codec;

	// bitstream
	int bits;
	int bitpos;
	const uint8_t *bitptr;
	const uint8_t *bitend;

	boolmem_t ValidSeq;
	boolmem_t ErrorShowed;
	boolmem_t SliceFound;
	boolmem_t OnlyIVOP;
	int Frame;
	tick_t StartTime;
	int32_t StartState;

	int bufframe;		// frame number of buffer's previous state
	int refframe;
	int lastrefframe;	// frame number of last refframe
	int mapofs;			// mapofs + (framemap[pos] >> 1) is the last time that block was updated
	int currframemap;	// (frame - mapofs) << 1
	int qscale;
	int skip;
    int last_dc[3]; 
    int fmv[1+6]; // last y, new y[4], new uv[2]
    int bmv[1+6]; // last y, new y[4], new uv[2]

	idct_block_t* blockptr;
	IDCT_BLOCK_DECLARE

	int mb_xsize;
	int mb_ysize;
	int pos_end;
	int prediction_type;
	int frame_state; // -1:no info 0:decoding 1:need startframe

	int full_pixel[2];
	int fcode[2];

	uint8_t zigzag[64];
    uint8_t IntraMatrix[64];
    uint8_t InterMatrix[64];

	uint8_t framemap[MB_X*MB_Y]; // when the last time the block changed (and resuce needed flag)

	int width;
	int height;
	int aspect;

} mpeg_decode;

#endif
