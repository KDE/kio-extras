/*
    SPDX-License-Identifier: GPL-2.0-or-later
    SPDX-FileCopyrightText: 2000 Caldera Systems Inc.
    SPDX-FileCopyrightText: 2020 Harald Sitter <sitter@kde.org>
    SPDX-FileContributor: Matthew Peterson <mpeterson@caldera.com>
*/

#include "smbcontext.h"

#include <KConfig>
#include <KConfigGroup>

#include "smb-logsettings.h"
#include "smbauthenticator.h"

SMBContext::SMBContext(SMBAuthenticator *authenticator)
    : m_context(smbc_new_context(), &freeContext)
    , m_authenticator(authenticator)
{
    Q_ASSERT(m_context);
    if (!m_context) {
        return;
    }

    qCDebug(KIO_SMB_LOG) << "auth_initialize_smbc";

    KConfig cfg("kioslaverc", KConfig::SimpleConfig);
    int debugLevel = cfg.group("SMB").readEntry("DebugLevel", 0);
    qCDebug(KIO_SMB_LOG) << "Setting debug level to:" << debugLevel;

    smbc_setOptionUserData(m_context.get(), this);
    smbc_setFunctionAuthDataWithContext(m_context.get(), auth_cb);
    smbc_setDebug(m_context.get(), debugLevel);

    /* Enable Kerberos support */
    smbc_setOptionUseKerberos(m_context.get(), 1);
    smbc_setOptionFallbackAfterKerberos(m_context.get(), 1);

    if (!smbc_init_context(m_context.get())) {
        m_context.reset();
        return;
    }

    smbc_set_context(m_context.get());

    // TODO: refactor; checkPassword should query this on
    // demand to not run into situations where we may have cached
    // the workgroup early on and it changed since. Needs context
    // being held in the worker though, which opens us up to nullptr
    // problems should checkPassword be called without init first.
    authenticator->setDefaultWorkgroup(smbc_getWorkgroup(*this));
}

bool SMBContext::isValid() const
{
    return smbcctx() && authenticator();
}

void SMBContext::auth_cb(SMBCCTX *context,
                         const char *server,
                         const char *share,
                         char *workgroup,
                         int wgmaxlen,
                         char *username,
                         int unmaxlen,
                         char *password,
                         int pwmaxlen)
{
    // Unfortunately because the callback API doesn't support callback specific user_data we need
    // to route all auths through our context object otherwise the authenticator would have
    // to twiddle the global context user_data and that seems much worse :|
    if (context != nullptr) {
        static_cast<SMBContext *>(smbc_getOptionUserData(context))
            ->m_authenticator->auth(context, server, share, workgroup, wgmaxlen, username, unmaxlen, password, pwmaxlen);
    }
}

void SMBContext::freeContext(SMBCCTX *ptr)
{
    smbc_free_context(ptr, 1);
}
