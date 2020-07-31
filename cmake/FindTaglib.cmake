# - Try to find the Taglib library
# Once done this will define
#
#  TAGLIB_FOUND - system has the taglib library
#  TAGLIB_INCLUDES - the taglib includes
#  TAGLIB_LIBRARIES - The libraries needed to use taglib

# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2006 Laurent Montel <montel@kde.org>

if(NOT TAGLIB_MIN_VERSION)
  set(TAGLIB_MIN_VERSION "1.4")
endif(NOT TAGLIB_MIN_VERSION)

if(NOT WIN32)
    find_program(TAGLIBCONFIG_EXECUTABLE NAMES taglib-config PATHS
       ${BIN_INSTALL_DIR}
    )
endif(NOT WIN32)

#reset vars
set(TAGLIB_LIBRARIES)
set(TAGLIB_CFLAGS)

# if taglib-config has been found
if(TAGLIBCONFIG_EXECUTABLE)

  exec_program(${TAGLIBCONFIG_EXECUTABLE} ARGS --version RETURN_VALUE _return_VALUE OUTPUT_VARIABLE TAGLIB_VERSION)

  exec_program(${TAGLIBCONFIG_EXECUTABLE} ARGS --libs RETURN_VALUE _return_VALUE OUTPUT_VARIABLE TAGLIB_LIBRARIES)

  exec_program(${TAGLIBCONFIG_EXECUTABLE} ARGS --cflags RETURN_VALUE _return_VALUE OUTPUT_VARIABLE TAGLIB_CFLAGS)

  # Assume software will include by things like #include <taglib/tfile.h> instead of <tfile.h>
  string(REPLACE "/taglib" "" TAGLIB_STRIPPED_INCLUDE_PATHS "${TAGLIB_CFLAGS}")
  string(REGEX REPLACE " *-I" ";" TAGLIB_INCLUDES "${TAGLIB_STRIPPED_INCLUDE_PATHS}")

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Taglib FOUND_VAR TAGLIB_FOUND
                                    REQUIRED_VARS TAGLIB_LIBRARIES TAGLIB_INCLUDES
                                    VERSION_VAR TAGLIB_VERSION)
  mark_as_advanced(TAGLIB_CFLAGS TAGLIB_LIBRARIES TAGLIB_INCLUDES)

else(TAGLIBCONFIG_EXECUTABLE)

  include(FindPackageHandleStandardArgs)

  # To avoid issues, make the same assumption as above:
  # Assume software will include by things like #include <taglib/tfile.h> instead of <tfile.h>
  find_path(TAGLIB_INCLUDES
    NAMES
    taglib/tag.h
    PATHS
    ${INCLUDE_INSTALL_DIR}
  )

  # TODO: this originally used find_library_with_debug(),
  # which is only available with KDELibs4Support, with
  # arguments "WIN32_DEBUG_POSTFIX d".  It is equivalent
  # to find_library on all platforms other than Win32.
  find_library(TAGLIB_LIBRARIES
    NAMES tag
    PATHS
    ${LIB_INSTALL_DIR}
  )

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Taglib DEFAULT_MSG
                                    TAGLIB_INCLUDES TAGLIB_LIBRARIES)
endif(TAGLIBCONFIG_EXECUTABLE)
