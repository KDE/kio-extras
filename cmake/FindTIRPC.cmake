# - Try to find TI-RPC
#
# The following variables will be available once found :
#
# TIRPC_INCLUDE_DIRS - The TI-RPC headers location
# TIRPC_LIBRARIES - Link these to use TI-RPC
# TIRPC_VERSION - The TIRPC version
#
#=============================================================================
# Copyright (c) 2017 Christophe Giboudeaux <christophe@krop.fr>
#
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. The name of the author may not be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#=============================================================================

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
