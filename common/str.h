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
 * $Id: str.h 498 2006-01-03 12:01:57Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __STR_H
#define __STR_H

#define LANG_ID			FOURCC('L','A','N','G')

#define LANG_EN			FOURCC('E','N','_','_')
#define LANG_DEFAULT	LANG_EN

#define TSIZEOF(name)	(sizeof(name)/sizeof(tchar_t))

typedef struct strid
{
	int Class;
	int No;

} strid;

DLL void String_Init();
DLL void String_Done();

DLL int LangEnum(int Class, int No);
DLL const tchar_t* LangStr(int Class, int Id);
DLL const tchar_t* LangStrDef(int Class, int Id); // in English

DLL void StringAdd(bool_t Default, int Class,int No,const tchar_t*);
DLL void StringAddPrint(bool_t Default, int Class,int No,const tchar_t*, ...);

DLL void StringAddText(const char* Data,int Length);
DLL bool_t StringAddBinary(const void* Data, size_t Length);
DLL bool_t StringIsBinary(int Class, int Id);

DLL bool_t IsSpace(int);
DLL bool_t IsAlpha(int);
DLL bool_t IsDigit(int);
DLL int Hex(int);

DLL int GetCodePage(const tchar_t* ContentType);

DLL void StrToTcs(tchar_t* Out,size_t OutLen,const char* In);
DLL void StrToTcsEx(tchar_t* Out,size_t OutLen,const char* In,int CodePage);
DLL void WcsToTcs(tchar_t* Out,size_t OutLen,const uint16_t* In);
DLL void UTF8ToTcs(tchar_t* Out,size_t OutLen,const char* In);
DLL void AsciiToTcs(tchar_t* Out,size_t OutLen,const char* In);
DLL void TcsToAscii(char* Out,size_t OutLen,const tchar_t* In);
DLL void TcsToUTF8(char* Out,size_t OutLen,const tchar_t* In);
DLL void TcsToStr(char* Out,size_t OutLen,const tchar_t* In);
DLL void TcsToStrEx(char* Out,size_t OutLen,const tchar_t* In,int CodePage);

DLL void tcscpy_s(tchar_t* Out,size_t OutLen,const tchar_t* In);
DLL void tcsncpy_s(tchar_t* Out,size_t OutLen,const tchar_t* In,size_t n);
DLL void tcscat_s(tchar_t* Out,size_t OutLen,const tchar_t* In);
DLL int tcsicmp(const tchar_t* a,const tchar_t* b);
DLL int tcsnicmp(const tchar_t* a,const tchar_t* b,size_t n);
DLL void tcsupr(tchar_t* a);
DLL void tcsuprto(tchar_t* s, tchar_t Delimiter);
DLL tchar_t* tcsdup(const tchar_t*);
DLL int stscanf(const tchar_t* In, const tchar_t* Mask, ...);
DLL void stprintf_s(tchar_t* Out,size_t OutLen,const tchar_t* Mask, ...);
DLL void stcatprintf_s(tchar_t* Out,size_t OutLen,const tchar_t* Mask, ...);
DLL void vstprintf_s(tchar_t* Out,size_t OutLen,const tchar_t* Mask,va_list Arg);

DLL uint32_t UpperFourCC(uint32_t);
DLL void FourCCToString(tchar_t* Out, size_t OutLen, uint32_t FourCC);
DLL uint32_t StringToFourCC(const tchar_t* In, bool_t Upper);
DLL void FractionToString(tchar_t* Out, size_t OutLen, const fraction* Fraction, bool_t Percent, int Decimal);
DLL int StringToInt(const tchar_t* In, bool_t Hex);
DLL void IntToString(tchar_t* Out, size_t OutLen, int Int, bool_t Hex);
DLL void TickToString(tchar_t* Out, size_t OutLen, tick_t Tick, bool_t MS, bool_t Extended, bool_t Fix);
DLL void HotKeyToString(tchar_t* Out, size_t OutLen, int HotKey);
DLL void BoolToString(tchar_t* Out, size_t OutLen, bool_t Bool);
DLL void GUIDToString(tchar_t* Out, size_t OutLen, const guid*);
DLL bool_t StringToGUID(const tchar_t* In, guid*);

#endif

