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
 * $Id: filedb_palmos.c 332 2005-11-06 14:31:57Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"

#if defined(TARGET_PALMOS)

#include "pace.h"

typedef struct filedb
{
	stream Stream;

	tchar_t URL[MAXPATH];
	FileHand File;
	int Length;
	int Pos;
	bool_t Silent;
	bool_t Create;

	DmSearchStateType SearchState;
	LocalID CurrentDB; 
	const tchar_t* Exts;
	bool_t ExtFilter;

} filedb;

static int Get(filedb* p, int No, void* Data, int Size)
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

static int Read(filedb* p,void* Data,int Size)
{
	Err Error;
	Int32 Readed = FileRead(p->File,Data,1,Size,&Error);

	if (Readed > 0)
		p->Pos += Readed;
	else
	if (Error != fileErrEOF)
		Readed = -1;

	return Readed;
}

static int ReadBlock(filedb* p,block* Block,int Ofs,int Size)
{
	Err Error;
	Int32 Readed;

	if (IsHeapStorage(Block))
		Readed = FileDmRead(p->File,(void*)Block->Ptr,Ofs,1,Size,&Error);
	else
		Readed = FileRead(p->File,(void*)(Block->Ptr+Ofs),1,Size,&Error);

	if (Readed > 0)
		p->Pos += Readed;
	else
	if (Error != fileErrEOF)
		Readed = -1;

	return Readed;
}

static int Seek(filedb* p,int Pos,int SeekMode)
{
	FileOriginEnum Origin;
	Err Error;

	switch (SeekMode)
	{
	default:
	case SEEK_SET: Origin = fileOriginBeginning; break;
	case SEEK_CUR: Origin = fileOriginCurrent; break;
	case SEEK_END: Origin = fileOriginEnd; break;
	}

	Error = FileSeek(p->File,Pos,Origin);

	if (Error == errNone || Error == fileErrEOF)
	{
		p->Pos = FileTell(p->File,NULL,NULL);
		return p->Pos;
	}

	return -1;
}

static int Open(filedb* p, const tchar_t* URL, bool_t ReOpen)
{
	if (p->File)
	{
		FileClose(p->File);
		p->File = NULL;
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
		p->File = FileOpen(0,GetMime(URL,NULL,0,NULL), 0, 0, fileModeReadOnly | fileModeAnyTypeCreator, NULL);
		if (!p->File)
		{
			if (!ReOpen && !p->Silent)
				ShowError(0,ERR_ID,ERR_FILE_NOT_FOUND,URL);
			return ERR_FILE_NOT_FOUND;
		}

		tcscpy_s(p->URL,TSIZEOF(p->URL),URL);

		if (ReOpen)
			FileSeek(p->File,p->Pos,fileOriginBeginning);
		else
		{
			p->Length = -1;
			if (FileSeek(p->File,0,fileOriginEnd) == errNone)
				p->Length = FileTell(p->File,NULL,NULL);
			FileSeek(p->File,0,fileOriginBeginning);
		}

		p->Pos = FileTell(p->File,NULL,NULL);
	}
	return ERR_NONE;
}

bool_t DBFrom(uint16_t Card,uint32_t DB,tchar_t* URL,int URLLen)
{
	char Name[48];
	UInt16 Attr = 0;

	DmDatabaseInfo(Card,DB,Name,&Attr,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
	if (Attr & dmHdrAttrStream)
	{
		stprintf_s(URL,URLLen,T("mem://%s"),Name);
		return 1;
	}
	return 0;
}

static int EnumDir(filedb* p,const tchar_t* URL,const tchar_t* Exts,bool_t ExtFilter,streamdir* Item)
{
	Boolean New = 0;
	UInt16 card; 

	if (URL)
	{
		p->CurrentDB = 0;
		p->Exts = Exts;
		p->ExtFilter = ExtFilter;
		New = 1;
	}

	Item->FileName[0] = 0;
	Item->DisplayName[0] = 0;

	while (DmGetNextDatabaseByTypeCreator(New, &p->SearchState, 0, 0, 0, &card, &p->CurrentDB)==errNone)
	{
		char Name[48];
		UInt16 Attr = 0;
		UInt32 Date = (UInt32)-1;
		UInt32 Size = 0;

		DmDatabaseInfo(card,p->CurrentDB,Name,&Attr,NULL,NULL,&Date,NULL,NULL,NULL,NULL,NULL,NULL);

		if (Attr & dmHdrAttrStream)
		{
			Item->Type = CheckExts(Name,p->Exts);
			if (Item->Type || !p->ExtFilter)
			{
				DmDatabaseSize(card,p->CurrentDB,NULL,NULL,&Size);

				tcscpy_s(Item->FileName,TSIZEOF(Item->FileName),Name);
				Item->Date = Date;
				Item->Size = Size;
				return ERR_NONE;
			}
		}

		New = 0;
	}

	return ERR_END_OF_FILE;
}


static int Set(filedb* p, int No, const void* Data, int Size)
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

static int Create(filedb* p)
{
	p->Stream.Get = (nodeget)Get;
	p->Stream.Set = (nodeset)Set;
	p->Stream.Read = (streamread)Read;
	p->Stream.ReadBlock = (streamreadblock)ReadBlock;
	p->Stream.Seek = (streamseek)Seek;
	p->Stream.EnumDir = (streamenumdir)EnumDir;
	return ERR_NONE;
}

static void Delete(filedb* p)
{
	Open(p,NULL,0);
}

static const nodedef FileDb =
{
	sizeof(filedb),
	FILEDB_ID,
	STREAM_CLASS,
	PRI_MINIMUM,		
	(nodecreate)Create,
	(nodedelete)Delete,
};

void FileDb_Init()
{
	NodeRegisterClass(&FileDb);
}

void FileDb_Done()
{
	NodeUnRegisterClass(FILEDB_ID);
}

#endif

