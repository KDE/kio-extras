bin_PROGRAMS = kio_media_mounthelper

INCLUDES = $(all_includes) -I$(srcdir)/../libmediacommon
AM_LDFLAGS = $(all_libraries)

kio_media_mounthelper_SOURCES = kio_media_mounthelper.cpp

kio_media_mounthelper_LDFLAGS = $(KDE_RPATH) $(LIB_KDECORE) $(LIB_KDEUI) $(LIB_KSYCOCA) $(LIB_KIO) $(all_libraries)
kio_media_mounthelper_LDADD = ../libmediacommon/libmediacommon.la

METASOURCES = AUTO

