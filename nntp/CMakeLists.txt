
project(kioslave-nntp)



########### next target ###############

set(kio_nntp_PART_SRCS nntp.cpp )

kde4_automoc(${kio_nntp_PART_SRCS})

kde4_add_plugin(kio_nntp ${kio_nntp_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kio_nntp )

target_link_libraries(kio_nntp  ${KDE4_KIO_LIBS}  )

install_targets(${PLUGIN_INSTALL_DIR} kio_nntp )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR} FILES nntp.protocol nntps.protocol )




#original Makefile.am contents follow:

#INCLUDES= -I$(srcdir)/../.. -I$(srcdir)/.. $(all_includes)
#
######## Files
#
#kde_module_LTLIBRARIES = kio_nntp.la
#
#kio_nntp_la_SOURCES = nntp.cpp
#kio_nntp_la_LIBADD  = $(LIB_KIO)
#kio_nntp_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
#
#METASOURCES = AUTO
#
#noinst_HEADERS = nntp.h
#
#kdelnk_DATA = nntp.protocol nntps.protocol
#kdelnkdir = $(kde_servicesdir)
#
#messages:
#	$(XGETTEXT) *.cpp -o $(podir)/kio_nntp.pot
