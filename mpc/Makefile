PLUGIN=mpc
DEFS= -DMPC_FIXED_POINT -DMPC_LITTLE_ENDIAN -Impc/libmusepack/include
DEFFILE = mpc/mpc.def
SRCFILES_ARM  = mpc/stdafx.c
SRCFILES_ARM += mpc/mpc.c
SRCFILES_ARM += mpc/libmusepack/src/huffsv46.c
SRCFILES_ARM += mpc/libmusepack/src/huffsv7.c
SRCFILES_ARM += mpc/libmusepack/src/idtag.c
SRCFILES_ARM += mpc/libmusepack/src/mpc_decoder.c
SRCFILES_ARM += mpc/libmusepack/src/requant.c
SRCFILES_ARM += mpc/libmusepack/src/streaminfo.c
SRCFILES_ARM += mpc/libmusepack/src/synth_filter.c

SPECDIR = mpc/libmusepack/src/
SPECFLAGS = -w

include plugin.inc
