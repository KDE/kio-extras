


########### next target ###############

set(localdomainurifilter_PART_SRCS localdomainurifilter.cpp )


kde4_add_plugin(localdomainurifilter WITH_PREFIX ${localdomainurifilter_PART_SRCS})


target_link_libraries(localdomainurifilter  ${KDE4_KDE3SUPPORT_LIBS} ${KDE4_KIO_LIBS} ${QT_QT3SUPPORT_LIBRARY})

install(TARGETS localdomainurifilter  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### next target ###############

set(klocaldomainurifilterhelper_SRCS klocaldomainurifilterhelper.c )


kde4_add_executable(klocaldomainurifilterhelper NOGUI ${klocaldomainurifilterhelper_SRCS})

target_link_libraries(klocaldomainurifilterhelper  ${KDE4_KDECORE_LIBS} )

install(TARGETS klocaldomainurifilterhelper DESTINATION ${BIN_INSTALL_DIR})


########### install files ###############

install( FILES localdomainurifilter.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

