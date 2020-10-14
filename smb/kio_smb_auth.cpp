/*
    SPDX-License-Identifier: GPL-2.0-or-later
    SPDX-FileCopyrightText: 2000 Caldera Systems, Inc.
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

    info.setExtraField("anonymous", true); // arbitrary default for dialog
    info.setExtraField("domain", m_context.authenticator()->defaultWorkgroup());

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
