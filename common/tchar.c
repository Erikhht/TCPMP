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
 * $Id: tchar.c 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "common.h"

void tcscpy_s(tchar_t* Out,size_t OutLen,const tchar_t* In)
{
	if (OutLen>0)
	{
		size_t n = min(tcslen(In),OutLen-1);
		memcpy(Out,In,n*sizeof(tchar_t));
		Out[n] = 0;
	}
}

void tcsncpy_s(tchar_t* Out,size_t OutLen,const tchar_t* In,size_t n)
{
	if (OutLen>0)
	{
		n = min(min(tcslen(In),n),OutLen-1);
		memcpy(Out,In,n*sizeof(tchar_t));
		Out[n] = 0;
	}
}

void tcscat_s(tchar_t* Out,size_t OutLen,const tchar_t* In)
{
	if (OutLen>0)
	{
		size_t n = tcslen(Out);
		tcscpy_s(Out+n,OutLen-n,In);
	}
}

tchar_t* tcsdup(const tchar_t* s)
{
	size_t n = tcslen(s)+1;
	tchar_t* p = (tchar_t*)malloc(n*sizeof(tchar_t));
	if (p) tcscpy_s(p,n,s);
	return p;
}

void StrToTcs(tchar_t* Out,size_t OutLen,const char* In)
{
	StrToTcsEx(Out,OutLen,In,Context()->CodePage);
}

void TcsToStr(char* Out,size_t OutLen,const tchar_t* In)
{
	TcsToStrEx(Out,OutLen,In,Context()->CodePage);
}

void BoolToString(tchar_t* Out, size_t OutLen, bool_t Bool)
{
	tcscpy_s(Out,OutLen,LangStrDef(PLATFORM_ID,Bool?PLATFORM_YES:PLATFORM_NO));
};

uint32_t StringToFourCC(const tchar_t* In, bool_t Upper)
{
	char s[4+1];
	tchar_t Up[4+1];
	int i;

	if (Upper)
	{
		tcscpy_s(Up,TSIZEOF(Up),In);
		tcsupr(Up);
		In = Up;
	}
	TcsToAscii(s,sizeof(s),In);

	for (i=0;i<4;++i)
		if (!s[i])
			for (;i<4;++i)
				s[i] = '_';

	return FOURCC(s[0],s[1],s[2],s[3]);
}

void FourCCToString(tchar_t* Out, size_t OutLen, uint32_t FourCC)
{
	uint32_t s[2];
	s[0] = FourCC;
	s[1] = 0;
	AsciiToTcs(Out,OutLen,(char*)s);
}

uint32_t UpperFourCC(uint32_t FourCC)
{
	return (toupper((FourCC >> 0) & 255) << 0) |
		   (toupper((FourCC >> 8) & 255) << 8) |
		   (toupper((FourCC >> 16) & 255) << 16) |
		   (toupper((FourCC >> 24) & 255) << 24);
}

// gcc 2.97 bug...
static const tchar_t mask_d[] = T("%d");
static const tchar_t mask_X[] = T("%X");
static const tchar_t mask_0x08X[] = T("0x%08X");

void FractionToString(tchar_t* Out, size_t OutLen, const fraction* p, bool_t Percent, int Decimal)
{
	int a,b,i;
	int Num = p->Num;
	int Den = p->Den;

	if (Percent)
	{
		while (abs(Num) > MAX_INT/100)
		{
			Num >>= 1;
			Den >>= 1;
		}
		Num *= 100;
	}

	if (Den)
	{
		if (Den<0)
		{
			Num = -Num;
			Den = -Den;
		}
		for (i=0,b=1;i<Decimal;++i,b*=10);
		if (Num>0)
		{
			// rounding
			a = Den/(2*b);
			if (Num<MAX_INT-a)
				Num += a;
			else
				Num = MAX_INT;
		}
		a=Num/Den;
		Num -= a*Den;
		b=(int)(((int64_t)Num*b)/Den);
	}
	else
		a=b=0;

	if (Decimal)
		stprintf_s(Out,OutLen,T("%d.%0*d"),a,Decimal,b);
	else
		stprintf_s(Out,OutLen,mask_d,a);

	if (Percent)
		tcscat_s(Out,OutLen,T("%"));

}

int StringToInt(const tchar_t* In, bool_t Hex)
{
	int v=0;
	stscanf(In,Hex ? mask_X:mask_d,&v);
	return v;
}

void IntToString(tchar_t* Out, size_t OutLen, int p, bool_t Hex)
{
	stprintf_s(Out,OutLen,Hex ? mask_0x08X:mask_d,p);
}

