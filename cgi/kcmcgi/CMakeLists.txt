add_library(kcm_cgi MODULE kcmcgi.cpp)
target_link_libraries(kcm_cgi KF5::KDE4Support KF5::KIOCore KF5::ConfigGui)

install(TARGETS kcm_cgi  DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES kcmcgi.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

