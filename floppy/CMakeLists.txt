



########### next target ###############

set(kio_floppy_PART_SRCS kio_floppy.cpp program.cpp )


kde4_add_plugin(kio_floppy ${kio_floppy_PART_SRCS})


target_link_libraries(kio_floppy  KF5::KIOCore )

install(TARGETS kio_floppy  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES floppy.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )

