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
 * $Id: mem_palmos.c 543 2006-01-07 22:06:24Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"

//#define TRACKMEMORY

#if defined(TARGET_PALMOS)

#define MIN_DYNAMIC_LIMIT	32768
#define TRY_DYNAMIC			3*1024*1024
#define FTRID_MIN			4096
#define FTRID_MAX			16384
#define MAX_CACHE			32768

#include "pace.h"

extern SysAppInfoPtr SysGetAppInfo(SysAppInfoPtr *uiAppPP, SysAppInfoPtr* actionCodeAppPP);

#ifdef HAVE_PALMONE_SDK
#include <68K/System/PmPalmOSNVFS.h>
#endif

static block FlushCache = {NULL};
static int OutOfMemory = 0;
static UInt16 StorageId = 1;

void free(void *p)
{
	if (p)
	{
#ifdef TRACKMEMORY
		int q = AvailMemory();
#endif
		MemPtrFree(p);
#ifdef TRACKMEMORY
		DebugMessage("%08x free:%d %d",p,AvailMemory(),q-AvailMemory());
#endif
	}
}

void EnableOutOfMemory()
{
	OutOfMemory--;
}

void DisableOutOfMemory()
{
	OutOfMemory++;
}

void ShowOutOfMemory()
{
	if (OutOfMemory<=0)
	{
		RectangleType r;
		Coord Width;
		Coord Height;
		const tchar_t* s = LangStr(ERR_ID,ERR_OUT_OF_MEMORY);
		WinHandle Old = WinSetDrawWindow(WinGetDisplayWindow());
		WinGetDisplayExtent(&Width, &Height);

		if (Width<60)
			Width=60;
		r.topLeft.x = (Coord)(Width-80);
		r.topLeft.y = 16;
		r.extent.x = 79;
		r.extent.y = 15; 

		WinEraseRectangle(&r, 0);
		WinDrawChars(s, (Int16)strlen(s), (Coord)(r.topLeft.x+1), (Coord)(r.topLeft.y+1));

		if (Old)
			WinSetDrawWindow(Old);
	}
}

void* malloc(size_t n)
{
#ifdef TRACKMEMORY
	int q = AvailMemory();
#endif
	void *p = NULL;
	if (n)
	{
		do
		{
			if ((size_t)AvailMemory()>MIN_DYNAMIC_LIMIT+n)
				p = MemGluePtrNew(n);
		} while (!p && NodeHibernate());

		if (!p)
		{
			//DebugMessage("malloc %d %d",n,AvailMemory());
			ShowOutOfMemory();
		}
	}

#ifdef TRACKMEMORY
	DebugMessage("%08x malloc:%d %d %d",p,n,q,q-AvailMemory());
	if (n>1024)
		ThreadSleep(300);
#endif
	return p;
}

void* realloc(void* p,size_t n)
{
#ifdef TRACKMEMORY
	int q = AvailMemory();
#endif
	if (!p)
		return malloc(n);
	else
	if (!n)
	{
		MemPtrFree(p);
#ifdef TRACKMEMORY
		DebugMessage("%08x free:%d %d",p,AvailMemory(),q-AvailMemory());
#endif
		return NULL;
	}
	else
	if (MemPtrResize(p,n)!=errNone)
	{
		void *q = NULL;
		do
		{
			if ((size_t)AvailMemory()>MIN_DYNAMIC_LIMIT+n)
				q = MemGluePtrNew(n);
		} while (!q && NodeHibernate());
		if (q) 
		{
			memcpy(q,p,MemPtrSize(p));
			MemPtrFree(p);
		}
		else
			ShowOutOfMemory();
		p = q;
    }
#ifdef TRACKMEMORY
	DebugMessage("%08x realloc:%d %d %d",p,n,q,q-AvailMemory());
	if (n>1024)
		ThreadSleep(300);
#endif
	return p;
}

void* CodeAlloc(int Size) {	return malloc(Size); }
void CodeFree(void* Code,int Size) { free(Code); }
void CodeLock(void* Code,int Size) {}

void CodeUnlock(void* Code,int Size) 
{
	// instruction cache flush needed
#ifdef ARM
	if (FlushCache.Ptr)
		((void(*)())FlushCache.Ptr)();
#endif
}

void WriteBlock(block* Block,int Ofs,const void* Src,int Length)
{
	if (IsHeapStorage(Block))
		DmWrite((void*)Block->Ptr,Ofs,Src,Length);
	else
		memcpy((char*)Block->Ptr+Ofs,Src,Length);
}

void FreeBlock(block* Block)
{
	if (Block)
	{
		context* p = Context();
		if (IsHeapStorage(Block))
		{
			int FtrId = Block->Id;
			if (p->FtrId > FtrId)
				p->FtrId = FtrId;
			FtrPtrFree(p->ProgramId,(UInt16)FtrId);
		}
		else
			free((char*)Block->Ptr);
		Block->Id = 0;
		Block->Ptr = NULL;
	}
}

bool_t SetHeapBlock(int n,block* Block,int Heap)
{
	block New;
	if (!Block->Id && (Heap & HEAP_DYNAMIC))
		return 0;
	if (IsHeapStorage(Block) && (Heap & HEAP_STORAGE))
		return 0;
	if (!AllocBlock(n,&New,1,Heap))
		return 0;
	WriteBlock(&New,0,Block->Ptr,n);
	FreeBlock(Block);
	*Block = New;
	return 1;
}

#ifdef ARM
static NOINLINE void FlushData(volatile const void* ptr)
{
	const int zero = 0;
	asm volatile ("mcr p15, 0, %0, c7, c14, 1" : : "r" (ptr));  // clean+invalidate D entry
	asm volatile ("mcr p15, 0, %0, c7, c10, 4" : : "r" (zero)); // drain write buffer
}

static NOINLINE void FlushTLB()
{
	const int zero = 0;
	asm volatile ("mcr p15, 0, %0, c8, c7, 0" : : "r" (zero)); // invalidate I and D TLB
}
#endif

static NOINLINE int GetFtrId(context* p)
{
	UInt32 v;
	int FtrId = max(p->FtrId,FTRID_MIN);
	for (;FtrId<FTRID_MAX;++FtrId)
		if (FtrGet(p->ProgramId,(UInt16)FtrId,&v)==ftrErrNoSuchFeature)
			return FtrId;
	return -1;
}

bool_t AllocBlock(size_t n,block* Block,bool_t Optional,int Heap)
{
	context* p = Context();
	uint32_t Storage,Max;
	void* Ptr = NULL;

	if (!n)
	{
		Block->Id = 0;
		Block->Ptr = NULL;
		return 1;
	}

	if (Heap == HEAP_ANY && !p->TryDynamic)
		Heap = HEAP_STORAGE;

	if ((Heap & HEAP_STORAGEWR) && (n<16384 || !QueryAdvanced(ADVANCED_MEMORYOVERRIDE)))
		Heap &= ~HEAP_STORAGEWR;

	if (Optional)
	{
		if ((Heap & (HEAP_STORAGE|HEAP_STORAGEWR)) && MemHeapFreeBytes(StorageId,&Storage,&Max)==errNone && (int)Storage<128*1024+n)
			Heap &= ~(HEAP_STORAGE|HEAP_STORAGEWR);

		if ((Heap & HEAP_DYNAMIC) && AvailMemory()<((Heap & HEAP_STORAGE)?2048:512)*1024+n)
			Heap &= ~HEAP_DYNAMIC;
	}

	if (Heap & HEAP_STORAGEWR)
	{
		int FtrId = GetFtrId(p);
		if (FtrId>=0)
		{
			do
			{
				if (FtrPtrNew(p->ProgramId,(UInt16)FtrId,n,&Ptr)==errNone)
					break;
			} while (NodeHibernate());

			if (Ptr)
			{
				uint32_t Phy = MemVirtToPhy(Ptr);
				void* PtrWr = MemAsWritable(Phy);

				if (!PtrWr || memcmp(Ptr,PtrWr,n)!=0)
				{
					//ShowMessage("","Memory tweak failed for address virt:%08x phy:%08x tmp:%08x\nPlease report to developer!",Ptr,Phy,PtrWr);
					PtrWr = NULL;
				}

				if (PtrWr)
				{
					p->FtrId = FtrId+1;
					Block->Id = FtrId;
					Block->Ptr = PtrWr;
					return 1;
				}

				FtrPtrFree(p->ProgramId,(UInt16)FtrId);
				Ptr = NULL;
			}
		}
	}

	if (Heap & HEAP_DYNAMIC)
	{
		Ptr = malloc(n);
		if (Ptr)
		{
			Block->Ptr = Ptr;
			Block->Id = 0;
			return 1;
		}
	}

	if (Heap & HEAP_STORAGE)
	{
		int FtrId = GetFtrId(p);
		if (FtrId>=0)
		{
			do
			{
				if (MemHeapFreeBytes(StorageId,&Storage,&Max)==errNone && (int)Max<n+8192)
					continue;
				if (FtrPtrNew(p->ProgramId,(UInt16)FtrId,n,&Ptr)==errNone)
					break;
			} while (!Optional && NodeHibernate());

			if (Ptr)
			{
				p->FtrId = FtrId+1;
				Block->Id = FtrId;
				Block->Ptr = Ptr;
				return 1;
			}
		}
	}

	if (!Optional)
		ShowOutOfMemory();
	return 0;
}

void* PhyMemAlloc(int Length,phymemblock* Blocks,int* BlockCount) { return NULL; }
void PhyMemFree(void* p,phymemblock* Blocks,int BlockCount) {}
void* PhyMemBeginEx(phymemblock* Blocks,int BlockCount,bool_t Cached) { return NULL; }
void* PhyMemBegin(uint32_t Phy,uint32_t Length,bool_t Cached) { return NULL; }
void PhyMemEnd(void* Virt) {}

void CheckHeap() 
{
	assert(MemHeapCheck(MemHeapID(0,0))==0);
}
 
size_t AvailMemory()
{
	uint32_t Max = 0;
	uint32_t Free = 0;
	MemHeapFreeBytes(MemHeapID(0,0),&Free,&Max);
	return Free;
}

void CodeFindPages(void* Ptr,uint8_t** PMin,uint8_t** PMax,size_t* PPageSize)
{
	if (PMin)
		*PMin = (uint8_t*)Ptr;
	if (PMax)
		*PMax = (uint8_t*)Ptr;
	if (PPageSize)
		*PPageSize = 4096;
}

bool_t MemGetInfo(memoryinfo* Info)
{
#ifdef ARM
	int Model = QueryPlatform(PLATFORM_MODEL);
	if (Model == MODEL_TUNGSTEN_T || Model == MODEL_TUNGSTEN_T2)
	{
		if (Info)
		{
			memset(Info,0,sizeof(memoryinfo));
			Info->TLB = (uint32_t*)0x20010000;
			Info->MemBase[0] = 0x10100000;
			Info->MemSize[0] = 14;
			Info->UnusedArea[0] = 0xB0200000;
			Info->MemBase[1] = 0x11000000;
			Info->MemSize[1] = 16;
			Info->UnusedArea[1] = 0xB1000000;
		}
		return 1;
	}
#endif
	return 0;
}

void* MemAsWritable(uint32_t Phy)
{
#ifdef ARM
	memoryinfo Info;
	uint32_t v;
	bool_t Changed;
	int n,i,j;

	if (!Phy)
		return NULL;

	if (!MemGetInfo(&Info))
		return NULL;

	for (n=0;n<2;++n)
		if (Phy >= Info.MemBase[n] && Phy < Info.MemBase[n]+(Info.MemSize[n]<<20))
		{
			Phy = Phy - Info.MemBase[n] + Info.UnusedArea[n];

			Changed = 0;
			for (n=0;n<2;++n)
			{
				j = Info.UnusedArea[n]>>20;
				v = Info.MemBase[n] | 0xC4E;
				for (i=0;i<Info.MemSize[n];i++,v+=1024*1024,++j)
					if (Info.TLB[j] != v)
					{
						Info.TLB[j] = v;
						FlushData(Info.TLB+j);
						Changed = 1;
					}
			}

			if (Changed)
				FlushTLB();
			return (void*)Phy;
		}

#endif
	return NULL;
}

void* MemPhyToVirt(uint32_t Phy)
{
#ifdef ARM
	int Model = QueryPlatform(PLATFORM_MODEL);
	switch (Model)
	{
	case MODEL_TUNGSTEN_T3:
	case MODEL_TUNGSTEN_T5:
	case MODEL_ZIRE_72:
	case MODEL_LIFEDRIVE:
	case MODEL_PALM_TX:
	case MODEL_TREO_650:
		if (Phy >= 0x44000000 && Phy < 0x44100000) // XScale LCD
			return (void*)(Phy+0x94000000-0x44000000);
		break;
	case MODEL_TUNGSTEN_C:
	case MODEL_TUNGSTEN_E2:
		if (Phy >= 0x44000000 && Phy < 0x44100000) // XScale LCD
			return (void*)(Phy+0x92000000-0x44000000);
		break;
	//case MODEL_TUNGSTEN_T: //(borderless mode not supported)
	case MODEL_ZIRE_71:
	case MODEL_TUNGSTEN_E:
	case MODEL_TUNGSTEN_T2:
		if (Phy >= 0xFFF00000) // OMAP
			return (void*)Phy;
		break;
	}
#endif
	return NULL;
}

uint32_t MemVirtToPhy(void* p)
{
#ifdef ARM
	int n;
	memoryinfo Info;
	uint32_t Ptr = (uint32_t)p;
	int Model = QueryPlatform(PLATFORM_MODEL);
	switch (Model)
	{
	case MODEL_ZIRE_72:
	case MODEL_TUNGSTEN_C:
	case MODEL_TUNGSTEN_E2:
	case MODEL_TUNGSTEN_T3:
		if (Ptr >= 0x00000000 && Ptr < 0x04000000) //total memory
			return Ptr+0xA0000000;
		break;
	case MODEL_TUNGSTEN_T5:
		if (Ptr >= 0x00200000 && Ptr < 0x00600000) //dynamic
			return Ptr+0xA1C00000-0x00200000;
		// no info about storage yet...
		break;
	case MODEL_TREO_650:
		if (Ptr >= 0x64000000 && Ptr < 0x64500000) //dynamic
			return Ptr+0xA1A00000-0x64000000;
		if (Ptr >= 0x73100000 && Ptr < 0x73c00000) //storage
			return Ptr+0xA0F00000-0x73100000;
		break;
	case MODEL_LIFEDRIVE:
		if (Ptr >= 0x00200000 && Ptr < 0x00800000) //dynamic
			return Ptr+0xA1A00000-0x00200000;
		if (Ptr >= 0x00800000 && Ptr < 0x01430000) //storage
			return Ptr+0xA0DD0000-0x00800000;
		break;
	case MODEL_PALM_TX:
		if (Ptr >= 0x00200000 && Ptr < 0x00800000) //dynamic
			return Ptr+0xA1A00000-0x00200000;
		if (Ptr >= 0x00800000 && Ptr < 0x013D0000) //storage
			return Ptr+0xA0E30000-0x00800000;
		break;
	case MODEL_ZIRE_71:
		if (Ptr >= 0x00100000 && Ptr < 0x00200000) //dynamic
			return Ptr+0x10010000-0x00100000;
		if (Ptr >= 0x00200000 && Ptr < 0x00fe0000) //storage
			return Ptr+0x10120000-0x00200000;
		break;
	case MODEL_TUNGSTEN_E:
		if (Ptr >= 0x00100000 && Ptr < 0x00300000) //dynamic
			return Ptr+0x10020000-0x00100000;
		if (Ptr >= 0x00300000 && Ptr < 0x01fd0000) //storage
			return Ptr+0x10230000-0x00300000;
		break;
	case MODEL_TUNGSTEN_T:
	case MODEL_TUNGSTEN_T2:
		if (Ptr >= 0x00100000 && Ptr < 0x001c9000) //dynamic
			return Ptr+0x10037000-0x00100000;
		if (Ptr >= 0x00200000 && Ptr < 0x01000000) //storage
			return Ptr+0x10100000-0x00200000;
		if (Ptr >= 0x01000000 && Ptr < 0x02000000) //T2 additional storage
			return Ptr+0x11000000-0x01000000;
		break;
	}

	if (MemGetInfo(&Info))
		for (n=0;n<2;++n)
			if (Ptr >= Info.UnusedArea[n] && Ptr < Info.UnusedArea[n]+(Info.MemSize[n]<<20))
				return Ptr-Info.UnusedArea[n]+Info.MemBase[n];

#endif
	return 0;
}

void Mem_Init()
{

	Context()->TryDynamic = Context()->StartUpMemory > TRY_DYNAMIC;
	StorageId = MemHeapID(0,1);

#ifdef HAVE_PALMONE_SDK
	{
		UInt32 Version;
		if (FtrGet(sysFtrCreator,sysFtrNumDmAutoBackup,&Version)==errNone && Version==1)
			StorageId = 1|dbCacheFlag;
	}
#endif

#ifdef ARM
	if (AllocBlock(MAX_CACHE+sizeof(uint32_t),&FlushCache,0,HEAP_ANY))
	{
		uint32_t p[64];
		int i;
		for (i=0;i<64;++i)
			p[i] = 0xE51F0008; //ldr r0,[pc,#-8]
		for (i=0;i<=MAX_CACHE-sizeof(p);i+=sizeof(p))
			WriteBlock(&FlushCache,i,p,sizeof(p));
		p[0] = 0xE1A0F00E; // mov pc,lr
		WriteBlock(&FlushCache,i,p,sizeof(uint32_t));
	}
#endif
}

void Mem_Done()
{
#ifdef ARM
	FreeBlock(&FlushCache);
#endif
}

#endif
