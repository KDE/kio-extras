add_definitions(-DTRANSLATION_DOMAIN=\"kio5_archive\")

if(BUILD_TESTING)
    add_subdirectory(autotests)
endif()

set(LIBKIOARCHIVE_VERSION "5.97.0")
set(LIBKIOARCHIVE_SOVERSION "5")

include(ECMSetupVersion)
ecm_setup_version(${LIBKIOARCHIVE_VERSION} VARIABLE_PREFIX "KIOARCHIVE"
                  VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/kioarchive_version.h"
                  PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KioArchiveConfigVersion.cmake"
                  SOVERSION ${LIBKIOARCHIVE_SOVERSION})

set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/KioArchive")

configure_package_config_file(
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

ecm_qt_declare_logging_category(kio_archive_debug_SRCS
    HEADER kio_archive_debug.h
    IDENTIFIER KIO_ARCHIVE_LOG
    CATEGORY_NAME kf.kio.slaves.archive
    OLD_CATEGORY_NAMES log_kio_archive
    DESCRIPTION "KIO archive"
    EXPORT KIO_EXTRAS
)

############### ArchiveProtocolBase library ###########

add_library(kioarchive kio_archivebase.cpp ${kio_archive_debug_SRCS})

include(GenerateExportHeader)
generate_export_header(kioarchive BASE_NAME libkioarchive EXPORT_FILE_NAME libkioarchive_export.h)

target_compile_definitions(kioarchive
    PRIVATE
        -DKIOCORE_DISABLE_DEPRECATED_BEFORE_AND_AT=0x054400 # need to support KIO::detailsToStatDetails
)

target_link_libraries(kioarchive
    PUBLIC
    KF5::Archive
    KF5::KIOCore
    PRIVATE
    KF5::I18n
    Qt5::Network
    Qt5::DBus
)

set_target_properties(kioarchive PROPERTIES
    VERSION ${KIOARCHIVE_VERSION}
    SOVERSION ${KIOARCHIVE_SOVERSION}
    EXPORT_NAME KioArchive
)

install(TARGETS kioarchive EXPORT KioArchiveTargets LIBRARY NAMELINK_SKIP ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})

############### Archive KIOslave ######################

add_library(kio_archive MODULE kio_archive.cpp ${kio_archive_debug_SRCS})
target_link_libraries(kio_archive KF5::KIOCore KF5::Archive kioarchive Qt5::Network)
set_target_properties(kio_archive PROPERTIES OUTPUT_NAME "archive")
install(TARGETS kio_archive DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kio)
