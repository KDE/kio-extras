


########### next target ###############

set(kcm_cgi_PART_SRCS kcmcgi.cpp )


kde4_add_plugin(kcm_cgi ${kcm_cgi_PART_SRCS})


target_link_libraries(kcm_cgi  KF5::KIOCore )

install(TARGETS kcm_cgi  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES kcmcgi.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

