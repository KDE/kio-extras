

add_subdirectory( kdedmodule ) 
add_subdirectory( entries ) 
add_subdirectory( tests ) 



########### next target ###############
set(libkiosystem_SRCS kio_system.cpp systemimpl.cpp)
set(kio_system_PART_SRCS ${libkiosystem_SRCS})

kde4_automoc(${kio_system_PART_SRCS})

kde4_add_plugin(kio_system ${kio_system_PART_SRCS})


target_link_libraries(kio_system  ${KDE4_KIO_LIBS})

install(TARGETS kio_system  DESTINATION ${PLUGIN_INSTALL_DIR} )



########### install files ###############

install( FILES system.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )

