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
 * $Id: node.h 615 2006-01-26 16:57:51Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __NODE_H
#define __NODE_H

#define MAXDATA				(MAXPATH*sizeof(tchar_t))

//----------------------------------------------------------------
// data types

// bool_t
#define TYPE_BOOL			1
// int
#define TYPE_INT			2
// fraction_t
#define TYPE_FRACTION		3
// null terminated tchar_t[]
#define TYPE_STRING			4
// rect
#define TYPE_RECT			5
// point
#define TYPE_POINT			6
// rgbval_t
#define TYPE_RGB			7
// node* (format1: node class)
#define TYPE_NODE			10
// comment pin 
#define TYPE_COMMENT		11
// flow packet pin 
#define TYPE_PACKET			12
// tick_t
#define TYPE_TICK			13
// blitfx
#define TYPE_BLITFX			14
// virtual key code (int)
#define TYPE_HOTKEY			16
// reset button
#define TYPE_RESET			17
// not value, just label for settings
#define TYPE_LABEL			18
// binary data (format1: size)
#define TYPE_BINARY			19
// notify 
#define TYPE_NOTIFY			20

//----------------------------------------------------------------
// data flags

#define DF_RDONLY			0x00000001
#define DF_SETUP			0x00000002
#define DF_HIDDEN			0x00000004
#define DF_MINMAX			0x00000008
#define DF_INPUT			0x00000020
#define DF_OUTPUT			0x00000040
#define DF_NOSAVE			0x00000080
#define DF_SOFTRESET		0x00000100
#define DF_RESTART			0x00000200
#define DF_CHECKLIST		0x00000400
#define DF_HEX				0x00000800
#define DF_ENUMSTRING		0x00001000
#define DF_PERCENT			0x00002000
#define DF_NOWRAP			0x00004000
#define DF_GAP				0x00008000
#define DF_RESYNC			0x00010000
#define DF_MSEC				0x00020000
#define DF_KBYTE			0x00040000
#define DF_NEG				0x00080000
#define DF_SHORTLABEL		0x00100000
#define DF_MHZ				0x00200000
#define DF_ENUMCLASS		0x00400000
#define DF_ENUMUNSORT		0x00800000

#define PERCENT_ONE			1024

typedef struct datadef
{
	int	No;
	int	Type;
	int Flags;
	int Format1; // DF_MINMAX:min, DF_ENUMSRING:string class, DF_ENUMCLASS:node class, nodeclass
	int	Format2; // DF_MINMAX:max
	const tchar_t* Name;
	int Class;			
	int Size;			

} datadef;

typedef struct datatable
{
	int	No;
	int	Type;
	int Flags;
	int Format1;
	int	Format2;

} datatable;
#define DATATABLE_END(Class) { Class, -1 }

//---------------------------------------------------------------
// node

#define NODE_CLASS			FOURCC('N','O','D','E')
#define NULL_ID				FOURCC('N','U','L','L')

// strings
#define NODE_NAME				0
#define NODE_CONTENTTYPE		1
#define NODE_EXTS				2
#define NODE_PROBE				3

// registry (private)
#define NODE_MODULE_PATH		4
#define NODE_PARENT				5
#define NODE_PRIORITY			6
#define NODE_FLAGS				7
#define NODE_MAX_REGISTRY		8

// refresh settings (it's not thread safe. playback decoding could be runing!)
#define NODE_SETTINGSCHANGED	8
// emergancy release of critical resources
#define NODE_CRASH				9
// release unneccessary memory (void)
#define NODE_HIBERNATE			10
#define NODE_PARTOF				11

// enumerate parameters (EnumNo decremented...)
typedef	int (*nodeenum)(void* This,int* EnumNo,datadef* Out);
// get parameter
typedef	int (*nodeget)(void* This,int No,void* Data,int Size);
// set parameter or send data
typedef	int (*nodeset)(void* This,int No,const void* Data,int Size);
// simple node function
typedef	int (*nodefunc)(void* This);

#define VMT_NODE			\
	int			Class;		\
	nodeenum	Enum;		\
	nodeget		Get;		\
	nodeset		Set;

typedef struct node
{
	int			Class;
	nodeenum	Enum;
	nodeget		Get;
	nodeset		Set;

} node;

typedef struct pin
{
	node* Node;			
	int No;

} pin;


//-----------------------------------------------------------------
// node class definition

#define CF_SIZE			0x00FFFFFF
#define CF_GLOBAL		0x01000000
#define CF_SETTINGS		0x02000000
#define CF_ABSTRACT		0x08000000

// create node (optional)
typedef	int (*nodecreate)(node* This);
// delete node (optional)
typedef	void (*nodedelete)(node* This);

typedef struct nodedef
{
	int				Flags;
	int				Class;
	int				ParentClass;
	int				Priority;
	nodecreate		Create;
	nodedelete		Delete;

} nodedef;

//---------------------------------------------------------------
// node priority classes

#define PRI_MINIMUM			1
#define PRI_DEFAULT		 1000
#define PRI_MAXIMUM		10000

//---------------------------------------------------------------
// hibernate levels

#define HIBERNATE_UNUSED	0
#define HIBERNATE_SOFT		1
#define HIBERNATE_HARD		2

#define HIBERNATEMODES		3

//---------------------------------------------------------------
// functions managing node meta information

void Node_Init();
void Node_Done();

void Static_Init();
void Static_Done();

void Plugins_Init();
void Plugins_Done();

DLL void NodeRegisterClass(const nodedef*);
DLL void NodeUnRegisterClass(int Class);
DLL int NodeEnumClass(array* List, int Class); // List=NULL just returns the first class
DLL int NodeEnumClassEx(array* List, int Class, 
						const tchar_t* ContentType, const tchar_t* URL, const void* Data, int Length);
DLL const nodedef* NodeClassDef(int Class);
DLL bool_t NodeIsClass(int Class, int PartOfClass);

// functions for existing nodes

DLL node* NodeCreate(int Class);
DLL void NodeDelete(node*);
DLL bool_t NodeHibernate();
DLL int NodeSettingsChanged();
DLL int NodeBroadcast(int Msg,const void* Data,int Size);
DLL node* NodeEnumObject(array* List, int Class);

// registry handling

#ifdef REGISTRY_GLOBAL
DLL void NodeRegLoadGlobal();
DLL void NodeRegSaveGlobal();
#else
DLL bool_t NodeRegLoadValue(int Class, int Id, void* Data, int Size, int Type);
DLL void NodeRegSaveValue(int Class, int Id, const void* Data, int Size, int Type);
DLL void NodeRegLoad(node*);
DLL void NodeRegSave(node*);
#endif

// helper funcions

DLL int NodeEnumTable(int* EnumNo, datadef* Out, const datatable*);
DLL bool_t NodeDataDef(node* p, int No, datadef* Out);
DLL int NodeEnum(node* p, int No, datadef* Out);
DLL bool_t NodeLocatePtr(void* Ptr, tchar_t* OutName, int OutLen, int* OutBase);
DLL void NodeDump(struct stream* File);
DLL bool_t NodeRegLoadString(int Class, int Id);
DLL void NodeRegSaveString(int Class, int Id);

// functions which should be exported in node DLLs

DLLEXPORT int DLLRegister(int ContextVersion);
DLLEXPORT void DLLUnRegister();
DLLEXPORT void DLLTest();
DLLEXPORT void DLLTest2();

// internal
void NodeLoadClass(const nodedef* Def,const tchar_t* Module,int ModuleId);
void NodeAddModule(const tchar_t* Path,int Id,int64_t Date,bool_t Load,bool_t Tmp);
bool_t NodeFindModule(const tchar_t* Path,int Id);
void* NodeLoadModule(const tchar_t* Path,int* Id,void** AnyFunc,void** Db);
void NodeFreeModule(void* Module,void* Db);
int NodeGetModuleCount();
const tchar_t* NodeGetModule(int No,int* Id,int64_t* Date);
int NodeGetClassCount();
const nodedef* NodeGetClass(int No,int* Module);

#endif
