########### kio_settings ###############

kde4_add_plugin(kio_settings kio_settings.cpp)

target_link_libraries(kio_settings  ${KDE4_KIO_LIBS} )

install(TARGETS kio_settings  DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install files ###############

install( FILES settings.protocol DESTINATION  ${SERVICES_INSTALL_DIR} )
