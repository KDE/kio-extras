include_directories( ${CMAKE_SOURCE_DIR}/runtime/kioslave/media/libmediacommon/ )


########### next target ###############

set(kded_medianotifier_PART_SRCS medianotifier.cpp ../mediamanageriface.cpp notificationdialog.cpp ${libmediacommon_SRCS} )

kde4_add_kcfg_files(kded_medianotifier_PART_SRCS ../libmediacommon/mediamanagersettings.kcfgc )

kde4_automoc(${kded_medianotifier_PART_SRCS})

kde4_add_ui3_files(kded_medianotifier_PART_SRCS notificationdialogview.ui )

kde4_add_plugin(kded_medianotifier ${kded_medianotifier_PART_SRCS})


target_link_libraries(kded_medianotifier  ${KDE4_KIO_LIBS}  )

install(TARGETS kded_medianotifier  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES medianotifier.desktop  DESTINATION  ${SERVICES_INSTALL_DIR}/kded )

