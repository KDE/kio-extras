kde4_add_kcfg_files(libmediacommon_SRCS ../libmediacommon/mediamanagersettings.kcfgc )

set(kded_mediamanager_PART_SRCS ${libmediacommon_SRCS}
   mediamanager.cpp 
   medialist.cpp 
   backendbase.cpp 
   fstabbackend.cpp 
   removablebackend.cpp 
   mediadirnotify.cpp )

qt4_add_dbus_adaptor( kded_mediamanager_PART_SRCS ../org.kde.MediaManager.xml mediamanager.h MediaManager)

if(COMPILE_LINUXCDPOLLING)
   set(kded_mediamanager_PART_SRCS
      ${kded_mediamanager_PART_SRCS}
      linuxcdpolling.cpp)
endif(COMPILE_LINUXCDPOLLING)


kde4_add_plugin(kded_mediamanager ${kded_mediamanager_PART_SRCS})


target_link_libraries(kded_mediamanager  ${KDE4_KIO_LIBS} )

install(TARGETS kded_mediamanager  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES mediamanager.desktop  DESTINATION  ${SERVICES_INSTALL_DIR}/kded )
