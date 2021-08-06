# - Try to find libssh
# Once done this will define
#
#  LIBSSH_FOUND - system has libssh
#  LIBSSH_INCLUDE_DIR - the libssh include directory
#  LIBSSH_LIBRARIES - Link these to use libssh
#  LIBSSH_DEFINITIONS - Compiler switches required for using libssh

# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2009-2014 Andreas Schneider <asn@cryptomilk.org>
# SPDX-FileCopyrightText: 2018 Harald Sitter <sitter@kde.org>

# Define an imported target to have compatibility with newer libssh and so
# we have a single target to use regardless of the code path taken in the
# finder and the actual libssh version defining the target.
macro(libssh_ensure_imported_target)
  if(NOT TARGET ssh)
      add_library(ssh SHARED IMPORTED)
      set_target_properties(ssh PROPERTIES
          IMPORTED_LOCATION "${LIBSSH_LIBRARIES}"
          INTERFACE_INCLUDE_DIRECTORIES "${LIBSSH_INCLUDE_DIR}"
      )
  endif()
endmacro()

# We prefer the config, but on Ubuntu 18.04 LTS (and to some extent later
# versions it seems) they've not packaged the config properly. So, go for the
# config by default and fall back to manual lookup iff the config was not found.
# https://bugs.kde.org/show_bug.cgi?id=400291
# https://bugs.launchpad.net/ubuntu/+source/libssh/+bug/1800135
find_package(libssh ${libssh_FIND_VERSION} NO_MODULE QUIET)
if(libssh_FOUND)
  # Certain versions with config may not have the target, so make sure it's
  # defined.
  libssh_ensure_imported_target()
  return()
endif()

find_path(LIBSSH_INCLUDE_DIR
  NAMES
    libssh/libssh.h
  PATHS
    /usr/include
    /usr/local/include
    /opt/local/include
    /sw/include
    ${CMAKE_INCLUDE_PATH}
    ${CMAKE_INSTALL_PREFIX}/include
)

find_library(SSH_LIBRARY
  NAMES
    ssh
    libssh
  PATHS
    /usr/lib
    /usr/local/lib
    /opt/local/lib
    /sw/lib
    ${CMAKE_LIBRARY_PATH}
    ${CMAKE_INSTALL_PREFIX}/lib
)

set(LIBSSH_LIBRARIES
    ${LIBSSH_LIBRARIES}
    ${SSH_LIBRARY}
)

message("LIB: ${LIBSSH_LIBRARIES}; INC: ${LIBSSH_INCLUDE_DIR};")

if (LIBSSH_INCLUDE_DIR AND libssh_FIND_VERSION)
  file(STRINGS ${LIBSSH_INCLUDE_DIR}/libssh/libssh.h LIBSSH_VERSION_MAJOR
    REGEX "#define[ ]+LIBSSH_VERSION_MAJOR[ ]+[0-9]+")

  # Older versions of libssh like libssh-0.2 have LIBSSH_VERSION but not LIBSSH_VERSION_MAJOR
  if (LIBSSH_VERSION_MAJOR)
    string(REGEX MATCH "[0-9]+" LIBSSH_VERSION_MAJOR ${LIBSSH_VERSION_MAJOR})
    file(STRINGS ${LIBSSH_INCLUDE_DIR}/libssh/libssh.h LIBSSH_VERSION_MINOR
      REGEX "#define[ ]+LIBSSH_VERSION_MINOR[ ]+[0-9]+")
    string(REGEX MATCH "[0-9]+" LIBSSH_VERSION_MINOR ${LIBSSH_VERSION_MINOR})
    file(STRINGS ${LIBSSH_INCLUDE_DIR}/libssh/libssh.h LIBSSH_VERSION_PATCH
      REGEX "#define[ ]+LIBSSH_VERSION_MICRO[ ]+[0-9]+")
    string(REGEX MATCH "[0-9]+" LIBSSH_VERSION_PATCH ${LIBSSH_VERSION_PATCH})

    set(LIBSSH_VERSION ${LIBSSH_VERSION_MAJOR}.${LIBSSH_VERSION_MINOR}.${LIBSSH_VERSION_PATCH})

  else (LIBSSH_VERSION_MAJOR)
    message(STATUS "LIBSSH_VERSION_MAJOR not found in ${LIBSSH_INCLUDE_DIR}/libssh/libssh.h, assuming libssh is too old")
    set(LIBSSH_FOUND FALSE)
  endif (LIBSSH_VERSION_MAJOR)
endif (LIBSSH_INCLUDE_DIR AND libssh_FIND_VERSION)

# If the version is too old, but libs and includes are set,
# find_package_handle_standard_args will set LIBSSH_FOUND to TRUE again,
# so we need this if() here.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libssh
                                  FOUND_VAR
                                    LIBSSH_FOUND
                                  REQUIRED_VARS
                                    LIBSSH_LIBRARIES
                                    LIBSSH_INCLUDE_DIR
                                  VERSION_VAR
                                    LIBSSH_VERSION)

libssh_ensure_imported_target()

# show the LIBSSH_INCLUDE_DIR and LIBSSH_LIBRARIES variables only in the advanced view
mark_as_advanced(LIBSSH_INCLUDE_DIR LIBSSH_LIBRARIES)
