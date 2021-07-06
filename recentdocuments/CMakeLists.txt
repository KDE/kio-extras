add_definitions(-DTRANSLATION_DOMAIN=\"kio5_recentdocuments\")

remove_definitions(-DQT_NO_CAST_FROM_ASCII)

add_library(kio_recentdocuments MODULE recentdocuments.cpp)
target_link_libraries(kio_recentdocuments KF5::KIOCore KF5::I18n KF5::ConfigCore Qt5::DBus Qt5::Network)
set_target_properties(kio_recentdocuments PROPERTIES OUTPUT_NAME "recentdocuments")
install(TARGETS kio_recentdocuments DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kio)

########### next target ###############

add_library(kded_recentdocumentsnotifier MODULE recentdocumentsnotifier.cpp)
set_target_properties(kded_recentdocumentsnotifier PROPERTIES OUTPUT_NAME recentdocumentsnotifier)
kcoreaddons_desktop_to_json(kded_recentdocumentsnotifier recentdocumentsnotifier.desktop)

target_link_libraries(kded_recentdocumentsnotifier KF5::KIOCore KF5::DBusAddons)

install(TARGETS kded_recentdocumentsnotifier  DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kded )

########### install files ###############
