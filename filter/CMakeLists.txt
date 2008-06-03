
macro_optional_find_package(BZip2)
macro_log_feature(BZIP2_FOUND "BZip2" "A high-quality data compressor" "http://www.bzip.org" FALSE "" "Provides the ability to read and write bzip2 compressed data files in the filter kioslave.")

########### next target ###############

set(kio_filter_PART_SRCS filter.cc )


kde4_add_plugin(kio_filter ${kio_filter_PART_SRCS})


target_link_libraries(kio_filter  ${KDE4_KIO_LIBS})

install(TARGETS kio_filter  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES gzip.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )
if(BZIP2_FOUND)
install( FILES bzip.protocol bzip2.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )
endif(BZIP2_FOUND)

