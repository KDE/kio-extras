


########### next target ###############

set(kded_homedirnotify_PART_SRCS homedirnotify.cpp homedirnotifymodule.cpp )

kde4_automoc(${kded_homedirnotify_PART_SRCS})

kde4_add_plugin(kded_homedirnotify ${kded_homedirnotify_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} kded_homedirnotify )

target_link_libraries(kded_homedirnotify  ${KDE4_KIO_LIBS})

install(TARGETS kded_homedirnotify  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES homedirnotify.desktop  DESTINATION  ${SERVICES_INSTALL_DIR}/kded )

