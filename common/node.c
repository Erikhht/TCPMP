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
 * $Id: node.c 585 2006-01-16 09:48:55Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "common.h"

#define MODULE_PATH			FOURCC('M','P','A','T')
#define PLUGIN_KEEPALIVE	3*60

typedef struct nodeclass
{
	nodedef Def;
	bool_t Registered;
	int ModuleNo;
	struct nodeclass* Parent;

} nodeclass;

typedef struct nodemodule
{
	int Id;
	int ObjectCount;
	bool_t Tmp;
	int64_t Date;
	int KeepAlive;
	void* Module;
	void* Db;
	void* Func;
	uint8_t* Min;
	uint8_t* Max;

} nodemodule;

static NOINLINE void FreeModule(nodemodule* p)
{
	NodeFreeModule(p->Module,p->Db);
	p->Module = NULL;
	p->Db = NULL;
	p->Func = NULL;
	p->Min = NULL;
}

void NodeAddModule(const tchar_t* Path,int Id,int64_t Date,bool_t Load,bool_t Tmp)
{
	context* p = Context();
	LockEnter(p->NodeLock);
	if (ArrayAppend(&p->NodeModule,NULL,sizeof(nodemodule),256))
	{
		int No = ARRAYCOUNT(p->NodeModule,nodemodule)-1;
		nodemodule* Module = ARRAYBEGIN(p->NodeModule,nodemodule)+No;
		memset(Module,0,sizeof(nodemodule));
		Module->Id = Id;
		Module->Date = Date;
		Module->Tmp = Tmp;

		StringAdd(1,MODULE_PATH,No,Path);
		if (Load)
		{
			p->LoadModuleNo = No;
			Module->Module = NodeLoadModule(Path,&Module->Id,&Module->Func,&Module->Db);
			p->LoadModuleNo = 0;
#ifdef PLUGINCLEANUP
			FreeModule(Module);
#endif
		}
	}
	LockLeave(p->NodeLock);
}

static NOINLINE bool_t TmpModule(context* p,int No)
{
	return ARRAYBEGIN(p->NodeModule,nodemodule)[No].Tmp;
}

static NOINLINE int FindModule(context* p,const tchar_t* Path,int Id)
{
	// important to find from the begining for Palm OS 
	// so the exising plugins are found first

	int No,Count;
	int Result = -1;

	LockEnter(p->NodeLock);
	Count = ARRAYCOUNT(p->NodeModule,nodemodule);
	if (!Path) Path = T("");
	for (No=0;No<Count;++No)
	{
		bool_t SameId = ARRAYBEGIN(p->NodeModule,nodemodule)[No].Id == Id;
		bool_t SameName = tcsicmp(Path,LangStrDef(MODULE_PATH,No))==0;

		if (SameId && Id!=0) // same Id means same module
			SameName = 1;
		if (SameName && !Id)
			SameId = 1;

		if (SameId && SameName)
		{
			Result = No;
			break;
		}
	}
	LockLeave(p->NodeLock);
	return Result;
}

int NodeGetClassCount()
{
	return ARRAYCOUNT(Context()->NodeClass,nodeclass*);
}

const nodedef* NodeGetClass(int No,int* Module)
{
	nodeclass* i = ARRAYBEGIN(Context()->NodeClass,nodeclass*)[No];
	if (Module)
		*Module = i->ModuleNo;
	return &i->Def;
}

int NodeGetModuleCount()
{
	return ARRAYCOUNT(Context()->NodeModule,nodemodule);
}

const tchar_t* NodeGetModule(int No,int* Id,int64_t* Date)
{
	*Id = ARRAYBEGIN(Context()->NodeModule,nodemodule)[No].Id;
	*Date = ARRAYBEGIN(Context()->NodeModule,nodemodule)[No].Date;
	return LangStrDef(MODULE_PATH,No);
}

bool_t NodeFindModule(const tchar_t* Path,int Id)
{
	return FindModule(Context(),Path,Id)>0;
}

static int CmpClass(const nodeclass* const* a, const nodeclass* const* b)
{
	int AClass = (*a)->Def.Class;
	int BClass = (*b)->Def.Class;
	if (AClass > BClass) return 1;
	if (AClass < BClass) return -1;
	return 0;
}

static int CmpClassPri(const nodeclass* const* a, const nodeclass* const* b)
{
	int APriority = (*a)->Def.Priority;
	int BPriority = (*b)->Def.Priority;
	if (APriority > BPriority) return -1;
	if (APriority < BPriority) return 1;
	return CmpClass(a,b);
}

static NOINLINE nodeclass* FindClass(context* p,int Class)
{
	int Pos;
	bool_t Found;
	nodeclass Item;
	nodeclass* Ptr = &Item;

	LockEnter(p->NodeLock);
	Item.Def.Class = Class;
	Pos = ArrayFind(&p->NodeClass,ARRAYCOUNT(p->NodeClass,nodeclass*),
	                sizeof(nodeclass*),&Ptr,(arraycmp)CmpClass,&Found);
	if (Found)
		Ptr = ARRAYBEGIN(p->NodeClass,nodeclass*)[Pos];
	else
		Ptr = NULL;
	LockLeave(p->NodeLock);
	return Ptr;
}

static int CmpNodePri(const node* const* a,const node* const* b)
{
	context* p = Context();
	const nodeclass* ca = FindClass(p,(*a)->Class);
	const nodeclass* cb = FindClass(p,(*b)->Class);
	return CmpClassPri(&ca,&cb);
}

static NOINLINE int ReleaseModules(context* p,bool_t Force)
{
	int n=0;
	int Time = Force ? MAX_INT:GetTimeTick();
	nodemodule* j;
	LockEnter(p->NodeLock);
	for (j=ARRAYBEGIN(p->NodeModule,nodemodule);j!=ARRAYEND(p->NodeModule,nodemodule);++j)
		if (j->ObjectCount==0 && j->Module && j->KeepAlive < Time)
		{
			//DEBUG_MSG1(-1,T("FreeModule %s"),LangStrDef(MODULE_PATH,j-ARRAYBEGIN(p->NodeModule,nodemodule)));
			FreeModule(j);
			++n;
		}
	LockLeave(p->NodeLock);
	return n;
}

static NOINLINE void UnlockModule(context* p,int No)
{
	nodemodule* Module = ARRAYBEGIN(p->NodeModule,nodemodule)+No;
	if (--Module->ObjectCount==0 && Module->Module)
	{
#ifdef PLUGINCLEANUP
		Module->KeepAlive = GetTimeTick() + GetTimeFreq()*PLUGIN_KEEPALIVE;
		ReleaseModules(p,0);
#endif
	}
}

static NOINLINE void CallDelete(context* p,node* Node,nodeclass* Class0,bool_t Global)
{
	nodeclass* Class;

	for (Class=Class0;Class;Class=Class->Parent)
		if (Class->Def.Delete)
			Class->Def.Delete(Node);

	if (!Global)
		for (Class=Class0;Class;Class=Class->Parent)
			UnlockModule(p,Class->ModuleNo);
}

static NOINLINE nodemodule* LoadModule(context* p,int No)
{
	nodemodule* Module = ARRAYBEGIN(p->NodeModule,nodemodule)+No;
	if (!Module->Module && No>0 && p->LoadModuleNo!=No)
	{	
		//DEBUG_MSG1(-1,T("LoadModule %s"),LangStrDef(MODULE_PATH,No));
		p->LoadModuleNo = No;
		Module->Module = NodeLoadModule(LangStrDef(MODULE_PATH,No),&Module->Id,&Module->Func,&Module->Db);
		p->LoadModuleNo = 0;
		if (!Module->Module)
			return NULL;
	}
	return Module;
}

static int CallCreate(context* p,node* Node,nodeclass* Class,bool_t Global)
{
	if (Class)
	{
		nodemodule* Module = LoadModule(p,Class->ModuleNo);
		if (!Module)
			return ERR_NOT_SUPPORTED;

		if (Class->Def.ParentClass && !Class->Parent)
			return ERR_NOT_SUPPORTED;

		if (CallCreate(p,Node,Class->Parent,Global)!=ERR_NONE)
			return ERR_NOT_SUPPORTED;

		if (!Class->Registered || (Class->Def.Create && Class->Def.Create(Node) != ERR_NONE))
		{
			CallDelete(p,Node,Class->Parent,Global);
			return ERR_NOT_SUPPORTED;
		}

		if (!Global)
			Module->ObjectCount++;
	}
	return ERR_NONE;
}

static void Delete(context* p,node** i)
{
	node* Node = *i;
	nodeclass* Item = FindClass(p,Node->Class);
	*i = NULL;
	if (Item)
		CallDelete(p,Node,Item,(Item->Def.Flags & CF_GLOBAL)!=0);
	free(Node);
}

static void Register(context* p,const nodedef* Def,bool_t Registered,int ModuleNo)
{
	nodeclass* Class;

	LockEnter(p->NodeLock);
	
	Class = FindClass(p,Def->Class);
	if (!Class)
	{
		nodeclass **c;

		Class = (nodeclass*) malloc(sizeof(nodeclass));
		if (!Class)
		{
			LockLeave(p->NodeLock);
			return;
		}

		Class->Def.Class = Def->Class;
		Class->Def.ParentClass = 0;
		Class->Registered = 0;
		Class->ModuleNo = 0;
		Class->Def.Flags = 0;
		Class->Def.Priority = -1;
		if (!ArrayAdd(&p->NodeClass,ARRAYCOUNT(p->NodeClass,nodeclass*),
		              sizeof(nodeclass*),&Class,(arraycmp)CmpClass,128))
		{
			free(Class);
			LockLeave(p->NodeLock);
			return;
		}
		
		// fix possible child classes
		for (c=ARRAYBEGIN(p->NodeClass,nodeclass*);c!=ARRAYEND(p->NodeClass,nodeclass*);++c)
			if ((*c)->Def.ParentClass == Def->Class)
				(*c)->Parent = Class;
	}
	else
	if (!Registered)
	{
		LockLeave(p->NodeLock);
		return;
	}

	if (Registered)
	{
		if (Class->Def.Priority > Def->Priority && Class->ModuleNo != ModuleNo)
		{
			LockLeave(p->NodeLock);
			return; // already exits with higher priority in another module
		}

#ifndef REGISTRY_GLOBAL
		if (ModuleNo>0 && (Class->ModuleNo != ModuleNo || Class->Def.ParentClass != Def->ParentClass || Class->Def.Flags != Def->Flags))
		{
			// save settings to registry for new classes 
			NodeRegSaveString(Def->Class,NODE_NAME);
			NodeRegSaveString(Def->Class,NODE_CONTENTTYPE);
			NodeRegSaveString(Def->Class,NODE_EXTS);
			NodeRegSaveString(Def->Class,NODE_PROBE);
			NodeRegSaveValue(Def->Class,NODE_MODULE_PATH,LangStrDef(MODULE_PATH,ModuleNo),MAXPATH,TYPE_STRING);
			NodeRegSaveValue(Def->Class,NODE_PARENT,&Def->ParentClass,sizeof(int),TYPE_INT);
			NodeRegSaveValue(Def->Class,NODE_FLAGS,&Def->Flags,sizeof(int),TYPE_INT);
		}
#endif
	}

	if (Class->Def.Priority != Def->Priority)
	{
		ArrayRemove(&p->NodeClassPri,ARRAYCOUNT(p->NodeClassPri,nodeclass*),
		            sizeof(nodeclass*),&Class,(arraycmp)CmpClassPri);
		Class->Def.Priority = Def->Priority;
		ArrayAdd(&p->NodeClassPri,ARRAYCOUNT(p->NodeClassPri,nodeclass*),
		         sizeof(nodeclass*),&Class,(arraycmp)CmpClassPri,128);

#ifndef REGISTRY_GLOBAL
		if (ModuleNo>0 && Registered)
			NodeRegSaveValue(Def->Class,NODE_PRIORITY,Def->Priority!=PRI_DEFAULT?&Def->Priority:NULL,sizeof(int),TYPE_INT);
#endif
	}

	Class->Def = *Def;
	Class->ModuleNo = ModuleNo;
	Class->Parent = FindClass(p,Def->ParentClass);
	Class->Registered = Registered;

	LockLeave(p->NodeLock);
}

void NodeLoadClass(const nodedef* Def,const tchar_t* Module,int ModuleId)
{
	context* p = Context();
	int ModuleNo = FindModule(p,Module,ModuleId);
	if (ModuleNo>=0)
		Register(p,Def,0,ModuleNo);
}

void NodeUnRegisterClass(int ClassId)
{
	context* p = Context();
	nodeclass* Class = FindClass(p,ClassId);
	if (Class && Class->Registered)
	{
		node **i;

		LockEnter(p->NodeLock);

		Class->Registered = 0;

		// delete all objects
		for (i=ARRAYBEGIN(p->Node,node*);i!=ARRAYEND(p->Node,node*);++i)
			if (*i && NodeIsClass((*i)->Class,ClassId))
				Delete(p,i);

		Class->Def.Create = NULL;
		Class->Def.Delete = NULL;

		LockLeave(p->NodeLock);
	}
}

static node* NodeCreateFromClass(context* p, nodeclass* Class)
{
	node **Empty = NULL;
	node **i;
	nodemodule* Module;
	node* Node;
	int Size;
	nodeclass *j;

	if (!Class || (Class->Def.Flags & CF_ABSTRACT))
		return NULL;

	Size = 0;
	for (j=Class;j;j=j->Parent)
		if (Size < (j->Def.Flags & CF_SIZE))
			Size = (j->Def.Flags & CF_SIZE);
	if (!Size)
		return NULL;

	LockEnter(p->NodeLock);

	if ((Class->Def.Flags & CF_GLOBAL) && 
		(Module = LoadModule(p,Class->ModuleNo))!=NULL &&
		(Node = NodeEnumObject(NULL,Class->Def.Class))!=NULL)
	{
		++Module->ObjectCount;
		LockLeave(p->NodeLock);
		return Node;
	}

	for (i=ARRAYBEGIN(p->Node,node*);i!=ARRAYEND(p->Node,node*);++i)
		if (!*i)
		{
			Empty = i;
			break;
		}

	if (!Empty)
	{
		if (!ArrayAppend(&p->Node,NULL,sizeof(node**),256))
		{
			LockLeave(p->NodeLock);
			return NULL;
		}
		Empty = ARRAYEND(p->Node,node*)-1;
	}

	*Empty = Node = (node*) malloc(Size);
	if (Node)
	{
		memset(Node,0,Size);
		Node->Class = Class->Def.Class;
		if (CallCreate(p,Node,Class,(Class->Def.Flags & CF_GLOBAL)!=0) != ERR_NONE)
		{
			for (i=ARRAYBEGIN(p->Node,node*);i!=ARRAYEND(p->Node,node*);++i)
				if (*i == Node)
					*i = NULL;
			free(Node);
			Node = NULL;
		}
		else
		{
#ifndef REGISTRY_GLOBAL
			if (Class->Def.Flags & CF_GLOBAL)
				NodeRegLoad(Node);
#endif
			Node->Set(Node,NODE_SETTINGSCHANGED,NULL,0); // should be after NodeRegLoad
		}
	}

	LockLeave(p->NodeLock);
	return Node;
}

void NodeRegisterClass(const nodedef* Def)
{
	context* p = Context();
	if (Def->Flags & CF_GLOBAL)
	{
		//create global object with fake class and don't register if fails
		nodeclass Class;
		memset(&Class,0,sizeof(Class));
		Class.Def = *Def;
		Class.ModuleNo = p->LoadModuleNo; 
		Class.Parent = FindClass(p,Def->ParentClass);
		Class.Registered = 1;
		if (!Class.Parent || !NodeCreateFromClass(p,&Class))
			return;
	}
	Register(p,Def,1,p->LoadModuleNo);
}

node* NodeCreate(int ClassId)
{
	context* p = Context();
	return NodeCreateFromClass(p,FindClass(p,ClassId));
}

void NodeDelete(node* Node)
{
	if (Node)
	{
		nodeclass* Class;
		context* p = Context();

		LockEnter(p->NodeLock); 
		Class = FindClass(p,Node->Class);
		if (Class && (Class->Def.Flags & CF_GLOBAL))
			UnlockModule(p,Class->ModuleNo);
		else
		{
			node **i;
			for (i=ARRAYBEGIN(p->Node,node*);i!=ARRAYEND(p->Node,node*);++i)
				if (*i == Node)
				{
					Delete(p,i);
					break;
				}
		}
		LockLeave(p->NodeLock);
	}
}

const nodedef* NodeClassDef(int Class)
{
	nodeclass* i = FindClass(Context(),Class);
	return i?&i->Def:NULL;
}

static NOINLINE bool_t GetNode(context* p,int n,node** Node)
{
	node** i;
	bool_t Result = 0;
	LockEnter(p->NodeLock);
	i = ARRAYBEGIN(p->Node,node*)+n;
	if (i<ARRAYEND(p->Node,node*))
	{
		*Node = *i;
		Result = 1;
	}
	LockLeave(p->NodeLock);
	return Result;
}

bool_t NodeHibernate()
{
	int Mode;
	int No;
	node* Node;
	context* p = Context();
	bool_t Changed = 0;
	
	if (p && !p->InHibernate)
	{
		p->InHibernate = 1;

#ifdef PLUGINCLEANUP
		if (ReleaseModules(p,1))
			Changed = 1;
#endif

		for (Mode=0;!Changed && Mode<HIBERNATEMODES;++Mode)
			for (No=0;GetNode(p,No,&Node);++No)
				if (Node && Node->Set && Node->Set(Node,NODE_HIBERNATE,&Mode,sizeof(int))==ERR_NONE)
					Changed = 1;

		if (!Changed && ReleaseModules(p,1))
			Changed = 1;

		p->InHibernate = 0;
	}
	return Changed;
}

int NodeBroadcast(int Msg,const void* Data,int Size)
{
	int No;
	context* p = Context();
	node* Node;

	for (No=0;GetNode(p,No,&Node);++No)
		if (Node && Node->Set)
			Node->Set(Node,Msg,Data,Size);

	return ERR_NONE;
}

int NodeSettingsChanged()
{
	return NodeBroadcast(NODE_SETTINGSCHANGED,NULL,0);
}

node* NodeEnumObject(array* List, int Class)
{
	context* p = Context();
	node **i;

	if (List)
		memset(List,0,sizeof(array));

	LockEnter(p->NodeLock);

	for (i=ARRAYBEGIN(p->Node,node*);i!=ARRAYEND(p->Node,node*);++i)
		if (*i && NodeIsClass((*i)->Class,Class))
		{
			if (!List)
			{
				node* Node = *i;
				LockLeave(p->NodeLock);
				return Node;
			}
			ArrayAppend(List,i,sizeof(node**),64);
		}

	LockLeave(p->NodeLock);

	if (List)
		ArraySort(List,ARRAYCOUNT(*List,node*),sizeof(node*),(arraycmp)CmpNodePri);

	return NULL;
}

bool_t NodeIsClass(int ClassId, int PartOfClass)
{
	nodeclass* Item;
	for (Item = FindClass(Context(),ClassId);Item;Item=Item->Parent)
		if (Item->Def.Class == PartOfClass)
			return 1;
	return 0;
}

static void DumpPtr(stream* File, tchar_t* s, node* Ptr, int No)
{
	if (Ptr)
	{
		tchar_t v[32];
		if (No)
			stprintf_s(v,TSIZEOF(v),T(":%d"),No);
		else
			v[0] = 0;
		StreamPrintf(File,T("%s%s%s (0x%08x)\n"),s,LangStrDef(Ptr->Class,0),v,(int)Ptr);
	}
	else
		StreamPrintf(File,T("%sNULL\n"),s);
}

static void DumpFormat(stream* File, tchar_t* s,const packetformat* Format)
{
	tchar_t v[8];
	switch (Format->Type)
	{
	case PACKET_VIDEO:
		StreamPrintf(File,T("%svideo"),s);
		if (Format->Format.Video.Pixel.Flags & PF_FOURCC) 
		{
			FourCCToString(v,TSIZEOF(v),Format->Format.Video.Pixel.FourCC);
			StreamPrintf(File,T(" fourcc=%s"),v);
		}
		else
			StreamPrintf(File,T(" bitcount=%d rmask=%04x gmask=%04x bmask=%04x"),Format->Format.Video.Pixel.BitCount,
				Format->Format.Video.Pixel.BitMask[0],Format->Format.Video.Pixel.BitMask[1],Format->Format.Video.Pixel.BitMask[2]);
		if (Format->Format.Video.Width) StreamPrintf(File,T(" width=%d"),Format->Format.Video.Width);
		if (Format->Format.Video.Height) StreamPrintf(File,T(" height=%d"),Format->Format.Video.Height);
		if (Format->Format.Video.Pitch) StreamPrintf(File,T(" pitch=%d"),Format->Format.Video.Pitch);
		if (Format->Format.Video.Direction) StreamPrintf(File,T(" dir=%d"),Format->Format.Video.Direction);
		if (Format->ByteRate) StreamPrintf(File,T(" bitrate=%d"),Format->ByteRate*8);
		StreamPrintf(File,T("\n"));
		break;
	case PACKET_AUDIO:
		StreamPrintf(File,T("%saudio"),s);
		if (Format->Format.Audio.Format) StreamPrintf(File,T(" fmt=%04x"),Format->Format.Audio.Format);
		if (Format->Format.Audio.SampleRate) StreamPrintf(File,T(" rate=%d"),Format->Format.Audio.SampleRate);
		if (Format->Format.Audio.Channels) StreamPrintf(File,T(" ch=%d"),Format->Format.Audio.Channels);
		if (Format->Format.Audio.Bits) StreamPrintf(File,T(" bits=%d"),Format->Format.Audio.Bits);
		if (Format->ByteRate) StreamPrintf(File,T(" bitrate=%d"),Format->ByteRate*8);
		StreamPrintf(File,T("\n"));
		break;
	case PACKET_SUBTITLE:
		FourCCToString(v,TSIZEOF(v),Format->Format.Subtitle.FourCC);
		StreamPrintf(File,T("%ssubtitle fourcc=%s\n"),s,v);
		break;
	case PACKET_NONE:
		StreamPrintf(File,T("%sempty packet format\n"),s);
		break;
	default:
		StreamPrintf(File,T("%sunknown packet format (%d)\n"),s,Format->Type);
		break;
	}
}

