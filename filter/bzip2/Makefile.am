INCLUDES = $(all_includes)
LDFLAGS = $(all_libraries) $(KDE_RPATH)
METASOURCES = AUTO

lib_LTLIBRARIES = kbzip2filter.la

kbzip2filter_la_SOURCES = kbzip2filter.cpp
kbzip2filter_la_LIBADD = $(LIB_KSYCOCA) $(LIBBZ2)
kbzip2filter_la_LDFLAGS = $(all_libraries) -module -avoid-version -no-undefined

kde_services_DATA = kbzip2filter.desktop

protocoldir = $(kde_servicesdir)
protocol_DATA = bzip.protocol bzip2.protocol

