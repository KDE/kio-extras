kcoreaddons_add_plugin(kio_filenamesearch INSTALL_NAMESPACE "kf${QT_MAJOR_VERSION}/kio")

target_link_libraries(kio_filenamesearch KF${QT_MAJOR_VERSION}::KIOCore KF${QT_MAJOR_VERSION}::DBusAddons Qt::Network KF${QT_MAJOR_VERSION}::KIOCore KF${QT_MAJOR_VERSION}::I18n)
target_sources(kio_filenamesearch PRIVATE kio_filenamesearch.cpp)

ecm_qt_declare_logging_category(kio_filenamesearch
    HEADER kio_filenamesearch_debug.h
    IDENTIFIER KIO_FILENAMESEARCH
    CATEGORY_NAME kf.kio.workers.filenamesearch
    OLD_CATEGORY_NAMES kio_filenamesearch kf.kio.slaves.filenamesearch
    DESCRIPTION "KIO filenamesearch"
    EXPORT KIO_EXTRAS
)

add_subdirectory(kded)
