## Makefile.am of kdebase/kioslave/man

INCLUDES= $(all_includes)
LDFLAGS = $(all_libraries) $(KDE_RPATH)

####### Files

lib_LTLIBRARIES = kio_nfs.la

kio_nfs_la_SOURCES = kio_nfs.cpp mount_xdr.cpp nfs_prot_xdr.cpp
kio_nfs_la_LIBADD = -lkio
kio_nfs_la_LDFLAGS = -module -avoid-version -no-undefined
noinst_HEADERS = nfs_prot.h mount.h

kioconfig_DATA = nfs.desktop
kioconfigdir = $(kde_confdir)/protocols

METASOURCES = AUTO


bin_SCRIPTS = 
