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
 * $Id: id3tag.c 620 2006-01-31 14:46:34Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "common.h"
#undef ZLIB_DLL
#include "zlib/zlib.h"

#define ENCODE_STR			0x00
#define ENCODE_WCS_LE		0x01
#define ENCODE_WCS_BE		0x02
#define ENCODE_UTF8			0x03

#define FLAG_UNSYNC			0x80
#define FLAG_EXTENDED		0x40
#define FLAG_FOOTER			0x10

#define FLAG4_GROUPID		0x40
#define FLAG4_COMPRESSION	0x08
#define FLAG4_UNSYNC		0x02
#define FLAG4_DATALENGTH	0x01
#define FLAG4_UNKNOWN		0xD4

#define FLAG3_GROUPID		0x20
#define FLAG3_COMPRESSION	0x80
#define FLAG3_UNKNOWN		0x5F

#define FIELD_STRING		1
#define FIELD_COMMENT		2
#define FIELD_APIC			3

static const int FrameInfo[] = // {ID3Id, CommentId, Type}
{
	// ID3v2.3/4
	FOURCC('T','I','T','2'), COMMENT_TITLE,		FIELD_STRING,
	FOURCC('T','P','E','1'), COMMENT_ARTIST,	FIELD_STRING,
	FOURCC('T','A','L','B'), COMMENT_ALBUM,		FIELD_STRING,
	FOURCC('T','R','C','K'), COMMENT_TRACK,		FIELD_STRING,
	FOURCC('T','Y','E','R'), COMMENT_YEAR,		FIELD_STRING,
	FOURCC('T','C','O','N'), COMMENT_GENRE,		FIELD_STRING,
	FOURCC('C','O','M','M'), COMMENT_COMMENT,	FIELD_COMMENT,
	FOURCC('A','P','I','C'), COMMENT_COVER,		FIELD_APIC,

	// ID3v2.2
	FOURCC('T','T','2',' '), COMMENT_TITLE,		FIELD_STRING,
	FOURCC('T','P','1',' '), COMMENT_ARTIST,	FIELD_STRING,
	FOURCC('T','A','L',' '), COMMENT_ALBUM,		FIELD_STRING,
	FOURCC('T','R','K',' '), COMMENT_TRACK,		FIELD_STRING,
	FOURCC('T','Y','E',' '), COMMENT_YEAR,		FIELD_STRING,
	FOURCC('T','C','O',' '), COMMENT_GENRE,		FIELD_STRING,
	FOURCC('C','O','M',' '), COMMENT_COMMENT,	FIELD_COMMENT,
	FOURCC('P','I','C',' '), COMMENT_COVER,		FIELD_APIC,

	0,0,
};

#define GENRE_COUNT	148

