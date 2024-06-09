add_definitions(-DTRANSLATION_DOMAIN=\"kio6_archive\")

if(BUILD_TESTING)
    add_subdirectory(autotests)
endif()

set(LIBKIOARCHIVE_VERSION "6.0.0")
set(LIBKIOARCHIVE_SOVERSION "6")
set(LIBKIOARCHIVE_INSTALL_INCLUDEDIR "${KDE_INSTALL_INCLUDEDIR}/KioArchive6")

include(ECMSetupVersion)
ecm_setup_version(${LIBKIOARCHIVE_VERSION} VARIABLE_PREFIX "KIOARCHIVE"
                  VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/kioarchive_version.h"
                  PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KioArchive6ConfigVersion.cmake"
                  SOVERSION ${LIBKIOARCHIVE_SOVERSION})

set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/KioArchive6")

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/KioArchive6Config.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/KioArchive6Config.cmake"
  INSTALL_DESTINATION  ${CMAKECONFIG_INSTALL_DIR}
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/KioArchive6Config.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/KioArchive6ConfigVersion.cmake"
  DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
  COMPONENT Devel
)

install(EXPORT KioArchiveTargets
    DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
    FILE KioArchiveTargets.cmake
)

install(FILES
    kio_archivebase.h
    ${CMAKE_CURRENT_BINARY_DIR}/libkioarchive_export.h
    ${CMAKE_CURRENT_BINARY_DIR}/kioarchive_version.h
    DESTINATION ${LIBKIOARCHIVE_INSTALL_INCLUDEDIR}
    COMPONENT Devel
)

ecm_qt_declare_logging_category(kio_archive_debug_SRCS
    HEADER kio_archive_debug.h
    IDENTIFIER KIO_ARCHIVE_LOG
    CATEGORY_NAME kf.kio.workers.archive
    OLD_CATEGORY_NAMES log_kio_archive kf.kio.slaves.archive
    DESCRIPTION "KIO archive"
    EXPORT KIO_EXTRAS
)

############### ArchiveProtocolBase library ###########

add_library(kioarchive6 kio_archivebase.cpp ${kio_archive_debug_SRCS})

include(GenerateExportHeader)
generate_export_header(kioarchive6 BASE_NAME libkioarchive EXPORT_FILE_NAME libkioarchive_export.h)

target_compile_definitions(kioarchive6
    PRIVATE
        -DKIOCORE_DISABLE_DEPRECATED_BEFORE_AND_AT=0x054400 # need to support KIO::detailsToStatDetails
)

target_link_libraries(kioarchive6
    PUBLIC
    KF6::Archive
    KF6::KIOCore
    PRIVATE
    KF6::I18n
    Qt::Network
)

set_target_properties(kioarchive6 PROPERTIES
    VERSION ${KIOARCHIVE_VERSION}
    SOVERSION ${KIOARCHIVE_SOVERSION}
    EXPORT_NAME KioArchive
)

target_include_directories(kioarchive6
    INTERFACE "$<INSTALL_INTERFACE:${LIBKIOARCHIVE_INSTALL_INCLUDEDIR}>"
)

install(TARGETS kioarchive6 EXPORT KioArchiveTargets LIBRARY NAMELINK_SKIP ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

############### Archive Worker ######################

add_library(kio_archive MODULE kio_archive.cpp ${kio_archive_debug_SRCS})
target_link_libraries(kio_archive KF6::KIOCore KF6::Archive kioarchive6 Qt::Network)
set_target_properties(kio_archive PROPERTIES OUTPUT_NAME "archive")
install(TARGETS kio_archive DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf6/kio)