void NodeDump(struct stream* File)
{
	context* p = Context();
	node **i;
	nodemodule* j;
	nodeclass** k;
	int No;

	for (No=0,j=ARRAYBEGIN(p->NodeModule,nodemodule);j!=ARRAYEND(p->NodeModule,nodemodule);++j,++No)
		TRY_BEGIN 
		{
			int ClassCount = 0;
			for (k=ARRAYBEGIN(p->NodeClass,nodeclass*);k!=ARRAYEND(p->NodeClass,nodeclass*);++k)
				if ((*k)->ModuleNo == No)
					++ClassCount;

			if (j->Func && !j->Min)
				CodeFindPages(j->Func,&j->Min,&j->Max,NULL);
			StreamPrintf(File,T("%s %08x-%08x obj:%d class:%d\n"),LangStrDef(MODULE_PATH,No),j->Min,j->Max,j->ObjectCount,ClassCount);
		}
		TRY_END

	StreamPrintf(File,T("\n\n"));

	//no NodeLock, becase may be stayed locked when dumping after crash 
	for (i=ARRAYBEGIN(p->Node,node*);i!=ARRAYEND(p->Node,node*);++i)
		if (*i)
			TRY_BEGIN 
			{
				tchar_t Flags[64];
				tchar_t s[256];
				tchar_t Buffer[512];
				const tchar_t* Name;
				rect Rect;
				packetformat Format;
				pin Pin;
				rgb RGB;
				point Point;
				bool_t Bool;
				tick_t Tick;
				fraction Fraction;
				node* Ptr;
				int Int,No;
				node* p = *i;
				datadef DataDef;
		
				StreamPrintf(File,T("%s (0x%08x)\n"),LangStrDef(p->Class,0),(int)p);

				for (No=0;NodeEnum(p,No,&DataDef)==ERR_NONE;++No)
				{
					TRY_BEGIN 
					{

						Name = LangStrDef(DataDef.Class,DataDef.No);
						if (!Name[0]) Name = DataDef.Name ? DataDef.Name : T("");

						Flags[0] = 0;
						if (DataDef.Flags & DF_INPUT)
							tcscat_s(Flags,TSIZEOF(Flags),T(":IN"));
						if (DataDef.Flags & DF_OUTPUT)
							tcscat_s(Flags,TSIZEOF(Flags),T(":OUT"));

						stprintf_s(s,TSIZEOF(s),T("  %s(%d)%s="),Name,DataDef.No,Flags);

						switch (DataDef.Type)
						{
						case TYPE_BOOL:
							if (p->Get(p,DataDef.No,&Bool,sizeof(bool_t))==ERR_NONE)
							{
								BoolToString(Buffer,TSIZEOF(Buffer),Bool);
								StreamPrintf(File,T("%s%s\n"),s,Buffer);
							}
							break;
						case TYPE_TICK:
							if (p->Get(p,DataDef.No,&Tick,sizeof(tick_t))==ERR_NONE)
							{
								TickToString(Buffer,TSIZEOF(Buffer),Tick,Tick<TICKSPERSEC,1,0);
								StreamPrintf(File,T("%s%s\n"),s,Buffer);
							}
							break;
						case TYPE_INT:
							if (p->Get(p,DataDef.No,&Int,sizeof(int))==ERR_NONE)
							{
								IntToString(Buffer,TSIZEOF(Buffer),Int,(DataDef.Flags & DF_HEX)!=0);
								StreamPrintf(File,T("%s%s\n"),s,Buffer);
							}
							break;
						case TYPE_HOTKEY:
							if (p->Get(p,DataDef.No,&Int,sizeof(int))==ERR_NONE)
							{
								HotKeyToString(Buffer,TSIZEOF(Buffer),Int);
								StreamPrintf(File,T("%s%s\n"),s,Buffer);
							}
							break;
						case TYPE_FRACTION:
							if (p->Get(p,DataDef.No,&Fraction,sizeof(fraction))==ERR_NONE)
							{
								FractionToString(Buffer,TSIZEOF(Buffer),&Fraction,1,2);
								StreamPrintf(File,T("%s%s\n"),s,Buffer);
							}
							break;
						case TYPE_STRING:
							if (p->Get(p,DataDef.No,Buffer,sizeof(Buffer))==ERR_NONE)
								StreamPrintf(File,T("%s%s\n"),s,Buffer);
							break;
						case TYPE_RECT:
							if (p->Get(p,DataDef.No,&Rect,sizeof(rect))==ERR_NONE)
								StreamPrintf(File,T("%s%d:%d:%d:%d\n"),s,Rect.x,Rect.y,Rect.Width,Rect.Height);
							break;
						case TYPE_POINT:
							if (p->Get(p,DataDef.No,&Point,sizeof(point))==ERR_NONE)
								StreamPrintf(File,T("%s%d:%d\n"),s,Point.x,Point.y);
							break;
						case TYPE_RGB:
							if (p->Get(p,DataDef.No,&RGB.v,sizeof(RGB.v))==ERR_NONE)
								StreamPrintf(File,T("%s0x%082%082%082\n"),s,RGB.c.r,RGB.c.b,RGB.c.b);
							break;
						case TYPE_COMMENT:
							if (p->Get(p,DataDef.No,&Pin,sizeof(Pin))==ERR_NONE)
								DumpPtr(File,s,Pin.Node,Pin.No);
							break;
						case TYPE_PACKET:
							if (p->Get(p,DataDef.No|PIN_FORMAT,&Format,sizeof(Format))==ERR_NONE)
								DumpFormat(File,s,&Format);
							if (p->Get(p,DataDef.No,&Pin,sizeof(Pin))==ERR_NONE)
								DumpPtr(File,s,Pin.Node,Pin.No);
							break;
						case TYPE_NODE:
							if (p->Get(p,DataDef.No,&Ptr,sizeof(Ptr)) == ERR_NONE)
								DumpPtr(File,s,Ptr,0);
							break;
						}
					}
					TRY_END
				}
			}
			TRY_END

	StreamPrintf(File,T("\n\n"));

	for (k=ARRAYBEGIN(p->NodeClass,nodeclass*);k!=ARRAYEND(p->NodeClass,nodeclass*);++k)
		TRY_BEGIN 
		{
			tchar_t s[16],s2[16];
			int Class = (*k)->Def.Class;
			FourCCToString(s,TSIZEOF(s),Class);
			FourCCToString(s2,TSIZEOF(s2),(*k)->Def.ParentClass);
			StreamPrintf(File,T("%s %s p:%d r:%d "),s,s2,(*k)->Parent!=NULL,(*k)->Registered);
			StreamPrintf(File,T("c:%s e:%s p:%s m:%s\n"),
				LangStrDef(Class,NODE_CONTENTTYPE),
				LangStrDef(Class,NODE_EXTS),
				LangStrDef(Class,NODE_PROBE),
				LangStrDef(MODULE_PATH,(*k)->ModuleNo));
		}
		TRY_END
}

