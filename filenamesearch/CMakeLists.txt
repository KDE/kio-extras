add_library(kio_filenamesearch MODULE kio_filenamesearch.cpp)
target_link_libraries(kio_filenamesearch KF5::KIOCore)

install(TARGETS kio_filenamesearch DESTINATION ${PLUGIN_INSTALL_DIR})
install(FILES filenamesearch.protocol DESTINATION  ${SERVICES_INSTALL_DIR})
