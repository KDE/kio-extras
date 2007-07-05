
add_subdirectory( searchproviders ) 



########### next target ###############

set(kuriikwsfilter_PART_SRCS kuriikwsfiltereng.cpp kuriikwsfilter.cpp searchprovider.cpp )

kde4_add_ui_files(kuriikwsfilter_PART_SRCS ikwsopts_ui.ui searchproviderdlg_ui.ui)

kde4_automoc(${kuriikwsfilter_PART_SRCS})

kde4_add_plugin(kuriikwsfilter WITH_PREFIX ${kuriikwsfilter_PART_SRCS})


target_link_libraries(kuriikwsfilter   ${KDE4_KIO_LIBS} ${QT_QT3SUPPORT_LIBRARY} ${KDE4_KDE3SUPPORT_LIBS} )

install(TARGETS kuriikwsfilter  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### next target ###############

set(kurisearchfilter_PART_SRCS 
   kurisearchfilter.cpp 
   kuriikwsfiltereng.cpp 
   ikwsopts.cpp 
   searchproviderdlg.cpp 
   searchprovider.cpp )

kde4_automoc(${kurisearchfilter_PART_SRCS})


kde4_add_ui_files(kurisearchfilter_PART_SRCS ikwsopts_ui.ui searchproviderdlg_ui.ui)

kde4_add_plugin(kurisearchfilter WITH_PREFIX ${kurisearchfilter_PART_SRCS})


target_link_libraries(kurisearchfilter   ${QT_QTGUI_LIBRARY} ${QT_QT3SUPPORT_LIBRARY} ${KDE4_KIO_LIBS} ${KDE4_KDE3SUPPORT_LIBS})

install(TARGETS kurisearchfilter  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES kuriikwsfilter.desktop kurisearchfilter.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES searchprovider.desktop  DESTINATION  ${SERVICETYPES_INSTALL_DIR} )
install( FILES kuriikwsfilter.upd  DESTINATION  ${KCONF_UPDATE_INSTALL_DIR} )
