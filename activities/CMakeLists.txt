# vim:set softtabstop=3 shiftwidth=3 tabstop=3 expandtab:

add_definitions(-DTRANSLATION_DOMAIN=\"kio5_activities\")

remove_definitions(-DQT_NO_CAST_FROM_ASCII)

include_directories (
   ${CMAKE_CURRENT_SOURCE_DIR}
   ${CMAKE_CURRENT_BINARY_DIR}
   )

add_subdirectory(fileitemplugin)

########### next target ###############

add_library(kio_activities MODULE)
set_target_properties(kio_activities PROPERTIES
    OUTPUT_NAME "activities"
)

target_sources(kio_activities PRIVATE
   KioActivities.cpp
   utils/qsqlquery_iterator.cpp
   common/database/Database.cpp
   common/database/schema/ResourcesDatabaseSchema.cpp
   )

target_link_libraries (
   kio_activities
   Qt5::Sql
   KF5::KIOCore
   KF5::DBusAddons
   KF5::Activities
   KF5::I18n
   Qt5::Network
   )

install (
   TARGETS kio_activities
   DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kio
   )
