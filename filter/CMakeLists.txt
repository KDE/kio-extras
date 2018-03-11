include(ECMQtDeclareLoggingCategory)

set(kio_filter_SRCS filter.cc)

ecm_qt_declare_logging_category(kio_filter_SRCS
                                HEADER loggingcategory.h
                                IDENTIFIER KIO_FILTER_DEBUG
                                CATEGORY_NAME kio_filter
                                DEFAULT_SEVERITY Debug)

add_library(kio_filter MODULE ${kio_filter_SRCS})
target_link_libraries(kio_filter KF5::Archive KF5::KIOCore Qt5::Network)
set_target_properties(kio_filter PROPERTIES OUTPUT_NAME "filter")

install(TARGETS kio_filter DESTINATION ${PLUGIN_INSTALL_DIR}/kf5/kio)
install( FILES gzip.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )

if(KArchive_HAVE_BZIP2)
	install( FILES bzip.protocol bzip2.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )
endif()

if(KArchive_HAVE_LZMA)
	install( FILES lzma.protocol xz.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )
endif()
