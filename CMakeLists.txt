project(kio-extras)
set(PROJECT_VERSION "5.0.95")
set(PROJECT_VERSION_MAJOR 5)

cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)

include(FeatureSummary)

set(QT_MIN_VERSION "5.3.0")
find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS DBus Network Widgets)

find_package(Qt5Test ${QT_MIN_VERSION} CONFIG QUIET)
set_package_properties(Qt5Test PROPERTIES
         PURPOSE "Required for tests"
         TYPE OPTIONAL
         )
add_feature_info("Qt5Test" Qt5Test_FOUND "Required for building tests")
if (NOT Qt5Test_FOUND)
    set(BUILD_TESTING OFF CACHE BOOL "Build the testing tree.")
endif()

find_package(ECM 0.0.11 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(KF5 REQUIRED COMPONENTS
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
    KHtml
    KDE4Support
    Solid
)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)
include(ECMMarkNonGuiExecutable)
include(ECMMarkAsTest)
include(ECMPackageConfigHelpers)

find_package(SLP)
set_package_properties(SLP PROPERTIES DESCRIPTION "SLP (Service Location Protocol) implementation"
                       URL "http://www.openslp.org/"
                       TYPE OPTIONAL
                       PURPOSE "Provides SLP support in the network:/ kioslave."
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

find_package(LibSSH 0.6.0)
set_package_properties(LibSSH PROPERTIES DESCRIPTION "the SSH library with SFTP support"
                       URL "http://www.libssh.org/"
                       TYPE OPTIONAL
                       PURPOSE "Needed to build the SFTP kioslave"
                      )

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
else()
    # FIXME: on windows we ignore support until trash gets integrated
endif()

add_subdirectory( doc )

add_subdirectory( bookmarks )
add_subdirectory( filter )
if(Phonon4Qt5_FOUND)
    add_subdirectory( kfileaudiopreview )
endif()
add_subdirectory( info )
add_subdirectory( archive )
add_subdirectory( trash )
if(NOT WIN32)
    add_subdirectory( network )
endif()
add_subdirectory( desktop )
add_subdirectory( recentdocuments )
add_subdirectory( fish )
add_subdirectory( thumbnail )
add_subdirectory( docfilter )
if (LIBSSH_FOUND)
  add_subdirectory(sftp)
endif ()
add_subdirectory(settings)

if(NOT WIN32)
   add_subdirectory( man )
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

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