int NodeEnumTable(int* No, datadef* Out, const datatable* p)
{
	int Count;
	for (Count=0;p[Count].Type>=0;++Count);

	if (*No < Count)
	{
		Out->Class = p[Count].No;
		p += *No;
		Out->No = p->No;
		Out->Type = p->Type;
		Out->Flags = p->Flags;
		Out->Format1 = p->Format1;
		Out->Format2 = p->Format2;

		switch (Out->Type)
		{
		case TYPE_BOOL: Out->Size = sizeof(bool_t); break;
		case TYPE_INT: Out->Size = sizeof(int); break;
		case TYPE_HOTKEY: Out->Size = sizeof(int); break;
		case TYPE_FRACTION: Out->Size = sizeof(fraction); break;
		case TYPE_STRING: Out->Size = MAXDATA; break;
		case TYPE_RECT: Out->Size = sizeof(rect); break;
		case TYPE_POINT: Out->Size = sizeof(point); break;
		case TYPE_RGB: Out->Size = sizeof(rgbval_t); break;
		case TYPE_BINARY: Out->Size = Out->Format1; break;
		case TYPE_NODE: Out->Size = sizeof(node*); break;
		case TYPE_NOTIFY: Out->Size = sizeof(notify); break;
		case TYPE_COMMENT:
		case TYPE_PACKET: Out->Size = sizeof(pin); break;
		case TYPE_TICK: Out->Size = sizeof(tick_t); break;
		case TYPE_BLITFX: Out->Size = sizeof(blitfx); break;
		default:
			Out->Size = 0;
			break;
		}
		Out->Name = LangStr(Out->Class,Out->No);
		return ERR_NONE;
	}

	*No -= Count;
	return ERR_INVALID_PARAM;
}

bool_t NodeDataDef(node* p, int No, datadef* Out)
{
	int i;
	for (i=0;NodeEnum(p,i,Out)==ERR_NONE;++i)
		if (Out->No == No)
			return 1;
	return 0;
}

int NodeEnum(node* p,int No,datadef* Param)
{
	if (!p) return ERR_INVALID_PARAM;
	return p->Enum(p,&No,Param);
}

int NodeEnumClass(array* List, int Class)
{
	return NodeEnumClassEx(List,Class,NULL,NULL,NULL,0);
}

