# vim:set softtabstop=3 shiftwidth=3 tabstop=3 expandtab:

find_package (KF5 REQUIRED COMPONENTS KIO Activities)
find_package (Qt5 REQUIRED NO_MODULE COMPONENTS Sql)

include_directories (
   ${CMAKE_CURRENT_SOURCE_DIR}
   ${CMAKE_CURRENT_BINARY_DIR}
   )

set (kactivitymanagerd_fileitem_linking_plugin_SRCS
   FileItemLinkingPlugin.cpp
   FileItemLinkingPluginActionLoader.cpp
   ../utils/qsqlquery_iterator.cpp
   )

add_library (
   kactivitymanagerd_fileitem_linking_plugin MODULE
   ${kactivitymanagerd_fileitem_linking_plugin_SRCS}
   )

kcoreaddons_desktop_to_json (
   kactivitymanagerd_fileitem_linking_plugin
   kactivitymanagerd_fileitem_linking_plugin.desktop
   SERVICE_TYPES kfileitemactionplugin.desktop
   )

target_link_libraries (
   kactivitymanagerd_fileitem_linking_plugin
   Qt5::Core
   Qt5::Sql
   KF5::KIOFileWidgets
   KF5::I18n
   KF5::Activities
   )

install (
   TARGETS kactivitymanagerd_fileitem_linking_plugin
   DESTINATION ${KDE_INSTALL_PLUGINDIR}
   )
install (
   FILES kactivitymanagerd_fileitem_linking_plugin.desktop
   DESTINATION ${KDE_INSTALL_KSERVICES5DIR}
   )
