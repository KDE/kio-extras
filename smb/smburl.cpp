/////////////////////////////////////////////////////////////////////////////
//
// Project:     SMB kioslave for KDE2
//
// File:        smburl.cpp
//
// Abstract:    Utility class implementation used by SMBSlave
//
// Author(s):   Matthew Peterson <mpeterson@caldera.com>
//
//---------------------------------------------------------------------------
//
// Copyright (c) 2000  Caldera Systems, Inc.
// Copyright (c) 2020  Harald Sitter <sitter@kde.org>
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2.1 of the License, or
// (at your option) any later version.
//
//     This program is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with this program; see the file COPYING.  If not, please obtain
//     a copy from https://www.gnu.org/copyleft/gpl.html
//
/////////////////////////////////////////////////////////////////////////////

#include "smburl.h"
#include "smb-logsettings.h"

#include <QDir>
#include <QHostAddress>
#include <KConfig>
#include <KIO/Global>

SMBUrl::SMBUrl(const QUrl &kurl)
    : QUrl(kurl)
{
    // We treat cifs as an alias but need to translate it to smb.
    // https://bugs.kde.org/show_bug.cgi?id=327295
    // It's not IANA registered and also libsmbc internally expects
    // smb URIs so we do very broadly coerce cifs to smb.
    // Also see SMBSlave::checkURL.
    if (scheme() == "cifs") {
        setScheme("smb");
    }
    updateCache();
}

SMBUrl::SMBUrl() = default;
SMBUrl::SMBUrl(const SMBUrl &other) = default;
SMBUrl::~SMBUrl() = default;
SMBUrl &SMBUrl::operator=(const SMBUrl &) = default;

void SMBUrl::addPath(const QString &filedir)
{
    if (path().length() > 0 && path().at(path().length() - 1) != QLatin1Char('/')) {
        QUrl::setPath(path() + QLatin1Char('/') + filedir);
    } else {
        QUrl::setPath(path() + filedir);
    }
    updateCache();
}

void SMBUrl::cdUp()
{
    setUrl(KIO::upUrl(*this).url());
    updateCache();
}

void SMBUrl::updateCache()
{
    QUrl::setPath(QDir::cleanPath(path()));

    // SMB URLs are UTF-8 encoded
    qCDebug(KIO_SMB_LOG) << "updateCache " << QUrl::path();

    QUrl sambaUrl(*this);

    const QHostAddress address(sambaUrl.host());
    switch (address.protocol()) {
    case QAbstractSocket::IPv6Protocol: {
        // Convert to Windows IPv6 literal to bypass limitations in samba.
        // https://bugzilla.samba.org/show_bug.cgi?id=14297
        // https://docs.microsoft.com/en-us/windows/win32/api/winnetwk/nf-winnetwk-wnetaddconnection2a
        // https://devblogs.microsoft.com/oldnewthing/20100915-00/?p=12863
        // https://www.samba.org/~idra/code/nss-ipv6literal/README.html
        // https://ipv6-literal.com
        QString literal = address.toString();
        literal.replace(':', '-'); // address
        literal.replace('%', 's'); // scope
        if (literal.front() == '-') {
            // Special prefix for [::f] so it doesn't start with a dash.
            literal.prepend('0');
        }
        if (literal.back() == '-') {
            // Special suffix, also cannot end with a dash.
            literal.append('0');
        }
        literal += ".ipv6-literal.net"; // reserved host host
        qCDebug(KIO_SMB_LOG) << "converting IPv6 to literal " << host() << literal;
        sambaUrl.setHost(literal);
        break;
    }
    case QAbstractSocket::IPv4Protocol:
    case QAbstractSocket::AnyIPProtocol:
    case QAbstractSocket::UnknownNetworkLayerProtocol:
        break;
    }

    if (sambaUrl.url() == "smb:/")
        m_surl = "smb://";
    else
        m_surl = sambaUrl.toString(QUrl::PrettyDecoded).toUtf8();

    m_type = SMBURLTYPE_UNKNOWN;
    // update m_type
    (void)getType();
}

SMBUrlType SMBUrl::getType() const
{
    if (m_type != SMBURLTYPE_UNKNOWN)
        return m_type;

    if (scheme() != "smb") {
        m_type = SMBURLTYPE_UNKNOWN;
        return m_type;
    }

    if (path().isEmpty() || path(QUrl::FullyDecoded) == "/") {
        if (host().isEmpty())
            m_type = SMBURLTYPE_ENTIRE_NETWORK;
        else
            m_type = SMBURLTYPE_WORKGROUP_OR_SERVER;
        return m_type;
    }

    // Check for the path if we get this far
    m_type = SMBURLTYPE_SHARE_OR_PATH;

    return m_type;
}

SMBUrl SMBUrl::partUrl() const
{
    if (m_type == SMBURLTYPE_SHARE_OR_PATH && !fileName().isEmpty()) {
        SMBUrl url(*this);
        url.setPath(path() + QLatin1String(".part"));
        return url;
    }

    return SMBUrl();
}