add_definitions(-DTRANSLATION_DOMAIN=\"kio5_recentdocuments\")

remove_definitions(-DQT_NO_CAST_FROM_ASCII)

add_library(kio_recentdocuments MODULE recentdocuments.cpp)
ecm_qt_declare_logging_category(kio_recentdocuments
    HEADER recentdocuemnts_log.h
    IDENTIFIER LOG_RECENTDOCUMENTS
    CATEGORY_NAME kf.kio.workers.recentdocuments
    OLD_CATEGORY_NAMES kf.kio.slaves.recentdocuments
    DESCRIPTION "KIO recentdocuments"
    EXPORT KIO_EXTRAS
)
target_link_libraries(kio_recentdocuments KF5::KIOCore KF5::I18n KF5::ConfigCore Qt::DBus Qt::Network)
set_target_properties(kio_recentdocuments PROPERTIES OUTPUT_NAME "recentdocuments")
install(TARGETS kio_recentdocuments DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf${QT_MAJOR_VERSION}/kio)

########### next target ###############

add_library(kded_recentdocumentsnotifier MODULE recentdocumentsnotifier.cpp)
set_target_properties(kded_recentdocumentsnotifier PROPERTIES OUTPUT_NAME recentdocumentsnotifier)

target_link_libraries(kded_recentdocumentsnotifier KF5::KIOCore KF5::DBusAddons)

install(TARGETS kded_recentdocumentsnotifier  DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf${QT_MAJOR_VERSION}/kded )

########### install files ###############
