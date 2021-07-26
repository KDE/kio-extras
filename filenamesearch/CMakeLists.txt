kcoreaddons_add_plugin(kio_filenamesearch INSTALL_NAMESPACE "kf5/kio")

target_link_libraries(kio_filenamesearch KF5::KIOCore KF5::DBusAddons Qt5::Network KF5::KIOCore KF5::I18n)
target_sources(kio_filenamesearch PRIVATE kio_filenamesearch.cpp)

ecm_qt_declare_logging_category(kio_filenamesearch
    HEADER kio_filenamesearch_debug.h
    IDENTIFIER KIO_FILENAMESEARCH
    CATEGORY_NAME kf.kio.slaves.filenamesearch
    OLD_CATEGORY_NAMES kio_filenamesearch
    DESCRIPTION "KIO filenamesearch"
    EXPORT KIO_EXTRAS
)

add_subdirectory(kded)
