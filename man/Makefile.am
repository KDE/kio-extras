## Makefile.am of kdebase/kioslave/man

INCLUDES= $(all_includes)
LDFLAGS = $(all_libraries) $(KDE_RPATH)

####### Files

lib_LTLIBRARIES = kio_man.la

kio_man_la_SOURCES = kio_man.cpp
kio_man_la_LIBADD = -lkio
kio_man_la_LDFLAGS = -module -avoid-version -no-undefined
noinst_HEADERS = kio_man.h

kdelnk_DATA = man.protocol
kdelnkdir = $(kde_servicesdir)

METASOURCES = AUTO


bin_SCRIPTS = man2html


messages:
	$(XGETTEXT) -C *.cpp *.h -o $(podir)/kio_man.pot
