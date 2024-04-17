add_definitions(-DTRANSLATION_DOMAIN=\"kio6_cmdtoolsearch\")

kcoreaddons_add_plugin(kio_cmdtoolsearch INSTALL_NAMESPACE "kf6/kio")

configure_file(config.h.cmake config.h)

add_library(cmdtool_static STATIC)
set_property(TARGET cmdtool_static PROPERTY POSITION_INDEPENDENT_CODE ON)
target_sources(cmdtool_static PRIVATE cmdtool.cpp cmdtool_manager.cpp)
target_link_libraries(cmdtool_static
    Qt::Core
)

target_link_libraries(kio_cmdtoolsearch cmdtool_static KF6::KIOCore KF6::DBusAddons Qt::Network KF6::KIOCore KF6::I18n)
target_sources(kio_cmdtoolsearch PRIVATE kio_cmdtoolsearch.cpp)

ecm_qt_declare_logging_category(cmdtool_static
    HEADER kio_cmdtoolsearch_debug.h
    IDENTIFIER KIO_CMDTOOLSEARCH
    CATEGORY_NAME kf.kio.workers.cmdtoolsearch
    OLD_CATEGORY_NAMES kio_cmdtoolsearch kf.kio.slaves.cmdtoolsearch
    DESCRIPTION "KIO cmdtoolsearch"
    EXPORT KIO_EXTRAS
)

if(NOT WIN32)
    install(DIRECTORY tools/ DESTINATION ${KDE_INSTALL_DATADIR}/cmdtoolsearch USE_SOURCE_PERMISSIONS)
    if (CMAKE_SYSTEM_NAME MATCHES "Linux")
        install(DIRECTORY defaults/linux/ DESTINATION ${KDE_INSTALL_DATADIR}/cmdtoolsearch)
    elseif (CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
        #FIXME: We don't have a working CmdTool::isOnHdd() on FreeBSD, so rg and fd are excluded from the defaults
        install(DIRECTORY defaults/freebsd/ DESTINATION ${KDE_INSTALL_DATADIR}/cmdtoolsearch)
    endif()
endif()

if (BUILD_TESTING)
    add_subdirectory(autotests)
    add_subdirectory(tests)
endif()
