## Makefile.am of kdebase/kioslave/man

INCLUDES= $(all_includes)
LDFLAGS = $(all_libraries) $(KDE_RPATH)

####### Files

lib_LTLIBRARIES = kio_help.la

kio_help_la_SOURCES = kio_help.cpp
kio_help_la_LIBADD = -lkio
kio_help_la_LDFLAGS = -module -avoid-version -no-undefined
noinst_HEADERS = kio_help.h

kdelnk_DATA = help.protocol
kdelnkdir = $(kde_servicesdir)

METASOURCES = AUTO