static const char* const GenreName[GENRE_COUNT] = 
{
	"Blues",
	"Classic Rock",
	"Country",
    "Dance",
    "Disco",
    "Funk",
    "Grunge",
    "Hip-Hop",
    "Jazz",
    "Metal",
    "New Age",
    "Oldies",
    "Other",
    "Pop",
    "R&B",
    "Rap",
    "Reggae",
    "Rock",
    "Techno",
    "Industrial",
    "Alternative",
    "Ska",
    "Death Metal",
    "Pranks",
    "Soundtrack",
    "Euro-Techno",
    "Ambient",
    "Trip-Hop",
    "Vocal",
    "Jazz+Funk",
    "Fusion",
    "Trance",
    "Classical",
    "Instrumental",
    "Acid",
    "House",
    "Game",
    "Sound Clip",
    "Gospel",
    "Noise",
    "AlternRock",
    "Bass",
    "Soul",
    "Punk",
    "Space",
    "Meditative",
    "Instrumental Pop",
    "Instrumental Rock",
    "Ethnic",
    "Gothic",
    "Darkwave",
    "Techno-Industrial",
    "Electronic",
    "Pop-Folk",
    "Eurodance",
    "Dream",
    "Southern Rock",
    "Comedy",
    "Cult",
    "Gangsta",
    "Top 40",
    "Christian Rap",
    "Pop/Funk",
    "Jungle",
    "Native American",
    "Cabaret",
    "New Wave",
    "Psychedelic",
    "Rave",
    "Showtunes",
    "Trailer",
    "Lo-Fi",
    "Tribal",
    "Acid Punk",
    "Acid Jazz",
    "Polka",
    "Retro",
    "Musical",
    "Rock & Roll",
    "Hard Rock",
    "Folk",
    "Folk-Rock",
    "National Folk",
    "Swing",
    "Fast Fusion",
    "Bebob",
    "Latin",
    "Revival",
    "Celtic",
    "Bluegrass",
    "Avantgarde",
    "Gothic Rock",
    "Progressive Rock",
    "Psychedelic Rock",
    "Symphonic Rock",
    "Slow Rock",
    "Big Band",
    "Chorus",
    "Easy Listening",
    "Acoustic",
    "Humour",
    "Speech",
    "Chanson",
    "Opera",
    "Chamber Music",
    "Sonata",
    "Symphony",
    "Booty Bass",
    "Primus",
    "Porn Groove",
    "Satire",
    "Slow Jam",
    "Club",
    "Tango",
    "Samba",
    "Folklore",
    "Ballad",
    "Power Ballad",
    "Rhythmic Soul",
    "Freestyle",
    "Duet",
    "Punk Rock",
    "Drum Solo",
    "A Capella",
    "Euro-House",
    "Dance Hall",
    "Goa",
    "Drum & Bass",
    "Club-House",
    "Hardcore",
    "Terror",
    "Indie",
    "BritPop",
    "Negerpunk",
    "Polsk Punk",
    "Beat",
    "Christian Gangsta Rap",
    "Heavy Metal",
    "Black Metal",
    "Crossover",
    "Contemporary Christian",
    "Christian Rock",
    "Merengue",
    "Salsa",
    "Thrash Metal",
    "Anime",
    "JPop",
    "Synthpop",
};

typedef struct id3v1
{
	char Head[3];
	char Title[30];
	char Artist[30];
	char Album[30];
	char Year[4];
	char Comment[28];
	uint8_t Null;
	uint8_t Track;
	uint8_t Genre;

} id3v1;

static NOINLINE void GetFrameName(int Id,tchar_t* Out,int OutLen)
{
	tcscpy_s(Out,OutLen,PlayerComment(Id));
	tcscat_s(Out,OutLen,T("="));
}

static NOINLINE void AddFieldStr(pin* Pin,int Id,const tchar_t* Value);
static NOINLINE void AddGenre(pin* Pin,int No)
{
	tchar_t Genre[32];
	StrToTcs(Genre,TSIZEOF(Genre),GenreName[No]);
	AddFieldStr(Pin,COMMENT_GENRE,Genre);
}

static NOINLINE void AddField(pin* Pin,int Id,tchar_t* Value)
{
	tchar_t* p;
	if (Id == COMMENT_GENRE)
	{
		int Genre;
		p = tcschr(Value,'=');
		if (p && stscanf(p,T("=(%d)"),&Genre)==1 && Genre<GENRE_COUNT)
		{
			tchar_t* q=++p;
			while (*q && *q!=')') ++q;
			if (q[0]==')' && q[1]) // custom name
				while ((*(p++)=*(++q))!=0);
			else
			{
				AddGenre(Pin,Genre);
				return;
			}
		}
	}
	if (Id != COMMENT_COMMENT)
		for (p=Value;*p;++p)
			if (*p==10) *p=' ';
	Pin->Node->Set(Pin->Node,Pin->No,Value,(tcslen(Value)+1)*sizeof(tchar_t));
}

static NOINLINE int ReadWcs(const uint16_t* Value,int Len,bool_t Swap,tchar_t* Out,int OutLen)
{
	uint16_t* Src;
	int i;

	if (Out)
		Out[0] = 0;

	for (i=0;i<Len;++i)
		if (LOAD16(Value+i)==0)
		{
			Len=i;
			break;
		}

	if (Len)
	{
		Src = malloc((Len+1)*sizeof(uint16_t));
		if (Src)
		{
			if (Swap)
				for (i=0;i<Len;++i)
					Src[i] = LOAD16SW(Value+i);
			else
				memcpy(Src,Value,Len*sizeof(uint16_t));

			for (i=Len;i>0 && IsSpace(Src[i-1]);--i);
			Src[i] = 0;

			if (Out)
				WcsToTcs(Out,OutLen,Src);
		}
		free(Src);
	}
	return (Len+1)*sizeof(uint16_t);
}

static NOINLINE int ReadStr(const char* Value,int Len,bool_t UTF8,tchar_t* Out,int OutLen)
{
	char* Src;
	int i;

	if (Out)
		Out[0] = 0;

	if (Len<0) 
		Len=strlen(Value);
	else
		for (i=0;i<Len;++i)
			if (Value[i]==0)
			{
				Len=i;
				break;
			}

	if (Len)
	{
		Src = malloc(Len+1);
		if (Src)
		{
			memcpy(Src,Value,Len);

			for (i=Len;i>0 && IsSpace(Src[i-1]);--i);
			Src[i] = 0;

			if (Out)
			{
				if (UTF8)
					UTF8ToTcs(Out,OutLen,Src);
				else
					StrToTcs(Out,OutLen,Src);
			}
		}
		free(Src);
	}
	return Len+1;
}

static NOINLINE void AddFieldInt(pin* Pin,int Id,int Value)
{
	tchar_t s[40];
	GetFrameName(Id,s,TSIZEOF(s));
	IntToString(s+tcslen(s),TSIZEOF(s)-tcslen(s),Value,0);
	AddField(Pin,Id,s);
}

static NOINLINE void AddFieldStr(pin* Pin,int Id,const tchar_t* Value)
{
	if (Value[0])
	{
		tchar_t s[512];
		GetFrameName(Id,s,TSIZEOF(s));
		tcscat_s(s,TSIZEOF(s),Value);
		AddField(Pin,Id,s);
	}
}

static NOINLINE void AddFieldAttachment(pin* Pin,int Id,filepos_t Pos,int Size,const tchar_t* ContentType)
{
	if (Size>0)
	{
		tchar_t s[512];
		GetFrameName(Id,s,TSIZEOF(s));
		stcatprintf_s(s,TSIZEOF(s),T(":%d:%d:%s"),(int)Pos,Size,ContentType);
		AddField(Pin,Id,s);
	}
}

static NOINLINE int ReadStrEncode(const void* Value,int Len,int Encode,tchar_t* Out,int OutLen)
{
	switch (Encode)
	{
	case ENCODE_STR: return ReadStr(Value,Len,0,Out,OutLen);
	case ENCODE_UTF8: return ReadStr(Value,Len,1,Out,OutLen);
#ifdef IS_BIG_ENDIAN
	case ENCODE_WCS_LE: return ReadWcs(Value,Len,1,Out,OutLen);
	case ENCODE_WCS_BE: return ReadWcs(Value,Len,0,Out,OutLen);
#else
	case ENCODE_WCS_BE: return ReadWcs(Value,Len,1,Out,OutLen);
	case ENCODE_WCS_LE: return ReadWcs(Value,Len,0,Out,OutLen);
#endif
	}
	return 0;
}
 
void ID3v1_Parse(const void* Ptr, pin* Pin)
{
	tchar_t Value[128];
	id3v1 p;
	memcpy(&p,Ptr,sizeof(id3v1));

	if (!Pin ||p.Head[0] != 'T' || p.Head[1] != 'A' || p.Head[2] != 'G') 
		return;

	ReadStr(p.Title,30,0,Value,TSIZEOF(Value));
	AddFieldStr(Pin,COMMENT_TITLE,Value);

	ReadStr(p.Artist,30,0,Value,TSIZEOF(Value));
	AddFieldStr(Pin,COMMENT_ARTIST,Value);

	ReadStr(p.Album,30,0,Value,TSIZEOF(Value));
	AddFieldStr(Pin,COMMENT_ALBUM,Value);

	ReadStr(p.Year,4,0,Value,TSIZEOF(Value));
	AddFieldStr(Pin,COMMENT_YEAR,Value);

	if (p.Genre < GENRE_COUNT)
		AddGenre(Pin,p.Genre);

    if (p.Null==0 && p.Track!=0) 
	{
		ReadStr(p.Comment,28,0,Value,TSIZEOF(Value));
		AddFieldStr(Pin,COMMENT_COMMENT,Value);

		AddFieldInt(Pin,COMMENT_TRACK,p.Track);
	}
	else
	{
		ReadStr(p.Comment,30,0,Value,TSIZEOF(Value));
		AddFieldStr(Pin,COMMENT_COMMENT,Value);
	}
}

bool_t ID3v1_Genre(int No, tchar_t* Genre, size_t GenreLen)
{
	if (No>=0 && No<GENRE_COUNT)
	{
		AsciiToTcs(Genre,GenreLen,GenreName[No]);
		return 1;
	}
	return 0;
}

static NOINLINE int Read7Bit(const uint8_t* p,int n)
{
	int i,v = 0;
	for (i=0;i<n;++i)
	{
		v <<= 7;
		v |= p[i] & 127;
	}
	return v;
}

static NOINLINE int Read8Bit(const uint8_t* p,int n)
{
	int i,v = 0;
	for (i=0;i<n;++i)
	{
		v <<= 8;
		v |= p[i];
	}
	return v;
}

static NOINLINE int UnSync(uint8_t* dst,const uint8_t* p,int n)
{
	uint8_t* dst0 = dst;
	const uint8_t* pe = p+n;
	for (;p!=pe;++p,++dst)
	{
		*dst = *p;
		if (p+1!=pe && p[0] == 0xFF && p[1] == 0x00)
			++p;
	}
	return dst-dst0;
}

size_t ID3v2_Query(const void* Ptr,size_t Len)
{
	const uint8_t* Data = (const uint8_t*)Ptr;
	if (Len<10 || Data[0] != 'I' || Data[1] != 'D' || Data[2] != '3' ||
		Data[3] == 0xFF || Data[4] == 0xFF || 
		Data[6] >= 0x80 || Data[7] >= 0x80 ||
		Data[8] >= 0x80 || Data[9] >= 0x80)
		return 0;

	Len = Read7Bit(Data+6,4)+10;
    if (Data[3] >= 4 && (Data[5] & FLAG_FOOTER))
		Len += 10;

	return Len;
}

void ID3v2_Parse(const void* Ptr,size_t Len,pin* Pin,filepos_t Offset)
{
	const uint8_t* Data = (const uint8_t*)Ptr;
	size_t Size = ID3v2_Query(Ptr,Len);
	if (Size>0 && Pin)
	{
		int HeadFlag = Data[5];
		int Ver = Data[3];
		if (Ver >= 2 && Ver <= 4 && Size<=Len)
		{
			uint8_t* Tmp = NULL;

			// jump after header
			Data += 10;
			Size -= 10;
			
			if (Ver<4 && (HeadFlag & FLAG_UNSYNC))
			{
				// undo unsync coding
				Tmp = malloc(Size);
				if (Tmp)
				{
					Size = UnSync(Tmp,Data,Size);
					Data = Tmp;
				}
				else
					return;
			}

			if ((HeadFlag & FLAG_EXTENDED) && Size>=4)
			{
				// skip extended header
				size_t n = (Ver>=4)?Read7Bit(Data,4):Read8Bit(Data,4);
				Data += 4+n;
				Size -= 4+n;
			}

			while (Size > 0 && Data[0])
			{
				// parse frame
				uint8_t* Tmp = NULL;
				uint8_t* Tmp2 = NULL;
				const uint8_t* p;

				bool_t NeedDecompress = 0;
				bool_t NeedUnSync = 0;
				int Id = 0;
				int Len = 0;
				int Len2 = 0;
				int Flag = 0;
				const int* Info;

				switch (Ver)
				{
				case 2:
				    if (Size >= 6)
					{
						Id = FOURCC(Data[0],Data[1],Data[2],' ');
						Len = Read8Bit(Data+3,3);
						Data += 6;
						Size -= 6;
					}
					break;
				case 3:
				    if (Size >= 10)
					{
						Id = FOURCC(Data[0],Data[1],Data[2],Data[3]);
						Len = Read8Bit(Data+4,4);
						Flag = Read8Bit(Data+8,2);
						Data += 10;
						Size -= 10;

						if (Flag & FLAG3_UNKNOWN)
							Id = 0;

						if ((Flag & FLAG3_COMPRESSION) && Size>=4)
						{
							NeedDecompress = 1;
							Len2 = Read8Bit(Data,4);
							Data += 4;
							Size -= 4;
						}

						if ((Flag & FLAG3_GROUPID) && Size>=1)
						{
							Data++;
							Size--;
						}
					}
					break;
				case 4:
				    if (Size >= 10)
					{
						Id = FOURCC(Data[0],Data[1],Data[2],Data[3]);
						Len = Read8Bit(Data+4,4);
						Flag = Read8Bit(Data+8,2);
						Data += 10;
						Size -= 10;

						if (Flag & FLAG4_UNKNOWN)
							Id = 0;
						if ((Flag & FLAG4_GROUPID) && Size>=1)
						{
							Data++;
							Size--;
						}
						if ((Flag & FLAG4_DATALENGTH) && Size>=4)
						{
							Len2 = Read8Bit(Data,4);
							Data += 4;
							Size -= 4;
						}
						if (Flag & FLAG4_COMPRESSION)
							NeedDecompress = 1;

						if (Flag & FLAG4_UNSYNC)
							NeedUnSync = 1;
					}
					break;
				}

				p = Data;
				Data += Len;
				Size -= Len;

				if (Id && Size>=0)
					for (Info=FrameInfo;Info[0];Info+=3)
						if (Info[0]==Id)
						{
							int n;
							tchar_t Value[512];

							if (NeedUnSync)
							{
								Tmp = malloc(Len);
								if (!Tmp)
									break;
								Len = UnSync(Tmp,p,Len);
								p = Tmp;
							}

							if (NeedDecompress && Len2>0)
							{
								unsigned long n = Len2;
								Tmp2 = malloc(Len2);
								if (!Tmp2 || uncompress(Tmp2,&n,p,Len)!=Z_OK)
									break;
								Len = n;
								p = Tmp2;
							}

							switch (Info[2])
							{
							case FIELD_STRING:
								if (Len>1)
								{
									ReadStrEncode(p+1,Len-1,p[0],Value,TSIZEOF(Value));
									AddFieldStr(Pin,Info[1],Value);
								}
								break;
							case FIELD_COMMENT:
								if (Len>5)
									for (n=4;n<Len;++n)
										if (p[n]==0)
										{
											ReadStrEncode(p+n+1,Len-n-1,p[0],Value,TSIZEOF(Value));
											AddFieldStr(Pin,Info[1],Value);
											break;
										}
								break;
							case FIELD_APIC:
								if (Len>5)
								{
									int Encode = *(p++);
									--Len;

									if (Ver==2)
									{
										tcscpy_s(Value,TSIZEOF(Value),T("image/"));
										GetAsciiToken(Value+tcslen(Value),TSIZEOF(Value)-tcslen(Value),(const char*)p,3);
										Len -= 3;
										p += 3;

										if (tcsicmp(Value,T("image/jpg"))==0)
											tcscpy_s(Value,TSIZEOF(Value),T("image/jpeg"));
									}
									else
									{
										n = ReadStrEncode(p,Len,Encode,Value,TSIZEOF(Value));
										Len -= n;
										p += n;
									}
								
									++p;
									--Len;

									n = ReadStrEncode(p,Len,Encode,NULL,0); // skip description
									Len -= n;
									p += n;

									AddFieldAttachment(Pin,COMMENT_COVER,(p-(const uint8_t*)Ptr)+Offset,Len,Value);
								}
								break;
							}
							break;
						}

				free(Tmp);
				free(Tmp2);
			}

			free(Tmp);
		}
	}
}
