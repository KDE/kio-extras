/*
 * SPDX-FileCopyrightText: 2022 Kai Uwe Broulik <kde@broulik.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QString>
#include <QUrl>

class AfcUrl
{
public:
    explicit AfcUrl(const QUrl &url);

    enum class BrowseMode { None, FileSystem, Apps };

    QUrl url() const;

    BrowseMode browseMode() const;

    // NOTE make sure to resolve friendly names
    QString device() const;
    QString appId() const;
    QString path() const;

    bool isValid() const;

private:
    QUrl m_url;
    BrowseMode m_browseMode = BrowseMode::None;
    QString m_device;
    QString m_appId;
    QString m_path;
};
