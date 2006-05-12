
project(kioslave-tar)



########### next target ###############

set(kio_tar_PART_SRCS tar.cc )

kde4_automoc(${kio_tar_PART_SRCS})

kde4_add_plugin(kio_tar ${kio_tar_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kio_tar )

target_link_libraries(kio_tar  ${KDE4_KIO_LIBS} )

install_targets(${PLUGIN_INSTALL_DIR} kio_tar )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR} FILES tar.protocol ar.protocol zip.protocol )




#original Makefile.am contents follow:

### Makefile.am of kdebase/kioslave/tar
#
#INCLUDES = $(all_includes)
#AM_LDFLAGS = $(all_libraries) $(KDE_RPATH)
#METASOURCES = AUTO
#
#kde_module_LTLIBRARIES = kio_tar.la
#
#kio_tar_la_SOURCES = tar.cc
#kio_tar_la_LIBADD = $(LIB_KSYCOCA)
#kio_tar_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
#
#noinst_HEADERS = tar.h
#
#kdelnk_DATA = tar.protocol ar.protocol zip.protocol
#kdelnkdir = $(kde_servicesdir)
#
#messages:
#	$(XGETTEXT) *.cc -o $(podir)/kio_tar.pot
