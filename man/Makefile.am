## Makefile.am of kdebase/kioslave/man

INCLUDES= $(all_includes)
LDFLAGS = $(all_libraries) $(KDE_RPATH)

EXTRA_PROGRAMS = kio_man_test man2html

####### just for testing (j.habenicht@europemail.com, 15.02.2001)

kio_man_test_SOURCES = kio_man_test.cpp
kio_man_test_LDFLAGS = $(all_libraries) $(KDE_RPATH)
kio_man_test_LDADD = man2html.lo kio_man.lo $(LIB_KFILE) $(LIB_KDEUI) $(LIB_KDECORE) $(LIB_QT)

####### Files

kde_module_LTLIBRARIES = kio_man.la  libkmanpart.la

kio_man_la_SOURCES = man2html.cpp kio_man.cpp
kio_man_la_LIBADD = $(LIB_KSYCOCA)
kio_man_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
noinst_HEADERS = kio_man.h

libkmanpart_la_SOURCES = kmanpart.cpp
libkmanpart_la_LIBADD = -lkhtml $(LIB_KPARTS)
libkmanpart_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN)

kdelnk_DATA = man.protocol kmanpart.desktop
kdelnkdir = $(kde_servicesdir)

METASOURCES = AUTO

messages:
	$(XGETTEXT) -C *.cpp *.h -o $(podir)/kio_man.pot

man2html_SOURCES = dummy.cpp
man2html_LDADD = man2html_simple.o $(LIB_QT)
man2html_LDFLAGS = $(all_libraries) $(KDE_RPATH)

dummy.cpp:
	echo > $@

man2html_simple.o: $(srcdir)/man2html.cpp
	-rm -f man2html_simple.cpp
	$(LN_S) $(srcdir)/man2html.cpp man2html_simple.cpp
	$(CXX) $(DEFS) $(DEFAULT_INCLUDES) -DSIMPLE_MAN2HTML $(INCLUDES) $(CPPFLAGS) $(CXXFLAGS) -c man2html_simple.cpp

