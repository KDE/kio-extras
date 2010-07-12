add_subdirectory(searchproviders) 

set(kuriikwsfilter_PART_SRCS kuriikwsfiltereng.cpp kuriikwsfilter.cpp searchprovider.cpp)

kde4_add_ui_files(kuriikwsfilter_PART_SRCS ikwsopts_ui.ui searchproviderdlg_ui.ui)

kde4_add_plugin(kuriikwsfilter ${kuriikwsfilter_PART_SRCS})

target_link_libraries(kuriikwsfilter ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS})
if(WINCE)
target_link_libraries(kuriikwsfilter ${WCECOMPAT_LIBRARIES})
endif(WINCE)

install(TARGETS kuriikwsfilter DESTINATION ${PLUGIN_INSTALL_DIR})

set(kurisearchfilter_PART_SRCS 
   kurisearchfilter.cpp 
   kuriikwsfiltereng.cpp 
   ikwsopts.cpp 
   searchproviderdlg.cpp 
   searchprovider.cpp )

kde4_add_ui_files(kurisearchfilter_PART_SRCS ikwsopts_ui.ui searchproviderdlg_ui.ui)

kde4_add_plugin(kurisearchfilter ${kurisearchfilter_PART_SRCS})

target_link_libraries(kurisearchfilter ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS})
if(WINCE)
target_link_libraries(kurisearchfilter ${WCECOMPAT_LIBRARIES})
endif(WINCE)

install(TARGETS kurisearchfilter DESTINATION ${PLUGIN_INSTALL_DIR})

########### install files ###############

install( FILES kuriikwsfilter.desktop kurisearchfilter.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES searchprovider.desktop  DESTINATION  ${SERVICETYPES_INSTALL_DIR} )
install( FILES kuriikwsfilter.upd  DESTINATION  ${KCONF_UPDATE_INSTALL_DIR} )
