
project(kioslave-filter)


macro_optional_find_package(BZip2)

########### next target ###############

set(kio_filter_PART_SRCS filter.cc )

kde4_automoc(${kio_filter_PART_SRCS})

kde4_add_plugin(kio_filter ${kio_filter_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kio_filter )

target_link_libraries(kio_filter  ${KDE4_KIO_LIBS})

install_targets(${PLUGIN_INSTALL_DIR} kio_filter )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR} FILES gzip.protocol )
if(BZIP2_FOUND)
	install_files( ${SERVICES_INSTALL_DIR} FILES bzip.protocol bzip2.protocol )
endif(BZIP2_FOUND)




#original Makefile.am contents follow:

#KDE_CPPFLAGS = -DQT_NO_CAST_ASCII
#
#INCLUDES = $(all_includes)
#
#kde_module_LTLIBRARIES = kio_filter.la
#
#kio_filter_la_SOURCES = filter.cc
#kio_filter_la_LIBADD = $(LIB_KSYCOCA)
#kio_filter_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
#noinst_HEADERS = filter.h
#
#METASOURCES = AUTO
#
#protocoldir = $(kde_servicesdir)
#
#if include_BZIP2
#BZIP2FILES=bzip.protocol bzip2.protocol
#endif
#
#protocol_DATA = gzip.protocol $(BZIP2FILES)
#
