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
 * $Id: format_subtitle.c 271 2005-08-09 08:31:35Z picard $
 *
 * The Core Pocket Media Player
 * Copyright (c) 2004-2005 Gabor Kovacs
 *
 ****************************************************************************/

#include "common.h"

typedef struct format_subtitleitem
{
	bool_t Allocated;
	tick_t RefTime;
	struct format_subtitleitem* Next;
	char* s;

} format_subtitleitem;

typedef struct format_subtitle
{
	format_stream s;
	format_subtitleitem* List;
	format_subtitleitem* Pos;

} format_subtitle;

format_stream* Format_LoadSubTitle(format_base* p, stream* Input)
{
	buffer m = {NULL};
	BufferAlloc(&m,4096,1);
	BufferStream(&m,Input);

	//!SUBTITLE
	/*				
		p->SubTitle = (format_stream*) CAlloc(sizeof(format_stream),1);
		if (p->SubTitle)
		{
			*int* Format = (int*)Format_StreamFormat(p->SubTitle,PACKET_SUBTITLE,sizeof(int));
			if (Format)
				*Format = STRING_OEM;
			p->SubTitle->LastTime = -1;
		}
	*/

	BufferClear(&m);
	return NULL;
}

void Format_FreeSubTitle(format_base* p,format_stream* s)
{
	format_subtitleitem* i;
	format_subtitleitem* Last=NULL;
	for (i=((format_subtitle*)s)->List;i;i=i->Next)
		if (i->Allocated)
		{
			free(Last);
			Last = i; 
		}
	free(Last);

	PacketFormatClear(&s->Format);
	free(s);
}

void Format_ProcessSubTitle(format_base* p,format_stream* s)
{
	if (!s->Pending)
	{
		format_subtitleitem* Last;
		format_subtitle* Stream = (format_subtitle*)s;
		tick_t Time = p->ProcessTime;

		if (Time<0)
		{
			int No;
			// benchmark: find first video stream with valid LastTime
			for (No=0;No<p->StreamCount;++No)
				if (p->Streams[No]->Format.Type == PACKET_VIDEO &&
					p->Streams[No]->LastTime >= 0)
				{
					Time = p->Streams[No]->LastTime;
					break;
				}

			if (Time<0)
				return;
		}

		if (Stream->s.LastTime<0)
			Stream->Pos = Stream->List;
		Stream->s.LastTime = Time;

		Last = NULL;
		while (Stream->Pos && Stream->Pos->RefTime<Time)
		{
			Last=Stream->Pos;
			Stream->Pos = Stream->Pos->Next;
		}

		if (!Last)
			return;

		Stream->s.Packet.RefTime = Last->RefTime;
		Stream->s.Packet.Key = 1;
		Stream->s.Packet.Data[0] = Last->s;
		Stream->s.Packet.Length = strlen(Last->s);
		Stream->s.Pending = 1;
	}

	s->State.CurrTime = p->ProcessTime;
	if (s->Process(s->Pin.Node,&s->Packet,&s->State) != ERR_BUFFER_FULL)
		s->Pending = 0;
}


