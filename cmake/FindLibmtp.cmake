# SPDX-FileCopyrightText: 2014 Jan Grulich <jgrulich@redhat.com>
# SPDX-FileCopyrightText: 2021 Friedrich W. H. Kossebau <kossebau@kde.org>
#
# SPDX-License-Identifier: BSD-3-Clause

#[=======================================================================[.rst:
FindLibmtp
----------

Try to find the libmtp library.

This will define the following variables:

``Libmtp_FOUND``
    TRUE if (the requested version of) libmtp is available
``Libmtp_VERSION``
    The version of libmtp
``Libmtp_LIBRARIES``
    The libraries of libmtp for use with target_link_libraries()
``Libmtp_INCLUDE_DIRS``
    The include dirs of libmtp for use with target_include_directories()
``Libmtp_DEFINITIONS``
    Compiler switches required for using libmtp

If ``Libmtp_FOUND`` is TRUE, it will also define the following imported
target:

``Libmtp::Libmtp``
    The libmtp library
#]=======================================================================]

find_package(PkgConfig QUIET)
pkg_check_modules(PC_Libmtp PC_MTP QUIET libmtp>=${Libmtp_FIND_VERSION})

find_library(Libmtp_LIBRARIES
    NAMES mtp
    HINTS ${PC_Libmtp_LIBRARY_DIRS}
)

find_path(Libmtp_INCLUDE_DIRS
    NAMES libmtp.h
    HINTS ${PC_Libmtp_INCLUDE_DIRS}
)

set(Libmtp_VERSION ${PC_Libmtp_VERSION})
set(Libmtp_DEFINITIONS ${PC_Libmtp_CFLAGS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libmtp
    FOUND_VAR
        Libmtp_FOUND
    REQUIRED_VARS
        Libmtp_LIBRARIES
        Libmtp_INCLUDE_DIRS
    VERSION_VAR
        Libmtp_VERSION
)

if(Libmtp_FOUND AND NOT TARGET Libmtp::Libmtp)
    add_library(Libmtp::Libmtp UNKNOWN IMPORTED)
    set_target_properties(Libmtp::Libmtp PROPERTIES
        IMPORTED_LOCATION "${Libmtp_LIBRARIES}"
        INTERFACE_COMPILE_OPTIONS "${Libmtp_DEFINITIONS}"
        INTERFACE_INCLUDE_DIRECTORIES "${Libmtp_INCLUDE_DIRS}"
    )
endif()

mark_as_advanced(Libmtp_LIBRARIES Libmtp_INCLUDE_DIRS Libmtp_VERSION Libmtp_DEFINITIONS)

include(FeatureSummary)
set_package_properties(Libmtp PROPERTIES
    DESCRIPTION "libmtp, an Initiator implementation of the Media Transfer Protocol (MTP)"
    URL "http://libmtp.sourceforge.net/"
)
