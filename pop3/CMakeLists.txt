kde4_header()

macro_optional_find_package(Sasl2)

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${SASL2_INCLUDE_DIR} )

########### next target ###############

set(kio_pop3_PART_SRCS pop3.cc )

kde4_automoc(${kio_pop3_PART_SRCS})

kde4_add_plugin(kio_pop3 ${kio_pop3_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kio_pop3 )

target_link_libraries(kio_pop3  ${LIB_KIO} ${SASL2_LIBRARIES})

install_targets(${LIB_INSTALL_DIR}/kde4 kio_pop3 )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR} FILES pop3.protocol pop3s.protocol )

kde4_footer()



#original Makefile.am contents follow:

#INCLUDES= -I$(srcdir)/../.. -I$(srcdir)/.. $(SSL_INCLUDES) $(all_includes)
#
######## Files
#
#kde_module_LTLIBRARIES = kio_pop3.la
#
#kio_pop3_la_SOURCES = pop3.cc
#kio_pop3_la_LIBADD  = $(LIB_KIO) $(SASL2_LIBS)
#kio_pop3_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
#
#noinst_HEADERS = pop3.h
#
#kdelnk_DATA = pop3.protocol pop3s.protocol
#kdelnkdir = $(kde_servicesdir)
#
#messages:
#	$(XGETTEXT) *.cc -o $(podir)/kio_pop3.pot
