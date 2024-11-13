# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2024 Harald Sitter <sitter@kde.org>

if(NOT LINUX)
    return()
endif()

option(WITH_LIBPROXY "Build with libproxy, required for WPAD problem detection" ON)
if(NOT WITH_LIBPROXY)
    return()
endif()

find_package(PkgConfig)
pkg_check_modules(libproxy REQUIRED IMPORTED_TARGET libproxy-1.0)
add_feature_info("libproxy" libproxy_FOUND "libproxy for WPAD problem detection")

if(NOT libproxy_FOUND)
    return()
endif()

kcoreaddons_add_plugin(wpad-detector SOURCES module.cpp INSTALL_NAMESPACE "kf6/kded")
target_link_libraries(wpad-detector
    KF6::I18n
    KF6::ConfigCore
    KF6::DBusAddons
    KF6::Notifications
    KF6::CoreAddons
    Qt6::Network
)

add_executable(wpad-detector-helper main.cpp)
target_link_libraries(wpad-detector-helper PkgConfig::libproxy)
install(TARGETS wpad-detector-helper DESTINATION ${KDE_INSTALL_LIBEXECDIR})
