ACLOCAL_AMFLAGS = -I m4
SUBDIRS = .
pkglib_LTLIBRARIES =
CORE = ../core

AM_CFLAGS = -I$(srcdir)/$(CORE)
COMMON_LIBADD = $(CORE)/libme_core.la

if ENABLE_KMD
libconsumer_kmd_la_SOURCES = consumer_kmd.c
libconsumer_kmd_la_CFLAGS = $(AM_CFLAGS) @OVIS_LIB_INCDIR_FLAG@
libconsumer_kmd_la_LIBADD = $(COMMON_LIBADD) @OVIS_LIB_LIBDIR_FLAG@ -lzap
pkglib_LTLIBRARIES += libconsumer_kmd.la
endif
