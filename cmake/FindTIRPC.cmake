# - Try to find TI-RPC
#
# The following variables will be available once found :
#
# TIRPC_INCLUDE_DIRS - The TI-RPC headers location
# TIRPC_LIBRARIES - Link these to use TI-RPC
# TIRPC_VERSION - The TIRPC version

# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2017 Christophe Giboudeaux <christophe@krop.fr>

find_package(PkgConfig QUIET)
pkg_check_modules(PC_TIRPC libtirpc)

find_path(TIRPC_INCLUDE_DIRS
  NAMES netconfig.h
  PATH_SUFFIXES tirpc
  HINTS ${PC_TIRPC_INCLUDE_DIRS}
)

find_library(TIRPC_LIBRARIES
  NAMES tirpc
  HINTS ${PC_TIRPC_LIBRARY_DIRS}
)

set(TIRPC_VERSION ${PC_TIRPC_VERSION})

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(TIRPC
      REQUIRED_VARS TIRPC_LIBRARIES TIRPC_INCLUDE_DIRS
      VERSION_VAR TIRPC_VERSION
)

mark_as_advanced(TIRPC_INCLUDE_DIRS TIRPC_LIBRARIES)
