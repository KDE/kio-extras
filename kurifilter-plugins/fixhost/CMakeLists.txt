
########### next target ###############

set(fixhosturifilter_PART_SRCS fixhosturifilter.cpp )

kde4_automoc(${fixhosturifilter_PART_SRCS})

kde4_add_plugin(fixhosturifilter WITH_PREFIX ${fixhosturifilter_PART_SRCS})


target_link_libraries(fixhosturifilter  ${KDE4_KIO_LIBS} ${QT_QT3SUPPORT_LIBRARY})

install(TARGETS fixhosturifilter  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES fixhosturifilter.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

