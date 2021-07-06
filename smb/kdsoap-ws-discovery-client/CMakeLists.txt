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

project(kdsoap-ws-discovery-client VERSION 0.2)

include(FeatureSummary)
find_package(ECM 5.54.0  NO_MODULE)
set_package_properties(ECM PROPERTIES TYPE REQUIRED DESCRIPTION "Extra CMake Modules." URL "https://projects.kde.org/projects/kdesupport/extra-cmake-modules")
feature_summary(WHAT REQUIRED_PACKAGES_NOT_FOUND FATAL_ON_MISSING_REQUIRED_PACKAGES)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

include(CTest)
include(KDEInstallDirs)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDECMakeSettings)

include(GenerateExportHeader)

set(REQUIRED_QT_VERSION 5.9.0)
find_package(Qt5 ${REQUIRED_QT_VERSION} REQUIRED NO_MODULE COMPONENTS Core Network Xml)

find_package(KDSoap 1.9.0 REQUIRED)
set_package_properties(KDSoap PROPERTIES
    DESCRIPTION "A Qt-based client-side and server-side SOAP component"
    URL "http://www.kdab.com/products/kd-soap"
    TYPE REQUIRED
    PURPOSE "Support for SOAP client protocol"
)

if (BUILD_TESTING)
    find_package(Qt5 ${REQUIRED_QT_VERSION} CONFIG REQUIRED Test)
endif()

include(ECMSetupVersion)
include(ECMGenerateHeaders)
include(ECMQtDeclareLoggingCategory)
include(ECMAddQch)

option(BUILD_QCH "Build API documentation in QCH format (for e.g. Qt Assistant, Qt Creator & KDevelop)" OFF)
add_feature_info(QCH ${BUILD_QCH} "API documentation in QCH format (for e.g. Qt Assistant, Qt Creator & KDevelop)")

ecm_setup_version(PROJECT
    VARIABLE_PREFIX WSDISCOVERY
    VERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/kdsoap_wsdiscovery_client_version.h"
    PACKAGE_VERSION_FILE "${CMAKE_CURRENT_BINARY_DIR}/KDSoapWSDiscoveryClientConfigVersion.cmake"
)

add_subdirectory(src)

# Disable for imported source
if (FALSE)
add_subdirectory(examples)
add_subdirectory(docs)  
if(BUILD_TESTING)
    add_subdirectory(autotests)
endif()

# create a Config.cmake and a ConfigVersion.cmake file and install them
set(CMAKECONFIG_INSTALL_DIR "${KDE_INSTALL_CMAKEPACKAGEDIR}/KDSoapWSDiscoveryClient")

if (BUILD_QCH)
    ecm_install_qch_export(
        TARGETS KDSoapWSDiscoveryClient_QCH
        FILE KDSoapWSDiscoveryClientQchTargets.cmake
        DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
        COMPONENT Devel
    )
    set(PACKAGE_INCLUDE_QCHTARGETS "include(\"\${CMAKE_CURRENT_LIST_DIR}/KDSoapWSDiscoveryClientQchTargets.cmake\")")
endif()

include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/KDSoapWSDiscoveryClientConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/KDSoapWSDiscoveryClientConfig.cmake"
    INSTALL_DESTINATION ${CMAKECONFIG_INSTALL_DIR}
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/kdsoap_wsdiscovery_client_version.h
        DESTINATION ${KDE_INSTALL_INCLUDEDIR}/KDSoapWSDiscoveryClient
        COMPONENT Devel)

install(FILES
            "${CMAKE_CURRENT_BINARY_DIR}/KDSoapWSDiscoveryClientConfig.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/KDSoapWSDiscoveryClientConfigVersion.cmake"
        DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
        COMPONENT Devel)

install(EXPORT KDSoapWSDiscoveryClientTargets
        DESTINATION "${CMAKECONFIG_INSTALL_DIR}"
        FILE KDSoapWSDiscoveryClientTargets.cmake
        NAMESPACE KDSoap::)

endif()
# Disable for imported source

feature_summary(
    WHAT ALL
    FATAL_ON_MISSING_REQUIRED_PACKAGES
)
