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
 * $Id: http_win32.c 615 2006-01-26 16:57:51Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common/common.h"
#include "http.h"

//#define DUMPHTTP

#define TIMEOUT_CONNECT		30
#define TIMEOUT_READ		180

#if defined(TARGET_WIN32) || defined(TARGET_WINCE)

#ifndef STRICT
#define STRICT
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#if _MSC_VER > 1000
#pragma warning(disable : 4201)
#endif
#include <wininet.h>

#define MAXPRAGMA		512
#define SEEK_READ		65536

typedef HANDLE HINTERNET;
#define HTTP_QUERY_CONTENT_TYPE			1
#define HTTP_QUERY_CONTENT_LENGTH		5
#define HTTP_QUERY_STATUS_CODE			19	
#define HTTP_QUERY_STATUS_TEXT			20
#define HTTP_QUERY_RAW_HEADERS          21
#define HTTP_QUERY_RAW_HEADERS_CRLF     22
#define HTTP_QUERY_ACCEPT_RANGES        42
#define HTTP_QUERY_PRAGMA               17
#define HTTP_QUERY_CUSTOM               65535

#define INTERNET_OPEN_TYPE_DIRECT		1
#define INTERNET_FLAG_NO_CACHE_WRITE    0x04000000
#define INTERNET_OPTION_CONNECT_TIMEOUT 2

typedef struct http
{
	stream Stream;
	HINTERNET Internet;
	HINTERNET Handle;
	filepos_t Length;
	filepos_t Pos;
	bool_t Silent;
	bool_t Ranges;
	bool_t ReOpen;
	pin Comments;

	bool_t Pending;
	void* Complete;
	DWORD CompleteResult;
	DWORD Avail;

	uint32_t MetaInt;
	uint32_t MetaLeft;

	char* EnumBuffer;
	char* EnumPos;
	int EnumType;
	const tchar_t* Exts;
	bool_t ExtFilter;
	int AccessType;
	int CodePage;

	tchar_t URL[MAXPATH];
	tchar_t Base[MAXPATH];
	tchar_t ContentType[MAXPATH];
	tchar_t PragmaSend[MAXPRAGMA];
	tchar_t PragmaGet[MAXPRAGMA];

	BOOL (WINAPI* InternetGoOnline)(LPCTSTR, HWND, DWORD);

} http;

static int Get(http* p, int No, void* Data, int Size)
{
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case STREAM_URL: GETSTRING(p->URL); break;
	case STREAM_BASE: GETSTRING(p->Base); break;
	case STREAM_SILENT: GETVALUE(p->Silent,bool_t); break;
	case STREAM_LENGTH: GETVALUECOND(p->Length,int,p->Length>=0); break;
	case STREAM_CONTENTTYPE: GETSTRING(p->ContentType); break;
	case STREAM_COMMENT: GETVALUE(p->Comments,pin); break;
	case STREAM_PRAGMA_SEND: GETSTRING(p->PragmaSend); break;
	case STREAM_PRAGMA_GET: GETSTRING(p->PragmaGet); break;
	}
	return Result;
}

static NOINLINE void GetLine(const tchar_t* In,tchar_t* Out,int OutLen)
{
	while (IsSpace(*In)) ++In;
	for (;*In && *In!=10 && *In!=13 && OutLen>1;++In,++Out,--OutLen)
		*Out = *In;
	*Out=0;
}

static void ProcessHead(http* p,const tchar_t* Head)
{
	tchar_t s[128];

#ifdef DUMPHTTP
	FILE* Dump;
	Dump = fopen("\\dumphttp.head","wb+");
	if (Dump)
	{
		fwrite(Head,1,tcslen(Head)*sizeof(tchar_t),Dump);
		fclose(Dump);
	}
#endif

	while (*Head)
	{
		if (tcsncmp(Head,T("Content-Type:"),13)==0 || tcsncmp(Head,T("content-type:"),13)==0)
		{
			GetLine(Head+13,p->ContentType,MAXPATH);
			tcsupr(p->ContentType);
		}

		if (tcsncmp(Head,T("icy-genre:"),10)==0)
		{
			tcscpy_s(s,TSIZEOF(s),T("GENRE="));
			GetLine(Head+10,s+6,128-6);
			if (p->Comments.Node)
				p->Comments.Node->Set(p->Comments.Node,p->Comments.No,s,sizeof(s));
		}
		else
		if (tcsncmp(Head,T("icy-name:"),9)==0)
		{
			tcscpy_s(s,TSIZEOF(s),T("TITLE="));
			GetLine(Head+9,s+6,128-6);
			if (p->Comments.Node)
				p->Comments.Node->Set(p->Comments.Node,p->Comments.No,s,sizeof(s));
		}
		else
		if (tcsncmp(Head,T("icy-metaint:"),12)==0)
		{
			GetLine(Head+12,s,128);
			p->MetaLeft = p->MetaInt = StringToInt(s,0);
		}

		while (*Head && *Head != 10)
			++Head;
		if (*Head == 10) 
			++Head;
	}
}

#ifdef DUMPHTTP
static FILE* Dump = NULL;
#endif

static void CALLBACK InternetCallback(HINTERNET hInternet,
                               DWORD dwContext,
                               DWORD dwInternetStatus,
                               LPVOID lpvStatusInformation,
                               DWORD dwStatusInformationLength)
{
	if (dwInternetStatus == INTERNET_STATUS_REQUEST_COMPLETE)
	{
		INTERNET_ASYNC_RESULT *Result = (INTERNET_ASYNC_RESULT*)lpvStatusInformation;
		http *p = (http*)dwContext;
		p->CompleteResult = Result->dwResult;
		EventSet(p->Complete);
	}
}

static DWORD Pending(http* p,DWORD v,int Timeout)
{
	if (v || GetLastError()!=ERROR_IO_PENDING)
		return v;

	p->Pending = 1;
	if (EventWait(p->Complete,GetTimeFreq()*Timeout))
	{
		int Result = p->CompleteResult;
		p->Pending = 0;
		return Result;
	}
	return 0;
}

static int Open(http* p, const tchar_t* URL, bool_t ReOpen, bool_t UsePragma)
{
	int Again = 0;

again:
	if (p->Handle)
	{
		InternetCloseHandle(p->Handle);
		p->Handle = 0;
	}
	if (p->Internet)
	{
		InternetCloseHandle(p->Internet);
		p->Internet = 0;
	}

	//DEBUG_MSG(-1,T("closed %d"),p->Pending);
	if (p->Pending)
	{
		EventWait(p->Complete,GetTimeFreq());
		p->Pending = 0;
	}

	p->ContentType[0] = 0;
	p->Length = -1;
	if (!ReOpen)
	{
		p->AccessType = INTERNET_OPEN_TYPE_DIRECT;
		p->URL[0] = 0;
	}
	else
		Sleep(200);

	free(p->EnumBuffer);
	p->EnumBuffer = NULL;
	p->EnumPos = NULL;
	p->EnumType =0;
	p->MetaInt = 0;
	p->MetaLeft = 0;
	p->ReOpen = 0;

#ifdef DUMPHTTP
	if (Dump)
	{
		fclose(Dump);
		Dump = NULL;
	}
#endif

	if (URL && URL[0])
	{
		tchar_t URL2[MAXPATH];
		tchar_t s[MAXPRAGMA+32];
		DWORD n;
		int Secure = 0;

#ifdef DUMPHTTP
		Dump = fopen("\\dumphttp.asf","wb+");
#endif

		if (tcsnicmp(URL,T("https"),5)==0)
			Secure = INTERNET_FLAG_SECURE|INTERNET_FLAG_KEEP_CONNECTION;

		if (tcsnicmp(URL,T("mms://"),6)==0)
		{
			tcscpy_s(URL2,TSIZEOF(URL2),T("http://"));
			tcscat_s(URL2,TSIZEOF(URL2),URL+6);
			URL = URL2;
		}

		if (!ReOpen && p->InternetGoOnline)
			p->InternetGoOnline(URL,Context()->Wnd,0);


retry:
//WMP9	p->Internet = InternetOpen(T("NSPlayer/4.1.0.3928"),p->AccessType,NULL,NULL,INTERNET_FLAG_ASYNC); 
		p->Internet = InternetOpen(T("NSPlayer/10.0.0.3802"),p->AccessType,NULL,NULL,INTERNET_FLAG_ASYNC); 
		InternetSetStatusCallback(p->Internet,InternetCallback);

		s[0] = 0;
		if (ReOpen && p->Ranges && p->Pos>0)
			stprintf_s(s,TSIZEOF(s),T("Range: bytes=%d-\n"),p->Pos);
		else
			p->Pos = 0;

		if (UsePragma)
			tcscat_s(s,TSIZEOF(s),p->PragmaSend);

		//DEBUG_MSG1(-1,T("open %d"),p->Pending);

		p->Handle = (HANDLE)Pending(p,(DWORD)InternetOpenUrl(p->Internet, URL, s, (DWORD)-1, INTERNET_FLAG_NO_CACHE_WRITE|Secure, (DWORD)p),TIMEOUT_CONNECT);
		if (!p->Handle)
		{
			if (!ReOpen && p->AccessType == INTERNET_OPEN_TYPE_DIRECT)
			{
				p->AccessType = INTERNET_OPEN_TYPE_PRECONFIG;
				InternetCloseHandle(p->Internet);
				goto retry;
			}

			if (!ReOpen && !p->Silent)
			{
				if (URL2==URL)
					ShowError(0,ERR_ID,ERR_MIME_NOT_FOUND,T("MMS"));
				else
					ShowError(0,ERR_ID,ERR_CONNECT_FAILED,URL);
			}
			return ERR_FILE_NOT_FOUND;
		}

		n = sizeof(p->URL);
		if (!InternetQueryOption(p->Handle, INTERNET_OPTION_URL, p->URL, &n))
			tcscpy_s(p->URL,TSIZEOF(p->URL),URL);

		SplitURL(p->URL,p->Base,TSIZEOF(p->Base),p->Base,TSIZEOF(p->Base),NULL,0,NULL,0);

		n = sizeof(p->ContentType);
		if (!HttpQueryInfo(p->Handle, HTTP_QUERY_CONTENT_TYPE , p->ContentType, &n, NULL))
			p->ContentType[0] = 0;

		tcsupr(p->ContentType);

		n = sizeof(s);
		if (HttpQueryInfo(p->Handle, HTTP_QUERY_STATUS_CODE , &s, &n, NULL) && n>0)
		{
			n = StringToInt(s,0);
			//DEBUG_MSG1(-1,T("HTTP status %d"),n);
			if (n>=400 && n<500) // client error
			{
				if (Again<5 && n!=404 && n!=410)
				{
					++Again;
					if (!ReOpen)
						Sleep(200);
					goto again;
				}

				if (!ReOpen && !p->Silent)
					switch (n)
					{
					case 401:
						ShowError(0,ERR_ID,ERR_UNAUTHORIZED);
						break;
					case 404:
					case 410:
						ShowError(0,ERR_ID,ERR_FILE_NOT_FOUND,URL);
						break;
					default:
						ShowError(0,ERR_ID,ERR_CONNECT_FAILED);
						break;
					}

				return ERR_FILE_NOT_FOUND;
			}
		}

		n = sizeof(s);
		if (HttpQueryInfo(p->Handle, HTTP_QUERY_CONTENT_LENGTH , &s, &n, NULL) && n>0)
		{
			n = StringToInt(s,0);
			if (n>0)
				p->Length = n + p->Pos;
		}

		if (UsePragma)
		{
			DWORD No = 0;
			p->PragmaGet[0] = 0;
			for (;;)
			{
				n = (TSIZEOF(p->PragmaGet) - tcslen(p->PragmaGet) - 2)*sizeof(tchar_t);
				if (!HttpQueryInfo(p->Handle, HTTP_QUERY_PRAGMA, p->PragmaGet+tcslen(p->PragmaGet), &n, &No) || n<=0)
					break;
				tcscat_s(p->PragmaGet,TSIZEOF(p->PragmaGet),T(","));
				++No;
			}

			{
				tchar_t* Head;
				n = 0;
				HttpQueryInfo(p->Handle, HTTP_QUERY_RAW_HEADERS_CRLF, NULL, &n, NULL);
				if (n>0 && (Head = (tchar_t*)malloc(n))!=NULL)
				{
					if (HttpQueryInfo(p->Handle, HTTP_QUERY_RAW_HEADERS_CRLF, Head, &n, NULL))
						ProcessHead(p,Head);
					free(Head);
				}
			}
		}

		if (!ReOpen)
		{
			n = sizeof(s);
			if (HttpQueryInfo(p->Handle, HTTP_QUERY_ACCEPT_RANGES, s, &n, NULL) && n>0)
			{
				tcsupr(s);
				p->Ranges = tcsstr(s,T("BYTES")) != NULL;
			}
			else 
				p->Ranges = p->Length>0; // assume range support for fixed length files
		}
	}
	else
		p->Length = -1;
	return ERR_NONE;
}

static int Set(http* p, int No, const void* Data, int Size)
{
	int Result = ERR_INVALID_PARAM;
	switch (No)
	{
	case STREAM_COMMENT: SETVALUE(p->Comments,pin,ERR_NONE); break;
	case STREAM_SILENT: SETVALUE(p->Silent,bool_t,ERR_NONE); break;
	case STREAM_PRAGMA_SEND: SETSTRING(p->PragmaSend); p->ReOpen = 1; break;
	case STREAM_PRAGMA_GET: SETSTRING(p->PragmaGet); break;
	case STREAM_URL:
		tcscpy_s(p->PragmaSend,TSIZEOF(p->PragmaSend),
			T("Icy-MetaData:1\n")
			T("Pragma: no-cache,rate=1.000000,stream-time=0,stream-offset=4294967295:4294967295,request-context=1,max-duration=2147492981\n"));
		Result = Open(p,(const tchar_t*)Data,0,1);
		break;
	}
	return Result;
}

static void ProcessMeta(http* p,char* Meta,int Size)
{
	tchar_t s[128];
	char *i,*j;
	for (i=Meta;i<Meta+Size;i++)
		if (memcmp(i,"StreamTitle='",13)==0 && i[13]!='\'')
		{
			for (j=i+13;j<Meta+Size;++j)
				if (*j=='\'')
					break;
			*j=0;
			tcscpy_s(s,TSIZEOF(s),T("TITLE="));
			StrToTcs(s+6,TSIZEOF(s)-6,i+13);
			if (p->Comments.Node)
				p->Comments.Node->Set(p->Comments.Node,p->Comments.No,s,sizeof(s));
			break;
		}
}

static int DataAvailable(http* p)
{
	for (;;)
	{
		if (p->Pending)
		{
			if (EventWait(p->Complete,0))
			{
				//DEBUG_MSG(-1,T("avail %d"),p->Avail);
				p->Pending = 0;
				return p->Avail;
			}
			break;
		}

		p->Avail = 0;
		if (InternetQueryDataAvailable(p->Handle,&p->Avail,0,0))
		{
			if (p->Avail==0)
				return -1;
			return p->Avail;
		}

		if (GetLastError()!=ERROR_IO_PENDING)
			return -1;

		//DEBUG_MSG(-1,T("pending"));
		p->Pending = 1;
	}
	return 0;
}

static int Read(http* p,void* Data,int Size)
{
#ifdef DUMPHTTP
	static bool_t SkipDump = 0;
#endif

	DWORD Readed;
	DWORD Error;

	if (p->Pending && EventWait(p->Complete,GetTimeFreq()*TIMEOUT_READ))
		p->Pending = 0;

	if (!p->Pending && Pending(p,InternetReadFile(p->Handle,Data,Size,&Readed),TIMEOUT_READ))
	{
		//DEBUG_MSG2(-1,T("readed %d %d"),Size,Readed);

		p->Pos += Readed;
		if (p->Pos == (int)Readed && Readed > 16)
		{
			int Code;
			tchar_t Head[16];

			GetAsciiToken(Head,16,Data,Readed);
			if (stscanf(Head,T("ICY %d"),&Code)==1)
			{
				unsigned int n;
				char* ch = (char*)Data;

				if (Code!=200)
					return -1;

				tcscpy_s(p->ContentType,TSIZEOF(p->ContentType),T("AUDIO/MPEG")); // default for ICY

				for (n=0;n<Readed-4;++n,++ch)
					if (ch[0]==10 && (ch[1]==10 || (ch[1]==13 && ch[2]==10)))
					{
						tchar_t* Head;

						n += 2;
						if (ch[1]==13)
							++n;

						Head = malloc(sizeof(tchar_t)*(n+1));
						if (Head)
						{
							ch[0] = 0;
							StrToTcs(Head,n+1,(char*)Data);
							ProcessHead(p,Head);
							free(Head);
						}

						Readed -= n;
						memmove(Data,(char*)Data+n,Readed);
						break;
					}
			}
		}

		if (p->MetaInt>0)
		{
			if (p->MetaLeft < Readed)
			{
				uint32_t Len;
				uint8_t* b = (uint8_t*)Data;

				while (p->MetaLeft < Readed)
				{
					// skip data bytes
					b += p->MetaLeft;
					Readed -= p->MetaLeft;

					Len = *b * 16;
					if (--Readed < Len)
					{
						// need more meta data 

						char Meta[16*255+1];
						memcpy(Meta,b+1,Readed);

						if (p->Pending && EventWait(p->Complete,GetTimeFreq()*TIMEOUT_READ))
							p->Pending = 0;

						if (!p->Pending && Pending(p,InternetReadFile(p->Handle,Meta+Readed,Len-Readed,&Readed),TIMEOUT_READ))
						{
							p->Pos += Readed;
							ProcessMeta(p,Meta,Len);
						}
						else
							goto error;

						Readed = 0;
					}
					else
					{
						Readed -= Len;
						if (Len)
							ProcessMeta(p,(char*)(b+1),Len);
						memmove(b,b+1+Len,Readed);
					}

					p->MetaLeft = p->MetaInt;
				}

				p->MetaLeft -= Readed;
				b += Readed;

				Len = b-(uint8_t*)Data;
#ifdef DUMPHTTP
				SkipDump = 1;
#endif
				Readed = Read(p,b,Size-Len);
#ifdef DUMPHTTP
				SkipDump = 0;
#endif

				if (Readed>=0)
					Readed += Len;
			}
			else
				p->MetaLeft -= Readed;
		}

#ifdef DUMPHTTP
		if (!SkipDump && Dump)
			fwrite(Data,1,Readed,Dump);
#endif
		return Readed;
	}

error:
	Error = GetLastError();

	if (p->Pending || Error == ERROR_NOT_CONNECTED || Error == ERROR_INVALID_HANDLE)
		Open(p,p->URL,1,1);

	return -1;
}

