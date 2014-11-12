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
 * $Id: aac_imdct.c 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../faad2/libfaad/common.h"

#ifdef LIBPAAC

#include "aac_imdct.h"
#include "../faad2/libfaad/mdct_tab.h"
#include "../faad2/libfaad/cfft_tab.h"
#include "../faad2/libfaad/kbd_win.h"
#include "../faad2/libfaad/sine_win.h"

/*
void AAC_Pre(real_t* Dst,const real_t* Src,const real_t* Table,int n);
void AAC_Post(real_t* Dst,const real_t* Src,const real_t* Table,int n);
void AAC_Pass2_N(real_t* Dst,const real_t* Src,const real_t* Table,int n,int m);
void AAC_Pass4_N(real_t* Dst,const real_t* Src,const real_t* Table,int n,int m);
void AAC_Pass4_1(real_t* Dst,const real_t* Src,int n);

void AAC_SubR(real_t* Dst,const real_t* a,const real_t* b,int n); // da=1, db=-1
void AAC_Mul(real_t* Dst,const real_t* a,const real_t* b,int n); // da=1, db=-1
void AAC_MulR(real_t* Dst,const real_t* a,const real_t* b,int n); // da=-1, db=-1
void AAC_AddMul(real_t* Dst,const real_t* a,const real_t* b,const real_t* c,int n); //da=1,db=1,dc=1
void AAC_AddNMul(real_t* Dst,const real_t* a,const real_t* b,const real_t* c,int n); //da=1,db=-1,dc=1
void AAC_AddMulMul(real_t* Dst,const real_t* a,const real_t* b1,const real_t* c1,const real_t* b2,const real_t* c2,int n); // da=1,db=1,dc=1
void AAC_AddMulNMul(real_t* Dst,const real_t* a,const real_t* b1,const real_t* c1,const real_t* b2,const real_t* c2,int n); // da=1,db=-1,dc=1
void AAC_MulMul(real_t* Dst,const real_t* b1,const real_t* c1,const real_t* b2,const real_t* c2,int n);
void AAC_MulNMul(real_t* Dst,const real_t* b1,const real_t* c1,const real_t* b2,const real_t* c2,int n);
void AAC_Copy(real_t* Dst,const real_t* Src,int n);
void AAC_Zero(real_t* Dst,int n);
*/

static void AAC_Pass4_1(real_t* Dst, const real_t* Src, int n)
{
	int i;
    for (i=0;i<n;++i)
    {
        real_t t1r, t1i, t2r, t2i, t3r, t3i, t4r, t4i;

        t2r = Src[0] + Src[4];
        t1r = Src[0] - Src[4];
        t2i = Src[1] + Src[5];
        t1i = Src[1] - Src[5];
        t3r = Src[2] + Src[6];
        t4i = Src[2] - Src[6];
        t3i = Src[7] + Src[3];
        t4r = Src[7] - Src[3];

        Dst[0+0*n] = t2r + t3r;
        Dst[0+4*n] = t2r - t3r;

        Dst[1+0*n] = t2i + t3i;
        Dst[1+4*n] = t2i - t3i;

        Dst[0+2*n] = t1r + t4r;
        Dst[0+6*n] = t1r - t4r;

        Dst[1+2*n] = t1i + t4i;
        Dst[1+6*n] = t1i - t4i;

		Src += 8;
		Dst += 2;
    }
}

static void AAC_Pass2_N(real_t* Dst, const real_t* Src, const real_t* Table, int n,int m)
{
	int i,j,o = n*m*2;
	for (i=0;i<n;++i)
	{
		for (j=0;j<m;++j)
		{
			real_t t2r, t2i;

            Dst[0] = Src[0+0*m] + Src[0+2*m];
            Dst[1] = Src[1+0*m] + Src[1+2*m];

            t2r = Src[0+0*m] - Src[0+2*m];
            t2i = Src[1+0*m] - Src[1+2*m];

            ComplexMult(Dst+1+o, Dst+0+o, t2i, t2r, Table[0], Table[1]);

			Table += 2;
			Dst += 2;
			Src += 2;
		}
		
		Src += 2*m;
		Table -= 2*m;
	}
}

static void AAC_Pass4_N(real_t* Dst, const real_t* Src, const real_t* Table, int n,int m)
{
	int i,j,o = n*m*2;
	for (i=0;i<n;++i)
	{
		for (j=0;j<m;++j)
		{

			real_t t1r, t1i, t2r, t2i, t3r, t3i, t4r, t4i;

			t2r = Src[0+0*m] + Src[0+4*m];
			t1r = Src[0+0*m] - Src[0+4*m];
			t2i = Src[1+0*m] + Src[1+4*m];
			t1i = Src[1+0*m] - Src[1+4*m];
			t3r = Src[0+2*m] + Src[0+6*m];
			t4i = Src[0+2*m] - Src[0+6*m];
			t3i = Src[1+6*m] + Src[1+2*m];
			t4r = Src[1+6*m] - Src[1+2*m];

            Dst[0] = t2r + t3r;
            Dst[1] = t2i + t3i;

            ComplexMult(Dst+1+o, Dst+0+o, 
				t1i + t4i, t1r + t4r, 
				Table[0], Table[1]);

            ComplexMult(Dst+1+2*o, Dst+0+2*o,
                t2i - t3i, t2r - t3r, 
				Table[m*2], Table[1+m*2]);

            ComplexMult(Dst+1+3*o, Dst+0+3*o,
                t1i - t4i, t1r - t4r, 
				Table[m*4], Table[1+m*4]);

			Dst += 2;
			Src += 2;
			Table += 2;
        }

		Src += 6*m;
		Table -= 2*m;
    }
}

static void AAC_Pre(real_t* Dst,const real_t* Src,const real_t* Table,int n)
{
	const real_t* Src2 = Src + n*2 - 1;
    do
    {
        ComplexMult(Dst+1,Dst+0,Src[0],Src2[0],Table[0],Table[1]);
		Dst += 2; Src += 2;	Src2 -= 2; Table += 2;
		--n;
    } while (n>0);
}

static void AAC_Post(real_t* Dst,const real_t* Src,const real_t* Table,int n)
{
	int i;
	real_t* Dst2;
	
	Dst += n;
	Dst2 = Dst+n;

	n >>= 1;
	for (i=0;i<n;++i)
	{
		ComplexMult(Dst2-1,Dst-1,Src[1],Src[0],-Table[0],-Table[1]);
//	    ComplexMult(Dst2-1,Dst-1,Src[1],Src[0],Table[0],Table[1]); Dst2[-1] = -Dst2[-1]; Dst[-1] = -Dst[-1];

		Dst -= 2;  Dst2 -= 2; Src += 2;	Table += 2;
	}

	for (i=0;i<n;++i)
	{
	    ComplexMult(Dst,Dst2,Src[1],Src[0],Table[0],Table[1]);
		Dst += 2;  Dst2 += 2; Src += 2;	Table += 2;
	}
}

static void AAC_Zero(real_t* Dst,int n)
{
	memset(Dst,0,sizeof(real_t)*n);
}

static void AAC_Copy(real_t* Dst,const real_t* Src,int n)
{
	memcpy(Dst,Src,sizeof(real_t)*n);
}

static void AAC_Mul(real_t* Dst,const real_t* a,const real_t* b,int n)
{
	do
	{
		Dst[0] = MUL_F(a[0],b[-1]);
		Dst[1] = MUL_F(a[1],b[-2]);
		Dst[2] = MUL_F(a[2],b[-3]);
		Dst[3] = MUL_F(a[3],b[-4]);
		a += 4; b -= 4; Dst += 4; n -= 4;
	} while (n>0);
}

static void AAC_MulR(real_t* Dst,const real_t* a,const real_t* b,int n)
{
	do
	{
		Dst[0] = MUL_F(a[-1],b[-1]);
		Dst[1] = MUL_F(a[-2],b[-2]);
		Dst[2] = MUL_F(a[-3],b[-3]);
		Dst[3] = MUL_F(a[-4],b[-4]);
		a -= 4; b -= 4; Dst += 4; n -= 4;
	} while (n>0);
}

