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
 * $Id: intel2700g_idct420.h 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

static void FUNC(IDCTProcess)(void* p,int x,int y)
{
	int Pos;

	x <<= 4;
	y <<= 4; 
	MIRRORXY

	Pos = (x << IW_XSHIFT) | (y << IW_YSHIFT);

	PVR(p)->mx = x;
	PVR(p)->my = y;
	PVR(p)->SubPos = 0;

	PVR(p)->ImageWrite[1] = 
	PVR(p)->ImageWrite[2] = (Pos >> 1) |
		GXVA_CMD_IMAGE_WRITE |
		GXVA_IMAGE_WRITE_WIDTH_8 |
		GXVA_IMAGE_WRITE_HGHT_8;

	PVR(p)->ImageWrite[0] = Pos |
		GXVA_CMD_IMAGE_WRITE |
		GXVA_IMAGE_WRITE_WIDTH_16 |
		GXVA_IMAGE_WRITE_HGHT_16;
}

static INLINE void FUNC(IDCT8x8)(pvr* p,idct_block_t *Block,int Length,const uint8_t* Scan)
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

		IZZDATA
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

static void FUNC(IDCTCopy16x16)(void* p,int x,int y,int No)
{
	int FetchA,FetchB,ImageWrite;

	x <<= 4;
	y <<= 4;
	MIRRORXY

	FetchA = 
		(x << (FA_XSHIFT+1)) | 
		(y << (FA_YSHIFT+1)) |
		GXVA_CMD_FETCH_PRED_A |
		GXVA_FETCH_A_FIRST_PRED |
		(No ? GXVA_FETCH_A_SRC_SEL_REF2 : GXVA_FETCH_A_SRC_SEL_REF1);

	FetchB = 
		GXVA_CMD_FETCH_PRED_B |
		GXVA_FETCH_B_ACC_Y_POS_0 |
		GXVA_FETCH_B_ACC_X_POS_0;

	ImageWrite = 
		(x << IW_XSHIFT) |
		(y << IW_YSHIFT) |
		GXVA_CMD_IMAGE_WRITE |
		GXVA_IMAGE_WRITE_WIDTH_16 |
		GXVA_IMAGE_WRITE_HGHT_16;

	PVR(p)->M24VA_WriteMCCmdData(
		GXVA_CMD_MC_OPP_TYPE |
		GXVA_MC_TYPE_COL_PLANE_Y |
		GXVA_MC_TYPE_BLK_WIDTH_16 |
		GXVA_MC_TYPE_BLK_HGHT_16);

	PVR(p)->M24VA_WriteMCCmdData(FetchB);
	PVR(p)->M24VA_WriteMCCmdData(FetchA);
	PVR(p)->M24VA_WriteMCCmdData(ImageWrite);

	FetchA &= ~(GXVA_FETCH_A_PREDX_MASK|GXVA_FETCH_A_PREDY_MASK);
	FetchA |= 	
		(x << FA_XSHIFT) |
		(y << FA_YSHIFT);

	ImageWrite = 
		((x >> 1) << IW_XSHIFT) |
		((y >> 1) << IW_YSHIFT) |
		GXVA_CMD_IMAGE_WRITE |
		GXVA_IMAGE_WRITE_WIDTH_8 |
		GXVA_IMAGE_WRITE_HGHT_8;

	PVR(p)->M24VA_WriteMCCmdData(
		GXVA_CMD_MC_OPP_TYPE |
		GXVA_MC_TYPE_COL_PLANE_U |
		GXVA_MC_TYPE_BLK_WIDTH_8 |
		GXVA_MC_TYPE_BLK_HGHT_8);

	PVR(p)->M24VA_WriteMCCmdData(FetchB);
	PVR(p)->M24VA_WriteMCCmdData(FetchA);
	PVR(p)->M24VA_WriteMCCmdData(ImageWrite);

	PVR(p)->M24VA_WriteMCCmdData(
		GXVA_CMD_MC_OPP_TYPE |
		GXVA_MC_TYPE_COL_PLANE_V |
		GXVA_MC_TYPE_BLK_WIDTH_8 |
		GXVA_MC_TYPE_BLK_HGHT_8);

	PVR(p)->M24VA_WriteMCCmdData(FetchB);
	PVR(p)->M24VA_WriteMCCmdData(FetchA);
	PVR(p)->M24VA_WriteMCCmdData(ImageWrite);
}

static INLINE int FUNC(MVConvert)(const int16_t* MV,int x,int y)
{
	return 
		(((x MVDIRX MV[MVXIDX]) << FA_XSHIFT) & FA_XMASK) |
		(((y MVDIRY MV[MVYIDX]) << FA_YSHIFT) & FA_YMASK);
}

static void FUNC(IDCTMComp16x16Back)(void* p,const int16_t* MVBack,const int16_t* MVFwd)
{
	int mx,my,cmd;

	PVR(p)->M24VA_WriteMCCmdData(PVR(p)->MCompType|GXVA_MC_TYPE_COL_PLANE_Y|GXVA_MC_TYPE_BLK_WIDTH_16|GXVA_MC_TYPE_BLK_HGHT_16);

	PVR(p)->M24VA_WriteMCCmdData(GXVA_CMD_FETCH_PRED_B|GXVA_FETCH_B_ACC_Y_POS_0|GXVA_FETCH_B_ACC_X_POS_0);

	mx = PVR(p)->mx;
	my = PVR(p)->my;
	cmd = GXVA_CMD_FETCH_PRED_A | GXVA_FETCH_A_FIRST_PRED | GXVA_FETCH_A_SRC_SEL_REF1;

	PVR(p)->FetchA[0][0] = FUNC(MVConvert)(MVBack+8,mx,my) | cmd;
	PVR(p)->FetchA[0][1] = FUNC(MVConvert)(MVBack+10,mx,my) | cmd;
	PVR(p)->M24VA_WriteMCCmdData(FUNC(MVConvert)(MVBack+0,mx*2,my*2) | cmd);
}

static void FUNC(IDCTMComp16x16BackFwd)(void* p,const int16_t* MVBack,const int16_t* MVFwd)
{
	int mx,my,cmd;

	PVR(p)->M24VA_WriteMCCmdData(PVR(p)->MCompType|GXVA_MC_TYPE_COL_PLANE_Y|GXVA_MC_TYPE_BLK_WIDTH_16|GXVA_MC_TYPE_BLK_HGHT_16);
	PVR(p)->M24VA_WriteMCCmdData(GXVA_CMD_FETCH_PRED_B|GXVA_FETCH_B_ACC_Y_POS_0|GXVA_FETCH_B_ACC_X_POS_0);

	PVR(p)->FetchA[0][0] = 0;
	PVR(p)->FetchA[0][1] = 0;
	PVR(p)->FetchA[1][0] = 0;
	PVR(p)->FetchA[1][1] = 0;

	mx = PVR(p)->mx;
	my = PVR(p)->my;
	cmd = GXVA_FETCH_A_FIRST_PRED;

	if (MVBack)
	{
		cmd |= GXVA_CMD_FETCH_PRED_A|GXVA_FETCH_A_SRC_SEL_REF1;

		PVR(p)->FetchA[0][0] = FUNC(MVConvert)(MVBack+8,mx,my) | cmd;
		PVR(p)->FetchA[0][1] = FUNC(MVConvert)(MVBack+10,mx,my) | cmd;
		PVR(p)->M24VA_WriteMCCmdData(FUNC(MVConvert)(MVBack+0,mx*2,my*2) | cmd);

		cmd = 0;
	}
	if (MVFwd)
	{
		cmd |= GXVA_CMD_FETCH_PRED_A|GXVA_FETCH_A_SRC_SEL_REF2;

		PVR(p)->FetchA[1][0] = FUNC(MVConvert)(MVFwd+8,mx,my) | cmd;
		PVR(p)->FetchA[1][1] = FUNC(MVConvert)(MVFwd+10,mx,my) | cmd;
		PVR(p)->M24VA_WriteMCCmdData(FUNC(MVConvert)(MVFwd+0,mx*2,my*2) | cmd);
	}
}

static void FUNC(IDCTMComp8x8Back)(void* p,const int16_t* MVBack,const int16_t* MVFwd)
{
	int mx = PVR(p)->mx;
	int my = PVR(p)->my;
	int cmd = GXVA_CMD_FETCH_PRED_A | GXVA_FETCH_A_FIRST_PRED | GXVA_FETCH_A_SRC_SEL_REF1;

	PVR(p)->M24VA_WriteMCCmdData(PVR(p)->MCompType|GXVA_MC_TYPE_COL_PLANE_Y|GXVA_MC_TYPE_BLK_WIDTH_8|GXVA_MC_TYPE_BLK_HGHT_8);

	PVR(p)->FetchA[0][0] = FUNC(MVConvert)(MVBack+8,mx,my) | cmd;
	PVR(p)->FetchA[0][1] = FUNC(MVConvert)(MVBack+10,mx,my) | cmd;

	mx <<= 1;
	my <<= 1;

	PVR(p)->M24VA_WriteMCCmdData(GXVA_CMD_FETCH_PRED_B|MVPOS_00);
	PVR(p)->M24VA_WriteMCCmdData(FUNC(MVConvert)(MVBack+MVIDX_00,mx,my) | cmd);
	PVR(p)->M24VA_WriteMCCmdData(GXVA_CMD_FETCH_PRED_B|MVPOS_01);
	PVR(p)->M24VA_WriteMCCmdData(FUNC(MVConvert)(MVBack+MVIDX_01,mx+16,my) | cmd);
	PVR(p)->M24VA_WriteMCCmdData(GXVA_CMD_FETCH_PRED_B|MVPOS_10);
	PVR(p)->M24VA_WriteMCCmdData(FUNC(MVConvert)(MVBack+MVIDX_10,mx,my+16) | cmd);
	PVR(p)->M24VA_WriteMCCmdData(GXVA_CMD_FETCH_PRED_B|MVPOS_11);
	PVR(p)->M24VA_WriteMCCmdData(FUNC(MVConvert)(MVBack+MVIDX_11,mx+16,my+16) | cmd);
}

static void FUNC(IDCTMComp8x8BackFwd)(void* p,const int16_t* MVBack,const int16_t* MVFwd)
{
	int mx = PVR(p)->mx;
	int my = PVR(p)->my;
	int cmd = GXVA_FETCH_A_FIRST_PRED;

	PVR(p)->FetchA[0][0] = 0;
	PVR(p)->FetchA[0][1] = 0;
	PVR(p)->FetchA[1][0] = 0;
	PVR(p)->FetchA[1][1] = 0;

	PVR(p)->M24VA_WriteMCCmdData(PVR(p)->MCompType|GXVA_MC_TYPE_COL_PLANE_Y|GXVA_MC_TYPE_BLK_WIDTH_8|GXVA_MC_TYPE_BLK_HGHT_8);

	if (MVBack)
	{
		cmd |= GXVA_CMD_FETCH_PRED_A|GXVA_FETCH_A_SRC_SEL_REF1;

		PVR(p)->FetchA[0][0] = FUNC(MVConvert)(MVBack+8,mx,my) | cmd;
		PVR(p)->FetchA[0][1] = FUNC(MVConvert)(MVBack+10,mx,my) | cmd;

		mx <<= 1;
		my <<= 1;

		PVR(p)->M24VA_WriteMCCmdData(GXVA_CMD_FETCH_PRED_B|MVPOS_00);
		PVR(p)->M24VA_WriteMCCmdData(FUNC(MVConvert)(MVBack+MVIDX_00,mx,my) | cmd);
		PVR(p)->M24VA_WriteMCCmdData(GXVA_CMD_FETCH_PRED_B|MVPOS_01);
		PVR(p)->M24VA_WriteMCCmdData(FUNC(MVConvert)(MVBack+MVIDX_01,mx+16,my) | cmd);
		PVR(p)->M24VA_WriteMCCmdData(GXVA_CMD_FETCH_PRED_B|MVPOS_10);
		PVR(p)->M24VA_WriteMCCmdData(FUNC(MVConvert)(MVBack+MVIDX_10,mx,my+16) | cmd);
		PVR(p)->M24VA_WriteMCCmdData(GXVA_CMD_FETCH_PRED_B|MVPOS_11);
		PVR(p)->M24VA_WriteMCCmdData(FUNC(MVConvert)(MVBack+MVIDX_11,mx+16,my+16) | cmd);

		mx >>= 1;
		my >>= 1;
		cmd = 0;
	}

	if (MVFwd)
	{
		cmd |= GXVA_CMD_FETCH_PRED_A|GXVA_FETCH_A_SRC_SEL_REF2;

		PVR(p)->FetchA[1][0] = FUNC(MVConvert)(MVFwd+8,mx,my) | cmd;
		PVR(p)->FetchA[1][1] = FUNC(MVConvert)(MVFwd+10,mx,my) | cmd;

		mx <<= 1;
		my <<= 1;

		PVR(p)->M24VA_WriteMCCmdData(GXVA_CMD_FETCH_PRED_B|MVPOS_00);
		PVR(p)->M24VA_WriteMCCmdData(FUNC(MVConvert)(MVFwd+MVIDX_00,mx,my) | cmd);
		PVR(p)->M24VA_WriteMCCmdData(GXVA_CMD_FETCH_PRED_B|MVPOS_01);
		PVR(p)->M24VA_WriteMCCmdData(FUNC(MVConvert)(MVFwd+MVIDX_01,mx+16,my) | cmd);
		PVR(p)->M24VA_WriteMCCmdData(GXVA_CMD_FETCH_PRED_B|MVPOS_10);
		PVR(p)->M24VA_WriteMCCmdData(FUNC(MVConvert)(MVFwd+MVIDX_10,mx,my+16) | cmd);
		PVR(p)->M24VA_WriteMCCmdData(GXVA_CMD_FETCH_PRED_B|MVPOS_11);
		PVR(p)->M24VA_WriteMCCmdData(FUNC(MVConvert)(MVFwd+MVIDX_11,mx+16,my+16) | cmd);
	}
}

