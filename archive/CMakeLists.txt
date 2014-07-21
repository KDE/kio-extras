add_definitions(-DTRANSLATION_DOMAIN=\"kio_archive\")

add_subdirectory(tests)

add_library(kio_archive MODULE kio_archive.cpp)
target_link_libraries(kio_archive KF5::KIOCore KF5::Archive KF5::KDE4Support)

install(TARGETS kio_archive DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES tar.protocol ar.protocol zip.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )
