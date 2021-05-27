########### next target ###############
add_definitions(-DTRANSLATION_DOMAIN=\"kio5_bookmarks\")

set(kio_bookmarks_PART_SRCS kio_bookmarks.cpp kio_bookmarks_html.cpp kio_bookmarks_pixmap.cpp)

add_library(kio_bookmarks MODULE ${kio_bookmarks_PART_SRCS})

target_link_libraries(kio_bookmarks KF5::KIOCore KF5::I18n KF5::Bookmarks KF5::Solid KF5::GuiAddons KF5::KIOFileWidgets)

set_target_properties(kio_bookmarks PROPERTIES OUTPUT_NAME "bookmarks")

install(TARGETS kio_bookmarks DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kio)


########### install files ###############
install( FILES kio_bookmarks.css DESTINATION  ${KDE_INSTALL_DATADIR}/kio_bookmarks )
