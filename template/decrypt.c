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
 * $Id: audiotemplate.c 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common/common.h"
#include "decrypt.h"

#if 0

// header:
// DWORD magic number 'ABCD'
// DWORD length
// DWORD xor start position

typedef struct decrypt
{
	stream Stream;
	bool_t Silent;
	FILE* File;
	int XOR;
	int StartXOR;
	int Length;
	int HeadSize;
	tchar_t URL[MAXPATH];

} decrypt;


static int Get(decrypt* p, int No, void* Data, int Size)
{
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case STREAM_URL: GETSTRING(p->URL); break;
	case STREAM_SILENT: GETVALUE(p->Silent,bool_t); break;
	case STREAM_LENGTH: GETVALUECOND(p->Length,int,p->Length>=0); break;
	}
	return Result;
}

static int Open(decrypt* p, stream* Input)
{
	if (p->File)
	{
		fclose(p->File);
		p->File = NULL;
	}
	p->Length = -1;
	p->URL[0] = 0;

	if (Input)
	{
		char URL[MAXPATH];
		uint32_t Head[3];

		if (Input->Get(Input,STREAM_URL,p->URL,sizeof(p->URL)) != ERR_NONE)
			return ERR_INVALID_DATA;
		
		TcsToStr(URL,sizeof(URL),p->URL);
		p->File = fopen(URL,"rb");
		if (!p->File)
		{
			if (!p->Silent)
				ShowError(0,ERR_ID,ERR_FILE_NOT_FOUND,p->URL);
			return ERR_FILE_NOT_FOUND;
		}

		p->HeadSize = fread(&Head,1,sizeof(Head),p->File);
		if (p->HeadSize != sizeof(Head))
			return ERR_INVALID_DATA;

		p->Length = INT32LE(Head[1]);
		p->StartXOR = INT32LE(Head[2]);

		if (Head[0] != FOURCCLE('A','B','C','D'))
			return ERR_INVALID_DATA;

		p->XOR = p->StartXOR;

		Input->Set(Input,STREAM_URL,NULL,0); // original input can be closed
	}
	return ERR_NONE;
}

static int Set(decrypt* p, int No, const void* Data, int Size)
{
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case STREAM_SILENT: SETVALUE(p->Silent,bool_t,ERR_NONE); break;
	case STREAMPROCESS_INPUT:
		Result = Open(p,*(stream**)Data);
		break;
	}
	return Result;
}

static int Read(decrypt* p,void* Data,int Size)
{
	int Readed = fread(Data,1,Size,p->File);
	if (Readed>0)
	{
		uint8_t* v = (uint8_t*)Data;
		int i;
		for (i=0;i<Readed;++i,++v)
			*v = (uint8_t)(*v ^ p->XOR++);
	}
	return Readed;
}

static int ReadBlock(decrypt* p,block* Block,int Ofs,int Size)
{
	return Read(p,(char*)(Block->Ptr+Ofs),Size);
}

static int Seek(decrypt* p,int Pos,int SeekMode)
{
	int FilePos;

	if (SeekMode == SEEK_SET)
		Pos += p->HeadSize;

	if (fseek(p->File,Pos,SeekMode) != 0)
		return ERR_NOT_SUPPORTED;

	FilePos = ftell(p->File) - p->HeadSize;
	p->XOR = p->StartXOR + FilePos;
	return FilePos;
}

static int Create(decrypt* p)
{
	p->Stream.Get = (nodeget)Get,
	p->Stream.Set = (nodeset)Set,
	p->Stream.Read = Read;
	p->Stream.ReadBlock = ReadBlock;
	p->Stream.Seek = Seek;
	return ERR_NONE;
}

static void Delete(decrypt* p)
{
	if (p->File)
		fclose(p->File);
}

static const nodedef Decrypt =
{
	sizeof(decrypt),
	DECRYPT_ID,
	STREAMPROCESS_CLASS,
	PRI_DEFAULT,
	(nodecreate)Create,
	(nodedelete)Delete,
};

void Decrypt_Init()
{
	StringAdd(1,DECRYPT_ID,NODE_NAME,T("Decypher"));
	StringAdd(1,DECRYPT_ID,NODE_EXTS,T("env:V;ena:A"));
	StringAdd(1,DECRYPT_ID,NODE_PROBE,T("eq(le32,'ABCD')"));
	NodeRegisterClass(&Decrypt);
}

void Decrypt_Done()
{
	NodeUnRegisterClass(DECRYPT_ID);
}
#else
void Decrypt_Init() {}
void Decrypt_Done() {}
#endif
