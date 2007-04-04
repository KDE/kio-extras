

add_definitions (-DQT3_SUPPORT)

########### next target ###############

set(kshorturifilter_PART_SRCS kshorturifilter.cpp )

kde4_automoc(${kshorturifilter_PART_SRCS})

kde4_add_plugin(kshorturifilter WITH_PREFIX ${kshorturifilter_PART_SRCS})


target_link_libraries(kshorturifilter  ${KDE4_KIO_LIBS} ${QT_QT3SUPPORT_LIBRARY})

install(TARGETS kshorturifilter  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES kshorturifilter.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES kshorturifilterrc  DESTINATION  ${CONFIG_INSTALL_DIR} )

