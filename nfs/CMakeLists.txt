
project(kioslave-nfs)



########### next target ###############

set(kio_nfs_PART_SRCS kio_nfs.cpp mount_xdr.c nfs_prot_xdr.c )

kde4_automoc(${kio_nfs_PART_SRCS})

kde4_add_plugin(kio_nfs ${kio_nfs_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kio_nfs )

target_link_libraries(kio_nfs  ${KDE4_KDE3SUPPORT_LIBS})

install(TARGETS kio_nfs  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES nfs.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )




#original Makefile.am contents follow:

### Makefile.am of kdebase/kioslave/man
#
#INCLUDES= $(all_includes)
#AM_LDFLAGS = $(all_libraries) $(KDE_RPATH)
#
######## Files
#
#kde_module_LTLIBRARIES = kio_nfs.la
#
#kio_nfs_la_SOURCES = kio_nfs.cpp mount_xdr.c nfs_prot_xdr.c
#kio_nfs_la_LIBADD = -lkio $(LIBRPC)
#kio_nfs_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
#noinst_HEADERS = nfs_prot.h mount.h
#
#kdelnk_DATA = nfs.protocol
#kdelnkdir = $(kde_servicesdir)
#
#METASOURCES = AUTO
#
#$(srcdir)/mount_xdr.c: $(srcdir)/mount.x
#	cd $(srcdir) && rpcgen ./mount.x
#
#
#$(srcdir)/nfs_prot_xdr.c: $(srcdir)/nfs_prot.x
#	cd $(srcdir) && rpcgen ./nfs_prot.x
#
#messages:
#	$(XGETTEXT) *.cpp -o $(podir)/kio_nfs.pot
