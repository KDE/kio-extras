add_definitions(-DTRANSLATION_DOMAIN=\"kio5_settings\")

add_library(kio_settings MODULE kio_settings.cpp)
target_link_libraries(kio_settings KF5::KDELibs4Support KF5::KIOCore)
set_target_properties(kio_settings PROPERTIES OUTPUT_NAME "settings")

install(TARGETS kio_settings  DESTINATION ${PLUGIN_INSTALL_DIR}/kf5/kio )
install( FILES settings.protocol DESTINATION  ${SERVICES_INSTALL_DIR} )
