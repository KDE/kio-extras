
find_package(BZip2 QUIET)
set_package_properties(BZip2 PROPERTIES DESCRIPTION "A high-quality data compressor"
                       URL "http://www.bzip.org"
                       TYPE OPTIONAL
                       PURPOSE "Provides the ability to read and write bzip2 compressed data files in the filter kioslave."
                      )

find_package(LibLZMA QUIET)
set_package_properties(LibLZMA PROPERTIES DESCRIPTION "A very high compression ratio data compressor"
                       URL "http://tukaani.org/xz/"
                       TYPE OPTIONAL
                       PURPOSE "Provides the ability to read and write xz compressed data files."
                      )


add_library(kio_filter MODULE filter.cc)
target_link_libraries(kio_filter KF5::KDE4Support KF5::Archive KF5::KIOCore)

install(TARGETS kio_filter DESTINATION ${PLUGIN_INSTALL_DIR})
install( FILES gzip.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )

if(BZIP2_FOUND)
	install( FILES bzip.protocol bzip2.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )
endif()

if(LIBLZMA_FOUND)
	install( FILES lzma.protocol xz.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )
endif()
