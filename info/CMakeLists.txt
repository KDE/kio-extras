
project(kioslave-info)



########### next target ###############

set(kio_info_PART_SRCS info.cc )

kde4_automoc(${kio_info_PART_SRCS})

kde4_add_plugin(kio_info ${kio_info_PART_SRCS})


target_link_libraries(kio_info  ${KDE4_KIO_LIBS} )

install(TARGETS kio_info  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES kde-info2html.conf DESTINATION  ${DATA_INSTALL_DIR}/kio_info )
install( FILES info.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( PROGRAMS kde-info2html DESTINATION  ${DATA_INSTALL_DIR}/kio_info )

