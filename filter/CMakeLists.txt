remove_definitions(-DQT_NO_CAST_FROM_ASCII)

add_library(kio_filter MODULE)
set_target_properties(kio_filter PROPERTIES
    OUTPUT_NAME "filter"
)

target_sources(kio_filter PRIVATE filter.cpp)

ecm_qt_declare_logging_category(kio_filter
    HEADER loggingcategory.h
    IDENTIFIER KIO_FILTER_DEBUG
    CATEGORY_NAME kf.kio.slaves.filter
    OLD_CATEGORY_NAMES kio_filter
    DEFAULT_SEVERITY Debug
    DESCRIPTION "KIO filter"
    EXPORT KIO_EXTRAS
)

target_link_libraries(kio_filter KF5::Archive KF5::KIOCore Qt5::Network)

install(TARGETS kio_filter DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kio)
