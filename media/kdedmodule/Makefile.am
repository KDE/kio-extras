kde_module_LTLIBRARIES = kded_mediamanager.la

INCLUDES = $(all_includes) -I../libmediacommon
kded_mediamanager_la_SOURCES = mediamanager.cpp mediamanager.skel medialist.cpp backendbase.cpp fstabbackend.cpp
kded_mediamanager_la_LDFLAGS = $(all_libraries) -module -avoid-version
kded_mediamanager_la_LIBADD = $(LIB_KSYCOCA) ../libmediacommon/libmediacommon.la

METASOURCES = AUTO

servicesdir = $(kde_servicesdir)/kded
services_DATA = mediamanager.desktop