static void AAC_SubR(real_t* Dst,const real_t* a,const real_t* b,int n)
{
	do
	{
		Dst[0] = a[0]-b[-1];
		Dst[1] = a[1]-b[-2];
		Dst[2] = a[2]-b[-3];
		Dst[3] = a[3]-b[-4];
		a += 4;	b -= 4;	Dst += 4; n -= 4;
	} while (n>0);
}

static void AAC_AddMul(real_t* Dst,const real_t* a,const real_t* b,const real_t* c,int n)
{
	do
	{
		Dst[0] = a[0]+MUL_F(b[0],c[0]);
		Dst[1] = a[1]+MUL_F(b[1],c[1]);
		Dst[2] = a[2]+MUL_F(b[2],c[2]);
		Dst[3] = a[3]+MUL_F(b[3],c[3]);
		a += 4; b += 4;	c += 4;	Dst += 4; n -= 4;
	} while (n>0);
}

static void AAC_AddMulMul(real_t* Dst,const real_t* a,const real_t* b1,const real_t* c1,const real_t* b2,const real_t* c2,int n)
{
	do
	{
		Dst[0] = a[0]+MUL_F(b1[0],c1[-1])+MUL_F(b2[0],c2[0]);
		Dst[1] = a[1]+MUL_F(b1[1],c1[-2])+MUL_F(b2[1],c2[1]);
		Dst[2] = a[2]+MUL_F(b1[2],c1[-3])+MUL_F(b2[2],c2[2]);
		Dst[3] = a[3]+MUL_F(b1[3],c1[-4])+MUL_F(b2[3],c2[3]);
		a += 4; b1 += 4; b2 += 4; c1 -= 4; c2 += 4;	Dst += 4; n -= 4;
	} while (n>0);
}

static void AAC_MulMul(real_t* Dst,const real_t* b1,const real_t* c1,const real_t* b2,const real_t* c2,int n)
{
	do
	{
		Dst[0] = MUL_F(b1[0],c1[-1])+MUL_F(b2[0],c2[0]);
		Dst[1] = MUL_F(b1[1],c1[-2])+MUL_F(b2[1],c2[1]);
		Dst[2] = MUL_F(b1[2],c1[-3])+MUL_F(b2[2],c2[2]);
		Dst[3] = MUL_F(b1[3],c1[-4])+MUL_F(b2[3],c2[3]);
		b1 += 4; b2 += 4; c1 -= 4; c2 += 4; Dst += 4; n -= 4;
	} while (n>0);
}

static void AAC_AddNMul(real_t* Dst,const real_t* a,const real_t* b,const real_t* c,int n)
{
	do
	{
		Dst[0] = a[0]+MUL_F(-b[-1],c[0]);
		Dst[1] = a[1]+MUL_F(-b[-2],c[1]);
		Dst[2] = a[2]+MUL_F(-b[-3],c[2]);
		Dst[3] = a[3]+MUL_F(-b[-4],c[3]);
		a += 4; b -= 4;	c += 4; Dst += 4; n -= 4;
	} while (n>0);
}

static void AAC_AddMulNMul(real_t* Dst,const real_t* a,const real_t* b1,const real_t* c1,const real_t* b2,const real_t* c2,int n)
{
	do
	{
		Dst[0] = a[0]+MUL_F(b1[-1],c1[-1])+MUL_F(-b2[-1],c2[0]);
		Dst[1] = a[1]+MUL_F(b1[-2],c1[-2])+MUL_F(-b2[-2],c2[1]);
		Dst[2] = a[2]+MUL_F(b1[-3],c1[-3])+MUL_F(-b2[-3],c2[2]);
		Dst[3] = a[3]+MUL_F(b1[-4],c1[-4])+MUL_F(-b2[-4],c2[3]);
		a += 4;	b1 -= 4; b2 -= 4; c1 -= 4; c2 += 4; Dst += 4; n -= 4;
	} while (n>0);
}

