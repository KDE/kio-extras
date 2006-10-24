
project(kioslave-settings)



########### next target ###############

set(kio_settings_PART_SRCS kio_settings.cc )

kde4_automoc(${kio_settings_PART_SRCS})

kde4_add_plugin(kio_settings ${kio_settings_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kio_settings )

target_link_libraries(kio_settings  ${KDE4_KIO_LIBS} )

install(TARGETS kio_settings  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES settings.protocol programs.protocol applications.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )
