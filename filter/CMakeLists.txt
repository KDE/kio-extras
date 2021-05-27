set(kio_filter_SRCS filter.cc)

ecm_qt_declare_logging_category(kio_filter_SRCS
                                HEADER loggingcategory.h
                                IDENTIFIER KIO_FILTER_DEBUG
                                CATEGORY_NAME kio_filter
                                DEFAULT_SEVERITY Debug)

add_library(kio_filter MODULE ${kio_filter_SRCS})
target_link_libraries(kio_filter KF5::Archive KF5::KIOCore Qt5::Network)
set_target_properties(kio_filter PROPERTIES OUTPUT_NAME "filter")

install(TARGETS kio_filter DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kio)
