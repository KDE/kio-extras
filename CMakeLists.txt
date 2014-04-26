project(kio-extras)

cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)

set(QT_MIN_VERSION "5.2.0")
find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS DBus Network Widgets Test)

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
include(FeatureSummary)

find_package(SLP)
set_package_properties(SLP PROPERTIES DESCRIPTION "SLP (Service Location Protocol) implementation"
                       URL "http://www.openslp.org/"
                       TYPE OPTIONAL
                       PURPOSE "Provides SLP support in the network:/ kioslave."
                      )

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

add_subdirectory( doc )

add_subdirectory( bookmarks )
add_subdirectory( filter )
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
add_subdirectory( kurifilter-plugins )
if (LIBSSH_FOUND)
  add_subdirectory(sftp)
endif ()

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
