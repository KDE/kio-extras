kde4_header()

add_subdirectory( kcmcgi ) 

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### next target ###############

set(kio_cgi_PART_SRCS cgi.cpp )

kde4_automoc(${kio_cgi_PART_SRCS})

kde4_add_plugin(kio_cgi ${kio_cgi_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kio_cgi )

target_link_libraries(kio_cgi  ${QT_AND_KDECORE_LIBS} kio )

install_targets(${LIB_INSTALL_DIR}/kde4 kio_cgi )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR} FILES cgi.protocol )

kde4_footer()



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
