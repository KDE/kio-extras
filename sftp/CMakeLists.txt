add_definitions(-DTRANSLATION_DOMAIN=\"kio5_sftp\")

include_directories(${LIBSSH_INCLUDE_DIR})

add_library(kio_sftp MODULE kio_sftp.cpp kio_smtp_debug.cpp)
target_link_libraries(kio_sftp
   KF5::KIOCore
   KF5::WidgetsAddons # KMessageBox
   KF5::I18n
   ${LIBSSH_LIBRARIES})
set_target_properties(kio_sftp PROPERTIES OUTPUT_NAME "sftp")

install(TARGETS kio_sftp DESTINATION ${PLUGIN_INSTALL_DIR}/kf5/kio)

install( FILES sftp.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )
