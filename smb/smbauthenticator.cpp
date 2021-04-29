/*
    SPDX-License-Identifier: GPL-2.0-or-later
    SPDX-FileCopyrightText: 2000 Caldera Systems Inc.
    SPDX-FileCopyrightText: 2020 Harald Sitter <sitter@kde.org>
    SPDX-FileContributor: Matthew Peterson <mpeterson@caldera.com>
*/

#include "smbauthenticator.h"

#include <KConfig>
#include <KConfigGroup>
#include <KIO/AuthInfo>
#include <KLocalizedString>
#include <QTextCodec>

#include "smburl.h"
#include "smb-logsettings.h"

SMBAuthenticator::SMBAuthenticator(SMBAbstractFrontend &frontend)
    : m_frontend(frontend)
{
}

void SMBAuthenticator::loadConfiguration()
{
    KConfig cfg("kioslaverc", KConfig::NoGlobals);
    const KConfigGroup group = cfg.group("Browser Settings/SMBro");
    m_defaultUser = group.readEntry("User");
    //  m_default_workgroup=group.readEntry("Workgroup");
    //  m_showHiddenShares=group.readEntry("ShowHiddenShares", QVariant(false)).toBool();

    QString m_encoding = QTextCodec::codecForLocale()->name();
    m_defaultEncoding = group.readEntry("Encoding", m_encoding.toLower());

    // unscramble, taken from Nicola Brodu's smb ioslave
    // not really secure, but better than storing the plain password
    QString scrambled = group.readEntry("Password");
    m_defaultPassword = "";
    for (int i = 0; i < scrambled.length() / 3; i++) {
        QChar qc1 = scrambled[i * 3];
        QChar qc2 = scrambled[i * 3 + 1];
        QChar qc3 = scrambled[i * 3 + 2];
        unsigned int a1 = qc1.toLatin1() - '0';
        unsigned int a2 = qc2.toLatin1() - 'A';
        unsigned int a3 = qc3.toLatin1() - '0';
        unsigned int num = ((a1 & 0x3F) << 10) | ((a2 & 0x1F) << 5) | (a3 & 0x1F);
        m_defaultPassword[i] = QChar((uchar)((num - 17) ^ 173)); // restore
    }
}

QString SMBAuthenticator::defaultWorkgroup() const
{
    return m_defaultWorkgroup;
}

void SMBAuthenticator::setDefaultWorkgroup(const QString &workGroup)
{
    m_defaultWorkgroup = workGroup;
}

void SMBAuthenticator::auth(const char *server, const char *share, char *workgroup, int wgmaxlen, char *username, int unmaxlen, char *password, int pwmaxlen)
{
    qCDebug(KIO_SMB_LOG) << "auth_smbc_get_dat: set user=" << username << ", workgroup=" << workgroup
                         << " server=" << server << ", share=" << share;

    QString s_server = QString::fromUtf8(server);
    QString s_share = QString::fromUtf8(share);
    workgroup[wgmaxlen - 1] = 0;
    QString s_workgroup = QString::fromUtf8(workgroup);
    username[unmaxlen - 1] = 0;
    QString s_username = QString::fromUtf8(username);
    password[pwmaxlen - 1] = 0;
    QString s_password = QString::fromUtf8(password);

    KIO::AuthInfo info;
    info.url = QUrl("smb:///");
    info.url.setHost(s_server);
    info.url.setPath('/' + s_share);

    // check this to see if we "really" need to authenticate...
    if (SMBUrl(info.url).getType() == SMBURLTYPE_ENTIRE_NETWORK) {
        qCDebug(KIO_SMB_LOG) << "we don't really need to authenticate for this top level url, returning";
        return;
    }

    info.username = s_username;
    info.password = s_password;
    info.verifyPath = true;

    qCDebug(KIO_SMB_LOG) << "libsmb-auth-callback URL:" << info.url;

    // NOTE: By suggestion from upstream we do not default to any amount of
    //   anonymous/guest logins as it's not safe to do in many environments:
    //   https://bugzilla.samba.org/show_bug.cgi?id=14326

    if (m_frontend.checkCachedAuthentication(info)) {
        qCDebug(KIO_SMB_LOG) << "got password through cache" << info.username;
    } else if (!m_defaultUser.isEmpty()) {
        // user defined a default username/password in kcontrol; try this
        info.username = m_defaultUser;
        info.password = m_defaultPassword;
        qCDebug(KIO_SMB_LOG) << "trying defaults for user" << info.username;
    }

    // Make sure it'll be safe to cast to size_t (unsigned)
    Q_ASSERT(unmaxlen > 0);
    Q_ASSERT(pwmaxlen > 0);

    strncpy(username, info.username.toUtf8(), static_cast<size_t>(unmaxlen - 1));
    strncpy(password, info.password.toUtf8(), static_cast<size_t>(pwmaxlen - 1));
}
