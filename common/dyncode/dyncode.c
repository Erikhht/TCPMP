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
 * $Id: dyncode.c 585 2006-01-16 09:48:55Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"
#include "dyncode.h"

#ifdef CONFIG_DYNCODE

void MB() { Context()->CodeMoveBack = 1; }
void DS() { Context()->CodeDelaySlot = 1; }

void DynCode_Init()
{
	Context()->CodeLock = LockCreate();
}

void DynCode_Done()
{
	LockDelete(Context()->CodeLock);
	Context()->CodeLock = NULL;
}

static void FreeInst()
{
	context* c = Context();
	dyninst* p;
	for (p=(dyninst*)c->CodeInstBegin;p;)
	{
		dyninst* q = p;
		p = p->Next;
		if (q->CodeLarge)
			free(q->CodeLarge);
		free(q);
	}
	c->CodeInstBegin = NULL;
	c->CodeInstEnd = NULL;
	LockLeave(c->CodeLock);
}

static void FreeCode(dyncode* p)
{
	if (p->Code)
		CodeFree(p->Code,p->Allocated);
	p->Allocated = 0;
	p->Code = NULL;
}

void CodeInit(dyncode* p)
{
	p->Code = NULL;
	p->Allocated = p->Size = 0;
}

void CodeDone(dyncode* p)
{
	FreeCode(p);
}

void CodeStart(dyncode* p)
{
	context* c = Context();
	LockEnter(c->CodeLock);

	p->Size = 0;

	c->CodeFailed = 0;
	c->CodeMoveBack = 0;
	c->CodeDelaySlot = 0;

	InstInit();
}

void CodeBuild(dyncode* Code)
{
	dyninst* p;
	context* c = Context();

	Code->Size = 0;
	if (c->CodeFailed)
	{
		FreeInst();
		return;
	}

	for (p=(dyninst*)c->CodeInstBegin;p;p=p->Next)
		Code->Size += InstSize(p,Code->Size);

	if (Code->Size > Code->Allocated)
	{
		FreeCode(Code);
		Code->Allocated = (Code->Size + 511) & ~511;
		Code->Code = (char*) CodeAlloc(Code->Allocated);
	}

	if (Code->Code)
	{
		char* Addr;

		CodeLock(Code->Code,Code->Allocated);

		Addr = Code->Code;
		for (p=(dyninst*)c->CodeInstBegin;p;p=p->Next)
		{
			p->Address = Addr;
			Addr += InstSize(p,Addr - Code->Code);
		}

		for (p=(dyninst*)c->CodeInstBegin;p;p=p->Next)
		{
			if (p->ReAlloc && (!p->ReAlloc->Address || !InstReAlloc(p,p->ReAlloc)))
			{
				Code->Size = 0;
				assert(Code->Size);
				break;
			}
			if (p->CodeLarge)
				memcpy(p->Address,p->CodeLarge,p->CodeSize);
			else
			if (p->CodeSize>0)
			{
				if (p->CodeSize == sizeof(int32_t))
					*(int32_t*)p->Address = p->CodeSmall;
				else
				if (p->CodeSize == sizeof(int16_t))
					*(int16_t*)p->Address = (int16_t)p->CodeSmall;
				else
					memcpy(p->Address,&p->CodeSmall,p->CodeSize);
			}
		}

		if (Code->Code)
			CodeUnlock(Code->Code,Code->Allocated);

#ifdef DUMPCODE
		if (Code->Code)
		{
			char Name[64];
			FILE* f;
			sprintf(Name,"\\code%04x.bin",Code->Size);
			f = fopen(Name,"wb+");
			fwrite(Code->Code,1,Code->Size,f);
			fclose(f);
		}
#endif

	}
	else
		Code->Size = 0;

	FreeInst();
}

dyninst* InstCreate(const void* Code,int CodeSize, reg Wr, reg Rd1, reg Rd2, int RdFlags, int WrFlags)
{
	dyninst* p = (dyninst*) malloc(sizeof(dyninst));
	if (p)
	{
		p->Tag = 0;
		p->Next = NULL;
		p->Prev = NULL;
		p->Address = 0;
		p->Branch = 0;
		p->ReAlloc = NULL;
		p->RdFlags = RdFlags;
		p->WrFlags = WrFlags;
		p->RdRegs = 0;
		p->WrRegs = 0;

		p->CodeSize = CodeSize;
		if (CodeSize <= (int)sizeof(p->CodeSmall))
		{
			p->CodeLarge = NULL;
			if (Code)
				p->CodeSmall = *(int*)Code;
			else
				p->CodeSmall = 0;
		}
		else
		{
			p->CodeLarge = (char*) malloc(CodeSize);
			if (!p->CodeLarge)
			{
				free(p);
				return NULL;
			}
			if (Code)
				memcpy(p->CodeLarge,Code,CodeSize);
			else
				memset(p->CodeLarge,0,CodeSize);
		}
			
		if (Wr != NONE) { if (Wr<32) p->WrRegs |= 1 << Wr; else p->WrFlags |= 256 << (Wr-32); }
		if (Rd1 != NONE) { if (Rd1<32) p->RdRegs |= 1 << Rd1; else p->RdFlags |= 256 << (Rd1-32); }
		if (Rd2 != NONE) { if (Rd2<32) p->RdRegs |= 1 << Rd2; else p->RdFlags |= 256 << (Rd2-32); }
	}
	return p;
}

dyninst* InstCreate32(int Code32, reg Wr, reg Rd1, reg Rd2, int RdFlags, int WrFlags)
{
	return InstCreate(&Code32,4,Wr,Rd1,Rd2,RdFlags,WrFlags);
}

dyninst* InstCreate16(int Code16, reg Wr, reg Rd1, reg Rd2, int RdFlags, int WrFlags)
{
	return InstCreate(&Code16,2,Wr,Rd1,Rd2,RdFlags,WrFlags);
}

void InstAdd(dyninst* New,context* c)
{
	assert(New);

	if (New)
	{
		dyninst* p = c->CodeInstEnd; //insert after p

		if (c->CodeDelaySlot)
		{
			assert(p && !p->Branch && !InstReadWrite(p,New) && !InstReadWrite(New,p) && !InstBothWrite(p,New));
			p->Branch = 1;
			p = p->Prev;
		}

		if (c->CodeMoveBack)
			while (p && !p->Branch && !InstReadWrite(p,New) && !InstReadWrite(New,p) && !InstBothWrite(p,New))
				p = p->Prev;
		
		if (p)
		{
			if (p->Next) 
				p->Next->Prev = New;
			else
				c->CodeInstEnd = New;
			New->Next = p->Next;
			New->Prev = p;
			p->Next = New;
		}
		else
		{
			if (c->CodeInstBegin)
				((dyninst*)c->CodeInstBegin)->Prev = New;
			else
				c->CodeInstEnd = New;
			New->Next = (dyninst*)c->CodeInstBegin;
			New->Prev = NULL;
			c->CodeInstBegin = New;
		}
	}
	else
		c->CodeFailed = 1;

	c->CodeMoveBack = 0;
	c->CodeDelaySlot = 0;
}

dyninst* Label(bool_t DoPost)
{
	dyninst* p = InstCreate(NULL,0,NONE,NONE,NONE,0,0);
	if (p) p->Branch = 1;
	if (DoPost) InstPost(p);
	return p;
}

void Align(int i)
{
	dyninst* p = InstCreate(NULL,-i,NONE,NONE,NONE,0,0);
	if (p) p->Branch = 1;
	InstPost(p);
}

#else
void DynCode_Init() {}
void DynCode_Done() {}
#endif
