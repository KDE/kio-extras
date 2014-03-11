
add_subdirectory(tests)

########### next target ###############

set(kio_desktop_PART_SRCS kio_desktop.cpp)

add_library(kio_desktop MODULE  ${kio_desktop_PART_SRCS})

target_link_libraries(kio_desktop  KF5::KIOCore)

install(TARGETS kio_desktop DESTINATION  ${PLUGIN_INSTALL_DIR} )

########### next target ###############

set(kded_desktopnotifier_PART_SRCS  desktopnotifier.cpp )

add_library(kded_desktopnotifier MODULE  ${kded_desktopnotifier_PART_SRCS})

target_link_libraries(kded_desktopnotifier  KF5::KIOCore)

install(TARGETS kded_desktopnotifier  DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install files ###############

install( FILES desktopnotifier.desktop  DESTINATION  ${SERVICES_INSTALL_DIR}/kded )
install( FILES desktop.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )

install(FILES directory.desktop directory.trash DESTINATION ${DATA_INSTALL_DIR}/kio_desktop)
install(FILES Home.desktop DESTINATION ${DATA_INSTALL_DIR}/kio_desktop/DesktopLinks)

