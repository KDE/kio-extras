add_definitions(-DTRANSLATION_DOMAIN=\"kio6_sftp\")

remove_definitions(-DQT_NO_CAST_FROM_ASCII)

find_package(Microsoft.GSL REQUIRED)

cmake_push_check_state()
list(APPEND CMAKE_REQUIRED_LIBRARIES ssh)
check_symbol_exists(sftp_aio_begin_read "libssh/sftp.h" HAVE_SFTP_AIO)
cmake_pop_check_state()
if(HAVE_SFTP_AIO)
    add_definitions(-DHAVE_SFTP_AIO)
endif()

add_library(kio_sftp MODULE)
set_target_properties(kio_sftp PROPERTIES
    OUTPUT_NAME "sftp"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/kf6/kio"
)

target_sources(kio_sftp PRIVATE kio_sftp.cpp)

ecm_qt_declare_logging_category(kio_sftp
    HEADER kio_sftp_debug.h
    IDENTIFIER KIO_SFTP_LOG
    CATEGORY_NAME kf.kio.workers.sftp
    OLD_CATEGORY_NAMES log_kio_sftp kf.kio.slaves.sftp
    DESCRIPTION "KIO sftp"
    EXPORT KIO_EXTRAS
)

ecm_qt_declare_logging_category(kio_sftp
    HEADER kio_sftp_trace_debug.h
    IDENTIFIER KIO_SFTP_TRACE_LOG
    CATEGORY_NAME kf.kio.workers.sftp.trace
    OLD_CATEGORY_NAMES log_kio_sftp.trace kf.kio.slaves.sftp.trace
    DESCRIPTION "KIO sftp (trace)"
    EXPORT KIO_EXTRAS
)

if(WIN32)
    target_include_directories(kio_sftp PRIVATE ${QT_MKSPECS_DIR}/default) # for SYMLINKS
endif()

target_link_libraries(kio_sftp
    PRIVATE
        KF6::KIOCore
        KF6::WidgetsAddons # KMessageBox
        KF6::ConfigCore
        KF6::I18n
        Qt::Network
        QCoro::Core
        ssh
    INTERFACE
        Microsoft.GSL
)
kde_target_enable_exceptions(kio_sftp PRIVATE)

install(TARGETS kio_sftp DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf6/kio)
