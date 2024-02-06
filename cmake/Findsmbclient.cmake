# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2024 Harald Sitter <sitter@kde.org>

# This will define the following variables:
#
# ``smbclient_FOUND``
#     True if smbclient is available
# ``smbclient_VERSION``
#     The version of smbclient

find_package(PkgConfig QUIET)
pkg_check_modules(smbclient QUIET IMPORTED_TARGET GLOBAL smbclient)

if(TARGET PkgConfig::smbclient)
    add_library(smbclient::smbclient ALIAS PkgConfig::smbclient)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(smbclient
    REQUIRED_VARS
        smbclient_FOUND
    VERSION_VAR
        smbclient_VERSION
)

mark_as_advanced(smbclient_VERSION)
