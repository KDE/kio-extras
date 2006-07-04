
project(kioslave-cgi)

add_subdirectory( kcmcgi ) 



########### next target ###############

set(kio_cgi_PART_SRCS cgi.cpp )

kde4_automoc(${kio_cgi_PART_SRCS})

kde4_add_plugin(kio_cgi ${kio_cgi_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kio_cgi )

target_link_libraries(kio_cgi  ${KDE4_KIO_LIBS} )

install(TARGETS kio_cgi  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES cgi.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )




#original Makefile.am contents follow:

#SUBDIRS = kcmcgi
#
#INCLUDES = $(all_includes)
#
#kde_module_LTLIBRARIES = kio_cgi.la
#
#kio_cgi_la_SOURCES = cgi.cpp
#kio_cgi_la_LIBADD = $(LIB_KIO)
#kio_cgi_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
#
#noinst_HEADERS = cgi.h
#
#METASOURCES = AUTO
#
#kdelnkdir = $(kde_servicesdir)
#kdelnk_DATA = cgi.protocol
