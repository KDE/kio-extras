## Makefile.am of kdebase/kioslave/man

INCLUDES= $(all_includes)
LDFLAGS = $(all_libraries) $(KDE_RPATH)

####### Files

lib_LTLIBRARIES = kio_man.la

kio_man_la_SOURCES = kio_man.cpp
kio_man_la_LIBADD = -lkio
kio_man_la_LDFLAGS = -module -avoid-version -no-undefined
noinst_HEADERS = kio_man.h

kioconfig_DATA = man.desktop
kioconfigdir = $(kde_confdir)/protocols

METASOURCES = AUTO


bin_SCRIPTS = man2html
