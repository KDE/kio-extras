include_directories( ${CMAKE_SOURCE_DIR}/runtime/kioslave/media/libmediacommon/ )


########### next target ###############

set(kfile_media_PART_SRCS kfilemediaplugin.cpp ${libmediacommon_SRCS})

kde4_automoc(${kfile_media_PART_SRCS})

kde4_add_plugin(kfile_media ${kfile_media_PART_SRCS})


target_link_libraries(kfile_media  ${KDE4_KIO_LIBS} )

install(TARGETS kfile_media  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES kfile_media.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

