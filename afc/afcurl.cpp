/*
 * SPDX-FileCopyrightText: 2022 Kai Uwe Broulik <kde@broulik.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "afcurl.h"

AfcUrl::AfcUrl(const QUrl &url)
    : m_url(url)
{
    if (!url.isValid() || url.scheme() != QLatin1String("afc")) {
        return;
    }

    m_device = url.host().toUpper();
    m_path = url.path();

    if (!m_path.isEmpty()) {
        Q_ASSERT(m_path.startsWith(QLatin1Char('/')));
    }

    const QString appsPrefix = QLatin1Char('/') + appsTag();
    if (m_path == appsPrefix || m_path.startsWith(appsPrefix + QLatin1Char('/'))) {
        m_browseMode = BrowseMode::Apps;
        m_path = m_path.mid(appsPrefix.length());

        int slashAfterAppIdx = m_path.indexOf(QLatin1Char('/'), 1);
        if (slashAfterAppIdx == -1) {
            slashAfterAppIdx = m_path.length();
        }

        m_appId = m_path.mid(1, slashAfterAppIdx - 1); // exclude slashes in app ID
        m_path = m_path.mid(slashAfterAppIdx); // include leading slash in path
    } else {
        m_browseMode = BrowseMode::FileSystem;
    }

    if (m_path == QLatin1Char('/')) {
        m_path.clear();
    }
}

QUrl AfcUrl::url() const
{
    return m_url;
}

AfcUrl::BrowseMode AfcUrl::browseMode() const
{
    return m_browseMode;
}

QString AfcUrl::device() const
{
    return m_device;
}

QString AfcUrl::appId() const
{
    return m_appId;
}

QString AfcUrl::path() const
{
    return m_path;
}

bool AfcUrl::isValid() const
{
    return m_browseMode == BrowseMode::FileSystem || m_browseMode == BrowseMode::Apps;
}

QString AfcUrl::appsTag()
{
    return QStringLiteral("@apps");
}
