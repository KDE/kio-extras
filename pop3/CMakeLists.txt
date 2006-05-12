kde4_header()

project(kioslave-pop3)

set(pop3_optional_includes)
set(pop3_optional_libs)

macro_optional_find_package(Sasl2)

if(SASL2_FOUND)
   set(pop3_optional_includes ${pop3_optional_includes} ${SASL2_INCLUDE_DIR})
   set(pop3_optional_libs     ${pop3_optional_libs}     ${SASL2_LIBRARIES})
   set(HAVE_LIBSASL2 1)
else(SASL2_FOUND)
   set(HAVE_LIBSASL2 0)
endif(SASL2_FOUND)

configure_file(pop3-config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/pop3-config.h )

include_directories(  ${pop3_optional_includes}  )

########### next target ###############

set(kio_pop3_PART_SRCS pop3.cc )

kde4_automoc(${kio_pop3_PART_SRCS})

kde4_add_plugin(kio_pop3 ${kio_pop3_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kio_pop3 )

target_link_libraries(kio_pop3  ${KDE4_KIO_LIBS} ${pop3_optional_libs})

install_targets(${PLUGIN_INSTALL_DIR} kio_pop3 )


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
