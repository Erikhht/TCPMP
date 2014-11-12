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
 * $Id: node_palmos.c 345 2005-11-19 15:57:54Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"

#if defined(TARGET_PALMOS)

#define PREFID_NODE			2
#define PREFID_PLUGINS		100

#define FTRID_START			64
#define FTRID_STEP			64
#define FTRID_COUNT			60

#include "pace.h"

#ifdef HAVE_PALMONE_SDK
#include <68K/System/PmPalmOSNVFS.h>
#endif

void NodeRegSaveGlobal()
{
	uint8_t Data[MAXDATA];
	array Buffer = {NULL};
	array List;
	node **i;
	UInt16 PrefId=PREFID_NODE;
	uint32_t ProgramId = Context()->ProgramId;

	NodeEnumObject(&List,NODE_CLASS);

	for (i=ARRAYBEGIN(List,node*);i!=ARRAYEND(List,node*);++i)
	{
		int No,Total;
		datadef Type;

		for (No=0;NodeEnum(*i,No,&Type)==ERR_NONE;++No)
		{
			node* Node = *i;
			if ((Type.Flags & DF_SETUP) && !(Type.Flags & (DF_NOSAVE|DF_RDONLY)))
			{
				int Result = Node->Get(Node,Type.No,Data,Type.Size);
				if (Result == ERR_NONE || Result == ERR_NOT_SUPPORTED) // not supported settings should be still saved
				{
					if (Type.Type == TYPE_STRING)
						Type.Size = (tcslen((tchar_t*)Data)+1)*sizeof(tchar_t);

					Total = sizeof(int32_t)*3+ALIGN4(Type.Size);

					if (ARRAYCOUNT(Buffer,uint8_t)+Total > 0xF000)
					{
						PrefSetAppPreferences(ProgramId,PrefId++,CONTEXT_VERSION,ARRAYBEGIN(Buffer,uint8_t),(UInt16)ARRAYCOUNT(Buffer,uint8_t),0);
						ArrayDrop(&Buffer);
					}

					if (ArrayAppend(&Buffer,NULL,Total,4096))
					{
						int32_t* p = (int32_t*)(ARRAYEND(Buffer,uint8_t) - Total);
						memset(p,0,Total);
						p[0] = Node->Class;
						p[1] = Type.No;
						p[2] = Type.Size;
						memcpy(p+3,Data,Type.Size);
					}
				}
			}
		}
	}

	if (!ARRAYEMPTY(Buffer))
		PrefSetAppPreferences(ProgramId,PrefId++,CONTEXT_VERSION,ARRAYBEGIN(Buffer,uint8_t),(UInt16)ARRAYCOUNT(Buffer,uint8_t),0);

	ArrayClear(&List);
	ArrayClear(&Buffer);

	for (;;)
	{
		UInt16 PrefSize = 0;
		if (PrefGetAppPreferences(ProgramId, PrefId, NULL, &PrefSize, 0) == noPreferenceFound)
			break;
		PrefSetAppPreferences(ProgramId,PrefId++,CONTEXT_VERSION,NULL,0,0);
	}
}

void NodeRegLoadGlobal()
{
	array Buffer = {NULL};
	array List;
	node **i;
	int32_t* p;
	UInt16 PrefId;
	UInt16 PrefSize;
	uint32_t ProgramId = Context()->ProgramId;

	NodeEnumObject(&List,NODE_CLASS);

	for (PrefId=PREFID_NODE;;++PrefId)
	{
		PrefSize = 0;
		if (PrefGetAppPreferences(ProgramId, PrefId, NULL, &PrefSize, 0) == noPreferenceFound)
			break;

		ArrayDrop(&Buffer);
		if (ArrayAppend(&Buffer,NULL,PrefSize,16) && 
			PrefGetAppPreferences(ProgramId, PrefId, ARRAYBEGIN(Buffer,int32_t), &PrefSize, 0) != noPreferenceFound)
		{
			for (p=ARRAYBEGIN(Buffer,int32_t);p<ARRAYEND(Buffer,int32_t);p+=3+(p[2]+3)/4)
				for (i=ARRAYBEGIN(List,node*);i!=ARRAYEND(List,node*);++i)
					if ((*i)->Class == p[0])
					{
						(*i)->Set(*i,p[1],p+3,p[2]);
						break;
					}
		}
	}

	ArrayClear(&List);
	ArrayClear(&Buffer);
}

static NOINLINE void AddModule(const tchar_t* Path,int Type,int Date,int32_t* Modules)
{
	int No=0;
	int Count=0;

	if (Modules)
	{
		//don't load if already registered with same date
		Count = Modules[-1];
		for (No=0;No<Count;++No)
		{
			if ((!Type || Modules[0] == Type) &&
				tcscmp((const tchar_t*)(Modules+3),Path)==0)
			{
				Type = Modules[0];
				if (Modules[1] == Date)
				{
					Modules[1] = -1;
					break;
				}
				Modules[1] = -2;
			}
			Modules += 1+1+1+Modules[2]/4;
		}
	}

	NodeAddModule(Path,Type,Date,No>=Count,0);
}

static void VFSFindPlugins(uint16_t Ref,tchar_t* Path,int PathLen,int32_t* Modules,const tchar_t* Skip)
{
	FileRef	File;

	int PathPos = tcslen(Path);
	if (PathPos>0 && Path[PathPos-1]=='/')
		Path[PathPos-1] = 0;

	if (Skip && tcscmp(Path,Skip)==0)
		return;

	if (VFSFileOpen(Ref,Path,vfsModeRead,&File)==errNone)
	{
		FileInfoType Info;
		UInt32 Iter = vfsIteratorStart;

		tcscat_s(Path,PathLen,"/");
		PathPos = tcslen(Path);

		Info.nameP = Path + PathPos;
		Info.nameBufLen = (UInt16)((PathLen-PathPos)*sizeof(tchar_t));

		while (Iter != vfsIteratorStop && VFSDirEntryEnumerate(File,&Iter,&Info)==errNone)
		{
			if (tcsnicmp(Path+PathPos,T("tcpmp"),5)==0 &&
				(Path[PathPos+5]=='_' || Path[PathPos+5]==' '))
			{
				FileRef File = 0;
				VFSFileOpen(Ref,Path,vfsModeRead,&File);
				if (File)
				{
					UInt32 Date;
					if (VFSFileGetDate(File,vfsFileDateModified,&Date)==errNone)
					{
						tchar_t URL[MAXPATH]; 
						if (VFSFromVol(Ref,Path,URL,TSIZEOF(URL)))
							AddModule(URL,0,Date,Modules);
					}
					VFSFileClose(File);
				}
			}
		}

		VFSFileClose(File);
	}
}

static void FindPlugins(int32_t* Modules)
{
	bool_t CheckVFS = 1;
	Boolean New = 1;
	LocalID CurrentDB; 
	UInt16 Card; 
	DmSearchStateType SearchState;
	tchar_t Name[48];
	tchar_t URL[MAXPATH]; 
	UInt32 Type;
	UInt32 Date;

	CurrentDB = 0;
	while (DmGetNextDatabaseByTypeCreator(New, &SearchState, 0, Context()->ProgramId, 1, &Card, &CurrentDB)==errNone)
	{
		DmDatabaseInfo(Card,CurrentDB,Name,NULL,NULL,NULL,&Date,NULL,NULL,NULL,NULL,&Type,NULL);
		if ((Type >> 24) == 'X')
		{
			stprintf_s(URL,TSIZEOF(URL),T("mem://%s.prc"),Name);
			AddModule(URL,Type,Date,Modules);
			CheckVFS = 0;
		}
		New = 0;
	}

	if (NodeEnumClass(NULL,VFS_ID))
	{
		UInt16 Ref;
		uint32_t LaunchPath = 0;
		URL[0] = 0;

		if (FtrGet(Context()->ProgramId,20,&LaunchPath)==errNone)
		{
			Ref = ((vfspath*)LaunchPath)->volRefNum;
			SplitURL(((vfspath*)LaunchPath)->path,URL,TSIZEOF(URL),URL,TSIZEOF(URL),NULL,0,NULL,0);
			VFSFindPlugins(Ref,URL,TSIZEOF(URL),Modules,NULL);
		}

		if (CheckVFS || QueryAdvanced(ADVANCED_CARDPLUGINS))
		{
			UInt32 Iter = vfsIteratorStart;
			while (Iter != vfsIteratorStop && VFSVolumeEnumerate(&Ref,&Iter)==errNone)
			{
				tchar_t Path[MAXPATH];
				UInt16 Size = sizeof(Path);

				if (VFSGetDefaultDirectory(Ref,T(".prc"),Path,&Size)==errNone)
					VFSFindPlugins(Ref,Path,TSIZEOF(Path),Modules,URL);
			}
		}
	}
}

static NOINLINE int32_t* GetString(int32_t* p,const tchar_t** s)
{
	if (p[0])
	{
		*s = (const tchar_t*)(p+1); 
		p+=1+p[0]/4;
	}
	else
	{
		*s = NULL;
		++p;
	}
	return p;
}

void Plugins_Init()
{
	array Buffer = {NULL};
	int32_t* p;
	int32_t* Modules;
	int i,No,Count;
	bool_t b;
	UInt16 PrefSize;
	uint32_t ProgramId = Context()->ProgramId;
	node* Advanced = Context()->Advanced;

	PrefSize = 0;
	if (PrefGetAppPreferences(ProgramId, PREFID_PLUGINS, NULL, &PrefSize, 0) != noPreferenceFound &&
		ArrayAppend(&Buffer,NULL,PrefSize,16) &&
		PrefGetAppPreferences(ProgramId, PREFID_PLUGINS, ARRAYBEGIN(Buffer,int32_t), &PrefSize, 0) != noPreferenceFound)
	{
		// add all plugins and load changed/new ones
		p = ARRAYBEGIN(Buffer,int32_t);
		if (*p<0)
		{
			// params
			if (Advanced)
			{
				b = p[1] != 0;
				Advanced->Set(Advanced,ADVANCED_CARDPLUGINS,&b,sizeof(b));
			}
			p += 1-*p;
		}

		Count = *(p++);
		Modules = p;

		FindPlugins(Modules);
		
		// also add non existing modules (maybe SD card removed temporarily)
		for (No=0;No<Count;++No)
		{
			if (p[1] != -1 && p[1] != -2)
				NodeAddModule((const tchar_t*)(p+3),p[0],p[1],0,1);
			p += 1+1+1+p[2]/4;
		}

		// load classes (only if not exists already. don't want to override changed loaded plugins)
		while (*p)
		{
			nodedef Def;
			int Module;
			const tchar_t* Name;
			const tchar_t* ContentType;
			const tchar_t* Exts;
			const tchar_t* Probe;

			memset(&Def,0,sizeof(Def));

			Module = *p-1; ++p;
			Def.Class = *p; ++p;
			Def.ParentClass = *p; ++p;
			Def.Flags = *p; ++p;
			Def.Priority = *p; ++p;
			p = GetString(p,&Name);
			p = GetString(p,&ContentType);
			p = GetString(p,&Exts);
			p = GetString(p,&Probe);

			if (Module<Modules[-1] && !NodeEnumClass(NULL,Def.Class))
			{
				int32_t* p = Modules;
				for (i=0;i<Module;++i)
					p += 1+1+1+p[2]/4;

				if (Name) StringAdd(1,Def.Class,NODE_NAME,Name);
				if (ContentType) StringAdd(1,Def.Class,NODE_CONTENTTYPE,ContentType);
				if (Exts) StringAdd(1,Def.Class,NODE_EXTS,Exts);
				if (Probe) StringAdd(1,Def.Class,NODE_PROBE,Probe);

				NodeLoadClass(&Def,(const tchar_t*)(p+3),p[0]);
			}
		}
	}
	else
		FindPlugins(NULL);

	ArrayClear(&Buffer);
}

bool_t NOINLINE AppendInt(array* Buffer,int Data)
{
	return ArrayAppend(Buffer,&Data,sizeof(Data),4096);
}

bool_t NOINLINE AppendStr(array* Buffer,const tchar_t* Data)
{
	int Len = (tcslen((tchar_t*)Data)+1)*sizeof(tchar_t);
	int Total = ALIGN4(Len);
	if (!AppendInt(Buffer,Total) || !ArrayAppend(Buffer,NULL,Total,4096))
		return 0;
	memcpy(ARRAYEND(*Buffer,uint8_t)-Total,Data,Len);
	memset(ARRAYEND(*Buffer,uint8_t)-Total+Len,0,Total-Len);
	return 1;
}

bool_t NOINLINE AppendString(array* Buffer,int Class,int Id)
{
	if (StringIsBinary(Class,Id))
		return AppendInt(Buffer,0);
	return AppendStr(Buffer,LangStrDef(Class,Id));
}

void Plugins_Done()
{
	array Buffer = {NULL};
	uint32_t ProgramId = Context()->ProgramId;
	int64_t Date;
	int No,Count;
	int Id;

	AppendInt(&Buffer,-1);
	AppendInt(&Buffer,QueryAdvanced(ADVANCED_CARDPLUGINS));

	Count = NodeGetModuleCount();
	AppendInt(&Buffer,Count-1);
	for (No=1;No<Count;++No)
	{
		const tchar_t* Path = NodeGetModule(No,&Id,&Date);
		AppendInt(&Buffer,Id);
		AppendInt(&Buffer,(int)Date);
		AppendStr(&Buffer,Path);
	}

	Count = NodeGetClassCount();
	for (No=0;No<Count;++No)
	{
		const nodedef* Def = NodeGetClass(No,&Id);
		if (Id)
		{
			AppendInt(&Buffer,Id);
			AppendInt(&Buffer,Def->Class);
			AppendInt(&Buffer,Def->ParentClass);
			AppendInt(&Buffer,Def->Flags);
			AppendInt(&Buffer,Def->Priority);
			AppendString(&Buffer,Def->Class,NODE_NAME);
			AppendString(&Buffer,Def->Class,NODE_CONTENTTYPE);
			AppendString(&Buffer,Def->Class,NODE_EXTS);
			AppendString(&Buffer,Def->Class,NODE_PROBE);
		}
	}
	AppendInt(&Buffer,0);

	if (ARRAYCOUNT(Buffer,uint8_t)<0x8000)
		PrefSetAppPreferences(ProgramId,PREFID_PLUGINS,CONTEXT_VERSION,ARRAYBEGIN(Buffer,uint8_t),(UInt16)ARRAYCOUNT(Buffer,uint8_t),0);
	ArrayClear(&Buffer);
}

