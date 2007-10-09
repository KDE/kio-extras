include_directories( ../libmediacommon/ )


########### next target ###############

set(kio_media_mounthelper_SRCS kio_media_mounthelper.cpp ${libmediacommon_SRCS} )


kde4_add_executable(kio_media_mounthelper ${kio_media_mounthelper_SRCS})

target_link_libraries(kio_media_mounthelper  ${KDE4_KIO_LIBS} )

install(TARGETS kio_media_mounthelper DESTINATION ${BIN_INSTALL_DIR})

