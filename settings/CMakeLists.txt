########### kio_settings ###############

add_library(kio_settings MODULE kio_settings.cpp)

target_link_libraries(kio_settings  KF5::KIOCore )

install(TARGETS kio_settings  DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install files ###############

install( FILES settings.protocol DESTINATION  ${SERVICES_INSTALL_DIR} )
