add_definitions(-DTRANSLATION_DOMAIN=\"kio5_sftp\")

include_directories(${LIBSSH_INCLUDE_DIR})

set(kio_sftp_SRCS kio_sftp.cpp)

ecm_qt_declare_logging_category(kio_sftp_SRCS
    HEADER kio_sftp_debug.h
    IDENTIFIER KIO_SFTP_LOG
    CATEGORY_NAME log_kio_sftp)

ecm_qt_declare_logging_category(kio_sftp_SRCS
    HEADER kio_sftp_trace_debug.h
    IDENTIFIER KIO_SFTP_TRACE_LOG
    CATEGORY_NAME log_kio_sftp.trace)

add_library(kio_sftp MODULE ${kio_sftp_SRCS})
target_link_libraries(kio_sftp
   KF5::KIOCore
   KF5::WidgetsAddons # KMessageBox
   KF5::I18n
   Qt5::Network
   ${LIBSSH_LIBRARIES})
set_target_properties(kio_sftp PROPERTIES OUTPUT_NAME "sftp")

install(TARGETS kio_sftp DESTINATION ${PLUGIN_INSTALL_DIR}/kf5/kio)

install( FILES sftp.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )
