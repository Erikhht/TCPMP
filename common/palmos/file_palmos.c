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
 * $Id: file_palmos.c 332 2005-11-06 14:31:57Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"

// this is not a real file reader 
// just directory listing for different storage places

#if defined(TARGET_PALMOS)

#include "pace.h"

typedef struct file
{
	stream Stream;
	bool_t FileDb;
	UInt32 Iter;

} file;

static int EnumDir(file* p,const tchar_t* URL,const tchar_t* Exts,bool_t ExtFilter,streamdir* Item)
{
	UInt16 Ref;

	if (URL)
	{
		p->FileDb = 0;
		if (NodeEnumClass(NULL,VFS_ID))
			p->Iter = vfsIteratorStart;
		else
			p->Iter = vfsIteratorStop;
	}

	Item->FileName[0] = 0;
	Item->DisplayName[0] = 0;
	Item->Size = -1;
	Item->Date = -1;

	while (!Item->FileName[0] && p->Iter != vfsIteratorStop && VFSVolumeEnumerate(&Ref,&p->Iter)==errNone)
		VFSFromVol(Ref,NULL,Item->FileName,TSIZEOF(Item->FileName));

	if (!Item->FileName[0] && !p->FileDb)
	{
		p->FileDb = 1;
		tcscpy_s(Item->FileName,TSIZEOF(Item->FileName),T("mem://"));
	}

	return Item->FileName[0] ? ERR_NONE : ERR_END_OF_FILE;
}

static int Create(file* p)
{
	p->Stream.EnumDir = (streamenumdir)EnumDir;
	p->Iter = vfsIteratorStart;
	return ERR_NONE;
}

static const nodedef File = 
{
	sizeof(file),
	FILE_ID,
	STREAM_CLASS,
	PRI_MINIMUM,		
	(nodecreate)Create,
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
	//todo: for benchmark result saving
	return NULL;
}

void FileRelease(stream* p)
{
}

#endif

