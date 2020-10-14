/*
    SPDX-License-Identifier: GPL-2.0-or-later
    SPDX-FileCopyrightText: 2020 Harald Sitter <sitter@kde.org>
*/

#pragma once

extern "C" {
#include <libsmbclient.h>
}

#include <memory>

class SMBAuthenticator;

// Encapsulates an SMBC context
class SMBContext
{
public:
    SMBContext(SMBAuthenticator *authenticator);

    bool isValid() const;

    SMBCCTX *smbcctx() const
    {
        return m_context.get();
    }

    operator SMBCCTX *() const
    {
        return smbcctx();
    }

    // Authenticator associated with this context.
    // TODO: getter only used in checkpassword, could be refactored away maybe
    SMBAuthenticator *authenticator() const
    {
        return m_authenticator.get();
    }

private:
    static void auth_cb(SMBCCTX *context,
                        const char *server,const char *share,
                        char *workgroup, int wgmaxlen,
                        char *username, int unmaxlen,
                        char *password, int pwmaxlen);

    static void freeContext(SMBCCTX *ptr);

    std::unique_ptr<SMBCCTX, decltype(&freeContext)> m_context;
    std::unique_ptr<SMBAuthenticator> m_authenticator;
};
