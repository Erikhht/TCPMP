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
 * $Id: helper_base.c 548 2006-01-08 22:41:57Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "common.h"
#include "../splitter/asf.h"

int GCD(int a,int b)
{
	int c;
	while (b)
	{
		c = b;
		b = a % b;
		a = c;
	}
	return a;
}

void Simplify(fraction* f, int MaxNum, int MaxDen)
{
	int Den = abs(f->Den);
	int Num = abs(f->Num);

	if ((int64_t)Num*MaxDen < (int64_t)Den*MaxNum)
	{
		if (Den > MaxDen)
		{
			f->Num = Scale(f->Num,MaxDen,Den);
			f->Den = Scale(f->Den,MaxDen,Den);
		}
	}
	else
	{
		if (Num > MaxNum)
		{
			f->Num = Scale(f->Num,MaxNum,Num);
			f->Den = Scale(f->Den,MaxNum,Num);
		}
	}
}

void SwapPByte(uint8_t** a, uint8_t** b)
{
	uint8_t* t = *a;
	*a = *b;
	*b = t;
}

void SwapPChar(tchar_t** a, tchar_t** b)
{
	tchar_t* t = *a;
	*a = *b;
	*b = t;
}

void SwapInt(int* a, int* b)
{
	int t = *a;
	*a = *b;
	*b = t;
}

void SwapLong(long* a, long* b)
{
	long t = *a;
	*a = *b;
	*b = t;
}

void SwapBool(bool_t* a, bool_t* b)
{
	bool_t t = *a;
	*a = *b;
	*b = t;
}

void SwapPoint(point* p)
{
	int t = p->x;
	p->x = p->y;
	p->y = t;
}

void SwapRect(rect* r)
{
	int t1 = r->x;
	int t2 = r->Width;
	r->x = r->y;
	r->Width = r->Height;
	r->y = t1;
	r->Height = t2;
}

void* Alloc16(size_t n)
{
	char* p = (char*) malloc(n+sizeof(void*)+16);
	if (p)
	{
		uintptr_t i;
		char* p0 = p;
		p += sizeof(void*);
		i = (uintptr_t)p & 15;
		if (i) p += 16-i;
		((void**)p)[-1] = p0;
	}
	return p;
}

void Free16(void* p)
{
	if (p)
		free(((void**)p)[-1]);
}