void TickToString(tchar_t* Out, size_t OutLen, tick_t Tick, bool_t MS, bool_t Extended, bool_t Fix)
{
	tchar_t Sign[2] = {0};
	if (Tick<0) 
	{
		Sign[0] = '-';
		Tick = -Tick;
	}
	if (!MS)
	{
		int Hour,Min,Sec;
		Hour = Tick / 3600 / TICKSPERSEC;
		Tick -= Hour * 3600 * TICKSPERSEC;
		Min = Tick / 60 / TICKSPERSEC;
		Tick -= Min * 60 * TICKSPERSEC;
		Sec = Tick / TICKSPERSEC;
		Tick -= Sec * TICKSPERSEC;
		if (Hour)
			stprintf_s(Out,OutLen,T("%s%d:%02d"),Sign,Hour,Min);
		else
			stprintf_s(Out,OutLen,Fix?T("%s%02d"):T("%s%d"),Sign,Min);
		stcatprintf_s(Out,OutLen,T(":%02d"),Sec);
		if (Extended)
			stcatprintf_s(Out,OutLen,T(".%03d"),(Tick*1000)/TICKSPERSEC);
	}
	else
	{
		int i = Scale(Tick,100000,TICKSPERSEC);
		stprintf_s(Out,OutLen,T("%s%d.%02d ms"),Sign,i/100,i%100);
	}
}

void stprintf_s(tchar_t* Out,size_t OutLen,const tchar_t* Mask, ...)
{
	va_list Arg;
	va_start(Arg, Mask);
	vstprintf_s(Out,OutLen,Mask,Arg);
	va_end(Arg);
}

void stcatprintf_s(tchar_t* Out,size_t OutLen,const tchar_t* Mask, ...)
{
	size_t n = tcslen(Out);
	va_list Arg;
	va_start(Arg, Mask);
	vstprintf_s(Out+n,OutLen-n,Mask,Arg);
	va_end(Arg);
}

void vstprintf_s(tchar_t* Out,size_t OutLen,const tchar_t* Mask,va_list Arg)
{
	int n,vs,Width;
	unsigned int v,w,q,w0;
	bool_t ZeroFill;
	bool_t Unsigned;
	bool_t Sign;
	bool_t AlignLeft;
	const tchar_t *In;

	while (OutLen>1 && *Mask)
	{
		switch (*Mask)
		{
		case '%':
			++Mask;

			if (*Mask == '%')
			{
				*(Out++) = *(Mask++);
				--OutLen;
				break;
			}

			AlignLeft = *Mask=='-';
			if (AlignLeft)
				++Mask;

			ZeroFill = *Mask=='0';
			if (ZeroFill) 
				++Mask;

			Width = -1;
			if (IsDigit(*Mask))
			{
				Width = 0;
				for (;IsDigit(*Mask);++Mask)
					Width = Width*10 + (*Mask-'0');
			}
			if (*Mask == '*')
			{
				++Mask;
				Width = va_arg(Arg,int);
			}

			Unsigned = *Mask=='u';
			if (Unsigned)
				++Mask;

			switch (*Mask)
			{
			case 'd':
			case 'i':
			case 'X':
			case 'x':
				vs = va_arg(Arg,int);

				if (*Mask=='x' || *Mask=='X')
				{
					Unsigned = 1;
					q = 16;
					w = 0x10000000;
				}
				else
				{
					q = 10;
					w = 1000000000;
				}

				Sign = vs<0 && !Unsigned;
				if (Sign)
				{
					vs=-vs;
					--Width;
				}

				w0 = 1;
				for (n=1;n<Width;++n)
					w0 *= q;

				v = vs;
				while (v<w && w>w0)
					w/=q;

				while (w>0)
				{
					unsigned int i = v/w;
					v-=i*w;
					if (OutLen>1 && Sign && (w==1 || ZeroFill || i>0))
					{
						*(Out++) = '-';
						--OutLen;
						Sign = 0;
					}
					if (OutLen>1)
					{
						if (i==0 && !ZeroFill && w!=1)
							i = ' ';
						else
						{
							ZeroFill = 1;
							if (i>=10)
							{
								if (*Mask == 'X')
									i += 'A'-10;
								else
									i += 'a'-10;
							}
							else
								i+='0';
						}
						*(Out++) = (tchar_t)i;
						--OutLen;
					}
					w/=q;
				}

				break;

			case 'c':
				*(Out++) = (tchar_t)va_arg(Arg,int);
				--OutLen;
				break;

			case 's':
				In = va_arg(Arg,const tchar_t*);
				n = min(tcslen(In),OutLen-1);
				Width -= n;
				if (!AlignLeft)
					while (--Width>=0 && OutLen>1)
					{
						*Out++ = ' ';
						--OutLen;
					}
				memcpy(Out,In,n*sizeof(tchar_t));
				Out += n;
				OutLen -= n;
				while (--Width>=0 && OutLen>1)
				{
					*Out++ = ' ';
					--OutLen;
				}
				break;
			}

			++Mask;
			break;

		default:
			*(Out++) = *(Mask++);
			--OutLen;
			break;
		}
	}

	if (OutLen>0)
		*Out=0;
}

int stscanf(const tchar_t* In, const tchar_t* Mask, ...)
{
	va_list Arg;
	int n = 0;
	int Sign;
	int v;
	int Width;
	const tchar_t* In0;

	va_start(Arg, Mask);
	while (In && *In && *Mask)
	{
		switch (*Mask)
		{
		case '%':
			++Mask;

			Width = -1;
			if (IsDigit(*Mask))
			{
				Width = 0;
				for (;IsDigit(*Mask);++Mask)
					Width = Width*10 + (*Mask-'0');
			}

			switch (*Mask)
			{
			case 'X':
			case 'x':

				for (;IsSpace(*In);++In);
				v = 0;
				Sign = *In == '-';
				In0 = In;
				if (Sign) { ++In; --Width; }
				for (;Width!=0 && *In;++In,--Width)
				{
					int h = Hex(*In);
					if (h<0) break;
					v=v*16+h;
				}
				if (Sign) v=-v;
				if (In != In0)
				{
					*va_arg(Arg,int*) = v;
					++n;
				}
				else
					In = NULL;
				break;

			case 'd':
			case 'i':

				for (;IsSpace(*In);++In);
				v = 0;
				Sign = *In == '-';
				In0 = In;
				if (Sign) ++In;
				for (;Width!=0 && IsDigit(*In);++In,--Width)
					v=v*10+(*In-'0');
				if (Sign) v=-v;
				if (In != In0)
				{
					*va_arg(Arg,int*) = v;
					++n;
				}
				else
					In = NULL;
				break;

			case 'o':

				for (;IsSpace(*In);++In);
				v = 0;
				Sign = *In == '-';
				In0 = In;
				if (Sign) ++In;
				for (;Width!=0 && *In;++In,--Width)
				{
					if (*In >= '0' && *In <= '7')
						v=v*8+(*In-'0');
					else
						break;
				}
				if (Sign) v=-v;
				if (In != In0)
				{
					*va_arg(Arg,int*) = v;
					++n;
				}
				else
					In = NULL;
				break;
			}
			break;
		case 9:
		case ' ':
			for (;IsSpace(*In);++In);
			break;
		default:
			if (*Mask == *In)
				++In;
			else
			{
				In = NULL;
				n = -1;
			}
		}
		++Mask;
	}

	va_end(Arg);
	return n;
}

void GUIDToString(tchar_t* Out, size_t OutLen, const guid* p)
{
	stprintf_s(Out,OutLen,T("%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x"),
		(int)p->v1,p->v2,p->v3,p->v4[0],p->v4[1],p->v4[2],p->v4[3],
		p->v4[4],p->v4[5],p->v4[6],p->v4[7]);
}

bool_t StringToGUID(const tchar_t* In, guid* p)
{
	int i,v[10];
	if (stscanf(In,T("%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x"),
			&p->v1,v+0,v+1,v+2,v+3,v+4,v+5,v+6,v+7,v+8,v+9) < 11)
	{
		memset(p,0,sizeof(guid));
		return 0;
	}
	p->v2 = (uint16_t)v[0];
	p->v3 = (uint16_t)v[1];
	for (i=0;i<8;++i)
		p->v4[i] = (uint8_t)v[2+i];
	return 1;
}

bool_t IsSpace(int ch) { return ch==' ' || ch==9; }
bool_t IsAlpha(int ch) { return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'); }
bool_t IsDigit(int ch) { return ch>='0' && ch<='9'; }

int Hex(int ch) 
{
	if (IsDigit(ch))
		return ch-'0';
	if (ch >= 'a' && ch <= 'f')
		return ch-'a'+10;
	if (ch >= 'A' && ch <= 'F')
		return ch-'A'+10;
	return -1;
}

void tcsuprto(tchar_t* p, tchar_t Delimiter)
{
	for (;*p && *p!=Delimiter;++p)
		*p = (char)toupper(*p);
}

void tcsupr(tchar_t* p) 
{
	for (;*p;++p)
		*p = (char)toupper(*p);
}

