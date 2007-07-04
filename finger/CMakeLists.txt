



########### next target ###############

set(kio_finger_PART_SRCS kio_finger.cpp )

kde4_automoc(${kio_finger_PART_SRCS})

kde4_add_plugin(kio_finger ${kio_finger_PART_SRCS})


target_link_libraries(kio_finger  ${KDE4_KIO_LIBS} )

install(TARGETS kio_finger  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES kio_finger.pl kio_finger.css  DESTINATION  ${DATA_INSTALL_DIR}/kio_finger )
install( FILES finger.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )


