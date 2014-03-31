add_subdirectory( kcmcgi ) 

add_library(kio_cgi MODULE cgi.cpp)
target_link_libraries(kio_cgi KF5::KIOCore KF5::KDE4Support)

install(TARGETS kio_cgi DESTINATION ${PLUGIN_INSTALL_DIR} )
install(FILES cgi.protocol DESTINATION ${SERVICES_INSTALL_DIR} )

