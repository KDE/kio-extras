cmake_minimum_required(VERSION 3.16)

# KDE Applications Version, managed by release script
set (RELEASE_SERVICE_VERSION_MAJOR "22")
set (RELEASE_SERVICE_VERSION_MINOR "03")
set (RELEASE_SERVICE_VERSION_MICRO "90")
set (RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")
project(kio-extras VERSION ${RELEASE_SERVICE_VERSION})

set(QT_MIN_VERSION "5.15.0")
set(KF5_MIN_VERSION "5.89.0")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

include(ECMMarkNonGuiExecutable)
include(ECMMarkAsTest)
include(ECMOptionalAddSubdirectory)
include(ECMQtDeclareLoggingCategory)
include(CheckIncludeFile)
include(CMakePackageConfigHelpers)
include(FeatureSummary)

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS DBus Network Widgets Svg)

find_package(Qt5Test ${QT_MIN_VERSION} CONFIG QUIET)
set_package_properties(Qt5Test PROPERTIES
         PURPOSE "Required for tests"
         TYPE OPTIONAL
         )
add_feature_info("Qt5Test" Qt5Test_FOUND "Required for building tests")
if (NOT Qt5Test_FOUND)
    set(BUILD_TESTING OFF CACHE BOOL "Build the testing tree.")
endif()

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS
    Archive
    Config
    ConfigWidgets
    CoreAddons
    DBusAddons
    DocTools
    DNSSD
    I18n
    KIO
    Solid
    Bookmarks
    GuiAddons
    SyntaxHighlighting
)

include(KDEClangFormat)
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})

find_package(KF5Activities QUIET)
set_package_properties(KF5Activities PROPERTIES
    PURPOSE "Provides the activities:/ kioslave and fileitem plugin."
    TYPE OPTIONAL
)

find_package(Qt5Sql QUIET)
set_package_properties(Qt5Sql PROPERTIES
    PURPOSE "Provides the activities:/ kioslave and fileitem plugin."
    TYPE OPTIONAL
)

find_package(KF5ActivitiesStats 5.62 QUIET)
set_package_properties(KF5ActivitiesStats PROPERTIES
    PURPOSE "Provides the recentlyused:/ kioslave."
    TYPE OPTIONAL
)

find_package(Phonon4Qt5 4.6.60 NO_MODULE)
set_package_properties(Phonon4Qt5 PROPERTIES
   DESCRIPTION "Qt-based audio library"
   PURPOSE "Required for the audio preview plugin"
   TYPE OPTIONAL)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

if(NOT WIN32)
    # we need a version of samba which has already smbc_set_context(), Alex
    set(SAMBA_REQUIRE_SMBC_SET_CONTEXT TRUE)
    set(SAMBA_REQUIRE_SMBC_OPTION_SET TRUE)
    find_package(Samba)
    set_package_properties(Samba PROPERTIES DESCRIPTION "the SMB client library, a version with smbc_set_context() and smbc_option_set()"
                        URL "https://www.samba.org/"
                        TYPE OPTIONAL
                        PURPOSE "Needed to build the SMB kioslave"
                        )
endif()

find_package(libssh 0.8.3 MODULE)
set_package_properties(libssh PROPERTIES DESCRIPTION "the SSH library with SFTP support"
                       URL "https://www.libssh.org/"
                       TYPE OPTIONAL
                       PURPOSE "Needed to build the SFTP kioslave"
                      )

find_package(Libmtp 1.1.2)
set_package_properties(Libmtp PROPERTIES
                       TYPE OPTIONAL
                       PURPOSE "Needed to build the MTP kioslave"
                       )

check_include_file(utime.h HAVE_UTIME_H)

# ECM's KDECompilerSettings.cmake should take care of enabling supporting on
# 32bit architectures.
# Thorw a fatal error if off_t isn't >=64bit to ensure that large files are working
# as expected.
# BUG: 165449
if(UNIX)
    check_cxx_source_compiles("
        #include <sys/types.h>
        /* Check that off_t can represent 2**63 - 1 correctly.
            We can't simply define LARGE_OFF_T to be 9223372036854775807,
            since some C++ compilers masquerading as C compilers
            incorrectly reject 9223372036854775807.  */
        #define LARGE_OFF_T (((off_t) 1 << 62) - 1 + ((off_t) 1 << 62))

        int off_t_is_large[(LARGE_OFF_T % 2147483629 == 721 && LARGE_OFF_T % 2147483647 == 1) ? 1 : -1];
        int main() { return 0; }
    "
    OFFT_IS_64BIT)

    if(NOT OFFT_IS_64BIT)
        message(FATAL_ERROR "Large file support is not enabled.")
    endif()

    find_package(Gperf)
    set_package_properties(Gperf PROPERTIES TYPE OPTIONAL
                           PURPOSE "Needed to build the man kioslave"
                           )

    find_package(TIRPC)
    set_package_properties(TIRPC PROPERTIES TYPE OPTIONAL
                           PURPOSE "Needed to build the NFS kioslave"
                           )
else()
    # FIXME: on windows we ignore support until trash gets integrated
endif()

add_definitions(
    -DQT_NO_URL_CAST_FROM_STRING
    -DQT_NO_CAST_TO_ASCII
    -DQT_NO_CAST_FROM_ASCII
    -DQT_NO_CAST_FROM_BYTEARRAY
    -DQT_NO_KEYWORDS
    -DQT_NO_FOREACH
    -DQT_USE_QSTRINGBUILDER
    -DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT
    -DQT_DISABLE_DEPRECATED_BEFORE=0x050F00
    -DQT_DEPRECATED_WARNINGS_SINCE=0x060000
    -DKF_DISABLE_DEPRECATED_BEFORE_AND_AT=0x055200
    -DKF_DEPRECATED_WARNINGS_SINCE=0x060000
)

add_subdirectory( doc )

add_subdirectory( about )
if(TARGET KF5::Activities AND TARGET Qt5::Sql AND NOT WIN32)
  add_subdirectory( activities )
endif()
if(KF5ActivitiesStats_FOUND)
    add_subdirectory( recentlyused )
endif()
add_subdirectory( bookmarks )
add_subdirectory( filter )
if(Phonon4Qt5_FOUND)
    add_subdirectory( kfileaudiopreview )
endif()
add_subdirectory( info )
add_subdirectory( archive )
add_subdirectory( recentdocuments )
if (NOT WIN32)
    # does not compile: fish.cpp(41): fatal error C1083: Cannot open include file: 'sys/resource.h': No such file or directory
    # Used for getting the resource limit for closing all child process FDs. Could be completely replaced by fcloseall() if available for Unix or _fcloseall() for Windows, either conditionally on Q_OS_type or using a configure test.
    add_subdirectory( fish )
endif()
add_subdirectory( thumbnail )
add_subdirectory( docfilter )
if (libssh_FOUND)
    add_subdirectory(sftp)
endif ()
add_subdirectory( filenamesearch )
if (Libmtp_FOUND)
  add_subdirectory(mtp)
endif()

if(NOT WIN32)
   if(Gperf_FOUND)
     add_subdirectory( man )
   endif()
   if(TIRPC_FOUND)
     add_subdirectory( nfs )
   endif()
endif()

if(SAMBA_FOUND)
    add_subdirectory(smb)
endif()

configure_file (config-runtime.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-runtime.h )

ecm_qt_install_logging_categories(
    EXPORT KIO_EXTRAS
    FILE kio-extras.categories
    DESTINATION "${KDE_INSTALL_LOGGINGCATEGORIESDIR}"
)


feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
