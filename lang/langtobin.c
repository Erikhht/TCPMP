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
 * $Id: langtobin.c 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#define MAXLINE 1024

#define FOURCC(a,b,c,d) \
	(((unsigned char)a << 0) | ((unsigned char)b << 8) | \
	((unsigned char)c << 16) | ((unsigned char)d << 24))

#define LANG_DEFAULT		FOURCC('E','N','_','_')

static void Filter(char* i)
{
	char* j=i;
	for (;*i;++i,++j)
	{
		if (i[0]=='\\' && i[1]=='n')
		{
			*j=10;
			++i;
		}
		else
			*j=*i;
	}
	*j=0;
}

int ToFourCC(const char* In)
{
	char s[4+1];
	int i;

	for (i=0;i<4;++i)
		if (!In[i])
		{
			for (;i<4;++i)
				s[i] = '_';
		}
		else
			s[i] = (char)toupper(In[i]);

	return FOURCC(s[0],s[1],s[2],s[3]);
}

int ToInt(const char* In, int Hex)
{
	int v=0;
	sscanf(In,Hex ? "%X":"%d",&v);
	return v;
}

void Convert(const char* p, int n,FILE* out)
{
	char* s = (char*)malloc(MAXLINE*sizeof(char));
	int CodePage = -1;
	int Id,Len,Lang = 0;
	char* q;
	int i,No;

	if (s)
	{
		while (n>0)
		{
			for (i=0;n>0 && *p!=13 && *p!=10;++p,--n)
				if (i<MAXLINE-4)
					s[i++] = *p;
			for (;n>0 && (*p==13 || *p==10);++p,--n)
			s[i]=0;
			s[i+1]=0;
			s[i+2]=0;
			s[i+3]=0;

			for (i=0;isspace(s[i]);++i);
			if (s[i]==0) continue;
			if (s[i]==';')
			{
				sscanf(s+i,";CODEPAGE = %d",&CodePage);
				continue;
			}
			if (s[i]=='[')
			{
				++i;
				q = strchr(s+i,']');
				if (!q || CodePage<0) break; // invalid language file
				*q = 0;

				Lang = ToFourCC(s+i);
				if (Lang == FOURCC('D','E','F','A'))
					Lang = LANG_DEFAULT;

				fwrite(&Lang,1,sizeof(int),out);
				fwrite(&CodePage,1,sizeof(int),out);
			}
			else
			{
				q = strchr(s+i,'=');
				if (!q || !Lang) break; // invalid language file
				*q = 0;
				++q;

				if (strlen(s+i)>4)
				{
					if (strlen(s+i)<8)
						No = ToFourCC(s+i+4);
					else
					{
						No = ToInt(s+i+4,1);
						if (No >= 32768)
							No -= 65536;
					}
				}
				else
					No = 0;

				Filter(q);
				Id = ToFourCC(s+i);
				fwrite(&Id,1,sizeof(int),out);
				fwrite(&No,1,sizeof(int),out);
				Len = strlen(q)+1;
				Len = (Len+sizeof(int)-1) & ~(sizeof(int)-1);
				fwrite(q,1,Len,out);
			}
		}
	}

	free(s);
}

int main(int pc,const char** ps)
{
	FILE* in = fopen(ps[1],"rb");
	FILE* out = fopen(ps[2],"wb+");

	if (in && out)
	{
		int len = 0x10000;
		char* data = (char*)malloc(len);
		if (data)
		{
			len = fread(data,1,len,in);
			Convert(data,len,out);
			free(data);
		}
		fclose(in);
		fclose(out);
	}
	return 0;
}
