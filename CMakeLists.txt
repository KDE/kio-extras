cmake_minimum_required(VERSION 3.0)

# KDE Applications Version, managed by release script
set (KDE_APPLICATIONS_VERSION_MAJOR "18")
set (KDE_APPLICATIONS_VERSION_MINOR "03")
set (KDE_APPLICATIONS_VERSION_MICRO "70")
set (KDE_APPLICATIONS_VERSION "${KDE_APPLICATIONS_VERSION_MAJOR}.${KDE_APPLICATIONS_VERSION_MINOR}.${KDE_APPLICATIONS_VERSION_MICRO}")
project(kio-extras VERSION ${KDE_APPLICATIONS_VERSION})

include(FeatureSummary)

set(QT_MIN_VERSION "5.7.0")
set(KF5_MIN_VERSION "5.48.0")
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

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS
    Archive
    Config
    ConfigWidgets
    CoreAddons
    DBusAddons
    DocTools
    DNSSD
    IconThemes
    I18n
    KIO
    Solid
    Bookmarks
    GuiAddons
)
# As this is the check used for linkage, only require it in the same location...
if (UNIX)
    find_package(KF5Pty ${KF5_MIN_VERSION} REQUIRED)
endif()

include(CheckIncludeFile)
include(CMakePackageConfigHelpers)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMMarkNonGuiExecutable)
include(ECMMarkAsTest)
include(ECMOptionalAddSubdirectory)
include(ECMQtDeclareLoggingCategory)

add_definitions(-DQT_NO_URL_CAST_FROM_STRING)

find_package(KF5Activities QUIET)
set_package_properties(KF5Activities PROPERTIES
    PURPOSE "Provides the activities:/ kioslave and fileitem plugin."
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
                        URL "http://www.samba.org"
                        TYPE OPTIONAL
                        PURPOSE "Needed to build the SMB kioslave"
                        )
endif()

find_package(libssh 0.7.0 MODULE)
set_package_properties(libssh PROPERTIES DESCRIPTION "the SSH library with SFTP support"
                       URL "http://www.libssh.org/"
                       TYPE OPTIONAL
                       PURPOSE "Needed to build the SFTP kioslave"
                      )

find_package(Mtp)
set_package_properties(Mtp PROPERTIES DESCRIPTION "the MTP library"
                       URL "http://libmtp.sourceforge.net/"
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

    find_package(KF5KHtml QUIET)
    set_package_properties(KF5KHtml PROPERTIES TYPE OPTIONAL
                           PURPOSE "Needed to build the man kioslave"
                           )
else()
    # FIXME: on windows we ignore support until trash gets integrated
endif()

add_subdirectory( doc )

add_subdirectory( about )
if(KF5Activities_FOUND)
  add_subdirectory( activities )
endif()
add_subdirectory( bookmarks )
add_subdirectory( filter )
if(Phonon4Qt5_FOUND)
    add_subdirectory( kfileaudiopreview )
endif()
add_subdirectory( info )
add_subdirectory( archive )
if(NOT WIN32)
    add_subdirectory( network )
endif()
add_subdirectory( recentdocuments )
if (NOT WIN32)
    # does not compile: fish.cpp(41): fatal error C1083: Cannot open include file: 'sys/resource.h': No such file or directory
    # Used for getting the resource limit for closing all child process FDs. Could be completely replaced by fcloseall() if available for Unix or _fcloseall() for Windows, either conditionally on Q_OS_type or using a configure test.
    add_subdirectory( fish )
endif()
add_subdirectory( thumbnail )
add_subdirectory( docfilter )
if (libssh_FOUND AND HAVE_UTIME_H)
    # does not compile on Windows: kio_sftp.cpp(28): fatal error C1083: Cannot open include file: 'utime.h': No such file or directory
    add_subdirectory(sftp)
endif ()
add_subdirectory(settings)
add_subdirectory( filenamesearch )
if (MTP_FOUND)
  add_subdirectory(mtp)
endif()

if(NOT WIN32)
   if(Gperf_FOUND AND KF5KHtml_FOUND)
     add_subdirectory( man )
   endif()
   check_include_files(rpc/rpc.h HAVE_RPC_RPC_H)
   add_feature_info("NFS kioslave" HAVE_RPC_RPC_H "The RPC library is needed to build the NFS kioslave")
   if(HAVE_RPC_RPC_H)
     add_subdirectory( nfs )
   endif()
endif()

if(SAMBA_FOUND)
    add_subdirectory(smb)
endif()

configure_file (config-runtime.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-runtime.h )

install( FILES kio-extras.categories DESTINATION ${KDE_INSTALL_CONFDIR} )

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
