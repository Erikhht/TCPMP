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
 * $Id: dyncode.h 585 2006-01-16 09:48:55Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __DYNCODE_H
#define __DYNCODE_H

typedef uint8_t reg;

#define NONE	((reg)-1)	

typedef struct dyncode
{
	char* Code;
	int Size;
	int Allocated;
} dyncode;

typedef struct dyninst
{
	int Tag;
	struct dyninst* Next;
	struct dyninst* Prev;
	int RdFlags;
	int WrFlags;
	int RdRegs;
	int WrRegs;
	struct dyninst* ReAlloc;
	bool_t Branch;
	int CodeSmall;
	char* CodeLarge;
	int CodeSize;
	char* Address;

} dyninst;

#include "dyncode_arm.h"
#include "dyncode_mips.h"
#include "dyncode_sh3.h"

void DynCode_Init();
void DynCode_Done();

#ifdef CONFIG_DYNCODE
void CodeInit(dyncode*);
void CodeDone(dyncode*);
void CodeBuild(dyncode*);
void CodeStart(dyncode*);
#else
static INLINE void CodeInit(dyncode* p) { memset(p,0,sizeof(dyncode)); }
static INLINE void CodeDone(dyncode* p) {}
static INLINE void CodeBuild(dyncode* p) {}
static INLINE void CodeStart(dyncode* p) {}
#endif

void Align(int);
void MB();
void DS();
dyninst* Label(bool_t DoPost);

//-----------------------------------------------------------------------

static INLINE char* InstCode(dyninst* p)
{
	return p->CodeLarge ? p->CodeLarge : (char*)&p->CodeSmall;
}

static INLINE int InstSize(dyninst* p, int Pos)
{
	if (p->CodeSize >= 0)
		return p->CodeSize;
	Pos = Pos % -p->CodeSize;
	if (Pos)
		Pos = -p->CodeSize - Pos;
	return Pos;
}

static INLINE bool_t InstReadWrite(const dyninst* a, const dyninst* b)
{
	return (a->RdFlags & b->WrFlags) != 0 ||
		   (a->RdRegs & b->WrRegs) != 0;
}

static INLINE bool_t InstBothWrite(const dyninst* a, const dyninst* b)
{
	return (a->WrFlags & b->WrFlags) != 0 ||
		   (a->WrRegs & b->WrRegs) != 0;
}

static INLINE int RotateRight(int v, int Bits)
{
	Bits &= 31;
	return ((v >> Bits) & ((1 << (32-Bits))-1)) | (v << (32-Bits));
}

bool_t InstReAlloc(dyninst*,dyninst*);
void InstPost(dyninst*);
void InstInit();

void InstAdd(dyninst*,context*);
dyninst* InstCreate(const void* Code, int CodeSize, reg Wr, reg Rd1, reg Rd2, int RdFlags, int WrFlags);
dyninst* InstCreate16(int Code16, reg Wr, reg Rd1, reg Rd2, int RdFlags, int WrFlags);
dyninst* InstCreate32(int Code32, reg Wr, reg Rd1, reg Rd2, int RdFlags, int WrFlags);

#endif
