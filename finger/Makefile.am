## Makfile.am for kio_finger
## Edit from Makefile.am of kdebase/kioslave/man

INCLUDES= $(all_includes)
LDFLAGS = $(all_libraries) $(KDE_RPATH)

####### Files

lib_LTLIBRARIES = kio_finger.la

kio_finger_la_SOURCES = kio_finger.cpp 
kio_finger_la_LIBADD = -lkio
kio_finger_la_LDFLAGS = -module -avoid-version -no-undefined

noinst_HEADERS = kio_finger.h 

kdelnk_DATA = finger.protocol
kdelnkdir = $(kde_servicesdir)

data_DATA = kio_finger.pl 
datadir = $(kde_datadir)/kio_finger
EXTRA_DIST=$(data_DATA)   

METASOURCES = AUTO

