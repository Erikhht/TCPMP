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
 * $Id: vsprintf.c 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "../common.h"

int vsprintf(tchar_t *s0, const tchar_t *fmt, va_list args)
{
	tchar_t Num[80];
	tchar_t *s;

	for (s=s0;*fmt;++fmt)
	{
		if (fmt[0]=='%' && fmt[1])
		{
			const tchar_t *str;
			bool_t Left = 0;
			bool_t Sign = 0;
			bool_t Large = 0;
			bool_t ZeroPad = 0;
			int Width = -1;
			int Type = -1;
			int Base = 10;
			tchar_t ch,cs;
			int Len;
			long n;

			for (;;)
			{
				switch (*(++fmt)) 
				{
				case '-': Left = 1; continue;
				case '0': ZeroPad = 1; continue;
				default: break;
				}
				break;
			}

			if (IsDigit(*fmt))
			{
				Width = 0;
				for (;IsDigit(*fmt);++fmt)
					Width = Width*10 + (*fmt-'0');
			}
			else
			if (*fmt == '*')
			{
				++fmt;
				Width = va_arg(args, int);
				if (Width < 0)
				{
					Left = 1;
					Width = -Width;
				}
			}

			if (*fmt == 'h' || 
				*fmt == 'L' ||
				*fmt == 'l')
				Type = *(fmt++);

			switch (*fmt)
			{
			case 'c':
				for (;!Left && Width>1;--Width)
					*(s++) = ' ';
				*(s++) = (char)va_arg(args,int);
				for (;Width>1;--Width)
					*(s++) = ' ';
				continue;
			case 's':
				str = va_arg(args,const tchar_t*);
				if (!s)
					str = T("<NULL>");
				Len = tcslen(str);
				for (;!Left && Width>Len;--Width)
					*(s++) = ' ';
				for (;Len>0;--Len,--Width)
					*(s++) = *(str++);
				for (;Width>0;--Width)
					*(s++) = ' ';
				continue;
			case 'o':
				Base = 8;
				break;
			case 'X':
				Large = 1;
			case 'x':
				Base = 16;
				break;
			case 'i':
			case 'd':
				Sign = 1;
			case 'u':
				break;
			default:
				if (*fmt != '%')
					*(s++) = '%';
				*(s++) = *fmt;
				continue;
			}

			if (Type == 'l')
				n = va_arg(args,unsigned long);
			else
			if (Type == 'h')
				if (Sign)
					n = (short)va_arg(args,int);
				else
					n = (unsigned short)va_arg(args,unsigned int);
			else
			if (Sign)
				n = va_arg(args,int);
			else
				n= va_arg(args,unsigned int);

			if (Left)
				ZeroPad = 0;
			if (Large)
				str = T("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ");
			else
				str = T("0123456789abcdefghijklmnopqrstuvwxyz");

			ch = ' ';
			if (ZeroPad)
				ch = '0';
			cs = 0;

			if (n<0 && Sign)
			{
				cs = '-';
				n=-n;
			}

			Len = 0;
			if (n==0)
				Num[Len++] = '0';
			else
			{
				unsigned long un = n;
				while (un != 0)
				{
					Num[Len++] = str[un%Base];
					un /= Base;
				}
			}

			if (cs)
				++Len;

			for (;!Left && Width>Len;--Width)
				*(s++) = ch;

			if (cs)
			{
				*(s++) = cs;
				--Len;
				--Width;
			}

			for (;Len;--Width)
				*(s++) = Num[--Len];

			for (;Width>0;--Width)
				*(s++) = ' ';
		}
		else
			*(s++) = *fmt;
	}
	*(s++) = 0;
	return s-s0;
}

int sprintf(tchar_t *s, const tchar_t *fmt, ...)
{
	int i;
	va_list Args;
	va_start(Args,fmt);
	i=vsprintf(s,fmt,Args);
	va_end(Args);
	return i;
}
