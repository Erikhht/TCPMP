# Generated automatically from Makefile.in by configure.
# Makefile for prc-tools libc.
#
# This is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

srcdir = .
VPATH = .
#stop

prefix = /usr/arm-tcpmp-palmos
exec_prefix = ${prefix}
target_cpu = arm
target_alias = arm-tcpmp-palmos
tooldir = $(exec_prefix)/$(target_alias)

INSTALL = /usr/bin/install -c
INSTALL_DATA = ${INSTALL} -m 644

SDKFLAGS = -Wno-multichar -march=armv4 -fshort-enums -O3 -mno-apcs-frame -mtune=xscale -fomit-frame-pointer -fsigned-char 
MULTIFLAGS =

CC = $(target_alias)-gcc
AR = $(target_alias)-ar
RANLIB = $(target_alias)-ranlib

INSTALL_DIRS     = $(INSTALL_DIRS_arm)
INSTALL_HEADERS  = $(INSTALL_HEADERS_arm)
INSTALL_C_LIBS   = $(INSTALL_C_LIBS_arm)
INSTALL_CXX_LIBS = $(INSTALL_CXX_LIBS_arm)

INSTALL_LIBS = $(INSTALL_C_LIBS) $(INSTALL_CXX_LIBS)

LIBC_OBJS = $(LIBC_OBJS_arm)
LIBG_OBJS = $(LIBG_OBJS_arm)

# PRINT_FLOATS huh?
CFLAGS = -Wall -O2 -fno-builtin -DPRINT_FLOATS \
  -I$(srcdir)/include -I$(srcdir)/../include $(SDKFLAGS) $(MULTIFLAGS)

INCDIR = $(srcdir)/include
INSTALL_HFILES = $(INCDIR)/errno.h $(INCDIR)/malloc.h \
		 $(INCDIR)/setjmp.h $(INCDIR)/stdio.h \
		 $(INCDIR)/sys/types.h


INSTALL_DIRS_m68k     = include lib lib/mown-gp
INSTALL_HEADERS_m68k  = stdlib.h
INSTALL_C_LIBS_m68k   = libc.a mown-gp/libc.a libg.a mown-gp/libg.a
INSTALL_CXX_LIBS_m68k = libstdc++.a

LIBC_OBJS_m68k = \
	conio.o vsprintf.o printf.o setjmp.o \
	bcopy.o bzero.o \
	ctype.o isalnum.o isalpha.o isblank.o iscntrl.o isdigit.o isgraph.o \
	islower.o isprint.o ispunct.o isspace.o isupper.o isxdigit.o \
	tolower.o toupper.o \
	atoi.o atol.o atoll.o \
	_Strtoul.o _Strtoull.o strtol.o strtoll.o strtoul.o strtoull.o \
	malloc.o free.o realloc.o calloc.o \
	abort.o atexit.o \
	abs.o labs.o llabs.o div.o ldiv.o lldiv.o \
	memcpy.o memmove.o strcpy.o strncpy.o strcat.o strncat.o \
	memcmp.o strcmp.o strncmp.o memchr.o strchr.o strcspn.o strpbrk.o \
	strrchr.o strspn.o strstr.o strtok.o memset.o strlen.o

LIBG_OBJS_m68k =


INSTALL_DIRS_arm     = lib
INSTALL_HEADERS_arm  =
INSTALL_C_LIBS_arm   = libc.a
INSTALL_CXX_LIBS_arm = libstdc++.a

# The strtok() function is missing from the list of objects for ARM
# because, requiring a global, it doesn't work in an armlet.
# FIXME maybe add strtok_r() to help with this.

LIBC_OBJS_arm = \
	ctype.o isalnum.o isalpha.o isblank.o iscntrl.o isdigit.o isgraph.o \
	islower.o isprint.o ispunct.o isspace.o isupper.o isxdigit.o \
	tolower.o toupper.o \
	atoi.o atol.o atoll.o \
	_Strtoul.o _Strtoull.o strtol.o strtoll.o strtoul.o strtoull.o \
	abs.o labs.o llabs.o div.o ldiv.o lldiv.o \
	memcpy.o memmove.o strcpy.o strncpy.o strcat.o strncat.o \
	memcmp.o strcmp.o strncmp.o memchr.o strchr.o strcspn.o strpbrk.o \
	strrchr.o strspn.o strstr.o memset.o strlen.o

LIBG_OBJS_arm =


all: $(INSTALL_LIBS)

install-legacy: $(INSTALL_HFILES)
	$(INSTALL) -d $(DESTDIR)$(exec_prefix)/$(target_alias)/include
	$(INSTALL) -d $(DESTDIR)$(exec_prefix)/$(target_alias)/include/sys
	for f in $(INSTALL_HFILES); do \
	  $(INSTALL_DATA) $$f `echo $$f | sed -e 's:^$(INCDIR):$(DESTDIR)$(exec_prefix)/$(target_alias)/include:'`; \
	done

install: all install-legacy
	list="$(INSTALL_DIRS)"; for d in $$list; do \
	  $(INSTALL) -d $(DESTDIR)$(tooldir)/$$d; \
	done
	list="$(INSTALL_HEADERS)"; for f in $$list; do \
	  $(INSTALL_DATA) $(srcdir)/$(target_cpu)/$$f \
			  $(DESTDIR)$(tooldir)/include/$$f; \
	done
	list="$(INSTALL_LIBS)"; for f in $$list; do \
	  $(INSTALL_DATA) $$f $(DESTDIR)$(tooldir)/lib/$$f; \
	done

.PHONY: all install-legacy install clean


bootstrap_h = ../bootstrap/bootstrap.h ../include/NewTypes.h

ctype.o isalnum.o isalpha.o isblank.o iscntrl.o isdigit.o isgraph.o \
islower.o isprint.o ispunct.o isspace.o isupper.o isxdigit.o \
tolower.o toupper.o: ctype.c ../include/ctype.h
	$(CC) $(CFLAGS) -c -o $@ -DL`basename $@ .o` $(srcdir)/ctype.c

atoi.o atol.o _Strtoul.o \
strtol.o strtoul.o: intconv.c ../include/stdlib.h ../include/ctype.h
	$(CC) $(CFLAGS) -c -o $@ -DL`basename $@ .o` $(srcdir)/intconv.c

atoll.o _Strtoull.o \
strtoll.o strtoull.o: intconv.c ../include/stdlib.h ../include/ctype.h
	$(CC) $(CFLAGS) -c -o $@ -DL`basename $@ .o` -DLONG_LONG \
	  $(srcdir)/intconv.c

malloc.o free.o calloc.o realloc.o: malloc.c ../include/stdlib.h $(bootstrap_h)
	$(CC) $(CFLAGS) -c -o $@ -DL`basename $@ .o` $(srcdir)/malloc.c

atexit.o: atexit.c ../include/stdlib.h $(bootstrap_h)

abs.o labs.o llabs.o: abs.c ../include/stdlib.h
	$(CC) $(CFLAGS) -c -o $@ -DL`basename $@ .o` $(srcdir)/abs.c

div.o ldiv.o lldiv.o: division.c ../include/stdlib.h
	$(CC) $(CFLAGS) -c -o $@ -DL`basename $@ .o` $(srcdir)/division.c

memcpy.o memmove.o memcmp.o memchr.o memset.o: memstring.c ../include/string.h
	$(CC) $(CFLAGS) -c -o $@ -DL`basename $@ .o` $(srcdir)/memstring.c

strcpy.o strncpy.o strcat.o strncat.o strcmp.o strncmp.o \
strchr.o strcspn.o strpbrk.o strrchr.o strspn.o strstr.o \
strtok.o strlen.o: string.c ../include/string.h
	$(CC) $(CFLAGS) -c -o $@ -DL`basename $@ .o` $(srcdir)/string.c

conio.o: conio.c include/stdio.h $(bootstrap_h) ../bootstrap/bootstrap-ui.h


all-multilibs: libc.a libg.a

libc.a: $(LIBC_OBJS)
	rm -f libc.a
	$(AR) cur libc.a $(LIBC_OBJS)
	$(RANLIB) libc.a

mown-gp/libc.a: sub-multilibs

mown-gp/Makefile: Makefile
	if [ ! -d mown-gp ]; then mkdir mown-gp; fi
	sed '1,/^#stop/s,= \.,= ../.,' Makefile > mown-gp/Makefile

sub-multilibs: mown-gp/Makefile
	cd mown-gp; $(MAKE) CC="$(CC)" AR="$(AR)" RANLIB="$(RANLIB)" \
	  SDKFLAGS="$(SDKFLAGS)" MULTIFLAGS=-mown-gp all-multilibs

.PHONY: all-multilibs sub-multilibs


libg.a: $(LIBG_OBJS)
	rm -f libg.a
	$(AR) cur libg.a $(LIBG_OBJS)
	$(RANLIB) libg.a


# Yes, this library really is empty.  But we need it to exist to keep the
# linker happy if people link with m68k-palmos-g++.
libstdc++.a:
	$(AR) cur libstdc++.a
	$(RANLIB) libstdc++.a


.S.o:
	$(CC) $(TARGETFLAGS) $(SDKFLAGS) $(DEFINES) -c $<

clean:
	-rm -f *.o lib*.a
