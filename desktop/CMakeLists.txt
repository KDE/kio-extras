########### next target ###############

set(kio_desktop_PART_SRCS kio_desktop.cpp)

kde4_add_plugin(kio_desktop  ${kio_desktop_PART_SRCS})

target_link_libraries(kio_desktop  ${KDE4_KIO_LIBS})

install(TARGETS kio_desktop DESTINATION  ${PLUGIN_INSTALL_DIR} )

########### next target ###############

set(kded_desktopnotifier_PART_SRCS  desktopnotifier.cpp )

kde4_add_plugin(kded_desktopnotifier  ${kded_desktopnotifier_PART_SRCS})

target_link_libraries(kded_desktopnotifier  ${KDE4_KIO_LIBS})

install(TARGETS kded_desktopnotifier  DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install files ###############

install( FILES desktopnotifier.desktop  DESTINATION  ${SERVICES_INSTALL_DIR}/kded )
install( FILES desktop.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )

