SUBDIRS = libxslt

INCLUDES= $(all_includes)
LDFLAGS = $(all_libraries) $(KDE_RPATH)

####### Files

kde_module_LTLIBRARIES = kio_help.la

kio_help_la_SOURCES = kio_help.cpp main.cpp xslt.cpp
kio_help_la_LIBADD = -lkio libxslt/libxslt.la
kio_help_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN) -R/usr/local/lib
noinst_HEADERS = kio_help.h

kdelnk_DATA = help.protocol
kdelnkdir = $(kde_servicesdir)

bin_PROGRAMS = meinproc saxon
meinproc_LDADD = $(LIB_KDECORE) libxslt/libxslt.la
meinproc_SOURCES = meinproc.cpp xslt.cpp

saxon_LDADD = $(LIB_KDECORE) libxslt/libxslt.la
saxon_SOURCES = saxon.cpp xslt.cpp 

METASOURCES = AUTO

messages:
	$(XGETTEXT) *.cpp -o $(podir)/kio_help.pot

install-data-local:
	-rm -rf $(kde_datadir)/ksgmltools2/docbook
	cp -r $(srcdir)/docbook $(kde_datadir)/ksgmltools2
	-rm -rf $(kde_datadir)/ksgmltools2/customization
	cp -r $(srcdir)/customization $(kde_datadir)/ksgmltools2


