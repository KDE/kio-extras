# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2024 Harald Sitter <sitter@kde.org>

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
