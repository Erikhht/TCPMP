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
 * $Id: intel2700g_idct422.h 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

void IDCTProcess_422(void* p,int x,int y)
{
	int Pos;

	x <<= 4;
	y <<= 3; 

	Pos = (x << GXVA_IMAGE_WRITE_X_SHIFT) | (y << GXVA_IMAGE_WRITE_Y_SHIFT);

	PVR(p)->mx = x;
	PVR(p)->my = y;
	PVR(p)->SubPos = 0;

	PVR(p)->ImageWrite[1] = 
	PVR(p)->ImageWrite[2] = (Pos >> 1) |
		GXVA_CMD_IMAGE_WRITE |
		GXVA_IMAGE_WRITE_WIDTH_8 |
		GXVA_IMAGE_WRITE_HGHT_4;

	PVR(p)->ImageWrite[0] = Pos |
		GXVA_CMD_IMAGE_WRITE |
		GXVA_IMAGE_WRITE_WIDTH_16 |
		GXVA_IMAGE_WRITE_HGHT_8;

	PVR(p)->M24VA_WriteMCCmdData(GXVA_CMD_MC_OPP_TYPE|GXVA_MC_TYPE_COL_PLANE_Y);
}

static INLINE void IDCT8x8_422(pvr* p,idct_block_t *Block,int Length,const uint8_t* Scan)
{
	const uint8_t *ScanEnd = Scan + Length;
	int16_t* Data = p->IZZ;
	int v;												
	int d;
	for (;;Data+=2)									
	{	
		do
		{
			if (Scan == ScanEnd) goto end;
			v = *(Scan++);
			d = Block[v];
		} while (!d);

		v <<= 1;
		Data[0] = (int16_t)v;
		Data[1] = (int16_t)d;	
	}				

end:	
	if (Data == p->IZZ)
	{
		*(int32_t*)Data = 0;
		Data += 2;
	}
	Data[-2] |= 1;
	p->M24VA_WriteIZZBlock((gx_int32*)p->IZZ,(gx_int32*)Data - (gx_int32*)p->IZZ);
}

static INLINE void IDCT8x4_422(pvr* p,idct_block_t *Block,int Length,const uint8_t* Scan)
{
	const uint8_t *ScanEnd = Scan + Length;
	int16_t* Data = p->IZZ;
	int v,w;												
	int d;
	for (;;Data+=2)									
	{	
		do
		{
skip:
			if (Scan == ScanEnd) goto end;
			w = Scan[64];
			v = *(Scan++);
			if (w>=64) goto skip;
			d = Block[v];
		} while (!d);

		w <<= 1;
		Data[0] = (int16_t)w;
		Data[1] = (int16_t)d;	
	}				

end:	
	if (Data == p->IZZ)
	{
		*(int32_t*)Data = 0;
		Data += 2;
	}
	Data[-2] |= 1;
	p->M24VA_WriteIZZBlock((gx_int32*)p->IZZ,(gx_int32*)Data - (gx_int32*)p->IZZ);
}

int Intra422[4] =
{
	GXVA_CMD_IDCT_MODE|GXVA_IDCT_MODE_INTRA|GXVA_IDCT_BLK_SIZEX_8|GXVA_IDCT_BLK_SIZEY_8|GXVA_IDCT_MODE_BLK_TOP_LEFT,
	GXVA_CMD_IDCT_MODE|GXVA_IDCT_MODE_INTRA|GXVA_IDCT_BLK_SIZEX_8|GXVA_IDCT_BLK_SIZEY_8|GXVA_IDCT_MODE_BLK_TOP_RGHT,
	GXVA_CMD_IDCT_MODE|GXVA_IDCT_MODE_INTRA|GXVA_IDCT_BLK_SIZEX_8|GXVA_IDCT_BLK_SIZEY_8|GXVA_IDCT_MODE_BLK_TOP_LEFT,
	GXVA_CMD_IDCT_MODE|GXVA_IDCT_MODE_INTRA|GXVA_IDCT_BLK_SIZEX_8|GXVA_IDCT_BLK_SIZEY_8|GXVA_IDCT_MODE_BLK_TOP_LEFT
};

void IDCTIntra8x8_422(void* p,idct_block_t *Block,int Length,int ScanType)
{
	int SubPos = PVR(p)->SubPos++;
	if (SubPos >= 2)
	{
		PVR(p)->M24VA_WriteMCCmdData(
			(SubPos - 1)| // GXVA_MC_TYPE_COL_PLANE_U, GXVA_MC_TYPE_COL_PLANE_V
			GXVA_CMD_MC_OPP_TYPE);
		PVR(p)->M24VA_WriteMCCmdData(Intra422[SubPos]);
		IDCT8x4_422(PVR(p),Block,Length,ScanTable8x4[ScanType]);
		PVR(p)->M24VA_WriteMCCmdData(PVR(p)->ImageWrite[SubPos-1]);
	}
	else
	{
		PVR(p)->M24VA_WriteMCCmdData(Intra422[SubPos]);
		IDCT8x8_422(PVR(p),Block,Length,ScanTable[ScanType]);
		if (SubPos)
			PVR(p)->M24VA_WriteMCCmdData(PVR(p)->ImageWrite[0]);
	}
}

