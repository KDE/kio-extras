## Makefile.am of kdebase/kioslave/man

INCLUDES= $(all_includes)
LDFLAGS = $(all_libraries) $(KDE_RPATH)

####### Files

lib_LTLIBRARIES = kio_man.la

kio_man_la_SOURCES = kio_man.cpp man2html.cpp
kio_man_la_LIBADD = -lksycoca
kio_man_la_LDFLAGS = -module -avoid-version -no-undefined
noinst_HEADERS = kio_man.h

kdelnk_DATA = man.protocol
kdelnkdir = $(kde_servicesdir)

METASOURCES = AUTO

messages:
	$(XGETTEXT) -C *.cpp *.h -o $(podir)/kio_man.pot

EXTRA_PROGRAMS = man2html_simple
man2html_simple_SOURCES = man2html_simple.cpp
man2html_simple_LDADD = $(LIB_QT)
man2html_simple_LDFLAGS = $(all_libraries) $(KDE_RPATH)

man2html: man2html.cpp
	$(MAKE) DEFS="-DSIMPLE_MAN2HTML $(DEFS)" man2html_simple
	-rm -f man2html
	$(LN_S) man2html_simple man2html

man2html_simple.cpp: 
	$(LN_S) $(srcdir)/man2html.cpp man2html_simple.cpp
