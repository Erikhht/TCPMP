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
 * $Id: vfs_palmos.c 339 2005-11-15 11:22:45Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"

#if defined(TARGET_PALMOS)

#include "pace.h"

typedef struct vfs
{
	stream Stream;

	tchar_t URL[MAXPATH];
	UInt16 Vol;
	FileRef	File;
	int Length;
	int Pos;
	bool_t Silent;
	bool_t Create;

	UInt32 Iter;
	const tchar_t* Exts;
	bool_t ExtFilter;

} vfs;

static int Get(vfs* p, int No, void* Data, int Size)
{
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case STREAM_URL: GETSTRING(p->URL); break;
	case STREAM_SILENT: GETVALUE(p->Silent,bool_t); break;
	case STREAM_LENGTH: GETVALUECOND(p->Length,int,p->Length>=0); break;
	case STREAM_CREATE: GETVALUE(p->Create,bool_t); break;
	}
	return Result;
}

static int Open(vfs* p, const tchar_t* URL, bool_t ReOpen);

static int Read(vfs* p,void* Data,int Size)
{
	uint32_t Readed = 0;
	Err Error = VFSFileRead(p->File,Size,Data,&Readed);

	if ((Error==errNone && Readed>0) || Error==vfsErrFileEOF)
	{
		p->Pos += Readed;
		return Readed;
	}

	if (Error == vfsErrFileBadRef)
		Open(p,p->URL,1);

	return -1;
}

static int ReadBlock(vfs* p,block* Block,int Ofs,int Size)
{
	uint32_t Readed = 0; 
	Err Error;

	if (IsHeapStorage(Block))
		Error = VFSFileReadData(p->File,Size,(void*)Block->Ptr,Ofs,&Readed);
	else
		Error = VFSFileRead(p->File,Size,(void*)(Block->Ptr+Ofs),&Readed);

	if ((Error==errNone && Readed>0) || Error==vfsErrFileEOF)
	{
		p->Pos += Readed;
		return Readed;
	}

	if (Error == vfsErrFileBadRef)
		Open(p,p->URL,1);

	return -1;
}

static int Seek(vfs* p,int Pos,int SeekMode)
{
	FileOrigin Origin;
	Err Error;

	switch (SeekMode)
	{
	default:
	case SEEK_SET: Origin = vfsOriginBeginning; break;
	case SEEK_CUR: Origin = vfsOriginCurrent; break;
	case SEEK_END: Origin = vfsOriginEnd; break;
	}

	Error = VFSFileSeek(p->File,Origin,Pos);

	if (Error == errNone || Error == vfsErrFileEOF)
	{
		UInt32 Pos;
		if (VFSFileTell(p->File,&Pos) == errNone)
			p->Pos = Pos;
		return p->Pos;
	}

	return -1;
}

static UInt16 FindVol(int Slot)
{
	UInt16 Ref;
	UInt32 Iter = vfsIteratorStart;
	while (Iter != vfsIteratorStop && VFSVolumeEnumerate(&Ref,&Iter)==errNone)
	{
		VolumeInfoType Info;
		VFSVolumeInfo(Ref,&Info);

		if (Slot==0 && Info.mediaType == 'pose')
			return Ref;
 
		if (Slot<0 && Info.mountClass == vfsMountClass_Simulator && Info.slotRefNum==0xFFFF+Slot)
			return Ref;

		if (Slot>0 && Info.mountClass == vfsMountClass_SlotDriver && Info.slotRefNum==Slot)
			return Ref;
	}
	return vfsInvalidVolRef;
}

const tchar_t* VFSToVol(const tchar_t* URL,uint16_t* Vol)
{
	int Slot = 0;
	bool_t Found = 0;
	tchar_t Mime[MAXPATH];
	const tchar_t* Name = GetMime(URL,Mime,TSIZEOF(Mime),NULL);

	if (Name != URL)
		--Name; // need the '/'

	if (tcsncmp(Mime,"pose",4)==0 || stscanf(Mime,"slot%d",&Slot)==1 || stscanf(Mime,"vol%d",&Slot)==1)
		Found = 1;
	else
	if (stscanf(Mime,"simu%d",&Slot)==1)
	{
		Found = 1;
		Slot = -Slot;
	}
		
	if (Found && (*Vol=FindVol(Slot))!=vfsInvalidVolRef)
		return Name;

	return NULL;
}

bool_t VFSFromVol(uint16_t Vol,const tchar_t* Path,tchar_t* URL,int URLLen)
{
	int Slot;
	VolumeInfoType Info;
	VFSVolumeInfo(Vol,&Info);
	Slot = Info.slotRefNum;
	if (Slot > 0xFFF0)
		Slot = 0xFFFF-Slot;

	URL[0] = 0;
	if (!Path) Path = T("/");
	if (Info.mediaType == 'pose')
	{
		stprintf_s(URL,URLLen,T("pose:/%s"),Path);
		return 1;
	}
	if (Info.mountClass == vfsMountClass_Simulator)
	{
		stprintf_s(URL,URLLen,T("simu%d:/%s"),Slot,Path);
		return 1;
	}
	if (Info.mountClass == vfsMountClass_SlotDriver)
	{
		stprintf_s(URL,URLLen,(Info.attributes & vfsVolumeAttrNonRemovable)?T("vol%d:/%s"):T("slot%d:/%s"),Slot,Path);
		return 1;
	}
	return 0;
}

bool_t FileExits(const tchar_t* URL)
{
	// open supports VFS
	const tchar_t* Name;
	UInt16 Vol;
	FileRef	File = 0;

	Name = VFSToVol(URL,&Vol);
	if (!Name)
		return 0;

	VFSFileOpen(Vol,Name,vfsModeRead,&File);
	if (!File)
		return 0;

	VFSFileClose(File);
	return 1;
}

static bool_t InternalOpen(vfs* p,const tchar_t* URL)
{
	const tchar_t* Name;

	if (p->File)
	{
		VFSFileClose(p->File);
		p->File = 0;
	}

	Name = VFSToVol(URL,&p->Vol);
	if (Name)
		VFSFileOpen(p->Vol,Name,vfsModeRead,&p->File);

	if (!p->File)
		return 0;

	tcscpy_s(p->URL,TSIZEOF(p->URL),URL);
	return 1;
}

static int Open(vfs* p, const tchar_t* URL, bool_t ReOpen)
{
	if (p->File)
	{
		VFSFileClose(p->File);
		p->File = 0;
	}

	p->Length = -1;
	if (!ReOpen)
		p->URL[0] = 0;
#ifdef MULTITHREAD
	else
		ThreadSleep(GetTimeFreq()/5);
#endif

	if (URL && URL[0])
	{
		UInt32 Pos = 0;

		if (!InternalOpen(p,URL))
		{
			if (!ReOpen && !p->Silent)
				ShowError(0,ERR_ID,ERR_FILE_NOT_FOUND,URL);
			return ERR_FILE_NOT_FOUND;
		}

		if (ReOpen)
			VFSFileSeek(p->File,vfsOriginBeginning,p->Pos);
		else
		{
			p->Length = -1;
			if (VFSFileSize(p->File,&Pos)==errNone)
				p->Length = Pos;
		}

		if (VFSFileTell(p->File,&Pos) == errNone)
			p->Pos = Pos;
	}
	return ERR_NONE;
}

static int EnumDir(vfs* p,const tchar_t* URL,const tchar_t* Exts,bool_t ExtFilter,streamdir* Item)
{
	tchar_t Path[MAXPATH];
	FileInfoType Info;

	if (URL)
	{
		UInt32 Attrib = 0;

		p->Exts = Exts;
		p->ExtFilter = ExtFilter;

		if (!InternalOpen(p,URL))
			return ERR_FILE_NOT_FOUND;

		VFSFileGetAttributes(p->File,&Attrib);
		
		if (!(Attrib & vfsFileAttrDirectory))
			return ERR_NOT_DIRECTORY;

		p->Iter = vfsIteratorStart;
	}

	Item->DisplayName[0] = 0;

	if (p->File)
	{
		Info.nameP = Item->FileName;
		Info.nameBufLen = sizeof(Item->FileName);

		while (p->Iter != vfsIteratorStop && VFSDirEntryEnumerate(p->File,&p->Iter,&Info)==errNone)
		{
			UInt32 Value;
			FileRef File = 0;

			if (Item->FileName[0]=='.') // skip unix/mac hidden files
				continue;
 
			AbsPath(Path,TSIZEOF(Path),Item->FileName,GetMime(p->URL,NULL,0,NULL));
			
			//currently Date and Size is not needed
			//VFSFileOpen(p->Vol,Path,vfsModeRead,&File);

			Item->Date = -1;
			if (File && VFSFileGetDate(File,vfsFileDateModified,&Value)==errNone)
				Item->Date = Value;

			if (Info.attributes & vfsFileAttrDirectory)
				Item->Size = -1;
			else
			{
				if (File && VFSFileSize(File,&Value)==errNone)
					Item->Size = Value;
				else
					Item->Size = 0;
				Item->Type = CheckExts(Item->FileName,p->Exts);
				if (!Item->Type && p->ExtFilter)
				{
					if (File)
						VFSFileClose(File);
					continue;
				}
			}

			if (File)
				VFSFileClose(File);

			return ERR_NONE;
		}

		VFSFileClose(p->File);
		p->File = 0;
	}

	Item->FileName[0] = 0;
	return ERR_END_OF_FILE;
}

static int Set(vfs* p, int No, const void* Data, int Size)
{
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case STREAM_SILENT: SETVALUE(p->Silent,bool_t,ERR_NONE); break;
	case STREAM_CREATE: SETVALUE(p->Create,bool_t,ERR_NONE); break;
	case STREAM_URL: Result = Open(p,(const tchar_t*)Data,0); break;
	}
	return Result;
}

static int Create(vfs* p)
{
	p->Stream.Get = (nodeget)Get;
	p->Stream.Set = (nodeset)Set;
	p->Stream.Read = (streamread)Read;
	p->Stream.ReadBlock = (streamreadblock)ReadBlock;
	p->Stream.Seek = (streamseek)Seek;
	p->Stream.EnumDir = (streamenumdir)EnumDir;
	return ERR_NONE;
}

static void Delete(vfs* p)
{
	Open(p,NULL,0);
}

static const nodedef VFS = 
{
	sizeof(vfs),
	VFS_ID,
	STREAM_CLASS,
	PRI_MINIMUM,		
	(nodecreate)Create,
	(nodedelete)Delete,
};

void VFS_Init()
{
	UInt32 Version;
	if (QueryPlatform(PLATFORM_MODEL) == MODEL_PALM_SIMULATOR || 
		(FtrGet(sysFileCExpansionMgr, expFtrIDVersion, &Version) == errNone && Version >= 1 &&
	     FtrGet(sysFileCVFSMgr, vfsFtrIDVersion, &Version) == errNone && Version >= 1))
		NodeRegisterClass(&VFS);
}

void VFS_Done()
{
	NodeUnRegisterClass(VFS_ID);
}

#endif

