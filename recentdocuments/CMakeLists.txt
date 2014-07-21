add_definitions(-DTRANSLATION_DOMAIN=\"kio_recentdocuments\")

add_library(kio_recentdocuments MODULE recentdocuments.cpp)
target_link_libraries(kio_recentdocuments KF5::KIOCore Qt5::DBus KF5::KDE4Support)

install(TARGETS kio_recentdocuments  DESTINATION ${PLUGIN_INSTALL_DIR})

########### next target ###############

add_library(kded_recentdocumentsnotifier MODULE recentdocumentsnotifier.cpp)
target_link_libraries(kded_recentdocumentsnotifier KF5::KIOCore KF5::DBusAddons KF5::KDE4Support)

install(TARGETS kded_recentdocumentsnotifier  DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install files ###############

install( FILES recentdocumentsnotifier.desktop  DESTINATION  ${SERVICES_INSTALL_DIR}/kded )
install( FILES recentdocuments.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )
