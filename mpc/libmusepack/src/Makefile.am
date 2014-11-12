## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

INCLUDES = -I$(top_builddir)/include

lib_LTLIBRARIES = libmusepack.la

libmusepack_la_SOURCES = \
	huffsv46.c \
	huffsv7.c \
	idtag.c \
	mpc_decoder.c \
	mpc_reader.c \
	requant.c \
	streaminfo.c \
	synth_filter.c
libmusepack_la_LDFLAGS = -no-undefined -version-info 2:0:0

noinst_PROGRAMS = sample
sample_SOURCES = sample.cpp
sample_LDADD = libmusepack.la

