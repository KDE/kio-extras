########### next target ###############

set(kio_bookmarks_PART_SRCS kio_bookmarks.cpp kio_bookmarks_html.cpp kio_bookmarks_pixmap.cpp)

kde4_add_plugin(kio_bookmarks ${kio_bookmarks_PART_SRCS})

target_link_libraries(kio_bookmarks  ${KDE4_KIO_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_KFILE_LIBS} ${KDE4_SOLID_LIBS})

install(TARGETS kio_bookmarks  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############
install( FILES kio_bookmarks.css DESTINATION  ${DATA_INSTALL_DIR}/kio_bookmarks )
install( FILES bookmarks.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )
