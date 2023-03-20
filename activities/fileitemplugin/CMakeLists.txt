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
   KF${QT_MAJOR_VERSION}::KIOFileWidgets
   KF${QT_MAJOR_VERSION}::I18n
   KF${QT_MAJOR_VERSION}::Activities
   )

install (
   TARGETS kactivitymanagerd_fileitem_linking_plugin
   DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf${QT_MAJOR_VERSION}/kfileitemaction
   )
