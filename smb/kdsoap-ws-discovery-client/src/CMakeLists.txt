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

set(GLOBAL_KSWSDL2CPP_OPTION)
get_filename_component(KDWSDL_IMPORT_PATH ../wsdl/ ABSOLUTE)
set(GLOBAL_KSWSDL2CPP_OPTION -import-path ${KDWSDL_IMPORT_PATH} -use-local-files-only)

set(KSWSDL2CPP_OPTION -namespace WSDiscovery200901 ${GLOBAL_KSWSDL2CPP_OPTION})
KDSOAP_GENERATE_WSDL(KDSoapWSDiscoveryClient_WSDL ${KDWSDL_IMPORT_PATH}/docs.oasis-open.org/ws-dd/discovery/1.1/os/wsdd-discovery-1.1-wsdl-os.wsdl)
set(KSWSDL2CPP_OPTION -namespace WSDiscovery200504 ${GLOBAL_KSWSDL2CPP_OPTION})
KDSOAP_GENERATE_WSDL(KDSoapWSDiscoveryClient_WSDL ${KDWSDL_IMPORT_PATH}/schemas.xmlsoap.org/ws/2005/04/discovery/ws-discovery.wsdl)

set(KDSoapWSDiscoveryClient_SRCS
    "wsdiscoveryclient.cpp"
    "wsdiscoveryprobejob.cpp"
    "wsdiscoverytargetservice.cpp"
    "wsdiscoveryserviceaggregator.cpp"
)

ecm_qt_declare_logging_category(KDSoapWSDiscoveryClient_SRCS
    HEADER loggingcategory.h
    IDENTIFIER KDSoapWSDiscoveryClient
    CATEGORY_NAME kdsoap.wsdiscoveryclient
    DEFAULT_SEVERITY Warning
)

add_library(KDSoapWSDiscoveryClient STATIC ${KDSoapWSDiscoveryClient_SRCS} ${KDSoapWSDiscoveryClient_WSDL})
generate_export_header(KDSoapWSDiscoveryClient BASE_NAME WSDiscoveryClient)
add_library(KDSoap::WSDiscoveryClient ALIAS KDSoapWSDiscoveryClient)

target_include_directories(KDSoapWSDiscoveryClient
    INTERFACE "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR}/KDSoapWSDiscoveryClient>"
)

target_link_libraries(KDSoapWSDiscoveryClient
    PUBLIC
        Qt5::Core
        KDSoap::kdsoap
    PRIVATE
)

set_target_properties(KDSoapWSDiscoveryClient PROPERTIES
    VERSION     ${WSDISCOVERY_VERSION}
    SOVERSION   ${WSDISCOVERY_SOVERSION}
    EXPORT_NAME "WSDiscoveryClient"
)

ecm_generate_headers(KDSoapWSDiscoveryClient_HEADERS
    HEADER_NAMES
    WSDiscoveryClient
    WSDiscoveryProbeJob
    WSDiscoveryTargetService
    WSDiscoveryServiceAggregator

    REQUIRED_HEADERS KDSoapWSDiscoveryClient_HEADERS
)

# install(TARGETS KDSoapWSDiscoveryClient
#         EXPORT KDSoapWSDiscoveryClientTargets
#         ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
# 
# install(FILES
#             ${CMAKE_CURRENT_BINARY_DIR}/wsdiscoveryclient_export.h
#             ${KDSoapWSDiscoveryClient_HEADERS}
#         DESTINATION ${KDE_INSTALL_INCLUDEDIR}/KDSoapWSDiscoveryClient
#         COMPONENT Devel)
# 
# if(BUILD_QCH)
#     ecm_add_qch(
#         KDSoapWSDiscoveryClient_QCH
#         NAME KDSoapWSDiscoveryClient
#         BASE_NAME KDSoapWSDiscoveryClient
#         VERSION ${WSDISCOVERY_VERSION}
#         ORG_DOMAIN net.meijn
#         SOURCES # using only public headers, to cover only public API
#             ${KDSoapWSDiscoveryClient_HEADERS}
#         MD_MAINPAGE "${CMAKE_SOURCE_DIR}/README.md"
#         LINK_QCHS
#             Qt5Core_QCH
#             KDSoap_QCH
#         BLANK_MACROS
#             KARCHIVE_EXPORT
#             KARCHIVE_DEPRECATED
#         TAGFILE_INSTALL_DESTINATION ${KDE_INSTALL_QTQCHDIR}
#         QCH_INSTALL_DESTINATION ${KDE_INSTALL_QTQCHDIR}
#         COMPONENT Devel
#     )
# endif()
