
project(kioslave-system)

add_subdirectory( kdedmodule ) 
add_subdirectory( entries ) 



########### next target ###############
set(libkiosystem_SRCS kio_system.cpp systemimpl.cpp)
set(kio_system_PART_SRCS ${libkiosystem_SRCS})

kde4_automoc(${kio_system_PART_SRCS})

kde4_add_plugin(kio_system ${kio_system_PART_SRCS})


target_link_libraries(kio_system  ${KDE4_KIO_LIBS})

install(TARGETS kio_system  DESTINATION ${PLUGIN_INSTALL_DIR} )



########### next target ###############
set(testsystem_SRCS testsystem.cpp )

kde4_automoc(${testsystem_SRCS})

kde4_add_test_executable(testsystem ${testsystem_SRCS} ${libkiosystem_SRCS})

target_link_libraries(testsystem  ${KDE4_KIO_LIBS} )

########### install files ###############

install( FILES system.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )

