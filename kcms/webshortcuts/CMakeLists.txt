# SPDX-FileCopyrightText: 2026 Tobias Fella <tobias.fella@kde.org>
# SPDX-License-Identifier: BSD-2-Clause

kcmutils_add_qml_kcm(kcm_webshortcuts
    SOURCES
        providersmodel.cpp
        providersmodel.h
        searchprovider.cpp
        searchprovider.h
        searchproviderregistry.cpp
        searchproviderregistry.h
        webshortcuts.cpp
        webshortcuts.h
)

target_link_libraries(kcm_webshortcuts PRIVATE
    Qt::Gui
    Qt::Qml
    KF6::ConfigCore
    KF6::CoreAddons
    KF6::I18n
    KF6::KCMUtils
    KF6::KIOCore
    KF6::KIOGui
    KF6::Service
)

if(USE_DBUS)
  target_link_libraries(kcm_webshortcuts PRIVATE Qt6::DBus)
  target_compile_definitions(kcm_webshortcuts PUBLIC -DWITH_DBUS)
endif()
