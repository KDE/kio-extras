add_definitions(-DTRANSLATION_DOMAIN=\"kio5_bookmarks\")

remove_definitions(-DQT_NO_CAST_FROM_ASCII)

add_library(kio_bookmarks MODULE)
set_target_properties(kio_bookmarks PROPERTIES
    OUTPUT_NAME "bookmarks"
)

target_sources(kio_bookmarks PRIVATE
    kio_bookmarks.cpp
    kio_bookmarks_html.cpp
    kio_bookmarks_pixmap.cpp
)

target_link_libraries(kio_bookmarks KF5::KIOCore KF5::I18n KF5::Bookmarks KF5::Solid KF5::GuiAddons KF5::KIOFileWidgets)

install(TARGETS kio_bookmarks DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kio)

install( FILES kio_bookmarks.css DESTINATION  ${KDE_INSTALL_DATADIR}/kio_bookmarks )
