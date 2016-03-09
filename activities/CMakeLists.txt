# vim:set softtabstop=3 shiftwidth=3 tabstop=3 expandtab:

add_definitions(-DTRANSLATION_DOMAIN=\"kio5_activities\")

include_directories (
   ${CMAKE_BINARY_DIR}/src
   ${CMAKE_CURRENT_SOURCE_DIR}
   ${CMAKE_CURRENT_BINARY_DIR}
   )

find_package (Qt5 REQUIRED NO_MODULE COMPONENTS Sql)
find_package (KF5 REQUIRED COMPONENTS KIO I18n Activities)

add_subdirectory(fileitemplugin)

########### next target ###############

set (kio_activities_PART_SRCS
   KioActivities.cpp
   utils/qsqlquery_iterator.cpp
   common/database/Database.cpp
   common/database/schema/ResourcesDatabaseSchema.cpp
   )

add_library (
   kio_activities MODULE
   ${kio_activities_PART_SRCS}
   )

target_link_libraries (
   kio_activities
   Qt5::Sql
   KF5::KIOCore
   KF5::DBusAddons
   KF5::Activities
   KF5::I18n
   )

install (
   TARGETS kio_activities
   DESTINATION ${KDE_INSTALL_PLUGINDIR}
   )

########### install files ###############

install (
   FILES activities.protocol
   DESTINATION ${KDE_INSTALL_KSERVICES5DIR}
   )
