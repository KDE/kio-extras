add_definitions(-DTRANSLATION_DOMAIN=\"kio5_archive\")

add_subdirectory(autotests)

add_library(kio_archive MODULE kio_archive.cpp kio_archive_debug.cpp)
target_link_libraries(kio_archive KF5::KIOCore KF5::Archive KF5::KDELibs4Support)
set_target_properties(kio_archive PROPERTIES OUTPUT_NAME "archive")

install(TARGETS kio_archive DESTINATION ${PLUGIN_INSTALL_DIR}/kf5/kio)
install(FILES tar.protocol ar.protocol zip.protocol DESTINATION ${SERVICES_INSTALL_DIR})

