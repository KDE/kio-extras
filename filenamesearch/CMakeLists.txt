add_library(kio_filenamesearch MODULE kio_filenamesearch.cpp)
target_link_libraries(kio_filenamesearch KF5::KIOCore KF5::DBusAddons Qt5::Network)
set_target_properties(kio_filenamesearch PROPERTIES OUTPUT_NAME "filenamesearch")

install(TARGETS kio_filenamesearch DESTINATION ${PLUGIN_INSTALL_DIR}/kf5/kio)
install(FILES filenamesearch.protocol DESTINATION  ${SERVICES_INSTALL_DIR})

add_subdirectory(kded)
