/*
 * Copyright (c) 2004-2005 Mike Matsnev.  All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice immediately at the beginning of the file, without modification,
 *    this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Absolutely no warranty of function or purpose is made by the author
 *    Mike Matsnev.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * $Id: dumptrack.c,v 1.3 2005/03/07 11:24:21 mike Exp $
 * 
 */

/*
 * This is a complete sample of MatroskaParser usage. The program will dump
 * one of source file tracks to stdout.
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

/* windows-specific */
#ifdef WIN32
#include <io.h>
#endif

#include "../MatroskaParser/MatroskaParser.h"

/* first we need to create an I/O object that the parser will use to read the
 * source file
 */
struct StdIoStream {
  struct InputStream  base;
  FILE		      *fp;
  int		      error;
};
typedef struct StdIoStream StdIoStream;

#define	CACHESIZE     65536

/* StdIoStream methods */

/* read count bytes into buffer starting at file position pos
 * return the number of bytes read, -1 on error or 0 on EOF
 */
int   StdIoRead(StdIoStream *st, ulonglong pos, void *buffer, int count) {
  size_t  rd;
  if (fseek(st->fp, pos, SEEK_SET)) {
    st->error = errno;
    return -1;
  }
  rd = fread(buffer, 1, count, st->fp);
  if (rd == 0) {
    if (feof(st->fp))
      return 0;
    st->error = errno;
    return -1;
  }
  return rd;
}

/* scan for a signature sig(big-endian) starting at file position pos
 * return position of the first byte of signature or -1 if error/not found
 */
longlong StdIoScan(StdIoStream *st, ulonglong start, unsigned signature) {
  int	      c;
  unsigned    cmp = 0;
  FILE	      *fp = st->fp;

  if (fseek(fp, start, SEEK_SET))
    return -1;

  while ((c = getc(fp)) != EOF) {
    cmp = ((cmp << 8) | c) & 0xffffffff;
    if (cmp == signature)
      return ftell(fp) - 4;
  }

  return -1;
}

/* return cache size, this is used to limit readahead */
unsigned StdIoGetCacheSize(StdIoStream *st) {
  return CACHESIZE;
}

/* return last error message */
const char *StdIoGetLastError(StdIoStream *st) {
  return strerror(st->error);
}

/* memory allocation, this is done via stdlib */
void  *StdIoMalloc(StdIoStream *st, size_t size) {
  return malloc(size);
}

void  *StdIoRealloc(StdIoStream *st, void *mem, size_t size) {
  return realloc(mem,size);
}

void  StdIoFree(StdIoStream *st, void *mem) {
  free(mem);
}

/* progress report handler for lengthy operations
 * returns 0 to abort operation, nonzero to continue
 */
int   StdIoProgress(StdIoStream *st, ulonglong cur, ulonglong max) {
  return 1;
}

/* main program */
int   main(int argc,char **argv) {
  StdIoStream	st;
  MatroskaFile	*mf;
  char		err_msg[256];
  unsigned	i;
  int		track;
  FILE		*out = stdout;

  if (argc < 2 || argc > 3) {
    fprintf(stderr,"Usage: %s filename.mkv [track_num]\n",argv[0]);
    return 1;
  }

#ifdef WIN32
  { /* windows-specific, turn off text translation */
    int nfd = dup(fileno(stdout));
    _setmode(nfd, _O_BINARY);
    out = _fdopen(nfd,"wb");
  }
#endif

  /* fill in I/O object */
  memset(&st,0,sizeof(st));
  st.base.read = StdIoRead;
  st.base.scan = StdIoScan;
  st.base.getsize = StdIoGetCacheSize;
  st.base.geterror = StdIoGetLastError;
  st.base.memalloc = StdIoMalloc;
  st.base.memrealloc = StdIoRealloc;
  st.base.memfree = StdIoFree;
  st.base.progress = StdIoProgress;

  /* open source file */
  st.fp = fopen(argv[1],"rb");
  if (st.fp == NULL) {
    fprintf(stderr,"Can't open '%s': %s\n",argv[1],strerror(errno));
    return 1;
  }
  setvbuf(st.fp, NULL, _IOFBF, CACHESIZE);

  /* initialize matroska parser */
  mf = mkv_OpenEx(&st.base, /* pointer to I/O object */
    0, /* starting position in the file */
    0, /* flags, you can set MKVF_AVOID_SEEKS if this is a non-seekable stream */
    err_msg, sizeof(err_msg)); /* error message is returned here */
  if (mf == NULL) {
    fclose(st.fp);
    fprintf(stderr,"Can't parse Matroska file: %s\n", err_msg);
    return 1;
  }

  /* if track_num was not specified, then list tracks */
  if (argc == 2) {
    for (i = 0; i < mkv_GetNumTracks(mf); ++i) {
      TrackInfo	*ti = mkv_GetTrackInfo(mf, i);
      printf("Track %u: %s\n", i, ti->CodecID);
    }
  } else {
    ulonglong	      StartTime, EndTime, FilePos;
    unsigned	      rt, FrameSize, FrameFlags;
    unsigned	      fb = 0;
    void	      *frame = NULL;
    CompressedStream  *cs = NULL;

    /* extract track */
    track = atoi(argv[2]);
    if (track < 0 || (unsigned)track >= mkv_GetNumTracks(mf)) {
      fprintf(stderr,"Invalid track number: %d\n", track);
      goto done;
    }

    /* mask other tracks because we don't need them */
    mkv_SetTrackMask(mf, ~(1 << track));

    /* init zlib decompressor if needed */
    if (mkv_GetTrackInfo(mf,track)->CompEnabled) {
      cs = cs_Create(mf, track, err_msg, sizeof(err_msg));
      if (cs == NULL) {
	fprintf(stderr,"Can't create decompressor: %s\n",err_msg);
	goto done;
      }
    }

    /* read frames from file */
    while (mkv_ReadFrame(mf, 0, &rt, &StartTime, &EndTime, &FilePos, &FrameSize, &FrameFlags) == 0)
    {
      if (cs) {
	char	buffer[1024];

	cs_NextFrame(cs,FilePos,FrameSize);
	for (;;) {
	  int rd = cs_ReadData(cs,buffer,sizeof(buffer));
	  if (rd < 0) {
	    fprintf(stderr,"Error decompressing data: %s\n",cs_GetLastError(cs));
	    goto done;
	  }
	  if (rd == 0)
	    break;
	  fwrite(buffer,1,rd,out);
	}
      } else {
	size_t	  rd;

	if (fseek(st.fp, FilePos, SEEK_SET)) {
	  fprintf(stderr,"fseek(): %s\n", strerror(errno));
	  goto done;
	}

	if (fb < FrameSize) {
	  fb = FrameSize;
	  frame = realloc(frame, fb);
	  if (frame == NULL) {
	    fprintf(stderr,"Out of memory\n");
	    goto done;
	  }
	}

	rd = fread(frame,1,FrameSize,st.fp);
	if (rd != FrameSize) {
	  if (rd == 0) {
	    if (feof(st.fp))
	      fprintf(stderr,"Unexpected EOF while reading frame\n");
	    else
	      fprintf(stderr,"Error reading frame: %s\n",strerror(errno));
	  } else
	    fprintf(stderr,"Short read while reading frame\n");
	  goto done;
	}

	fwrite(frame,1,FrameSize,out);
      }
    }
  }

done:
  /* close matroska parser */
  mkv_Close(mf);

  /* close file */
  fclose(st.fp);

  return 0;
}
