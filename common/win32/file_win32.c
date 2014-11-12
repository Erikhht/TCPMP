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
 * $Id: file_win32.c 543 2006-01-07 22:06:24Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"

#if defined(TARGET_WIN32) || defined(TARGET_WINCE)

#ifndef STRICT
#define STRICT
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifndef ERROR_INVALID_DRIVE_OBJECT
#define ERROR_INVALID_DRIVE_OBJECT		4321L
#endif

#ifndef ERROR_DEVICE_NOT_AVAILABLE
#define ERROR_DEVICE_NOT_AVAILABLE		4319L
#endif

#ifndef ERROR_DEVICE_REMOVED
#define ERROR_DEVICE_REMOVED            1617L
#endif

typedef struct filestream
{
	stream Stream;
	tchar_t URL[MAXPATH];
	HANDLE Handle;
	filepos_t Length;
	filepos_t Pos;
	bool_t Silent;
	bool_t Create;

	HANDLE Find;
	const tchar_t* Exts;
	bool_t ExtFilter;
	WIN32_FIND_DATA FindData;

} filestream;

static int Get(filestream* p, int No, void* Data, int Size)
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

static int Open(filestream* p, const tchar_t* URL, bool_t ReOpen)
{
	if (p->Handle)
		CloseHandle(p->Handle);
	p->Handle = 0;
	p->Length = -1;
	if (!ReOpen)
		p->URL[0] = 0;
	else
		Sleep(200);

	if (URL && URL[0])
	{
		HANDLE Handle;

		Handle = CreateFile(URL,p->Create?GENERIC_WRITE:GENERIC_READ,
			FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,p->Create?CREATE_ALWAYS:OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,NULL);

		if (Handle == INVALID_HANDLE_VALUE)
		{
			if (!ReOpen && !p->Silent)
				ShowError(0,ERR_ID,ERR_FILE_NOT_FOUND,URL);
			return ERR_FILE_NOT_FOUND;
		}

		tcscpy_s(p->URL,TSIZEOF(p->URL),URL);
		p->Handle = Handle;

		p->Length = GetFileSize(Handle,NULL);
		if (ReOpen)
			p->Pos = SetFilePointer(p->Handle,p->Pos,NULL,FILE_BEGIN);
		else
		{
			p->Pos = 0;

#if defined(TARGET_WINCE)

			// wince shortcut handling

			if (p->Length < MAXPATH && !p->Create)
			{
				uint32_t Readed;
				char ShortCut[MAXPATH];
				tchar_t URL[MAXPATH];
				char* ch;

				if (ReadFile(p->Handle,ShortCut,p->Length,&Readed,NULL))
				{
					ShortCut[Readed] = 0;

					for (ch=ShortCut;*ch && *ch!='#';++ch)
						if (!IsDigit(*ch))
							break;

					if (ch[0] == '#' && ch[1]!=':')
					{
						char* Head = ++ch;
						char* Tail;

						if (*ch == '"')
						{
							Head++;
							ch = strchr(ch+1,'"');
							if (ch)
								*(ch++) = 0;
						}

						if (ch)
						{
							Tail = strchr(ch,13);
							if (Tail)
							{
								*Tail = 0;
								ch = Tail+1;
							}
							if (!strchr(ch,13))
							{
								ch = strchr(ch,10);
								if (!ch || !strchr(ch+1,10))
								{
									if (ch) *ch = 0;

									StrToTcs(URL,TSIZEOF(URL),Head);
									return Open(p,URL,0);
								}
							}
						}
					}

					p->Pos = SetFilePointer(p->Handle,0,NULL,FILE_BEGIN);
				}
			}
#endif
		}
	}
	return ERR_NONE;
}

static int Set(filestream* p, int No, const void* Data, int Size)
{
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case STREAM_SILENT: SETVALUE(p->Silent,bool_t,ERR_NONE); break;
	case STREAM_CREATE: SETVALUE(p->Create,bool_t,ERR_NONE); break;
	case STREAM_URL:
		Result = Open(p,(const tchar_t*)Data,0);
		break;
	}
	return Result;
}

static int Read(filestream* p,void* Data,int Size)
{
	uint32_t Readed;
	DWORD Error;

	//Sleep(100); 

	//DEBUG_MSG3(-1,"FileRead: %08x %08x %d",p->Pos,SetFilePointer(p->Handle,0,NULL,FILE_CURRENT),Size);

	if (ReadFile(p->Handle,Data,Size,&Readed,NULL))
	{
		//DEBUG_MSG2(T("READ pos:%d len:%d"),p->Pos,Readed);
		p->Pos += Readed;
		return Readed;
	}

	Error = GetLastError();

	//DEBUG_MSG2(T("READ pos:%d error:%d"),p->Pos,Error);

	if (Error == ERROR_DEVICE_REMOVED || 
		Error == ERROR_DEVICE_NOT_AVAILABLE ||
		Error == ERROR_INVALID_HANDLE || 
		Error == ERROR_INVALID_DRIVE_OBJECT ||
		Error == ERROR_DEV_NOT_EXIST ||
		Error == ERROR_GEN_FAILURE)
		Open(p,p->URL,1);

	return -1;
}

static int ReadBlock(filestream* p,block* Block,int Ofs,int Size)
{
	return Read(p,(char*)(Block->Ptr+Ofs),Size);
}

static int Seek(filestream* p,int Pos,int SeekMode)
{
	int ReTry=3;
	int Result;
	DWORD Error;

	switch (SeekMode)
	{
	default:
	case SEEK_SET: SeekMode = FILE_BEGIN; break;
	case SEEK_CUR: SeekMode = FILE_CURRENT; break;
	case SEEK_END: SeekMode = FILE_END; break;
	}

	do
	{
		//DEBUG_MSG3(-1,"FileSeek: %08x %08x %d",SetFilePointer(p->Handle,0,NULL,FILE_CURRENT),Pos,SeekMode);

		Result = SetFilePointer(p->Handle,Pos,NULL,SeekMode);

		if (Result != -1)
		{
			p->Pos = Result;
			break;
		}

		Error = GetLastError();
		if (Error != ERROR_DEVICE_REMOVED && Error != ERROR_INVALID_HANDLE)
			break;

		Open(p,p->URL,1);
	}
	while (--ReTry>0);

	return Result;
}

static int Write(filestream* p,const void* Data,int Size)
{
	DWORD Written;
	if (WriteFile(p->Handle,Data,Size,&Written,NULL))
	{
		p->Pos += Written;
		return Written;
	}
	return -1;
}

static int EnumDir(filestream* p,const tchar_t* URL,const tchar_t* Exts,bool_t ExtFilter,streamdir* Item)
{
	if (URL)
	{
		DWORD Attrib;
		tchar_t Path[MAXPATH];

		p->Exts = Exts;
		p->ExtFilter = ExtFilter;

		if (p->Find != INVALID_HANDLE_VALUE)
		{
			FindClose(p->Find);
			p->Find = INVALID_HANDLE_VALUE;
		}

		stprintf_s(Path,TSIZEOF(Path),T("%s\\*.*"),URL);

		Attrib = GetFileAttributes(URL);
		if (URL[0] && Attrib == (DWORD)-1)
			return ERR_FILE_NOT_FOUND;

		if (!(Attrib & FILE_ATTRIBUTE_DIRECTORY))
			return ERR_NOT_DIRECTORY;

		p->Find = FindFirstFile(Path, &p->FindData);
	}

	Item->FileName[0] = 0;
	Item->DisplayName[0] = 0;

	while (!Item->FileName[0] && p->Find != INVALID_HANDLE_VALUE)
	{
		if (p->FindData.cFileName[0]!='.') // skip unix/mac hidden files and . .. directory entries
		{
			FILETIME Local;

			tcscpy_s(Item->FileName,TSIZEOF(Item->FileName),p->FindData.cFileName);

			if (FileTimeToLocalFileTime(&p->FindData.ftLastWriteTime,&Local))
				Item->Date = ((int64_t)Local.dwHighDateTime << 32)|Local.dwLowDateTime;
			else
				Item->Date = -1;

			if (p->FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				Item->Size = -1;
			else
			{
				Item->Size = p->FindData.nFileSizeLow;
				Item->Type = CheckExts(Item->FileName,p->Exts);

				if (!Item->Type && p->ExtFilter)
					Item->FileName[0] = 0; // skip
			}
		}
			
		if (!FindNextFile(p->Find,&p->FindData))
		{
			FindClose(p->Find);
			p->Find = INVALID_HANDLE_VALUE;
		}
	}

	if (!Item->FileName[0])
	{
		if (p->Find != INVALID_HANDLE_VALUE)
		{
			FindClose(p->Find);
			p->Find = INVALID_HANDLE_VALUE;
		}
		return ERR_END_OF_FILE;
	}

	return ERR_NONE;
}

static int Create(filestream* p)
{
	p->Stream.Get = (nodeget)Get,
	p->Stream.Set = (nodeset)Set,
	p->Stream.Read = Read;
	p->Stream.ReadBlock = ReadBlock;
	p->Stream.Write = Write;
	p->Stream.Seek = Seek;
	p->Stream.EnumDir = EnumDir;
	p->Find = INVALID_HANDLE_VALUE;
	return ERR_NONE;
}

static void Delete(filestream* p)
{
	Open(p,NULL,0);
	if (p->Find != INVALID_HANDLE_VALUE)
		FindClose(p->Find);
}

static const nodedef File =
{
	sizeof(filestream),
	FILE_ID,
	STREAM_CLASS,
	PRI_MINIMUM,
	(nodecreate)Create,
	(nodedelete)Delete,
};

void File_Init()
{
	NodeRegisterClass(&File);
}

void File_Done()
{
	NodeUnRegisterClass(FILE_ID);
}

stream* FileCreate(const tchar_t* Path)
{
	// create fake stream
	filestream* p = (filestream*)malloc(sizeof(filestream));
	memset(p,0,sizeof(filestream));
	p->Stream.Class = FILE_ID;
	p->Stream.Enum = (nodeenum)StreamEnum;
	Create(p);
	return &p->Stream;
}

void FileRelease(stream* p)
{
	if (p)
	{
		Delete((filestream*)p);
		free(p);
	}
}

bool_t FileExits(const tchar_t* Path)
{
	return GetFileAttributes(Path) != (DWORD)-1;
}

int64_t FileDate(const tchar_t* Path)
{
	int64_t Date = -1;
	HANDLE Find;
	SYSTEMTIME Time;
	WIN32_FIND_DATA FindData;

	Find = FindFirstFile(Path, &FindData);
	if (Find != INVALID_HANDLE_VALUE)
	{
		if (FileTimeToSystemTime(&FindData.ftLastWriteTime,&Time))
			Date = ((((((int64_t)Time.wYear*100+(int64_t)Time.wMonth)*100+(int64_t)Time.wDay)*100+
			       (int64_t)Time.wHour)*100+(int64_t)Time.wMinute)*100+(int64_t)Time.wSecond)*1000+(int64_t)Time.wMilliseconds;
		FindClose(Find);
	}
	return Date;
}

#endif
