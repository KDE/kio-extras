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

    QString iconPath() const;

    KIO::UDSEntry entry(const QString &name = QString()) const;

private:
    friend class AfcDevice;

    QString m_bundleId;
    QString m_displayName;
    QString m_iconPath;
    bool m_sharingEnabled = false;
};