static const int FUNC(Intra420)[6] =
{
	GXVA_CMD_IDCT_MODE|GXVA_IDCT_MODE_INTRA|GXVA_IDCT_BLK_SIZEX_8|GXVA_IDCT_BLK_SIZEY_8|IDCT_00,
	GXVA_CMD_IDCT_MODE|GXVA_IDCT_MODE_INTRA|GXVA_IDCT_BLK_SIZEX_8|GXVA_IDCT_BLK_SIZEY_8|IDCT_01,
	GXVA_CMD_IDCT_MODE|GXVA_IDCT_MODE_INTRA|GXVA_IDCT_BLK_SIZEX_8|GXVA_IDCT_BLK_SIZEY_8|IDCT_10,
	GXVA_CMD_IDCT_MODE|GXVA_IDCT_MODE_INTRA|GXVA_IDCT_BLK_SIZEX_8|GXVA_IDCT_BLK_SIZEY_8|IDCT_11,
	GXVA_CMD_IDCT_MODE|GXVA_IDCT_MODE_INTRA|GXVA_IDCT_BLK_SIZEX_8|GXVA_IDCT_BLK_SIZEY_8|GXVA_IDCT_MODE_BLK_TOP_LEFT,
	GXVA_CMD_IDCT_MODE|GXVA_IDCT_MODE_INTRA|GXVA_IDCT_BLK_SIZEX_8|GXVA_IDCT_BLK_SIZEY_8|GXVA_IDCT_MODE_BLK_TOP_LEFT
};

static const int FUNC(Inter420)[6] =
{
	GXVA_CMD_IDCT_MODE|GXVA_IDCT_BLK_SIZEX_8|GXVA_IDCT_BLK_SIZEY_8|IDCT_00,
	GXVA_CMD_IDCT_MODE|GXVA_IDCT_BLK_SIZEX_8|GXVA_IDCT_BLK_SIZEY_8|IDCT_01,
	GXVA_CMD_IDCT_MODE|GXVA_IDCT_BLK_SIZEX_8|GXVA_IDCT_BLK_SIZEY_8|IDCT_10,
	GXVA_CMD_IDCT_MODE|GXVA_IDCT_BLK_SIZEX_8|GXVA_IDCT_BLK_SIZEY_8|IDCT_11,
	GXVA_CMD_IDCT_MODE|GXVA_IDCT_BLK_SIZEX_8|GXVA_IDCT_BLK_SIZEY_8|GXVA_IDCT_MODE_BLK_TOP_LEFT,
	GXVA_CMD_IDCT_MODE|GXVA_IDCT_BLK_SIZEX_8|GXVA_IDCT_BLK_SIZEY_8|GXVA_IDCT_MODE_BLK_TOP_LEFT
};

