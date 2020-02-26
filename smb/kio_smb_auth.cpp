/////////////////////////////////////////////////////////////////////////////
//
// Project:     SMB kioslave for KDE2
//
// File:        kio_smb_auth.cpp
//
// Abstract:    member function implementations for SMBSlave that deal with
//              SMB directory access
//
// Author(s):   Matthew Peterson <mpeterson@caldera.com>
//
//---------------------------------------------------------------------------
//
// Copyright (c) 2000  Caldera Systems, Inc.
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

#include "kio_smb.h"
#include "kio_smb_internal.h"

#include <cstdlib>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <klocalizedstring.h>

void auth_smbc_get_data(SMBCCTX * context,
                        const char *server,const char *share,
                        char *workgroup, int wgmaxlen,
                        char *username, int unmaxlen,
                        char *password, int pwmaxlen)
{
    if (context != nullptr) {
#ifdef DEPRECATED_SMBC_INTERFACE
        auto *theSlave = static_cast<SMBSlave *>(smbc_getOptionUserData(context));
#else
        auto *theSlave = static_cast<SMBSlave *>(smbc_option_get(context, "user_data"));
#endif
        theSlave->auth_smbc_get_data(server, share,
                                     workgroup,wgmaxlen,
                                     username, unmaxlen,
                                     password, pwmaxlen);
    }

}

void SMBSlave::auth_smbc_get_data(const char *server,const char *share,
                                  char *workgroup, int wgmaxlen,
                                  char *username, int unmaxlen,
                                  char *password, int pwmaxlen)
{
    // check this to see if we "really" need to authenticate...
    SMBUrlType t = m_current_url.getType();
    if (t == SMBURLTYPE_ENTIRE_NETWORK) {
        qCDebug(KIO_SMB_LOG) << "we don't really need to authenticate for this top level url, returning";
        return;
    }
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

    info.username = s_username;
    info.password = s_password;
    info.verifyPath = true;
    info.setExtraField("domain", s_workgroup);

    qCDebug(KIO_SMB_LOG) << "libsmb-auth-callback URL:" << info.url;

    if (!checkCachedAuthentication(info)) {
        if (m_default_user.isEmpty()) {
            // ok, we do not know the password. Let's try anonymous before we try for real
            info.username = "anonymous";
            info.password.clear();
        } else {
            // user defined a default username/password in kcontrol; try this
            info.username = m_default_user;
            info.password = m_default_password;
        }

    } else
        qCDebug(KIO_SMB_LOG) << "got password through cache";

    // Make sure it'll be safe to cast to size_t (unsigned)
    Q_ASSERT(unmaxlen > 0);
    Q_ASSERT(pwmaxlen > 0);
    Q_ASSERT(wgmaxlen > 0);

    strncpy(username, info.username.toUtf8(), static_cast<size_t>(unmaxlen - 1));
    strncpy(password, info.password.toUtf8(), static_cast<size_t>(pwmaxlen - 1));
    // TODO: isEmpty guard can be removed in 20.08+
    //   It is only here to prevent us setting an empty work group if a user updates
    //   but doesn't restart so kiod5 could hold an old cache without domain
    //   field. In that event we'll leave the input workgroup as-is.
    const QString domain = info.getExtraField("domain").toString();
    if (!domain.isEmpty()) {
        strncpy(workgroup, domain.toUtf8(), static_cast<size_t>(wgmaxlen - 1));
    }
}

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
    info.setExtraField("domain", m_default_workgroup);

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

bool SMBSlave::auth_initialize_smbc()
{
    if (m_initialized_smbc) {
        return true;
    }

    qCDebug(KIO_SMB_LOG) << "auth_initialize_smbc";
    KConfig cfg("kioslaverc", KConfig::SimpleConfig);
    int debug_level = cfg.group("SMB").readEntry("DebugLevel", 0);

    qCDebug(KIO_SMB_LOG) << "smbc_new_context call";
    SMBCCTX *smb_context = smbc_new_context();
    if (!smb_context) {
        SlaveBase::error(ERR_INTERNAL, i18n("libsmbclient failed to create context"));
        return false;
    }

#ifdef DEPRECATED_SMBC_INTERFACE // defined by libsmbclient.h of Samba 3.2
    /* New libsmbclient interface of Samba 3.2 */
    smbc_setDebug(smb_context, debug_level);
    smbc_setFunctionAuthDataWithContext(smb_context, ::auth_smbc_get_data);
    smbc_setOptionUserData(smb_context, this);

    /* Enable Kerberos support */
    smbc_setOptionUseKerberos(smb_context, 1);
    smbc_setOptionFallbackAfterKerberos(smb_context, 1);
#else
    smb_context->debug = debug_level;
    smb_context->callbacks.auth_fn = NULL;
    smbc_option_set(smb_context, "auth_function", (void *)::auth_smbc_get_data);
    smbc_option_set(smb_context, "user_data", this);

#if defined(SMB_CTX_FLAG_USE_KERBEROS) && defined(SMB_CTX_FLAG_FALLBACK_AFTER_KERBEROS)
    smb_context->flags |= SMB_CTX_FLAG_USE_KERBEROS | SMB_CTX_FLAG_FALLBACK_AFTER_KERBEROS;
#endif
#endif /* DEPRECATED_SMBC_INTERFACE */

    if (!smbc_init_context(smb_context)) {
        smbc_free_context(smb_context, 0);
        smb_context = nullptr;
        SlaveBase::error(ERR_INTERNAL, i18n("libsmbclient failed to initialize context"));
        return false;
    }

    smbc_set_context(smb_context);

    // TODO: refactor; checkPassword should query this on
    // demand to not run into situations where we may have cached
    // the workgroup early on and it changed since. Needs context
    // being held in the slave though, which opens us up to nullptr
    // problems should checkPassword be called without init first.
    m_default_workgroup = smbc_getWorkgroup(smb_context);

    m_initialized_smbc = true;
    return true;
}

