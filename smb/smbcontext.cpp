/*
    SPDX-License-Identifier: GPL-2.0-or-later
    SPDX-FileCopyrightText: 2000 Caldera Systems Inc.
    SPDX-FileCopyrightText: 2020 Harald Sitter <sitter@kde.org>
    SPDX-FileContributor: Matthew Peterson <mpeterson@caldera.com>
*/

#include "smbcontext.h"

#include <KConfig>
#include <KConfigGroup>

#include "smbauthenticator.h"
#include "smb-logsettings.h"

SMBContext::SMBContext(SMBAuthenticator *authenticator)
    : m_context(smbc_new_context(), &freeContext)
    , m_authenticator(authenticator)
{
    Q_ASSERT(m_context);
    if (!m_context) {
        return;
    }

    authenticator->loadConfiguration();

    qCDebug(KIO_SMB_LOG) << "auth_initialize_smbc";

    KConfig cfg("kioslaverc", KConfig::SimpleConfig);
    int debugLevel = cfg.group("SMB").readEntry("DebugLevel", 0);
    qCDebug(KIO_SMB_LOG) << "Setting debug level to:" << debugLevel;

#ifdef DEPRECATED_SMBC_INTERFACE // defined by libsmbclient.h of Samba 3.2
    smbc_setOptionUserData(m_context.get(), this);
    smbc_setFunctionAuthDataWithContext(m_context.get(), auth_cb);

    /* New libsmbclient interface of Samba 3.2 */
    smbc_setDebug(m_context.get(), debugLevel);

    /* Enable Kerberos support */
    smbc_setOptionUseKerberos(m_context.get(), 1);
    smbc_setOptionFallbackAfterKerberos(m_context.get(), 1);
#else
    smbc_option_set(m_context.get(), "user_data", this);
    m_context->callbacks.auth_fn = NULL;
    smbc_option_set(m_context.get(), "auth_function", (void *)auth_cb);

    m_context->debug = debug_level;

#if defined(SMB_CTX_FLAG_USE_KERBEROS) && defined(SMB_CTX_FLAG_FALLBACK_AFTER_KERBEROS)
    m_context->flags |= SMB_CTX_FLAG_USE_KERBEROS | SMB_CTX_FLAG_FALLBACK_AFTER_KERBEROS;
#endif
#endif /* DEPRECATED_SMBC_INTERFACE */

    if (!smbc_init_context(m_context.get())) {
        m_context.reset();
        return;
    }

    smbc_set_context(m_context.get());

    // TODO: refactor; checkPassword should query this on
    // demand to not run into situations where we may have cached
    // the workgroup early on and it changed since. Needs context
    // being held in the slave though, which opens us up to nullptr
    // problems should checkPassword be called without init first.
    authenticator->setDefaultWorkgroup(smbc_getWorkgroup(*this));

    return;
}

bool SMBContext::isValid() const
{
    return smbcctx() && authenticator();
}

void SMBContext::auth_cb(SMBCCTX *context, const char *server, const char *share, char *workgroup, int wgmaxlen, char *username, int unmaxlen, char *password, int pwmaxlen)
{
    // Unfortunately because the callback API doesn't support callback specific user_data we need
    // to route all auths through our context object otherwise the authenticator would have
    // to twiddle the global context user_data and that seems much worse :|
    if (context != nullptr) {
#ifdef DEPRECATED_SMBC_INTERFACE
        auto *that = static_cast<SMBContext *>(smbc_getOptionUserData(context));
#else
        auto *that = static_cast<SMBCContext *>(smbc_option_get(context, "user_data"));
#endif
        that->m_authenticator->auth(server, share,
                                    workgroup,wgmaxlen,
                                    username, unmaxlen,
                                    password, pwmaxlen);
    }
}

void SMBContext::freeContext(SMBCCTX *ptr)
{
    smbc_free_context(ptr, 1);
}
