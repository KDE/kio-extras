include_directories( ${CMAKE_SOURCE_DIR}/runtime/kioslave/media/libmediacommon/
    ${CMAKE_BINARY_DIR}/runtime/kioslave/media/ )


########### next target ###############

set(kcm_media_PART_SRCS 
   notifiermodule.cpp 
   serviceconfigdialog.cpp 
   mimetypelistboxitem.cpp 
   managermodule.cpp 
   main.cpp ${libmediacommon_SRCS})


kde4_add_kcfg_files(kcm_media_PART_SRCS ../libmediacommon/mediamanagersettings.kcfgc )


kde4_automoc(${kcm_media_PART_SRCS})

kde4_add_ui3_files(kcm_media_PART_SRCS notifiermoduleview.ui serviceview.ui managermoduleview.ui )

kde4_add_plugin(kcm_media ${kcm_media_PART_SRCS})


target_link_libraries(kcm_media    ${KDE4_KDE3SUPPORT_LIBS})

install(TARGETS kcm_media  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES media.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )


