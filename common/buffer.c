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
 * $Id: buffer.c 432 2005-12-28 16:39:13Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "common.h"

void BufferClear(buffer* p)
{
	free(p->Data);
	p->Allocated = 0;
	p->ReadPos = 0;
	p->WritePos = 0;
	p->Data = NULL;
}

bool_t BufferAlloc(buffer* p, size_t Size, size_t Align)
{
	uint8_t* Data;
	Align--;
	Size = (Size + SAFETAIL + Align) & ~Align;
	Data = realloc(p->Data,Size);
	if (!Data)
		return 0;

	p->Data = Data;
	p->Allocated = Size - SAFETAIL;
	return 1;
}

void BufferDrop(buffer* p)
{
	//drop previous packets
	p->WritePos = 0;
	p->ReadPos = 0;
}

bool_t BufferStream(buffer* p,stream* Stream)
{
	if (p->WritePos > p->ReadPos+p->Allocated/2)
		return 1;

	if (p->ReadPos > p->Allocated/2)
		BufferPack(p,0);

	if (p->Allocated > p->WritePos)
	{
		int n = Stream->Read(Stream,p->Data+p->WritePos,p->Allocated - p->WritePos);
		if (n > 0)
			p->WritePos += n;
	}
	return p->WritePos > p->ReadPos;
}

bool_t BufferWrite(buffer* p, const void* Ptr, size_t Length, size_t Align)
{
	//append new data to buffer
	int WritePos = p->WritePos + (int)Length; //buffer!
	if (WritePos > p->Allocated && !BufferAlloc(p,WritePos,Align))
		return 0;
	if (Ptr)
		memcpy(p->Data+p->WritePos,Ptr,Length);
	p->WritePos = WritePos;
	return 1;
}

bool_t BufferRead(buffer* p, const uint8_t** Ptr, size_t Length)
{
	if (p->WritePos < p->ReadPos + (int)Length) //buffer!
		return 0;

	*Ptr = p->Data + p->ReadPos;
	p->ReadPos += (int)Length; //buffer!
	return 1;
}

void BufferPack(buffer* p, size_t Length)
{
	int Skip = p->ReadPos + (int)Length; //buffer!
	if (p->WritePos > Skip)
	{
		p->WritePos -= Skip;
		if (Skip)
			memmove(p->Data,p->Data+Skip,p->WritePos); // move end part to the beginning
	}
	else
		p->WritePos = 0;
	p->ReadPos = 0;
}

void ArrayClear(array* p)
{
	if (p->_Block.Ptr)
		FreeBlock(&p->_Block);
	else
		free(p->_Begin);
	p->_Begin = NULL;
	p->_End = NULL;
	p->_Allocated = 0;
}

void ArrayDrop(array* p)
{
	p->_End = p->_Begin;
}

bool_t ArrayAlloc(array* p,size_t Total,size_t Align)
{
	uint8_t* Data;
	--Align;
	Total = (Total + Align) & ~Align;
	Data = realloc(p->_Begin,Total);
	if (!Data)
		return 0;
	p->_End = Data + (p->_End - p->_Begin);
	p->_Begin = Data;
	p->_Allocated = Total;
	return 1;
}

bool_t ArrayAppend(array* p, const void* Ptr, size_t Length, size_t Align)
{
	size_t Total = p->_End - p->_Begin + Length;
	if (Total > p->_Allocated && !ArrayAlloc(p,Total,Align))
		return 0;
	if (Ptr)
		memcpy(p->_End,Ptr,Length);
	p->_End += Length;
	return 1;
}

void ArraySort(array* p, int Count, size_t Width, arraycmp Cmp)
{
	qsort(p->_Begin,Count,Width,Cmp);
}

int ArrayFind(const array* p, int Count, size_t Width, const void* Data, arraycmp Cmp, bool_t* Found)
{
	if (Cmp)
	{
		int i;
		int Mid = 0;
		int Lower = 0;
		int Upper = Count-1;

		while (Upper >= Lower) 
		{
			Mid = (Upper + Lower) >> 1;

			i = Cmp(p->_Begin+Width*Mid,Data);
			if (i>0)
				Upper = Mid-1;	
			else if (i<0)  		
				Lower = Mid+1;	
			else 
			{			        
				*Found = 1;
				return Mid;
			}
		}

		*Found = 0;
		if (Upper == Mid - 1)
			return Mid;		
		else                 
			return Lower;    
	}
	else
	{
		int No = 0;
		const uint8_t* i;
		for (i=p->_Begin;Count--;i+=Width,++No)
			if (memcmp(i,Data,Width)==0)
			{
				*Found = 1;
				return No;
			}

		*Found = 0;
		return No;
	}
}

bool_t ArrayAdd(array* p,int Count, size_t Width, const void* Data, arraycmp Cmp,size_t Align)
{
	int Pos;
	bool_t Found;

	Pos = ArrayFind(p,Count,Width,Data,Cmp,&Found);
	if (!Found)
	{
		if (!ArrayAppend(p,NULL,Width,Align))
			return 0;
		memmove(p->_Begin+Width*(Pos+1),p->_Begin+Width*Pos,(Count-Pos)*Width);
	}
	memcpy(p->_Begin+Width*Pos,Data,Width);
	return 1;
}

bool_t ArrayRemove(array* p, int Count, size_t Width, const void* Data, arraycmp Cmp)
{
	bool_t Found;
	int Pos = ArrayFind(p,Count,Width,Data,Cmp,&Found);
	if (Found)
	{
		memmove(p->_Begin+Pos*Width,p->_Begin+(Pos+1)*Width,(p->_End-p->_Begin)-(Pos+1)*Width);
		p->_End -= Width;
	}
	return Found;
}

void ArrayLock(array* p)
{
#ifdef TARGET_PALMOS
	if (!p->_Block.Ptr && p->_End != p->_Begin)
	{
		int n = p->_End-p->_Begin;
		if (AllocBlock(n,&p->_Block,1,HEAP_STORAGE))
		{
			WriteBlock(&p->_Block,0,p->_Begin,n);
			free(p->_Begin);
			p->_Begin = (uint8_t*)p->_Block.Ptr;
			p->_End = p->_Begin + n;
			p->_Allocated = 0;
		}
	}
#endif
}

bool_t ArrayUnlock(array* p,size_t Align)
{
#ifdef TARGET_PALMOS
	if (p->_Block.Ptr)
	{
		uint8_t* Mem;
		size_t n = p->_End-p->_Begin;
		size_t Total;

		--Align;
		Total = (n + Align) & ~Align;
		Mem = (uint8_t*) malloc(Total);
		if (!Mem)
			return 0;

		memcpy(Mem,p->_Begin,n);
		FreeBlock(&p->_Block);

		p->_Begin = Mem;
		p->_End = p->_Begin + n;
		p->_Allocated = Total;
	}
#endif
	return 1;
}
