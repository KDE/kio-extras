# Copyright (C) 2019 Casper Meijn <casper@meijn.net>
# SPDX-License-Identifier: GPL-3.0-or-later
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.7)

find_package(Doxygen
             OPTIONAL_COMPONENTS dot mscgen dia)
             
if(DOXYGEN_FOUND)             
    doxygen_add_docs(docs
        "README.md"
        "src/wsdiscoveryclient.h"
        "src/wsdiscoveryprobejob.h"
        "src/wsdiscoverytargetservice.h"
        "src/wsdiscoveryserviceaggregator.h"
        ALL
        USE_STAMP_FILE
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
        )
    install(DIRECTORY
            ${CMAKE_CURRENT_BINARY_DIR}/html/
        DESTINATION ${KDE_INSTALL_DATADIR}/doc/KDSoapWSDiscoveryClient
        COMPONENT Devel)
endif()
