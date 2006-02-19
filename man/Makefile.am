## Makefile.am of kdebase/kioslave/man

INCLUDES= $(all_includes)
AM_LDFLAGS = $(all_libraries) $(KDE_RPATH)

EXTRA_PROGRAMS = kio_man_test man2html

####### just for testing (j.habenicht@europemail.com, 15.02.2001)

kio_man_test_SOURCES = kio_man_test.cpp
kio_man_test_LDFLAGS = $(all_libraries) $(KDE_RPATH)
kio_man_test_LDADD = man2html.lo kio_man.lo $(LIB_KIO) $(LIB_KDEUI) $(LIB_KDECORE) $(LIB_QT)

####### Files

kde_module_LTLIBRARIES = kio_man.la  libkmanpart.la

kio_man_la_SOURCES = man2html.cpp kio_man.cpp
kio_man_la_LIBADD = $(LIB_KSYCOCA)
kio_man_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
noinst_HEADERS = kio_man.h
### TODO Why is man2htmk.h distributed?

libkmanpart_la_SOURCES = kmanpart.cpp
libkmanpart_la_LIBADD = -lkhtml $(LIB_KPARTS)
libkmanpart_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN)

kdelnk_DATA = man.protocol kmanpart.desktop
kdelnkdir = $(kde_servicesdir)

kio_man_data_DATA = kio_man.css
kio_man_datadir = $(kde_datadir)/kio_man
EXTRA_DIST=$(kio_man_data_DATA)

METASOURCES = AUTO

messages:
	$(XGETTEXT) *.cpp *.h -o $(podir)/kio_man.pot

man2html_SOURCES = dummy.cpp
man2html_LDADD = man2html_simple.o $(LIB_QT)
man2html_LDFLAGS = $(all_libraries) $(KDE_RPATH)

dummy.cpp:
	echo > $@

man2html_simple.o: $(srcdir)/man2html.cpp
	-rm -f man2html_simple.cpp
	$(LN_S) $(srcdir)/man2html.cpp man2html_simple.cpp
	$(CXX) $(DEFS) $(DEFAULT_INCLUDES) -DSIMPLE_MAN2HTML $(INCLUDES) $(CPPFLAGS) $(CXXFLAGS) -c man2html_simple.cpp

