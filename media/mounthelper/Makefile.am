bin_PROGRAMS = kio_media_mounthelper

INCLUDES = -I$(srcdir)/../libmediacommon $(all_includes)
AM_LDFLAGS = $(all_libraries)

kio_media_mounthelper_SOURCES = kio_media_mounthelper.cpp

kio_media_mounthelper_LDFLAGS = $(KDE_RPATH) $(all_libraries)
kio_media_mounthelper_LDADD = $(LIB_KIO) ../libmediacommon/libmediacommon.la

METASOURCES = AUTO