int NodeEnumClassEx(array* List, int Class, const tchar_t* ContentType, const tchar_t* URL, const void* Data, int Length)
{
	context* p = Context();
	nodeclass* v;
	nodeclass **i;

	if (ContentType && !ContentType[0])
		ContentType = NULL;

	if (URL && !URL[0])
		URL = NULL;

	if (List)
		memset(List,0,sizeof(array));

	LockEnter(p->NodeLock);
	for (i=ARRAYBEGIN(p->NodeClassPri,nodeclass*);i!=ARRAYEND(p->NodeClassPri,nodeclass*);++i)
		if (!((*i)->Def.Flags & CF_ABSTRACT) && !TmpModule(p,(*i)->ModuleNo)) // skip abstract
			for (v=*i;v;v=v->Parent)
				if (v->Def.Class == Class)
				{
					int Id = (*i)->Def.Class;

					if ((ContentType || URL || Data) &&
						(!ContentType || !CheckContentType(ContentType,LangStrDef(Id,NODE_CONTENTTYPE))) &&
						(!URL || !CheckExts(URL,LangStrDef(Id,NODE_EXTS))) &&
						(!Data || !DataProbe(Data,Length,LangStrDef(Id,NODE_PROBE))))
						continue;

					if (!List)
					{
						LockLeave(p->NodeLock);
						return Id;
					}
					ArrayAppend(List,&Id,sizeof(int),64);
					break;
				}

	LockLeave(p->NodeLock);
	return 0;
}

static int Enum(void* This,int* EnumNo,datadef* Out) {	return ERR_INVALID_PARAM; }
static int Get(void* This,int No,void* Data,int Size) { return ERR_INVALID_PARAM; }
static int Set(void* This,int No,const void* Data,int Size) { return ERR_INVALID_PARAM; }

static int Create(node* p)
{
	p->Enum = Enum;
	p->Get = Get;
	p->Set = Set;
	return ERR_NONE;
}

static const nodedef Node =
{
	CF_ABSTRACT,
	NODE_CLASS,
	0,
	PRI_DEFAULT,
	(nodecreate)Create
};

void Node_Init()
{
	context* p = Context();
	ArrayAlloc(&p->NodeClass,sizeof(nodeclass*)*100,128);
	ArrayAlloc(&p->NodeClassPri,sizeof(nodeclass*)*100,128);
	p->NodeLock = LockCreate();

	NodeAddModule(T("common"),0,0,0,0);
	if (!ARRAYEMPTY(p->NodeModule))
		*(void(**)())&ARRAYBEGIN(p->NodeModule,nodemodule)->Func = Node_Init;
	NodeRegisterClass(&Node);
}

void Node_Done()
{
	context* p = Context();
	nodeclass **i;
	NodeUnRegisterClass(NODE_CLASS);
#ifndef NDEBUG
	{
		nodemodule* j;
		for (j=ARRAYBEGIN(p->NodeModule,nodemodule);j!=ARRAYEND(p->NodeModule,nodemodule);++j)
		{
			if (j->ObjectCount!=0)
				DebugMessage(T("module problem %s"),LangStr(MODULE_PATH,j-ARRAYBEGIN(p->NodeModule,nodemodule)));
			assert(j->ObjectCount==0);
		}
	}
#endif
	ReleaseModules(p,1);
	for (i=ARRAYBEGIN(p->NodeClass,nodeclass*);i!=ARRAYEND(p->NodeClass,nodeclass*);++i)
		free(*i);
	ArrayClear(&p->Node);
	ArrayClear(&p->NodeClass);
	ArrayClear(&p->NodeClassPri);
	ArrayClear(&p->NodeModule);
	LockDelete(p->NodeLock);
	p->NodeLock = NULL;
}

bool_t NodeLocatePtr(void* Ptr, tchar_t* OutName, int OutLen, int* OutBase)
{
	if (Ptr)
	{
		context* p = Context();
		if (p) 
		{
			int No;
			uint8_t* v = (uint8_t*)Ptr;
			nodemodule *i;
			// no locking (dump after crash)
			for (No=0,i=ARRAYBEGIN(p->NodeModule,nodemodule);i!=ARRAYEND(p->NodeModule,nodemodule);++i,++No)
			{
				if (i->Func)
				{
					if (!i->Min)
						CodeFindPages(i->Func,&i->Min,&i->Max,NULL);

					if (i->Min <= v && i->Max > v)
					{
						tcscpy_s(OutName,OutLen,LangStrDef(MODULE_PATH,No));
						*OutBase = v - i->Min;
						return 1;
					}
				}
			}
		}
	}
	return 0;
}

#ifndef REGISTRY_GLOBAL

bool_t NodeRegLoadString(int Class, int Id)
{
	uint8_t Buffer[MAXDATA];
	if (NodeRegLoadValue(Class,Id,Buffer,sizeof(Buffer),TYPE_STRING))
	{
		StringAdd(1,Class,Id,(tchar_t*)Buffer);
		return 1;
	}
	return 0;
}

void NodeRegSaveString(int Class, int Id)
{
	const tchar_t* s = LangStrDef(Class,Id);
	int Len = tcslen(s);
	if (Len)
		++Len;
	else 
		s = NULL;
	NodeRegSaveValue(Class,Id,s,Len*sizeof(tchar_t),TYPE_STRING);
}

#endif
