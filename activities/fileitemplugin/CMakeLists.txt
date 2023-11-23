# vim:set softtabstop=3 shiftwidth=3 tabstop=3 expandtab:

add_library(kactivitymanagerd_fileitem_linking_plugin MODULE)

target_sources(kactivitymanagerd_fileitem_linking_plugin PRIVATE
   FileItemLinkingPlugin.cpp
   FileItemLinkingPluginActionLoader.cpp
   ../utils/qsqlquery_iterator.cpp
   )

target_link_libraries (
   kactivitymanagerd_fileitem_linking_plugin
   Qt::Core
   Qt::Sql
   KF6::KIOFileWidgets
   KF6::I18n
   Plasma::Activities
   )

install (
   TARGETS kactivitymanagerd_fileitem_linking_plugin
   DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf6/kfileitemaction
   )
