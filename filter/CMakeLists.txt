
project(kioslave-filter)


macro_optional_find_package(BZip2)

########### next target ###############

set(kio_filter_PART_SRCS filter.cc )

kde4_automoc(${kio_filter_PART_SRCS})

kde4_add_plugin(kio_filter ${kio_filter_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kio_filter )

target_link_libraries(kio_filter  ${KDE4_KIO_LIBS})

install(TARGETS kio_filter  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES gzip.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )
if(BZIP2_FOUND)
install( FILES bzip.protocol bzip2.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )
endif(BZIP2_FOUND)

