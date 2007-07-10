

add_subdirectory( kdedmodule ) 
add_subdirectory( tests ) 



set(libkioremote_SRCS kio_remote.cpp remoteimpl.cpp )

########### next target ###############

set(kio_remote_PART_SRCS ${libkioremote_SRCS})


kde4_add_plugin(kio_remote ${kio_remote_PART_SRCS})


target_link_libraries(kio_remote  ${KDE4_KIO_LIBS})

install(TARGETS kio_remote  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES remote.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )




