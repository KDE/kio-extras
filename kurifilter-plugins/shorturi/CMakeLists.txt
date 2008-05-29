


########### next target ###############

set(kshorturifilter_PART_SRCS kshorturifilter.cpp )


kde4_add_plugin(kshorturifilter ${kshorturifilter_PART_SRCS})


target_link_libraries(kshorturifilter  ${KDE4_KIO_LIBS})

install(TARGETS kshorturifilter  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES kshorturifilter.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES kshorturifilterrc  DESTINATION  ${CONFIG_INSTALL_DIR} )

