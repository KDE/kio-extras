## Makefile.am of kdebase/kioslave/info

INCLUDES = $(all_includes)

METASOURCES = AUTO

####### Files

kde_module_LTLIBRARIES = kio_info.la

kio_info_la_SOURCES = info.cc
kio_info_la_LIBADD = $(LIB_KIO)
kio_info_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
noinst_HEADERS = info.h

kdelnk_DATA = info.protocol
kdelnkdir = $(kde_servicesdir)

kio_info_data_DATA = kde-info2html.conf
kio_info_data_SCRIPTS = kde-info2html
kio_info_datadir = $(kde_datadir)/kio_info
