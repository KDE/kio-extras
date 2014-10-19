########### next target ###############
add_definitions(-DTRANSLATION_DOMAIN=\"kio_bookmarks\")

set(kio_bookmarks_PART_SRCS kio_bookmarks.cpp kio_bookmarks_html.cpp kio_bookmarks_pixmap.cpp)

add_library(kio_bookmarks MODULE ${kio_bookmarks_PART_SRCS})

target_link_libraries(kio_bookmarks KF5::KIOCore KF5::I18n KF5::Bookmarks KF5::Solid KF5::GuiAddons KF5::KIOFileWidgets)

install(TARGETS kio_bookmarks  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############
install( FILES kio_bookmarks.css DESTINATION  ${DATA_INSTALL_DIR}/kio_bookmarks )
install( FILES bookmarks.protocol  DESTINATION  ${SERVICES_INSTALL_DIR} )
