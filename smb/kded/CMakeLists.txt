# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2020 Harald Sitter <sitter@kde.org>

configure_file(config.h.cmake config.h)

add_executable(smbnotifier notifier.cpp)
target_link_libraries(smbnotifier KF${QT_MAJOR_VERSION}::KIOCore kio_smb_static)
target_link_options(smbnotifier PUBLIC "LINKER:--as-needed") # shrink to bare minimum we fork this a lot
install(TARGETS smbnotifier DESTINATION ${KDE_INSTALL_LIBEXECDIR_KF})

add_library(kded-smbwatcher MODULE watcher.cpp)
target_link_libraries(kded-smbwatcher KF${QT_MAJOR_VERSION}::DBusAddons KF${QT_MAJOR_VERSION}::KIOCore kio_smb_static)
set_target_properties(kded-smbwatcher PROPERTIES OUTPUT_NAME smbwatcher)
target_link_options(kded-smbwatcher PUBLIC "LINKER:--as-needed") # shrink to bare minimum we load this into kded
install(TARGETS kded-smbwatcher DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf${QT_MAJOR_VERSION}/kded)
