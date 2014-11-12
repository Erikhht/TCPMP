PLUGIN=asap
DEFS= -D ASAP -Iasap -Iasap/atari800/src -Iasap/asap -D RAND_MAX=0x7FFF
DEFFILE = asap/asap.def
SRCFILES_ARM  = asap/stdafx.c 
SRCFILES_ARM += asap/asapfmt.c 
SRCFILES_ARM += asap/asap/asap.c 
SRCFILES_ARM += asap/atari800/src/cpu.c 
SRCFILES_ARM += asap/atari800/src/mzpokeysnd.c 
SRCFILES_ARM += asap/atari800/src/pokeysnd.c 
#SRCFILES_ARM += asap/atari800/src/remez.c 

SPECDIR = asap/atari800/src
SPECFLAGS = -w

include plugin.inc
