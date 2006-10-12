
project(kioslave-floppy)



########### next target ###############

set(kio_floppy_PART_SRCS kio_floppy.cpp program.cpp )

kde4_automoc(${kio_floppy_PART_SRCS})

kde4_add_plugin(kio_floppy ${kio_floppy_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kio_floppy )

target_link_libraries(kio_floppy  ${KDE4_KIO_LIBS} )

install(TARGETS kio_floppy  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES floppy.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )

