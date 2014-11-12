PLUGIN=flac
DEFS= -DFLAC__INTEGER_ONLY_LIBRARY -Iflac/flac/src/libFLAC/include -Iflac/flac/include
DEFFILE = flac/flac.def
SRCFILES_ARM  = flac/stdafx.c
SRCFILES_ARM += flac/flac.c
SRCFILES_ARM += flac/flac/src/libFLAC/bitbuffer.c
SRCFILES_ARM += flac/flac/src/libFLAC/bitmath.c
SRCFILES_ARM += flac/flac/src/libFLAC/cpu.c
SRCFILES_ARM += flac/flac/src/libFLAC/crc.c
SRCFILES_ARM += flac/flac/src/libFLAC/fixed.c
SRCFILES_ARM += flac/flac/src/libFLAC/float.c
SRCFILES_ARM += flac/flac/src/libFLAC/format.c
SRCFILES_ARM += flac/flac/src/libFLAC/lpc.c
SRCFILES_ARM += flac/flac/src/libFLAC/md5.c
SRCFILES_ARM += flac/flac/src/libFLAC/memory.c
SRCFILES_ARM += flac/flac/src/libFLAC/metadata_object.c
SRCFILES_ARM += flac/flac/src/libFLAC/seekable_stream_decoder.c
SRCFILES_ARM += flac/flac/src/libFLAC/stream_decoder.c

SPECDIR = flac/flac/src/libFLAC/
SPECFLAGS = -w

include plugin.inc
