add_definitions(-DTRANSLATION_DOMAIN=\"kio5_sftp\")

remove_definitions(-DQT_NO_CAST_FROM_ASCII)

add_library(kio_sftp MODULE)
set_target_properties(kio_sftp PROPERTIES
    OUTPUT_NAME "sftp"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/kf5/kio"
)

target_sources(kio_sftp PRIVATE kio_sftp.cpp)

ecm_qt_declare_logging_category(kio_sftp
    HEADER kio_sftp_debug.h
    IDENTIFIER KIO_SFTP_LOG
    CATEGORY_NAME kf.kio.slaves.sftp
    OLD_CATEGORY_NAMES log_kio_sftp
    DESCRIPTION "KIO sftp"
    EXPORT KIO_EXTRAS
)

ecm_qt_declare_logging_category(kio_sftp
    HEADER kio_sftp_trace_debug.h
    IDENTIFIER KIO_SFTP_TRACE_LOG
    CATEGORY_NAME kf.kio.slaves.sftp.trace
    OLD_CATEGORY_NAMES log_kio_sftp.trace
    DESCRIPTION "KIO sftp (trace)"
    EXPORT KIO_EXTRAS
)

if(WIN32)
    target_include_directories(kio_sftp PRIVATE ${QT_MKSPECS_DIR}/default) # for SYMLINKS
endif()

target_link_libraries(kio_sftp
   KF5::KIOCore
   KF5::WidgetsAddons # KMessageBox
   KF5::I18n
   Qt5::Network
   ssh)

install(TARGETS kio_sftp DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kio)
