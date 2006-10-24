
project(kioslave-remote)

add_subdirectory( kdedmodule ) 



set(libkioremote_SRCS kio_remote.cpp remoteimpl.cpp )

########### next target ###############

set(kio_remote_PART_SRCS ${libkioremote_SRCS})

kde4_automoc(${kio_remote_PART_SRCS})

kde4_add_plugin(kio_remote ${kio_remote_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kio_remote )

target_link_libraries(kio_remote  ${KDE4_KIO_LIBS})

install(TARGETS kio_remote  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### next target ###############
if(KDE4_BUILD_TESTS)
set(testremote_SRCS testremote.cpp ${libkioremote_SRCS} )

kde4_automoc(${testremote_SRCS})

kde4_add_executable(testremote ${testremote_SRCS})

target_link_libraries(testremote  ${KDE4_KIO_LIBS} )

endif(KDE4_BUILD_TESTS)

########### install files ###############

install( FILES remote.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )




