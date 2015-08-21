add_definitions(-DTRANSLATION_DOMAIN=\"kio_recentdocuments\")

add_library(kio_recentdocuments MODULE recentdocuments.cpp)
target_link_libraries(kio_recentdocuments KF5::KIOCore Qt5::DBus KF5::KDELibs4Support)
set_target_properties(kio_recentdocuments PROPERTIES OUTPUT_NAME "recentdocuments")
install(TARGETS kio_recentdocuments DESTINATION ${PLUGIN_INSTALL_DIR}/kf5/kio)

########### next target ###############

add_library(kded_recentdocumentsnotifier MODULE recentdocumentsnotifier.cpp)
set_target_properties(kded_recentdocumentsnotifier PROPERTIES OUTPUT_NAME recentdocumentsnotifier)
kcoreaddons_desktop_to_json(kded_recentdocumentsnotifier recentdocumentsnotifier.desktop)

target_link_libraries(kded_recentdocumentsnotifier KF5::KIOCore KF5::DBusAddons KF5::KDELibs4Support)

install(TARGETS kded_recentdocumentsnotifier  DESTINATION ${PLUGIN_INSTALL_DIR}/kf5/kded )

########### install files ###############

install( FILES recentdocuments.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )
