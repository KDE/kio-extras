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

#include "smb-logsettings.h"
#include "smburl.h"

SMBAuthenticator::SMBAuthenticator(SMBAbstractFrontend &frontend)
    : m_frontend(frontend)
{
}

QString SMBAuthenticator::defaultWorkgroup() const
{
    return m_defaultWorkgroup;
}

void SMBAuthenticator::setDefaultWorkgroup(const QString &workGroup)
{
    m_defaultWorkgroup = workGroup;
}

void SMBAuthenticator::auth(SMBCCTX *context,
                            const char *server,
                            const char *share,
                            char *workgroup,
                            int wgmaxlen,
                            char *username,
                            int unmaxlen,
                            char *password,
                            int pwmaxlen)
{
    qCDebug(KIO_SMB_LOG) << "auth_smbc_get_dat: set user=" << username << ", workgroup=" << workgroup << " server=" << server << ", share=" << share;

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

    smbc_set_credentials_with_fallback(context, workgroup, username, password);
}
