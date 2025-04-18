cmake_minimum_required(VERSION 3.16)

# KDE Applications Version, managed by release script
set (RELEASE_SERVICE_VERSION_MAJOR "25")
set (RELEASE_SERVICE_VERSION_MINOR "07")
set (RELEASE_SERVICE_VERSION_MICRO "70")
set (RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")
project(kio-extras VERSION ${RELEASE_SERVICE_VERSION})

set(QT_MIN_VERSION "6.5.0")
set(KF_MIN_VERSION "6.0.0")

find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${ECM_MODULE_PATH})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDEGitCommitHooks)

include(ECMMarkNonGuiExecutable)
include(ECMMarkAsTest)
include(ECMOptionalAddSubdirectory)
include(ECMQtDeclareLoggingCategory)
include(ECMDeprecationSettings)
include(CheckIncludeFile)
include(CMakePushCheckState)
include(CMakePackageConfigHelpers)
include(FeatureSummary)

option(BUILD_DOC "Whether to build the documentation" ON)

find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS Gui Network Widgets Svg Core5Compat)

if (Qt6Gui_VERSION VERSION_GREATER_EQUAL "6.10.0")
    find_package(Qt6GuiPrivate REQUIRED)
endif()

find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS
    Archive
    Config
    ConfigWidgets
    CoreAddons
    DNSSD
    I18n
    KIO
    Solid
    GuiAddons
    SyntaxHighlighting
    KCMUtils
    TextWidgets
)

# shall we use DBus?
# enabled per default on Linux & BSD systems
set(USE_DBUS_DEFAULT OFF)
if(UNIX AND NOT APPLE AND NOT ANDROID AND NOT HAIKU)
    set(USE_DBUS_DEFAULT ON)
endif()
option(USE_DBUS "Build components using DBus" ${USE_DBUS_DEFAULT})
if(USE_DBUS)
    find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS DBus)
    find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS DBusAddons)
endif()

if(LINUX)
    option(WITH_LIBPROXY "Build with libproxy, required for WPAD problem detection" ON)
endif()
if(WITH_LIBPROXY)
    find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS Notifications)
    find_package(PkgConfig)
    pkg_check_modules(libproxy REQUIRED IMPORTED_TARGET libproxy-1.0)
    add_feature_info("libproxy" libproxy_FOUND "libproxy for WPAD problem detection (kcm_proxy)")
endif()

find_package(Qt6Test ${QT_MIN_VERSION} CONFIG QUIET)
set_package_properties(Qt6Test PROPERTIES
         PURPOSE "Required for tests"
         TYPE OPTIONAL
         )
add_feature_info("Qt6Test" Qt6Test_FOUND "Required for building tests")
if (NOT Qt6Test_FOUND)
    set(BUILD_TESTING OFF CACHE BOOL "Build the testing tree.")
endif()

find_package(QCoro6 REQUIRED COMPONENTS Core)
qcoro_enable_coroutines()

include(KDEClangFormat)
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})

if (NOT WIN32 AND USE_DBUS)
    option(BUILD_ACTIVITIES "Build recentlyused:/, activities:/ KIO workers and fileitem plugin." ON)
    if(BUILD_ACTIVITIES)
        find_package(Qt6Sql REQUIRED)
        find_package(PlasmaActivities REQUIRED)
        find_package(PlasmaActivitiesStats REQUIRED)
    endif()
endif()

if (BUILD_DOC)
    find_package(KF6DocTools ${KF_MIN_VERSION})
    set_package_properties(KF6DocTools PROPERTIES
        DESCRIPTION "Tools to generate documentation"
        TYPE REQUIRED
    )
endif()

find_package(Phonon4Qt6 4.6.60 NO_MODULE)
set_package_properties(Phonon4Qt6 PROPERTIES
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
                        PURPOSE "Needed to build the SMB KIO worker"
                        )
endif()

find_package(libssh 0.8.3 MODULE)
set_package_properties(libssh PROPERTIES DESCRIPTION "the SSH library with SFTP support"
                       URL "https://www.libssh.org/"
                       TYPE OPTIONAL
                       PURPOSE "Needed to build the SFTP KIO worker"
                      )

find_package(Libmtp 1.1.2)
set_package_properties(Libmtp PROPERTIES
                       TYPE OPTIONAL
                       PURPOSE "Needed to build the MTP KIO worker"
                       )

find_package(IMobileDevice)
set_package_properties(IMobileDevice PROPERTIES
                       TYPE OPTIONAL
                       PURPOSE "Needed to build the AFC (Apple File Conduit) KIO worker"
                      )

find_package(PList)
set_package_properties(PList PROPERTIES
                       TYPE OPTIONAL
                       PURPOSE "Needed to build the AFC (Apple File Conduit) KIO worker"
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
                           PURPOSE "Needed to build the man KIO worker"
                           )

    find_package(TIRPC)
    set_package_properties(TIRPC PROPERTIES TYPE OPTIONAL
                           PURPOSE "Needed to build the NFS KIO worker"
                           )
else()
    # FIXME: on windows we ignore support until trash gets integrated
endif()

ecm_set_disabled_deprecation_versions(
    QT 5.15.2
    KF 6.0
)

add_subdirectory(kcms)

if(BUILD_ACTIVITIES)
  add_subdirectory( activities )
  add_subdirectory( recentlyused )
endif()
add_subdirectory( filter )
if(Phonon4Qt6_FOUND)
    add_subdirectory( kfileaudiopreview )
endif()
add_subdirectory( info )
add_subdirectory( archive )
if (NOT WIN32)
    # does not compile: fish.cpp(41): fatal error C1083: Cannot open include file: 'sys/resource.h': No such file or directory
    # Used for getting the resource limit for closing all child process FDs. Could be completely replaced by fcloseall() if available for Unix or _fcloseall() for Windows, either conditionally on Q_OS_type or using a configure test.
    add_subdirectory( fish )
endif()
add_subdirectory( thumbnail )
if (libssh_FOUND)
    add_subdirectory(sftp)
endif ()

if (NOT WIN32 AND NOT APPLE AND USE_DBUS)
    add_subdirectory( filenamesearch )
endif()

if (Libmtp_FOUND AND USE_DBUS)
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

if(SAMBA_FOUND AND USE_DBUS)
    add_subdirectory(smb)
endif()

if(IMobileDevice_FOUND AND PList_FOUND)
    add_subdirectory(afc)
endif()

configure_file (config-runtime.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-runtime.h )

ecm_qt_install_logging_categories(
    EXPORT KIO_EXTRAS
    FILE kio-extras.categories
    DESTINATION "${KDE_INSTALL_LOGGINGCATEGORIESDIR}"
)

if(BUILD_DOC)
    add_subdirectory(doc)
    kdoctools_install(po)
endif()

ki18n_install(po)

kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
