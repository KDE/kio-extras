INCLUDES = $(all_includes)
LDFLAGS = $(all_libraries) $(KDE_RPATH)
METASOURCES = AUTO

lib_LTLIBRARIES = kgzipfilter.la

kgzipfilter_la_SOURCES = kgzipfilter.cpp
kgzipfilter_la_LIBADD = $(LIB_KSYCOCA) $(LIBZ)
kgzipfilter_la_LDFLAGS = $(all_libraries) -module -avoid-version -no-undefined

kde_services_DATA = kgzipfilter.desktop

