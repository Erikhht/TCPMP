PLUGIN=vorbishq
DEFS= -D BIG_ENDIAN=0 -D BYTE_ORDER=1 -D LITTLE_ENDIAN=1 
DEFS+= -D _ARM_ASSEM_
DEFFILE = vorbishq/vorbishq.def
SRCFILES_ARM  = vorbislq/stdafx.c
SRCFILES_ARM += vorbislq/ogg.c 
SRCFILES_ARM += vorbislq/oggembedded.c 
SRCFILES_ARM += vorbislq/oggpacket.c 
SRCFILES_ARM += vorbislq/vorbis.c
SRCFILES_ARM += vorbislq/tremor/bitwise.c 
SRCFILES_ARM += vorbislq/tremor/block.c 
SRCFILES_ARM += vorbislq/tremor/codebook.c
SRCFILES_ARM += vorbislq/tremor/floor0.c 
SRCFILES_ARM += vorbislq/tremor/floor1.c 
SRCFILES_ARM += vorbislq/tremor/framing.c
SRCFILES_ARM += vorbislq/tremor/info.c 
SRCFILES_ARM += vorbislq/tremor/mapping0.c 
SRCFILES_ARM += vorbislq/tremor/mdctvorbis.c
SRCFILES_ARM += vorbislq/tremor/registry.c 
SRCFILES_ARM += vorbislq/tremor/res012.c 
SRCFILES_ARM += vorbislq/tremor/sharedbook.c
SRCFILES_ARM += vorbislq/tremor/synthesis.c 
SRCFILES_ARM += vorbislq/tremor/window.c 

SPECDIR = vorbislq/tremor/
SPECFLAGS = -w

include plugin.inc
