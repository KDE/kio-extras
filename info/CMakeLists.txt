add_library(kio_info MODULE info.cc)
target_link_libraries(kio_info  KF5::KIOCore KF5::KDE4Support)

install(TARGETS kio_info  DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES kde-info2html.conf DESTINATION  ${DATA_INSTALL_DIR}/kio_info )
install( FILES info.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( PROGRAMS kde-info2html DESTINATION  ${DATA_INSTALL_DIR}/kio_info )