static int ReadBlock(http* p,block* Block,int Ofs,int Size)
{
	return Read(p,(char*)(Block->Ptr+Ofs),Size);
}

static filepos_t Seek(http* p,filepos_t Pos,int SeekMode)
{
	filepos_t Length;

	switch (SeekMode)
	{
	case SEEK_CUR: 
		Pos += p->Pos; 
		break;
	case SEEK_END: 
		if (p->Length<0) 
			return -1;
		Pos += p->Length;
		break;
	}

	Length = Pos - p->Pos;
	if (Length<0 && Pos < SEEK_READ)
	{
		p->Pos = 0;
		Open(p,p->URL,1,1);
		Length = Pos - p->Pos;
	}

	if (Length == 0 && !p->ReOpen)
		return p->Pos;

	if (Length > 0 && Length < SEEK_READ && !p->ReOpen)
	{
		int Len = (int)Length;
		void* Buffer = malloc(Len);
		if (Buffer)
		{
			Read(p,Buffer,Len);
			free(Buffer);
			if (p->Pos == Pos)
				return Pos;
		}
	}

	if (p->Ranges || !Pos)
	{
		p->Pos = Pos;
		if (Open(p,p->URL,1,1) == ERR_NONE)
			return p->Pos;
	}

	return -1;
}

static char* FindUpper(char* p,const char* s)
{
	int i;
	for (i=0;*p && s[i];++p)
		if (toupper(p[0]) == s[i])
			++i;
		else
			i=0;
	return s[i] ? NULL:p;
}

typedef struct htmlchar
{
	uint8_t ch;
	char sym[6+1];

} htmlchar;

static const htmlchar HTMLChar[] =
{
	{34,"quot"},
	{38,"amp"},
	{60,"lt"},
	{62,"gt"},
	{160,"nbsp"},
	{161,"iexcl"},
	{162,"cent"},
	{163,"pound"},
	{164,"curren"},
	{165,"yen"},
	{166,"brvbar"},
	{167,"sect"},
	{168,"uml"},
	{169,"copy"},
	{170,"ordf"},
	{171,"laquo"},
	{172,"not"},
	{173,"shy"},
	{174,"reg"},
	{175,"hibar"},
	{176,"deg"},
	{177,"plusmn"},
	{185,"sup1"},
	{178,"sup2"},
	{179,"sup3"},
	{180,"acute"},
	{181,"micro"},
	{182,"para"},
	{183,"middot"},
	{184,"cedil"},
	{186,"ordm"},
	{187,"raquo"},
	{188,"frac14"},
	{189,"frac12"},
	{190,"frac34"},
	{191,"iquest"},
	{192,"Agrave"},
	{193,"Aacute"},
	{194,"Acirc"},
	{195,"Atilde"},
	{196,"Auml"},
	{197,"Aring"},
	{198,"AElig"},
	{199,"Ccedil"},
	{200,"Egrave"},
	{201,"Eacute"},
	{202,"Ecirc"},
	{203,"Euml"},
	{204,"Igrave"},
	{205,"Iacute"},
	{206,"Icirc"},
	{207,"Iuml"},
	{208,"ETH"},
	{209,"Ntilde"},
	{210,"Ograve"},
	{211,"Oacute"},
	{212,"Ocirc"},
	{213,"Otilde"},
	{214,"Ouml"},
	{215,"times"},
	{216,"Oslash"},
	{217,"Ugrave"},
	{218,"Uacute"},
	{219,"Ucirc"},
	{220,"Uuml"},
	{221,"Yacute"},
	{222,"THORN"},
	{223,"szlig"},
	{224,"agrave"},
	{225,"aacute"},
	{226,"acirc"},
	{227,"atilde"},
	{228,"auml"},
	{229,"aring"},
	{230,"aelig"},
	{231,"ccedil"},
	{232,"egrave"},
	{233,"eacute"},
	{234,"ecirc"},
	{235,"euml"},
	{236,"igrave"},
	{237,"iacute"},
	{238,"icirc"},
	{239,"iuml"},
	{240,"eth"},
	{241,"ntilde"},
	{242,"ograve"},
	{243,"oacute"},
	{244,"ocirc"},
	{245,"otilde"},
	{246,"ouml"},
	{247,"divide"},
	{248,"oslash"},
	{249,"ugrave"},
	{250,"uacute"},
	{251,"ucirc"},
	{252,"uuml"},
	{253,"yacute"},
	{254,"thorn"},
	{255,"yuml"},
	{0},
};

static void HTMLCharParse(char* p)
{
	char* i;
	for (;*p;++p)
		if (*p=='&' && (i=strchr(p,';'))!=NULL)
		{
			const htmlchar* c;
			int n = i-p-1;
			for (c=HTMLChar;c->ch;++c)
				if (strncmp(c->sym,p+1,n)==0 && c->sym[n]==0)
				{
					*p = c->ch;
					memmove(p+1,i+1,strlen(i));
					break;
				}
		}
}

static void HTMLURLParse(char* p)
{
	for (;*p;++p)
		if (p[0]=='%' && Hex(p[1])>=0 && Hex(p[2])>=0)
		{
			*p = (char)((Hex(p[1])<<4)+Hex(p[2]));
			memmove(p+1,p+3,strlen(p+3)+1);
		}
}

// detect Apache directory listing order links
static bool_t ListingOrder(const tchar_t* Base,const tchar_t* FileName)
{
	tchar_t Path[MAXPATH];
	int n = tcslen(Base);
	AbsPath(Path,TSIZEOF(Path),FileName,Base);
	return tcsncmp(Path,Base,n)==0 && tcsnicmp(Path+n,T("/?C="),4)==0;
}

static int EnumDir(http* p,const tchar_t* URL,const tchar_t* Exts,bool_t ExtFilter,streamdir* Item)
{
	if (URL)
	{
		tchar_t Head[16];

		int Pos;
		int Result = Open(p,URL,0,0);
		if (Result != ERR_NONE)
			return Result;

		if (!tcsstr(p->ContentType,T("TEXT/HTML")))
		{
			Open(p,NULL,0,0);
			return ERR_NOT_DIRECTORY;
		}

		if (p->Length<=0)
			p->Length = 256*1024-1;
		if (p->Length >= 512*1024)
			p->Length = 512*1024-1;

		p->EnumBuffer = malloc((int)p->Length+1);
		if (!p->EnumBuffer)
			return ERR_OUT_OF_MEMORY;

		Pos = 0;
		while (Pos<p->Length)
		{
			int Size = Read(p,p->EnumBuffer+Pos,p->Length-Pos);
			if (Size <= 0)
				break;
			Pos += Size;
		}

		GetAsciiToken(Head,16,p->EnumBuffer,p->Length);
		if (tcsnicmp(Head,T("<ASX"),4)==0 || 
			tcsnicmp(Head,T("[playlist]"),10)==0 || 
			tcsnicmp(Head,T("[Reference]"),11)==0)
		{
			Open(p,NULL,0,0);
			return ERR_NOT_DIRECTORY;
		}

		p->CodePage = GetCodePage(p->ContentType);

		p->EnumBuffer[Pos] = 0;
		p->EnumPos = p->EnumBuffer;
		p->EnumType =0;
		p->Exts = Exts;
		p->ExtFilter = ExtFilter;
	}

	while (p->EnumType<4)
	{
		const char* Tag1;
		const char* Tag2;

		switch (p->EnumType)
		{
		case 0:
			Tag1 = "<A ";
			Tag2 = "HREF=";
			break;
		case 1:
			Tag1 = "<AREA ";
			Tag2 = "HREF=";
			break;
		case 2:
			Tag1 = "<EMBED ";
			Tag2 = "HREF=";
			break;
		default:
			Tag1 = "<EMBED ";
			Tag2 = "SRC=";
			break;
		}

		while (p->EnumPos && (p->EnumPos = FindUpper(p->EnumPos,Tag1))!=NULL)
		{
			char* End;
			p->EnumPos = FindUpper(p->EnumPos,Tag2);
			if (!p->EnumPos)
				break;
			if (*p->EnumPos=='"' || *p->EnumPos=='\'')
			{
				++p->EnumPos;
				End = strchr(p->EnumPos,p->EnumPos[-1]);
			}
			else
				End = strchr(p->EnumPos,'>');

			if (End)
			{
				int Len;
				char Endch = *End;
				*End=0;

				HTMLCharParse(p->EnumPos);
				StrToTcsEx(Item->FileName,TSIZEOF(Item->FileName),p->EnumPos,p->CodePage);
				HTMLURLParse(p->EnumPos);
				StrToTcsEx(Item->DisplayName,TSIZEOF(Item->DisplayName),p->EnumPos,p->CodePage);

				*End = Endch;
				p->EnumPos = End+1;

				Item->Date = -1;
				Item->Size = -1;
				Len = tcslen(Item->FileName);

				if (ListingOrder(p->Base,Item->FileName))
					continue;

				if (Len && Item->FileName[Len-1]=='/')
				{
					Item->FileName[Len-1]=0;
					return ERR_NONE;
				}

				if (CheckExts(Item->FileName,T("html:H;htm:H;asp:H;php:H")))
					return ERR_NONE;

				Item->Type = CheckExts(Item->FileName,p->Exts);
				Item->Size = 0;

				if (Item->Type || !p->ExtFilter)
					return ERR_NONE;
			}
		}

		p->EnumPos = p->EnumBuffer;
		++p->EnumType;
	}
	
	Open(p,NULL,0,0);
	return ERR_END_OF_FILE;
}

static int Create(http* p)
{
	HMODULE Module;

	p->Stream.Get = (nodeget)Get,
	p->Stream.Set = (nodeset)Set,
	p->Stream.Read = Read;
	p->Stream.ReadBlock = ReadBlock;
	p->Stream.Seek = Seek;
	p->Stream.EnumDir = EnumDir;
	p->Stream.DataAvailable = DataAvailable;
	p->Complete = EventCreate(0,0);

	Module = GetModuleHandle(T("wininet.dll"));
#ifdef UNICODE
	GetProc(&Module,&p->InternetGoOnline,T("InternetGoOnlineW"),1);
#else
	GetProc(&Module,&p->InternetGoOnline,T("InternetGoOnlineA"),1);
#endif

	return ERR_NONE;
}

static void Delete(http* p)
{
	Open(p,NULL,0,0);
	EventClose(p->Complete);
}

static const nodedef HTTP =
{
	sizeof(http),
	HTTP_ID,
	STREAM_CLASS,
	PRI_MINIMUM,
	(nodecreate)Create,
	(nodedelete)Delete,
};

static const nodedef MMS =
{
	0, // parent size
	MMS_ID,
	HTTP_ID,
	PRI_MINIMUM,
};

void HTTP_Init()
{
	NodeRegisterClass(&HTTP);
	NodeRegisterClass(&MMS);
}

void HTTP_Done()
{
	NodeUnRegisterClass(MMS_ID);
	NodeUnRegisterClass(HTTP_ID);
}

#endif
