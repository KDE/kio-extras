add_library(kio_filenamesearch MODULE kio_filenamesearch.cpp)
target_link_libraries(kio_filenamesearch KF5::KIOCore KF5::DBusAddons Qt5::Network KF5::KIOCore KF5::I18n)
set_target_properties(kio_filenamesearch PROPERTIES OUTPUT_NAME "filenamesearch")

ecm_qt_declare_logging_category(kio_filenamesearch
    HEADER kio_filenamesearch_debug.h
    IDENTIFIER KIO_FILENAMESEARCH
    CATEGORY_NAME kf.kio.slaves.filenamesearch
    OLD_CATEGORY_NAMES kio_filenamesearch
    DESCRIPTION "KIO filenamesearch"
    EXPORT KIO_EXTRAS
)

install(TARGETS kio_filenamesearch DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kio)

add_subdirectory(kded)
