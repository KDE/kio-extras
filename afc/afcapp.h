/*
 * SPDX-FileCopyrightText: 2022 Kai Uwe Broulik <kde@broulik.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
*/


#pragma once

#include <QString>

#include <plist/plist.h>

#include <KIO/UDSEntry>

class AfcApp
{
public:
    AfcApp();
    explicit AfcApp(plist_t app);

    bool isValid() const;

    QString bundleId() const;
    QString displayName() const;
    bool sharingEnabled() const;
    // TODO add a way to fetch the app icon, cache it, and use it as folder icon overlay

    KIO::UDSEntry entry(const QString &name = QString()) const;

private:
    QString m_bundleId;
    QString m_displayName;
    bool m_sharingEnabled = false;
};
