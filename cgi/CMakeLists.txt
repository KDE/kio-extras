

add_subdirectory( kcmcgi ) 



########### next target ###############

set(kio_cgi_PART_SRCS cgi.cpp )


add_library(kio_cgi MODULE ${kio_cgi_PART_SRCS})


target_link_libraries(kio_cgi  KF5::KIOCore )

install(TARGETS kio_cgi  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES cgi.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )

