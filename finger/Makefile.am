## Makfile.am for kio_finger
## Edit from Makefile.am of kdebase/kioslave/man

INCLUDES= $(all_includes)
LDFLAGS = $(all_libraries) $(KDE_RPATH)

####### Files

bin_PROGRAMS = YAfinger

YAfinger_SOURCES = YAfinger.cpp KDEfinger.cpp fingerSocket.cpp
YAfinger_LDFLAGS = $(all_libraries) $(KDE_RPATH)
YAfinger_LDADD = $(LIB_KFILE)   

lib_LTLIBRARIES = kio_finger.la

kio_finger_la_SOURCES = kio_finger.cpp 
kio_finger_la_LIBADD = -lkio
kio_finger_la_LDFLAGS = -module -avoid-version -no-undefined

noinst_HEADERS = kio_finger.h KDEfinger.h fingerSocket.h

kdelnk_DATA = finger.protocol
kdelnkdir = $(kde_servicesdir)

data_DATA = finger.pl fingerHead fingerTail
datadir = $(kde_datadir)/kfinger/kio_finger
EXTRA_DIST=$(data_DATA)   

METASOURCES = AUTO
