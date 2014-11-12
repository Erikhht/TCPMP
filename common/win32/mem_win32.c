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
 * $Id: mem_win32.c 430 2005-12-28 14:50:08Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"

#if defined(TARGET_WIN32) || defined(TARGET_WINCE)

//#define FIND		0xCF9390
//#define SAFEGUARD	32

#define BLOCKGROUP	8

#ifndef STRICT
#define STRICT
#endif
#include <windows.h>

#if defined(_MSC_VER) && !defined(TARGET_WINCE)
#include <crtdbg.h>
#endif

#if defined(TARGET_WINCE)

#define MAXPHY		16

#define CACHE_SYNC_INSTRUCTIONS		2
#define LOCKFLAG_WRITE				1
#define TBL_CACHE	0x08
#define TBL_BUFFER	0x04

typedef struct phymem
{
	char* Virt;
	uint32_t Phy; 
	uint32_t Length;
	uint32_t RefCount;
	phymemblock* Blocks;
	int BlockCount;

} phymem;

static phymem PhyMem[MAXPHY] = { 0 };

static BOOL (WINAPI* FuncVirtualSetAttributes)(LPVOID lpvAddress, DWORD cbSize, DWORD dwNewFlags, DWORD dwMask, LPDWORD lpdwOldFlags) = NULL;
static BOOL (WINAPI* FuncFreePhysMem)(LPVOID) = NULL;
static BOOL (WINAPI* FuncLockPages)(LPVOID lpvAddress,DWORD cbSize,PDWORD pPFNs,int fOptions) = NULL;
static BOOL (WINAPI* FuncUnlockPages)(LPVOID lpvAddress,DWORD cbSize) = NULL;
static LPVOID (WINAPI* FuncAllocPhysMem)(DWORD cbSize, DWORD fdwProtect, DWORD dwAlignmentMask, DWORD dwFlags, PULONG pPhysicalAddress) = NULL;
static void (WINAPI* FuncCacheSync)(DWORD) = NULL;
static BOOL (WINAPI* FuncVirtualCopy)(LPVOID, LPVOID, DWORD, DWORD) = NULL;

static HMODULE CoreDLL = NULL;

#endif

typedef struct blockgroup
{
	block Block;
	int Mask;
	int Unused; //align to 16bytes

} blockgroup;

static int PageSize = 4096;
static array BlockGroup = { NULL };

#ifndef NDEBUG
static int BlockAllocated = 0;
#endif

void* PhyMemAlloc(int Length,phymemblock* Blocks,int* BlockCount)
{
	void* p = NULL;

#if defined(TARGET_WINCE)
	if (*BlockCount>=1)
	{
		int n,Pos;
		int Count = 0;

		Length = (Length + PageSize-1) & ~(PageSize-1);

		if (FuncAllocPhysMem && FuncFreePhysMem && FuncVirtualCopy)
		{
			// try allocate in one continous block
			Blocks[0].Private = FuncAllocPhysMem(Length,PAGE_READWRITE,PageSize-1,0,&Blocks[0].Addr);
			if (Blocks[0].Private)
			{
				Blocks[0].Length = Length;
				Count = 1;
			}

			/* not worth it. allocphysmem is buggy anyway. won't be able to allocate per one pagesize

			if (!Count && *BlockCount>1)
			{
				int Left = Length;
				int BlockMax;
				int BlockSize = (Left+PageSize-1) & ~(PageSize-1);

				// allocate in separate blocks
			
				while (Count < *BlockCount && BlockSize > 0)
				{
					Blocks[Count].Private = FuncAllocPhysMem(BlockSize,PAGE_READWRITE,PageSize-1,0,&Blocks[Count].Addr);
					if (Blocks[Count].Private)
					{
						Blocks[Count].Length = BlockSize;
						++Count;
						Left -= BlockSize;
						BlockMax = (Left+PageSize-1) & ~(PageSize-1);
						if (BlockSize > BlockMax)
							BlockSize = BlockMax;
					}
					else
					if (BlockSize > PageSize*8)
						BlockSize = (BlockSize/2+PageSize-1) & ~(PageSize-1);
					else
						BlockSize -= PageSize;
				}

				if (Left>0)
				{
					for (n=0;n<Count;++n)
						FuncFreePhysMem(Blocks[n].Private);
					Count = 0;
				}
			}
			*/

			if (Count)
			{
				p = VirtualAlloc(NULL,Length,MEM_RESERVE,PAGE_READWRITE);

				if (p)
				{
					Pos = 0;
					for (n=0;n<Count;++n)
					{
						if (!FuncVirtualCopy((char*)p+Pos,(LPVOID)(Blocks[n].Addr >> 8),Blocks[n].Length, PAGE_READWRITE | PAGE_PHYSICAL))
							break;
						Pos += Blocks[n].Length;
					}
				
					if (n!=Count)
					{
						VirtualFree(p,0,MEM_RELEASE);
						p = NULL;
					}
				}

				if (!p)
				{
					for (n=0;n<Count;++n)
						FuncFreePhysMem(Blocks[n].Private);
					Count = 0;
				}
			}
		}
		
		if (!p && FuncLockPages && FuncUnlockPages)
		{
			do
			{
				p = VirtualAlloc(NULL,Length,MEM_COMMIT,PAGE_READWRITE);
			}
			while (!p && Length && NodeHibernate());

			if (p)
			{
				int Pages = Length / PageSize;
				DWORD* PFN = (DWORD*) alloca(sizeof(DWORD)*Pages);

				if (FuncLockPages(p,Length,PFN,LOCKFLAG_WRITE))
				{
					DWORD Last = (DWORD)-1;
					for (n=0;n<Pages;++n)
					{
						if (PFN[n] != Last)
						{
							if (Count >= *BlockCount)
							{
								Count = 0;
								break;
							}

							Blocks[Count].Addr = PFN[n];
							Blocks[Count].Length = PageSize;
							Blocks[Count].Private = NULL;
							++Count;
						}
						else
							Blocks[Count-1].Length += PageSize;

						Last = PFN[n] + PageSize;
					}

					if (!Count)
						FuncUnlockPages(p,Length);
				}
				
				if (!Count)
				{
					VirtualFree(p,0,MEM_RELEASE);
					p = NULL;
				}
			}
		}

#ifdef ARM
		if (p && FuncVirtualSetAttributes && (QueryPlatform(PLATFORM_CAPS) & CAPS_ARM_XSCALE))
			FuncVirtualSetAttributes(p,Length,TBL_CACHE|TBL_BUFFER,TBL_CACHE|TBL_BUFFER,NULL);
#endif

		*BlockCount = Count;
	}
#endif

	return p;
}

void PhyMemFree(void* p,phymemblock* Blocks,int BlockCount)
{
#if defined(TARGET_WINCE)
	if (p)
	{
		int n;

		if (FuncLockPages && FuncUnlockPages)
		{
			int Size = 0;
			for (n=0;n<BlockCount;++n)
			{
				if (Blocks[n].Private)
					break;
				Size += Blocks[n].Length;
			}
			if (Size)
				FuncUnlockPages(p,Size);
		}

		VirtualFree(p,0,MEM_RELEASE);

		if (FuncAllocPhysMem && FuncFreePhysMem && FuncVirtualCopy)
			for (n=0;n<BlockCount;++n)
				if (Blocks[n].Private)
					FuncFreePhysMem(Blocks[n].Private);
	}
#endif
}

#if defined(TARGET_WINCE)
static void SafeVirtualFree(PVOID p,DWORD Size)
{
#ifdef ARM
	// O2 Atom destroys video memory and LCD descriptors unless this code... (WM5)
	if (FuncVirtualSetAttributes)
		FuncVirtualSetAttributes(p,Size,0,0xFFFFF000,NULL);
#endif
	VirtualFree(p,0,MEM_RELEASE);
}
#endif

void* PhyMemBeginEx(phymemblock* Blocks,int BlockCount,bool_t Cached)
{
#if defined(TARGET_WINCE)
	if (FuncVirtualCopy && Blocks && BlockCount)
	{
		int n;
		int Mode;
		phymem* p;
		for (p=PhyMem;p!=PhyMem+MAXPHY;++p)
			if (p->BlockCount == BlockCount && p->Length == Blocks[0].Length && p->Phy == Blocks[0].Addr)
			{
				for (n=1;n<BlockCount;++n)
					if (p->Blocks[n].Addr != Blocks[n].Addr || p->Blocks[n].Length != Blocks[n].Length)
						break;

				if (n==BlockCount)
				{
					++p->RefCount;
					return p->Virt;
				}
			}

		if (BlockCount==1)
			return PhyMemBegin(Blocks[0].Addr,Blocks[0].Length,Cached);

		for (Mode=0;Mode<2;++Mode)
			for (p=PhyMem;p!=PhyMem+MAXPHY;++p)
			{
				if (Mode && !p->RefCount && p->Length)
				{
					free(p->Blocks);
					SafeVirtualFree(p->Virt,p->Length);
					memset(p,0,sizeof(phymem));
				}

				if (!p->Length)
				{
					p->Blocks = (phymemblock*) malloc(sizeof(phymemblock)*BlockCount);
					if (p->Blocks)
					{
						int Size = 0;
						for (n=0;n<BlockCount;++n)
							Size += Blocks[n].Length;

						p->BlockCount = BlockCount;
						p->Virt = (char*) VirtualAlloc(0, Size, MEM_RESERVE, PAGE_NOACCESS);
						if (p->Virt)
						{
							int Mode = PAGE_READWRITE | PAGE_PHYSICAL;
							char* Virt = p->Virt;

							if (!Cached)
								Mode |= PAGE_NOCACHE;

							for (n=0;n<BlockCount;++n)
							{
								if (!FuncVirtualCopy(Virt,(LPVOID)(Blocks[n].Addr >> 8), Blocks[n].Length,Mode))
									break;

								p->Blocks[n].Addr = Blocks[n].Addr;
								p->Blocks[n].Length = Blocks[n].Length;
								Virt += Blocks[n].Length;
							}

							if (n==BlockCount)
							{
								p->RefCount = 1;
								p->Phy = Blocks[0].Addr;
								p->Length = Blocks[0].Length;
								p->BlockCount = BlockCount;

#ifdef ARM
								if (Cached && FuncVirtualSetAttributes && (QueryPlatform(PLATFORM_CAPS) & CAPS_ARM_XSCALE))
									FuncVirtualSetAttributes(p->Virt,Size,TBL_CACHE|TBL_BUFFER,TBL_CACHE|TBL_BUFFER,NULL);
#endif
								return p->Virt;
							}

							VirtualFree(p->Virt,0,MEM_RELEASE);
						}
						free(p->Blocks);
					}

					memset(p,0,sizeof(phymem));
					return NULL;
				}
			}
	}	
#endif
	return NULL;
}

void* PhyMemBegin(uint32_t Phy,uint32_t Length,bool_t Cached)
{
#if defined(TARGET_WINCE)
	if (FuncVirtualCopy && Phy && Length)
	{
		int Mode;
		phymem* p;
		for (p=PhyMem;p!=PhyMem+MAXPHY;++p)
			if (p->Phy <= Phy && p->Phy+p->Length >= Phy+Length)
			{
				++p->RefCount;
				return p->Virt + (Phy - p->Phy);
			}

		for (Mode=0;Mode<2;++Mode)
			for (p=PhyMem;p!=PhyMem+MAXPHY;++p)
			{
				if (Mode && !p->RefCount && p->Length)
				{
					free(p->Blocks);
					SafeVirtualFree(p->Virt,p->Length);
					memset(p,0,sizeof(phymem));
				}

				if (!p->Length)
				{
					int Align = Phy & 4095;
					Phy -= Align;
					Length = (Length + Align + 4095) & ~4095;
					p->Virt = (char*) VirtualAlloc(0, Length, MEM_RESERVE, PAGE_NOACCESS);
					if (p->Virt)
					{
						int Mode = PAGE_READWRITE | PAGE_PHYSICAL;
						if (!Cached)
							Mode |= PAGE_NOCACHE;

						if (FuncVirtualCopy(p->Virt,(LPVOID)(Phy >> 8), Length, Mode))
						{
							p->RefCount = 1;
							p->Phy = Phy;
							p->Length = Length;
							p->BlockCount = 1;

#ifdef ARM
							if (Cached && FuncVirtualSetAttributes && (QueryPlatform(PLATFORM_CAPS) & CAPS_ARM_XSCALE))
								FuncVirtualSetAttributes(p->Virt,Length,TBL_CACHE|TBL_BUFFER,TBL_CACHE|TBL_BUFFER,NULL);
#endif
							return p->Virt + Align;
						}
						VirtualFree(p->Virt,0,MEM_RELEASE);
					}
					return NULL;
				}
			}
	}	
#endif
	return NULL;
}

void PhyMemEnd(void* Virt)
{
#if defined(TARGET_WINCE)
	if (Virt)
	{
		phymem* p;
		for (p=PhyMem;p!=PhyMem+MAXPHY;++p)
			if (p->RefCount && p->Virt <= (char*)Virt && p->Virt+p->Length > (char*)Virt)
			{
				--p->RefCount;
				break;
			}
	}
#endif
}

bool_t MemGetInfo(memoryinfo* p)
{
	return 0;
}

void* CodeAlloc(int Size)
{
	void* p;
	do 
	{
		p = VirtualAlloc(NULL,Size,MEM_COMMIT,PAGE_EXECUTE_READ);
	}
	while (!p && Size && NodeHibernate());
	return p;
}

void CodeFree(void* Code,int Size)
{
	VirtualFree(Code,Size,MEM_DECOMMIT);
	VirtualFree(Code,0,MEM_RELEASE);
}

void CodeLock(void* Code,int Size)
{
	DWORD Protect;
	VirtualProtect(Code,Size,PAGE_READWRITE|PAGE_NOCACHE,&Protect);
}

void CodeUnlock(void* Code,int Size)
{
	DWORD Protect;
	VirtualProtect(Code,Size,PAGE_EXECUTE_READ,&Protect);
#if defined(TARGET_WINCE)
	if (FuncCacheSync)
		FuncCacheSync(CACHE_SYNC_INSTRUCTIONS);
#endif
}

void CodeFindPages(void* Ptr,uint8_t** PMin,uint8_t** PMax,uintptr_t* PPageSize)
{
	uint8_t* Min;
	uint8_t* Max;
	uintptr_t PageSize = 
#if defined(MIPS)
	1024;
#else
	4096;
#endif
	if (PPageSize)
		*PPageSize = PageSize;

	Min = Max = (uint8_t*)((uintptr_t)Ptr & ~(PageSize-1));
	while (!IsBadCodePtr((FARPROC)(Min-PageSize)))
		Min -= PageSize;
	while (!IsBadCodePtr((FARPROC)Max))
		Max += PageSize;

	*PMin = Min;
	*PMax = Max;
}

void CheckHeap()
{
#if defined(_DEBUG) && defined(_MSC_VER) && (_MSC_VER < 1400)
	assert(_CrtCheckMemory());
#endif
}

//#define FIND 0xAD1A30

size_t AvailMemory()
{
	MEMORYSTATUS Status;
	Status.dwLength = sizeof(Status);

	GlobalMemoryStatus(&Status); 

	return Status.dwAvailPhys;
}

void WriteBlock(block* Block,int Ofs,const void* Src,int Length)
{
	memcpy((uint8_t*)Block->Ptr+Ofs,Src,Length);
}

void FreeBlock(block* p)
{
	if (p)
	{
#ifdef BLOCKGROUP
		if (p->Id) //group
		{
			blockgroup* g = ARRAYBEGIN(BlockGroup,blockgroup)+(p->Id>>8);
			int Bit = 1 << ((p->Id & 255)-1);
			if (g<ARRAYEND(BlockGroup,blockgroup) && g->Mask & Bit)
			{
				g->Mask &= ~Bit;
				if (!g->Mask)
					FreeBlock(&g->Block);
			}
			p->Ptr = NULL;
			p->Id = 0;
			return;
		}
#endif

#ifndef NDEBUG
		if (VirtualFree((void*)p->Ptr,0,MEM_RELEASE))
			--BlockAllocated;
#else
		VirtualFree((void*)p->Ptr,0,MEM_RELEASE);
#endif

		p->Ptr = NULL;
		p->Id = 0;
	}
}

bool_t SetHeapBlock(int n,block* Block,int Heap)
{
	return 0;
}

#ifdef BLOCKGROUP
static bool_t AllocBlockGroup(block* Block, bool_t Optional)
{
	int n;
	blockgroup* g;
	for (g=ARRAYBEGIN(BlockGroup,blockgroup);g!=ARRAYEND(BlockGroup,blockgroup);++g)
		if (g->Mask && g->Mask != (1<<BLOCKGROUP)-1)
			break;

	if (g==ARRAYEND(BlockGroup,blockgroup))
	{
		if (!Optional)
			return 0;

		for (g=ARRAYBEGIN(BlockGroup,blockgroup);g!=ARRAYEND(BlockGroup,blockgroup);++g)
			if (!g->Mask)
				break;

		if (g==ARRAYEND(BlockGroup,blockgroup))
		{
			if (!ArrayAppend(&BlockGroup,NULL,sizeof(blockgroup),64))
				return 0;
			g=ARRAYEND(BlockGroup,blockgroup)-1;
			g->Mask = 0;
		}

		if (!AllocBlock(BLOCKSIZE*BLOCKGROUP,&g->Block,1,HEAP_ANY))
			return 0;
	}

	for (n=0;n<BLOCKGROUP;++n)
		if (!(g->Mask & (1<<n)))
		{
			g->Mask |= (1<<n);
			Block->Id = ((g-ARRAYBEGIN(BlockGroup,blockgroup))<<8)+(n+1);
			Block->Ptr = g->Block.Ptr + n*BLOCKSIZE;
			return 1;
		}

	return 0;
}

#endif

bool_t AllocBlock(size_t n,block* Block,bool_t Optional,int Heap)
{
	void* p;

	if (Optional && AvailMemory() < (256+64)*1024+n) // we want to avoid OS low memory warning
		return 0;
#ifdef BLOCKGROUP
	if (n == BLOCKSIZE && AllocBlockGroup(Block,Optional))
		return 1;
#endif
	do
	{
		p = VirtualAlloc(NULL,n,MEM_COMMIT,PAGE_READWRITE);
	}
	while (!p && !Optional && n && NodeHibernate());

	Block->Ptr = p;
	Block->Id = 0;

#ifndef NDEBUG
	if (p) ++BlockAllocated;
#endif

	return p!=NULL;
}

#undef malloc
#undef realloc
#undef free

			
#ifdef SAFEGUARD
void* safeset(uint8_t* p,int n,bool_t clear)
{
	if (p)
	{
		*(int*)p = n;
		p += sizeof(int);
		memset(p,0xDA,SAFEGUARD);
		p += SAFEGUARD;
		if (clear)
			memset(p,0xCC,n);
		p += n;
		memset(p,0xDE,SAFEGUARD);
		p -= n;
	}
	return p;
}
void* safeget(uint8_t* p,bool_t clear)
{
	if (p)
	{
		int n = *(int*)(p-SAFEGUARD-sizeof(int));
		int i;
		for (i=0;i<SAFEGUARD;++i)
			assert(p[-i-1] == 0xDA);
		for (i=0;i<SAFEGUARD;++i)
			assert(p[n+i] == 0xDE);
		p -= SAFEGUARD+sizeof(int);
		if (clear)
			memset(p,0xEE,n+SAFEGUARD*2+sizeof(int));
	}
	return p;
}
#endif

void* malloc_win32(size_t n)
{
	void* p = NULL;
	if (n)
	{
		do
		{
#ifdef SAFEGUARD
			p = safeset(malloc(n+SAFEGUARD*2+sizeof(int)),n,1);
#else
			p = malloc(n);
#endif

#ifdef FIND
			if ((int)p == FIND)
				DebugBreak();
#endif
		} while (!p && NodeHibernate());
	}
	return p;
}

void* realloc_win32(void* p,size_t n)
{
	do
	{
#ifdef SAFEGUARD
		p = safeset(realloc(safeget(p,0),n+SAFEGUARD*2+sizeof(int)),n,p==NULL);
#else
		p = realloc(p,n);
#endif

#ifdef FIND
		if ((int)p == FIND)
			DebugBreak();
#endif
	} while (!p && n && NodeHibernate());
	return p;
}

void free_win32(void* p)
{
#ifdef SAFEGUARD
	free(safeget(p,1));
#else
	free(p);
#endif
}

void ShowOutOfMemory()
{
}

void EnableOutOfMemory()
{
}

void DisableOutOfMemory()
{
}

void Mem_Init()
{
	SYSTEM_INFO SysInfo;

#if defined(_DEBUG) && defined(_MSC_VER) && !defined(TARGET_WINCE)
	int Flag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	Flag |= _CRTDBG_LEAK_CHECK_DF;
	Flag |= _CRTDBG_CHECK_ALWAYS_DF;
	_CrtSetDbgFlag(Flag);
#endif

#if defined(TARGET_WINCE)
	CoreDLL = LoadLibrary(T("coredll.dll"));
	if (CoreDLL)
	{
		*(FARPROC*)&FuncCacheSync = GetProcAddress(CoreDLL,T("CacheSync"));
		*(FARPROC*)&FuncVirtualCopy = GetProcAddress(CoreDLL,T("VirtualCopy"));
		*(FARPROC*)&FuncVirtualSetAttributes = GetProcAddress(CoreDLL,T("VirtualSetAttributes"));
		*(FARPROC*)&FuncFreePhysMem = GetProcAddress(CoreDLL,T("FreePhysMem"));
		*(FARPROC*)&FuncAllocPhysMem = GetProcAddress(CoreDLL,T("AllocPhysMem"));
		*(FARPROC*)&FuncLockPages = GetProcAddress(CoreDLL,T("LockPages"));
		*(FARPROC*)&FuncUnlockPages = GetProcAddress(CoreDLL,T("UnlockPages"));
	}
#endif

	GetSystemInfo(&SysInfo);
	PageSize = SysInfo.dwPageSize;
}

void Mem_Done()
{
#if defined(TARGET_WINCE)
	phymem* p;
	for (p=PhyMem;p!=PhyMem+MAXPHY;++p)
		if (p->Length)
		{
			free(p->Blocks);
			SafeVirtualFree(p->Virt,p->Length);
			memset(p,0,sizeof(phymem));
		}

	if (CoreDLL) FreeLibrary(CoreDLL);
#endif
#ifndef NDEBUG
	assert(BlockAllocated==0);
#endif
	ArrayClear(&BlockGroup);
}

#endif
