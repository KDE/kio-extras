/*
    SPDX-License-Identifier: GPL-2.0-or-later
    SPDX-FileCopyrightText: 2000 Caldera Systems Inc.
    SPDX-FileCopyrightText: 2020-2021 Harald Sitter <sitter@kde.org>
    SPDX-FileContributor: Matthew Peterson <mpeterson@caldera.com>
*/

#include "smburl.h"
#include "smb-logsettings.h"

#include <QDir>
#include <QHostAddress>
#include <QUrlQuery>
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

    // NetBios workgroup names may contain characters that QUrl will not
    // allow in a host. Yet the SMB URI requires us to have the workgroup
    // in the host field when browsing a workgroup.
    // As a hacky workaround we'll not set a host but use a query param
    // when encountering a workgroup that causes QUrl to error out.
    // For libsmbc we then need to translate the query back to SMB URI.
    // Since this is super daft string construction it will doubltlessly
    // be imperfect and so we do still prefer deferring the string
    // construction to QUrl whenever possible.
    // https://support.microsoft.com/en-gb/help/909264/naming-conventions-in-active-directory-for-computers-domains-sites-and
    // https://bugs.kde.org/show_bug.cgi?id=204423
    //
    // Should we ever stop supporting workgroup browsing this entire
    // hack can be removed.
    QUrlQuery query(sambaUrl);
    const QString workgroup = query.queryItemValue("kio-workgroup");
    if (workgroup.isEmpty()) {
        // If we don't have a hack to apply we can simply defer to QUrl
        if (sambaUrl.url() == "smb:/") {
            m_surl = "smb://";
        } else {
            m_surl = sambaUrl.toString(QUrl::PrettyDecoded).toUtf8();
        }
    } else {
        // If we have a workgroup hack to apply we need to manually construct
        // the stringy URI.
        query.removeQueryItem("kio-workgroup");
        sambaUrl.setQuery(query);

        QString url;
        url = "smb://";
        if (!sambaUrl.userInfo().isEmpty()) {
            url += sambaUrl.userInfo() + "@";
        }
        url += workgroup;
        // Workgroups can have ports per the IANA definition of smb.
        if (sambaUrl.port() != -1) {
            url += ':' + QString::number(sambaUrl.port());
        }

        // Make sure to only use clear paths. libsmbc is allergic to excess slashes.
        QString path('/');
        if (!sambaUrl.host().isEmpty()) {
            path += sambaUrl.host();
        }
        if (!sambaUrl.path().isEmpty()) {
            path += sambaUrl.path();
        }
        url += QDir::cleanPath(path);

        if (!sambaUrl.query().isEmpty()) {
            url += '?' + sambaUrl.query();
        }
        if (!sambaUrl.fragment().isEmpty()) {
            url += '#' + sambaUrl.fragment();
        }
        m_surl = url.toUtf8();
    }

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
        if (host().isEmpty() && !query().contains("kio-workgroup"))
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
    const bool isRemoteFile = m_type == SMBURLTYPE_SHARE_OR_PATH && !fileName().isEmpty();
    const bool isLocalFile = scheme() == QLatin1String("file") && !fileName().isEmpty();
    //  Mind that filename doesn't necessarily mean it is a file.
    if (isRemoteFile || isLocalFile) {
        SMBUrl url(*this);
        url.setPath(path() + QLatin1String(".part"));
        return url;
    }

    return SMBUrl();
}
