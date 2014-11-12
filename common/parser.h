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
 * $Id: parser.h 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#ifndef __PARSER_H
#define __PARSER_H

#define MAXTOKEN		32
#define MAXLINE			1024

typedef struct parser
{
	stream* Stream;
	buffer Buffer;

} parser;

DLL void ParserStream(parser*, stream* Stream);
DLL void ParserDataFeed(parser* p,const void* Ptr,size_t Len);
DLL bool_t ParserLine(parser*, tchar_t* Out, size_t OutLen);
DLL bool_t ParserIsToken(parser*, const tchar_t* Token); // case insensitive
DLL const uint8_t* ParserPeek(parser*, size_t Len);

DLL bool_t ParserIsElement(parser*, tchar_t* Name, size_t NameLen);
DLL bool_t ParserElementContent(parser*, tchar_t* Out, size_t OutLen);
DLL void ParserElementSkip(parser*);

DLL bool_t ParserIsAttrib(parser*, tchar_t* Name, size_t NameLen);
DLL int ParserAttribInt(parser*);
DLL bool_t ParserAttribString(parser*, tchar_t* Out, size_t OutLen);
DLL void ParserAttribSkip(parser*);

#endif
