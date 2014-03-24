
add_library(kio_floppy MODULE kio_floppy.cpp program.cpp)
target_link_libraries(kio_floppy KF5::KIOCore KF5::KDE4Support)

install(TARGETS kio_floppy  DESTINATION ${PLUGIN_INSTALL_DIR} )
install( FILES floppy.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )

