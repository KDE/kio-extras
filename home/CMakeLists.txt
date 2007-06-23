
project(kioslave-home)

add_subdirectory( kdedmodule ) 
add_subdirectory( tests )


########### next target ###############

set(libkiohome_SRCS kio_home.cpp homeimpl.cpp)

set(kio_home_PART_SRCS ${libkiohome_SRCS})

kde4_automoc(${kio_home_PART_SRCS})

kde4_add_plugin(kio_home ${kio_home_PART_SRCS})


target_link_libraries(kio_home  ${KDE4_KIO_LIBS})

install(TARGETS kio_home  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES home.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )




