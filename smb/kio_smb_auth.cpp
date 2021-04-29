/*
    SPDX-License-Identifier: GPL-2.0-or-later
    SPDX-FileCopyrightText: 2000 Caldera Systems Inc.
    SPDX-FileCopyrightText: 2020 Harald Sitter <sitter@kde.org>
    SPDX-FileContributor: Matthew Peterson <mpeterson@caldera.com>
*/

#include "kio_smb.h"
#include "smburl.h"

#include <cstdlib>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <klocalizedstring.h>

int SMBSlave::checkPassword(SMBUrl &url)
{
    qCDebug(KIO_SMB_LOG) << "checkPassword for " << url;

    KIO::AuthInfo info;
    info.url = QUrl("smb:///");
    info.url.setHost(url.host());

    QString share = url.path();
    int index = share.indexOf('/', 1);
    if (index > 1)
        share = share.left(index);
    if (share.at(0) == '/')
        share = share.mid(1);
    info.url.setPath('/' + share);
    info.verifyPath = true;
    info.keepPassword = true;

    // By suggestion from upstream we do not split login variants through the UI but rather expect different
    // username inputs. This is also more in line with how logon works in windows.
    // https://bugzilla.samba.org/show_bug.cgi?id=14326
    // https://docs.microsoft.com/en-us/windows/win32/secauthn/user-name-formats
    info.setExtraField(QStringLiteral("username-context-help"),
                       xi18nc("@info:whatsthis",
                              "<para>There are various options for authenticating on SMB shares.</para>"
                              "<para><placeholder>username</placeholder>: When authenticating within a home network the username on the server is sufficient</para>"
                              "<para><placeholder>username@domain.com</placeholder>: Modern corporate logon names are formed like e-mail addresses</para>"
                              "<para><placeholder>DOMAIN\\username</placeholder>: For ancient corporate networks or workgroups you may need to prefix the NetBIOS domain name (pre-Windows 2000)</para>"
                              "<para><placeholder>anonymous</placeholder>: Anonymous logins can be attempted using empty username and password. Depending on server configuration non-empty usernames may be required</para>"
                             ));

    if (share.isEmpty())
        info.prompt = i18n("<qt>Please enter authentication information for <b>%1</b></qt>", url.host());
    else
        info.prompt = i18n(
                          "Please enter authentication information for:\n"
                          "Server = %1\n"
                          "Share = %2",
                          url.host(),
                          share);

    info.username = url.userName();
    qCDebug(KIO_SMB_LOG) << "call openPasswordDialog for " << info.url;

    const int passwordDialogErrorCode = openPasswordDialogV2(info);
    if (passwordDialogErrorCode == KJob::NoError) {
        qCDebug(KIO_SMB_LOG) << "openPasswordDialog returned " << info.username;
        url.setUser(info.username);

        if (info.keepPassword) {
            qCDebug(KIO_SMB_LOG) << "Caching info.username = " << info.username
                                 << ", info.url = " << info.url.toDisplayString();
            cacheAuthentication(info);
        }

        return KJob::NoError;
    }
    qCDebug(KIO_SMB_LOG) << "no value from openPasswordDialog; error:" << passwordDialogErrorCode;
    return passwordDialogErrorCode;
}
