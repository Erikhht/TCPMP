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
 * $Id: streams.c 585 2006-01-16 09:48:55Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "common.h"

static const datatable StreamParams[] = 
{
	{ STREAM_URL,			TYPE_STRING },
	{ STREAM_LENGTH,		TYPE_INT, DF_HIDDEN },
	{ STREAM_SILENT,		TYPE_BOOL, DF_HIDDEN },
	{ STREAM_CREATE,		TYPE_BOOL, DF_HIDDEN },
	{ STREAM_CONTENTTYPE,	TYPE_STRING, DF_HIDDEN|DF_RDONLY },
	{ STREAM_PRAGMA_SEND,	TYPE_STRING, DF_HIDDEN|DF_RDONLY },
	{ STREAM_PRAGMA_GET,	TYPE_STRING, DF_HIDDEN|DF_RDONLY },
	{ STREAM_COMMENT,		TYPE_COMMENT, DF_OUTPUT },
	{ STREAM_METAPROCESSOR,	TYPE_INT, DF_HIDDEN },

	DATATABLE_END(STREAM_CLASS)
};

int StreamEnum(void* p, int* No, datadef* Param)
{
	return NodeEnumTable(No,Param,StreamParams);
}

static int DummyRead(void* This,void* Data,int Size) { return -1; }
static int DummyReadBlock(void* This,block* Block,int Ofs,int Size) { return -1; }
static filepos_t DummySeek(void* This,filepos_t Pos,int SeekMode) { return -1; }
static int DummyWrite(void* This,const void* Data,int Size) { return -1; }
static int DummyEnumDir(void* This,const tchar_t* URL,const tchar_t* Exts,
						bool_t ExtFilter,streamdir* Item) { return ERR_FILE_NOT_FOUND; }

static int StreamCreate(stream* p)
{
	p->Enum = StreamEnum;
	p->Read = DummyRead;
	p->ReadBlock = DummyReadBlock;
	p->Write = DummyWrite;
	p->EnumDir = DummyEnumDir;
	p->Seek = DummySeek;
	return ERR_NONE;
}

static const nodedef Stream =
{
	sizeof(stream)|CF_ABSTRACT,
	STREAM_CLASS,
	NODE_CLASS,
	PRI_DEFAULT,
	(nodecreate)StreamCreate,
};

static const datatable StreamProcessParams[] = 
{
	{ STREAMPROCESS_INPUT,		TYPE_NODE, DF_HIDDEN, STREAM_CLASS },

	DATATABLE_END(STREAMPROCESS_CLASS)
};

static int StreamProcessEnum(void* p, int* No, datadef* Param)
{
	if (StreamEnum(p,No,Param)==ERR_NONE)
		return ERR_NONE;
	return NodeEnumTable(No,Param,StreamProcessParams);
}

typedef struct memstream
{
	stream s;
	int Pos;
	const uint8_t* Data;
	int Size;

} memstream;

static int MemRead(memstream* p,void* Data,int Size)
{
	int Pos = p->Pos;
	p->Pos += Size;
	if (p->Pos > p->Size)
	{
		Size -= p->Pos - p->Size;
		p->Pos = p->Size;
	}
	if (Size<0)
		Size=0;
	if (Size>0)
		memcpy(Data,p->Data+Pos,Size);
	return Size;
}

static int MemReadBlock(memstream* p,block* Block,int Ofs,int Size)
{
	int Pos = p->Pos;
	p->Pos += Size;
	if (p->Pos > p->Size)
	{
		Size -= p->Pos - p->Size;
		p->Pos = p->Size;
	}
	if (Size<0)
		Size=0;
	if (Size>0)
		WriteBlock(Block,Ofs,p->Data+Pos,Size);
	return Size;
}

static int MemSeek(memstream* p,int Pos,int SeekMode)
{
	switch (SeekMode)
	{
	default:
	case SEEK_SET: break;
	case SEEK_CUR: Pos += p->Pos; break;
	case SEEK_END: Pos += p->Size; break;
	}
	if (Pos<0)
		Pos=0;
	return p->Pos = Pos;
}

static int MemGet(memstream* p, int No, void* Data, int Size)
{
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case STREAM_LENGTH: GETVALUE(p->Size,int); break;
	}
	return Result;
}

static int MemSet(memstream* p, int No, const void* Data, int Size)
{
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case MEMSTREAM_DATA: 
		p->Data = Data;
		p->Size = Size;
		return ERR_NONE;
	}
	return Result;
}

static int MemCreate(memstream* p)
{
	p->s.Set = (nodeset)MemSet;
	p->s.Get = (nodeget)MemGet;
	p->s.Read = (streamread)MemRead;
	p->s.ReadBlock = (streamreadblock)MemReadBlock;
	p->s.Seek = (streamseek)MemSeek;
	return ERR_NONE;
}

static int StreamProcessCreate(stream* p)
{
	p->Enum = StreamProcessEnum;
	return ERR_NONE;
}

static const nodedef StreamProcess =
{
	sizeof(stream)|CF_ABSTRACT,
	STREAMPROCESS_CLASS,
	MEDIA_CLASS, // can't use STREAM_CLASS, because it can have NODE_EXTS and such...
	PRI_DEFAULT,
	(nodecreate)StreamProcessCreate,
};

static const nodedef MemStream =
{
	sizeof(memstream),
	MEMSTREAM_ID,
	STREAM_CLASS,
	PRI_DEFAULT,
	(nodecreate)MemCreate,
};

void Stream_Init()
{
	NodeRegisterClass(&Stream);
	NodeRegisterClass(&MemStream);
	NodeRegisterClass(&StreamProcess);
}

void Stream_Done()
{
	NodeUnRegisterClass(STREAMPROCESS_CLASS);
	NodeUnRegisterClass(MEMSTREAM_ID);
	NodeUnRegisterClass(STREAM_CLASS);
}

const tchar_t* GetMime(const tchar_t* URL, tchar_t* Mime, int MimeLen, bool_t* HasHost)
{
	const tchar_t* s = tcschr(URL,':');
	if (s && s[1] == '/' && s[2] == '/')
	{
		if (Mime)
			tcsncpy_s(Mime,MimeLen,URL,s-URL);
		if (HasHost)
			*HasHost = tcsnicmp(URL,T("file"),4)!=0 &&
			           tcsnicmp(URL,T("mem"),3)!=0 &&
			           tcsnicmp(URL,T("pose"),4)!=0 &&
			           tcsnicmp(URL,T("vol"),3)!=0 &&
			           tcsnicmp(URL,T("slot"),4)!=0 &&
					   tcsnicmp(URL,T("simu"),4)!=0;
		s += 3;
	}
	else
	{
		if (HasHost)
			*HasHost = 0;
		if (Mime)
			tcscpy_s(Mime,MimeLen,T("file"));
		s = URL;
	}
	return s;
}

stream* GetStream(const tchar_t* URL, bool_t Silent)
{
	tchar_t Mime[MAXPATH];
	stream* Stream;

	GetMime(URL,Mime,TSIZEOF(Mime),NULL);
	Stream = (stream*)NodeCreate(NodeEnumClassEx(NULL,STREAM_CLASS,Mime,NULL,NULL,0));

	if (!Stream && !Silent)
	{
		tcsupr(Mime);
		ShowError(0,ERR_ID,ERR_MIME_NOT_FOUND,Mime);
	}
	return Stream;
}

void StreamPrintfEx(stream* Stream, bool_t UTF8, const tchar_t* Msg,...)
{
	tchar_t s[512];
	va_list Arg;
	va_start(Arg,Msg);
	vstprintf_s(s,TSIZEOF(s),Msg,Arg);
	va_end(Arg);
	StreamText(Stream,s,UTF8);
}

void StreamPrintf(stream* Stream, const tchar_t* Msg,...)
{
	tchar_t s[512];
	va_list Arg;
	va_start(Arg,Msg);
	vstprintf_s(s,TSIZEOF(s),Msg,Arg);
	va_end(Arg);
	StreamText(Stream,s,0);
}

void StreamText(stream* Stream, const tchar_t* Msg, bool_t UTF8)
{
	int i,n = tcslen(Msg)*2+1;
	char* s = malloc(n);
	if (s)
	{
		if (UTF8)
			TcsToUTF8(s,n,Msg);
		else
			TcsToStr(s,n,Msg);

		i = strlen(s);
		
#if defined(TARGET_WINCE) || defined(TARGET_WIN32)
		{
			char* nl = s;
			while (i+1<n && (nl = strchr(nl,10))!=NULL)
			{
				memmove(nl+1,nl,i+1-(nl-s));
				*nl = 13;
				nl += 2;
				++i;
			}
		}
#endif

		Stream->Write(Stream,s,i);
		free(s);
	}
}


extern stream* FileCreate(const tchar_t*);
extern void FileRelease(stream*);

stream* StreamOpen(const tchar_t* Path, bool_t Write)
{
	stream* File = FileCreate(Path);
	if (File)
	{
		bool_t One = 1;
		File->Set(File,STREAM_SILENT,&One,sizeof(One));
		File->Set(File,STREAM_CREATE,&Write,sizeof(Write));

		if (File->Set(File,STREAM_URL,Path,(tcslen(Path)+1)*sizeof(tchar_t)) != ERR_NONE)
		{
			FileRelease(File);
			File = NULL;
		}
	}
	return File;
}

stream* StreamOpenMem(const void* Data,int Length)
{
	memstream* p = (memstream*)malloc(sizeof(memstream));
	if (p)
	{
		memset(p,0,sizeof(memstream));
		p->s.Class = MEMSTREAM_ID;
		MemCreate(p);
		p->s.Set(&p->s,MEMSTREAM_DATA,Data,Length);
	}
	return &p->s;
}

int StreamCloseMem(stream* p)
{
	//MemDelete(p);
	free(p);
	return 0;
}

filepos_t StreamSeek(stream* File, filepos_t Ofs, int Mode)
{
	return File->Seek(File,Ofs,Mode);
}

int StreamRead(stream* File, void* p, int n)
{
	return File->Read(File,p,n);
}

int StreamWrite(stream* File, const void* p, int n)
{
	return File->Write(File,p,n);
}

int StreamClose(stream* File)
{
	File->Set(File,STREAM_URL,NULL,0);
	FileRelease(File);
	return 0;
}