static void FUNC(IDCTIntra8x8)(void* p,idct_block_t *Block,int Length,int ScanType)
{
	int SubPos = PVR(p)->SubPos++;
	if (!SubPos)
		PVR(p)->M24VA_WriteMCCmdData(
			GXVA_CMD_MC_OPP_TYPE|
			GXVA_MC_TYPE_COL_PLANE_Y |
			GXVA_MC_TYPE_BLK_WIDTH_16 |
			GXVA_MC_TYPE_BLK_HGHT_16);
	else
	if (SubPos >= 4)
		PVR(p)->M24VA_WriteMCCmdData(
			(SubPos - 3)| // GXVA_MC_TYPE_COL_PLANE_U, GXVA_MC_TYPE_COL_PLANE_V
			GXVA_CMD_MC_OPP_TYPE|
			GXVA_MC_TYPE_BLK_WIDTH_8 |
			GXVA_MC_TYPE_BLK_HGHT_8);
	
	PVR(p)->M24VA_WriteMCCmdData(FUNC(Intra420)[SubPos]);
	FUNC(IDCT8x8)(PVR(p),Block,Length,ScanTable[ScanType]);

	SubPos -= 3;
	if (SubPos >= 0)
		PVR(p)->M24VA_WriteMCCmdData(PVR(p)->ImageWrite[SubPos]);
}

static void FUNC(IDCTInter8x8Back)(void* p,idct_block_t *Block,int Length)
{
	int SubPos = PVR(p)->SubPos++;
	if (SubPos >= 4)
	{
		PVR(p)->M24VA_WriteMCCmdData(
			PVR(p)->MCompType |
			GXVA_MC_TYPE_BLK_WIDTH_8 |
			GXVA_MC_TYPE_BLK_HGHT_8 |
			(SubPos - 3)); // GXVA_MC_TYPE_COL_PLANE_U, GXVA_MC_TYPE_COL_PLANE_V

		//FetchB
		PVR(p)->M24VA_WriteMCCmdData(
			GXVA_CMD_FETCH_PRED_B |
			GXVA_FETCH_B_ACC_Y_POS_0 |
			GXVA_FETCH_B_ACC_X_POS_0);

		//FetchA
		PVR(p)->M24VA_WriteMCCmdData(PVR(p)->FetchA[0][SubPos-4]);
	}
	
	if (Length)
	{
		PVR(p)->M24VA_WriteMCCmdData(FUNC(Inter420)[SubPos]);
		FUNC(IDCT8x8)(PVR(p),Block,Length,ScanTable[0]);
	}

	SubPos -= 3;
	if (SubPos >= 0)
		PVR(p)->M24VA_WriteMCCmdData(PVR(p)->ImageWrite[SubPos]);
}

static void FUNC(IDCTInter8x8BackFwd)(void* p,idct_block_t *Block,int Length)
{
	int* FetchA;
	int SubPos = PVR(p)->SubPos++;
	if (SubPos >= 4)
	{
		PVR(p)->M24VA_WriteMCCmdData(
			PVR(p)->MCompType |
			GXVA_MC_TYPE_BLK_WIDTH_8 |
			GXVA_MC_TYPE_BLK_HGHT_8 |
			(SubPos - 3)); // GXVA_MC_TYPE_COL_PLANE_U, GXVA_MC_TYPE_COL_PLANE_V

		//FetchB
		PVR(p)->M24VA_WriteMCCmdData(
			GXVA_CMD_FETCH_PRED_B |
			GXVA_FETCH_B_ACC_Y_POS_0 |
			GXVA_FETCH_B_ACC_X_POS_0);

		//FetchA
		FetchA = &PVR(p)->FetchA[0][SubPos-4];
		if (FetchA[0])
			PVR(p)->M24VA_WriteMCCmdData(FetchA[0]);
		if (FetchA[2])
			PVR(p)->M24VA_WriteMCCmdData(FetchA[2]);
	}
	
	if (Length)
	{
		PVR(p)->M24VA_WriteMCCmdData(FUNC(Inter420)[SubPos]);
		FUNC(IDCT8x8)(PVR(p),Block,Length,ScanTable[0]);
	}

	SubPos -= 3;
	if (SubPos >= 0)
		PVR(p)->M24VA_WriteMCCmdData(PVR(p)->ImageWrite[SubPos]);
}

#undef FUNC			
#undef FA_XMASK		
#undef FA_YMASK		
#undef FA_XSHIFT	
#undef FA_YSHIFT	
#undef IW_XSHIFT	
#undef IW_YSHIFT	
#undef MIRRORXY		
#undef IZZDATA
#undef MVDIRX		
#undef MVDIRY		
#undef MVPOS_00		
#undef MVIDX_00		
#undef MVPOS_01		
#undef MVIDX_01		
#undef MVPOS_10		
#undef MVIDX_10		
#undef MVPOS_11		
#undef MVIDX_11		
#undef IDCT_00		
#undef IDCT_01		
#undef IDCT_10	
#undef IDCT_11	
