## Makefile.am of kdebase/kioslave/floppy

INCLUDES= $(all_includes)
LDFLAGS = $(all_libraries) $(KDE_RPATH)

####### Files

lib_LTLIBRARIES = kio_floppy.la

kio_floppy_la_SOURCES = kio_floppy.cpp program.cpp
kio_floppy_la_LIBADD = -lkio
kio_floppy_la_LDFLAGS = -module -avoid-version -no-undefined
noinst_HEADERS = 

kdelnk_DATA = floppy.protocol
kdelnkdir = $(kde_servicesdir)

METASOURCES = AUTO

bin_SCRIPTS = 
