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
 * $Id: mem.h 531 2006-01-04 12:56:13Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __MEM_H
#define __MEM_H

#if defined(_WIN32) && (!defined(NO_PLUGINS) || defined(TARGET_PALMOS))
DLL void* malloc_win32(size_t);
DLL void* realloc_win32(void*,size_t);
DLL void free_win32(void*);
#define malloc(n) malloc_win32(n)
#define realloc(p,n) realloc_win32(p,n)
#define free(p) free_win32(p)
#endif

// OS level allocation
typedef struct block
{
	const uint8_t* Ptr;
	uint32_t Id;

} block;

#define HEAP_ANY			0x0003
#define HEAP_ANYWR			0x0005
#define HEAP_DYNAMIC		0x0001
#define HEAP_STORAGE		0x0002
#define HEAP_STORAGEWR		0x0004

DLL bool_t AllocBlock(size_t,block* Block,bool_t Optional,int Heap);
DLL void FreeBlock(block* Block);
DLL void WriteBlock(block* Block,int Ofs,const void* Src,int Length);
DLL bool_t SetHeapBlock(int,block* Block,int Heap);
static INLINE int IsHeapStorage(block* Block) { return Block->Id!=0; }

DLL size_t AvailMemory();
DLL void CheckHeap();

typedef struct phymemblock
{
	uint32_t Addr;
	uint32_t Length;
	void* Private; // needed by PhyMemAlloc/PhyMemFree
	
} phymemblock;

DLL void* PhyMemAlloc(int Length,phymemblock* Blocks,int* BlockCount);
DLL void PhyMemFree(void*,phymemblock* Blocks,int BlockCount);
DLL void* PhyMemBegin(uint32_t Addr,uint32_t Length,bool_t Cached); // single block mapping
DLL void* PhyMemBeginEx(phymemblock* Blocks,int BlockCount,bool_t Cached); // multi block mapping
DLL void PhyMemEnd(void*); 

DLL void* CodeAlloc(int Size);
DLL void CodeFree(void*,int Size);
DLL void CodeLock(void*,int Size);
DLL void CodeUnlock(void*,int Size);
DLL void CodeFindPages(void* Ptr,unsigned char** Min,unsigned char** Max,size_t* PageSize);

DLL void ShowOutOfMemory();
DLL void DisableOutOfMemory();
DLL void EnableOutOfMemory();

void Mem_Init();
void Mem_Done();

//-----------------------------------
// low level memory managing

DLL uint32_t MemVirtToPhy(void* Virt);
DLL void* MemPhyToVirt(uint32_t Phy);

typedef struct memoryinfo
{
	uint32_t *TLB;
	uint32_t MemBase[2];
	int MemSize[2]; //in MB
	uint32_t UnusedArea[2];

} memoryinfo;

DLL bool_t MemGetInfo(memoryinfo* Out);
DLL void* MemAsWritable(uint32_t Phy);

#endif
