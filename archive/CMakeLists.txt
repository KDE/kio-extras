add_definitions(-DTRANSLATION_DOMAIN=\"kio_archive\")

add_subdirectory(autotests)

set(LIBKIOARCHIVE_VERSION "5.97.0")
set(LIBKIOARCHIVE_SOVERSION "6")

include(ECMSetupVersion)
ecm_setup_version(${LIBKIOARCHIVE_VERSION} VARIABLE_PREFIX "KIOARCHIVE"
                  VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/kioarchive_version.h"
                  PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KioArchiveConfigVersion.cmake"
                  SOVERSION ${LIBKIOARCHIVE_SOVERSION})

set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/KioArchive")

ecm_configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/KioArchiveConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/KioArchiveConfig.cmake"
  INSTALL_DESTINATION  ${CMAKECONFIG_INSTALL_DIR}
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/KioArchiveConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/KioArchiveConfigVersion.cmake"
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
    DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF5}
    COMPONENT Devel
)

############### ArchiveProtocolBase library ###########

add_library(kioarchive kio_archivebase.cpp kio_archive_debug.cpp)

include(GenerateExportHeader)
generate_export_header(kioarchive BASE_NAME libkioarchive EXPORT_FILE_NAME libkioarchive_export.h)

target_link_libraries(kioarchive
    PUBLIC
    KF5::Archive
    KF5::KIOCore
    PRIVATE
    KF5::I18n
    KF5::KDELibs4Support
)

set_target_properties(kioarchive PROPERTIES
    VERSION ${KIOARCHIVE_VERSION_STRING}
    SOVERSION ${KIOARCHIVE_SOVERSION}
    EXPORT_NAME KioArchive
)

install(TARGETS kioarchive EXPORT KioArchiveTargets ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})

############### Archive KIOslave ######################

add_library(kio_archive MODULE kio_archive.cpp kio_archive_debug.cpp)
target_link_libraries(kio_archive KF5::KIOCore KF5::Archive kioarchive)
set_target_properties(kio_archive PROPERTIES OUTPUT_NAME "archive")
install(TARGETS kio_archive DESTINATION ${PLUGIN_INSTALL_DIR}/kf5/kio)
install(FILES tar.protocol ar.protocol zip.protocol DESTINATION ${SERVICES_INSTALL_DIR})