static void* Modules[FTRID_COUNT] = {NULL};

void NodeFreeModule(void* Module,void* Db)
{
	if (Module)
	{
		int i;
		void* UnRegister = PealGetSymbol(Module,T("DLLUnRegister"));
		if (UnRegister)
			((void(*)())UnRegister)();

		for (i=0;i<FTRID_COUNT;++i)
			if (Modules[i] == Module)
			{
				Modules[i] = NULL;
				break;
			}

		PealFreeModule(Module);
		if (Db)
			DmCloseDatabase(Db);
	}
}

static void* LoadPlugin(UInt16 Card,LocalID CurrentDB,bool_t TmpDb,void** Db,int* Id)
{
	void* Module = NULL;
	char Name[48];
	UInt32 Type;
	UInt16 Attr;
	Boolean MemDup = 0;
	Boolean MemSema = 0;
	int No;

	DmDatabaseInfo(Card,CurrentDB,Name,&Attr,NULL,NULL,NULL,NULL,NULL,NULL,NULL,&Type,NULL);
	if ((Type >> 24) != 'X')
	{
		if (TmpDb)
			DmDeleteDatabase(Card,CurrentDB);
		return NULL;
	}

	*Id = Type;

#ifdef ARM
	{
		UInt32 Version;
		FtrGet(sysFtrCreator, sysFtrNumROMVersion, &Version);
		MemSema = (Boolean)(Version < sysMakeROMVersion(6,0,0,sysROMStageDevelopment,0));
	}
#endif

#ifdef HAVE_PALMONE_SDK
	// duplicate plugins to memory with NVFS, because writing back reallocation 
	// causes the plugins to disappear sometimes from storage flash memory.
	if (!MemSema) // but don't care if can use memory semaphore
	{
		UInt32 Version;
		if (FtrGet(sysFtrCreator,sysFtrNumDmAutoBackup,&Version)==errNone && Version==1)
			MemDup = 1;
	}
#endif

	if (TmpDb)
	{
		Attr |= dmHdrAttrRecyclable;
		DmSetDatabaseInfo(Card,CurrentDB,NULL,&Attr,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
	}

	*Db = DmOpenDatabaseByTypeCreator(Type,Context()->ProgramId,dmModeReadOnly);
	if (*Db)
	{
		for (No=0;No<FTRID_COUNT;++No)
			if (!Modules[No])
				break;

		if (No<FTRID_COUNT)
		{
			Module = PealLoadModule((UInt16)(FTRID_START+No*FTRID_STEP),
			                        MemDup,(Boolean)Context()->LowMemory,MemSema);

			if (Module)
			{
				void* Register = PealGetSymbol(Module,T("DLLRegister"));
				if (Register)
				{
					int Result = ((int(*)(int))Register)(CONTEXT_VERSION);
					if (Result != ERR_NONE)
					{
						Register = NULL;
						if (Result == ERR_NOT_COMPATIBLE)
						{
							int n = tcslen(Name);
							if (n>7 && tcsnicmp(Name+n-7,T(" plugin"),7)==0)
								Name[n-7] = 0;
							ShowError(0,ERR_ID,ERR_NOT_COMPATIBLE,Name);
						}
					}
				}

				if (!Register)
				{
					PealFreeModule(Module);
					Module = NULL;
				}
				else
					Modules[No] = Module;
			}
		}

		if (!Module || MemDup)
		{
			DmCloseDatabase(*Db);
			*Db = NULL;
		}
	}

	return Module;
}

void* NodeLoadModule(const tchar_t* URL,int* Id,void** AnyFunc,void** Db)
{
	void* Module = NULL;
	UInt16 Card;
	LocalID CurrentDB;

	if (tcsncmp(URL,"mem",3)!=0)
	{
		uint16_t Vol;
		const tchar_t* Path = VFSToVol(URL,&Vol);
		if (Path && VFSImportDatabaseFromFile(Vol,Path,&Card,&CurrentDB)==errNone)
			Module = LoadPlugin(Card,CurrentDB,1,Db,Id);
	}
	else
	{
		DmSearchStateType SearchState;
		CurrentDB = 0;
		if (DmGetNextDatabaseByTypeCreator(1, &SearchState, *Id, Context()->ProgramId, 1, &Card, &CurrentDB)==errNone)
			Module = LoadPlugin(Card,CurrentDB,0,Db,Id);
	}

	return Module;
}

#endif
