## Makefile.am of kdebase/kioslave/man

INCLUDES= $(all_includes)
AM_LDFLAGS = $(all_libraries) $(KDE_RPATH)

####### Files

kde_module_LTLIBRARIES = kio_nfs.la

kio_nfs_la_SOURCES = kio_nfs.cpp mount_xdr.c nfs_prot_xdr.c
kio_nfs_la_LIBADD = -lkio $(LIBRPC)
kio_nfs_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
noinst_HEADERS = nfs_prot.h mount.h

kdelnk_DATA = nfs.protocol
kdelnkdir = $(kde_servicesdir)

METASOURCES = AUTO

$(srcdir)/mount_xdr.c: $(srcdir)/mount.x
	cd $(srcdir) && rpcgen ./mount.x


$(srcdir)/nfs_prot_xdr.c: $(srcdir)/nfs_prot.x
	cd $(srcdir) && rpcgen ./nfs_prot.x

messages:
	$(XGETTEXT) *.cpp -o $(podir)/kio_nfs.pot
