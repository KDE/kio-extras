# - Try to find the Taglib library
# Once done this will define
#
#  TAGLIB_FOUND - system has the taglib library
#  TAGLIB_INCLUDES - system has the taglib api
#  TAGLIB_LIBRARIES - The libraries needed to use taglib

# Copyright (c) 2017, Anthony Fieroni, <bvbfan@abv.bg>

set(TAGLIB_LIBRARIES)
set(TAGLIB_INCLUDES)

include(FindPackageHandleStandardArgs)

find_path(TAGLIB_INCLUDES
            NAMES tag.h
            PATH_SUFFIXES taglib
            PATHS
            ${INCLUDE_INSTALL_DIR}
)

find_library(TAGLIB_LIBRARIES
                NAMES tag
                PATHS
                ${LIB_INSTALL_DIR}
)

find_package_handle_standard_args(Taglib DEFAULT_MSG TAGLIB_INCLUDES TAGLIB_LIBRARIES)

if(TAGLIB_FOUND)
    message(STATUS "Taglib has been found for AudioThumbnail")
else(TAGLIB_FOUND)
    message(WARNING "Taglib is needed for AudioThumbnail")
endif(TAGLIB_FOUND)
