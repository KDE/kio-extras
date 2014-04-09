# cmake macro to test SLP LIB

# Copyright (c) 2006, 2007 Laurent Montel, <montel@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.


if (SLP_INCLUDE_DIR AND SLP_LIBRARIES)
    # Already in cache, be silent
    set(SLP_FIND_QUIETLY TRUE)
endif (SLP_INCLUDE_DIR AND SLP_LIBRARIES)


FIND_PATH(SLP_INCLUDE_DIR slp.h)

FIND_LIBRARY(SLP_LIBRARIES NAMES slp libslp)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(SLP DEFAULT_MSG SLP_LIBRARIES SLP_INCLUDE_DIR )

MARK_AS_ADVANCED(SLP_INCLUDE_DIR SLP_LIBRARIES)
