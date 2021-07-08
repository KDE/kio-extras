add_definitions(-DTRANSLATION_DOMAIN=\"kio5_info\")

remove_definitions(-DQT_NO_CAST_FROM_ASCII)

add_library(kio_info MODULE info.cpp)
target_link_libraries(kio_info
    KF5::I18n
    KF5::KIOCore
    Qt5::Network
)
set_target_properties(kio_info PROPERTIES OUTPUT_NAME "info")

ecm_qt_declare_logging_category(kio_info
    HEADER kio_info_debug.h
    IDENTIFIER LOG_KIO_INFO
    CATEGORY_NAME kf.kio.slaves.info
    OLD_CATEGORY_NAMES kio_info
    DESCRIPTION "KIO info"
    EXPORT KIO_EXTRAS
)

install(TARGETS kio_info DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kio)
install( FILES kde-info2html.conf DESTINATION  ${KDE_INSTALL_DATADIR}/kio_info )
install( PROGRAMS kde-info2html DESTINATION  ${KDE_INSTALL_DATADIR}/kio_info )
