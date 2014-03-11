



########### next target ###############

set(kio_finger_PART_SRCS kio_finger.cpp )


add_library(kio_finger MODULE ${kio_finger_PART_SRCS})


target_link_libraries(kio_finger KF5::KIOCore )

install(TARGETS kio_finger  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES kio_finger.pl kio_finger.css  DESTINATION  ${DATA_INSTALL_DIR}/kio_finger )
install( FILES finger.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )


