
project(kioslave-cgi)

add_subdirectory( kcmcgi ) 



########### next target ###############

set(kio_cgi_PART_SRCS cgi.cpp )

kde4_automoc(${kio_cgi_PART_SRCS})

kde4_add_plugin(kio_cgi ${kio_cgi_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kio_cgi )

target_link_libraries(kio_cgi  ${KDE4_KIO_LIBS} )

install(TARGETS kio_cgi  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES cgi.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )

