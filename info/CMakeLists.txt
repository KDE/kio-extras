add_definitions(-DTRANSLATION_DOMAIN=\"kio5_info\")

add_library(kio_info MODULE info.cc)
target_link_libraries(kio_info
    KF5::I18n
    KF5::KIOCore
    Qt5::Network
)
set_target_properties(kio_info PROPERTIES OUTPUT_NAME "info")

ecm_qt_declare_logging_category(kio_info
    HEADER kio_info_debug.h
    IDENTIFIER LOG_KIO_INFO
    CATEGORY_NAME kio_info
)

install(TARGETS kio_info DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kio)
install( FILES kde-info2html.conf DESTINATION  ${KDE_INSTALL_DATADIR}/kio_info )
install( FILES info.protocol  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )
install( PROGRAMS kde-info2html DESTINATION  ${KDE_INSTALL_DATADIR}/kio_info )