static void AAC_MulNMul(real_t* Dst,const real_t* b1,const real_t* c1,const real_t* b2,const real_t* c2,int n)
{
	do
	{
		Dst[0] = MUL_F(b1[-1],c1[-1])+MUL_F(-b2[-1],c2[0]);
		Dst[1] = MUL_F(b1[-2],c1[-2])+MUL_F(-b2[-2],c2[1]);
		Dst[2] = MUL_F(b1[-3],c1[-3])+MUL_F(-b2[-3],c2[2]);
		Dst[3] = MUL_F(b1[-4],c1[-4])+MUL_F(-b2[-4],c2[3]);
		b1 -= 4; b2 -= 4; c1 -= 4; c2 += 4; Dst += 4; n -= 4;
	} while (n>0);
}

static void AAC_IMDCT_2048(libpaac* p,real_t *Dst, real_t *Src)
{
	AAC_Pre(Dst,Src,mdct_tab_2048[0],512);
	AAC_Pass2_N(Src,Dst,cfft_tab_512[0],1,256);
	AAC_Pass4_N(Dst,Src,cfft_tab_512[0]+256*2,2,64);
	AAC_Pass4_N(Src,Dst,cfft_tab_512[0]+256*2+3*64*2,8,16);
	AAC_Pass4_N(Dst,Src,cfft_tab_512[0]+256*2+3*64*2+3*16*2,32,4);
	AAC_Pass4_1(Src,Dst,128);
	AAC_Post(Dst,Src,mdct_tab_2048[0],512);
}

static void AAC_IMDCT_256(libpaac* p,real_t *Dst, real_t *Src)
{
	AAC_Pre(Dst,Src,mdct_tab_256[0],64);
	AAC_Pass4_N(Src,Dst,cfft_tab_64[0],1,16);
	AAC_Pass4_N(Dst,Src,cfft_tab_64[0]+3*16*2,4,4);
	AAC_Pass4_1(Src,Dst,16);
	AAC_Post(Dst,Src,mdct_tab_256[0],64);
}

void AAC_Filter1024(libpaac* p, int Sequence, int Shape, int PrevShape, real_t *Data, real_t *Overlap, real_t *Tmp)
{
	const real_t *Window;
    switch (Sequence)
    {
    case 0:
		AAC_IMDCT_2048(p,Tmp,Data);

		Window = p->LongWindow[PrevShape];
		AAC_AddMul(Data,Overlap,Tmp,Window,512);	
		AAC_AddNMul(Data+512,Overlap+512,Tmp+512,Window+512,512);		

		Window = p->LongWindow[Shape];
		AAC_Mul(Overlap,Tmp+512,Window+1024,512);				
		AAC_MulR(Overlap+512,Tmp+1024,Window+512,512);	
        break;

    case 1:
		AAC_IMDCT_2048(p,Tmp,Data);

		Window = p->LongWindow[PrevShape];
		AAC_AddMul(Data,Overlap,Tmp,Window,512);								
		AAC_AddNMul(Data+512,Overlap+512,Tmp+512,Window+512,512);		

		Window = p->ShortWindow[Shape];
		AAC_Copy(Overlap,Tmp+512,448);										 
		AAC_Mul(Overlap+512-64,Tmp+1024-64,Window+128,64);			 
		AAC_MulR(Overlap+512,Tmp+1024,Window+64,64);
		AAC_Zero(Overlap+512+64,448);
        break;

    case 2:
		AAC_IMDCT_256(p,Tmp+0*128,Data+0*128);
		AAC_IMDCT_256(p,Tmp+1*128,Data+1*128);
		AAC_IMDCT_256(p,Tmp+2*128,Data+2*128);
		AAC_IMDCT_256(p,Tmp+3*128,Data+3*128);
		AAC_IMDCT_256(p,Tmp+4*128,Data+4*128);
		AAC_IMDCT_256(p,Tmp+5*128,Data+5*128);
		AAC_IMDCT_256(p,Tmp+6*128,Data+6*128);
		AAC_IMDCT_256(p,Tmp+7*128,Data+7*128);

		AAC_Copy(Data,Overlap,448);

		Window = p->ShortWindow[PrevShape];
		AAC_AddMul(Data+448+64*0,Overlap+448+64*0,Tmp+0*64,Window,64);
		AAC_AddNMul(Data+448+64*1,Overlap+448+64*1,Tmp+0*64+64,Window+64,64);

		Window = p->ShortWindow[Shape];
		AAC_AddMulMul(Data+448+64*2,Overlap+448+64*2,Tmp+1*64,Window+128,Tmp+2*64,Window,64);
		AAC_AddMulNMul(Data+448+64*3,Overlap+448+64*3,Tmp+1*64+64,Window+64,Tmp+2*64+64,Window+64,64);
		AAC_AddMulMul(Data+448+64*4,Overlap+448+64*4,Tmp+3*64,Window+128,Tmp+4*64,Window,64);
		AAC_AddMulNMul(Data+448+64*5,Overlap+448+64*5,Tmp+3*64+64,Window+64,Tmp+4*64+64,Window+64,64);
		AAC_AddMulMul(Data+448+64*6,Overlap+448+64*6,Tmp+5*64,Window+128,Tmp+6*64,Window,64);
		AAC_AddMulNMul(Data+448+64*7,Overlap+448+64*7,Tmp+5*64+64,Window+64,Tmp+6*64+64,Window+64,64);
		AAC_AddMulMul(Data+448+64*8,Overlap+448+64*8,Tmp+7*64,Window+128,Tmp+8*64,Window,64);

		AAC_MulNMul(Overlap+64*0,Tmp+7*64+64,Window+64,Tmp+8*64+64,Window+64,64);
		AAC_MulMul(Overlap+64*1,Tmp+9*64,Window+128,Tmp+10*64,Window,64);
		AAC_MulNMul(Overlap+64*2,Tmp+9*64+64,Window+64,Tmp+10*64+64,Window+64,64);
		AAC_MulMul(Overlap+64*3,Tmp+11*64,Window+128,Tmp+12*64,Window,64);
		AAC_MulNMul(Overlap+64*4,Tmp+11*64+64,Window+64,Tmp+12*64+64,Window+64,64);
		AAC_MulMul(Overlap+64*5,Tmp+13*64,Window+128,Tmp+14*64,Window,64);
		AAC_MulNMul(Overlap+64*6,Tmp+13*64+64,Window+64,Tmp+14*64+64,Window+64,64);
		AAC_Mul(Overlap+64*7,Tmp+15*64,Window+128,64);
		AAC_MulR(Overlap+64*8,Tmp+15*64+64,Window+64,64);
		AAC_Zero(Overlap+448+128,448);

        break;

    case 3:
		AAC_IMDCT_2048(p,Tmp,Data);

		Window = p->ShortWindow[PrevShape];
		AAC_Copy(Data,Overlap,448);
		AAC_AddMul(Data+512-64,Overlap+448,Tmp+512-64,Window,64);
		AAC_AddNMul(Data+512,Overlap+512,Tmp+512,Window+64,64);
		AAC_SubR(Data+512+64,Overlap+512+64,Tmp+448,448);

		Window = p->LongWindow[Shape];
		AAC_Mul(Overlap,Tmp+512,Window+1024,512);
		AAC_MulR(Overlap+512,Tmp+1024,Window+512,512);
		break;
    }

//	DebugMessage("FRAME %d",Sequence);
//	{ int i; for (i=0;i<1024;++i) DebugMessage("%d %d",Data[i],Overlap[i]);	}
}

void AAC_Filter_Init(libpaac* p, int FrameLength)
{
	p->LongWindow[0] = sine_long_1024;
	p->ShortWindow[0] = sine_short_128;
	p->LongWindow[1] = kbd_long_1024;
	p->ShortWindow[1] = kbd_short_128;
	p->Filter = (aac_filter) AAC_Filter1024;
}

#endif
