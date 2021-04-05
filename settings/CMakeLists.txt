add_definitions(-DTRANSLATION_DOMAIN=\"kio5_settings\")

add_library(kio_settings MODULE kio_settings.cpp)
target_link_libraries(kio_settings KF5::KIOCore KF5::Service Qt5::Network)
set_target_properties(kio_settings PROPERTIES OUTPUT_NAME "settings")

install(TARGETS kio_settings  DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kio )
install( FILES settings.protocol DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )
