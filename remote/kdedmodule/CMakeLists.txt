


########### next target ###############

set(kded_remotedirnotify_PART_SRCS remotedirnotify.cpp remotedirnotifymodule.cpp )


kde4_add_plugin(kded_remotedirnotify ${kded_remotedirnotify_PART_SRCS})


target_link_libraries(kded_remotedirnotify  ${KDE4_KIO_LIBS})

install(TARGETS kded_remotedirnotify  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES remotedirnotify.desktop  DESTINATION  ${SERVICES_INSTALL_DIR}/kded )

