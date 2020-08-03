/*
    SPDX-License-Identifier: GPL-2.0-or-later
    SPDX-FileCopyrightText: 2020 Harald Sitter <sitter@kde.org>
*/

#pragma once

#include <QString>

namespace KIO {
class AuthInfo;
}

// Abstracts SlaveBase API so Authenticator may be used without
// a SlaveBase for the KDirNotify implementation)
class SMBAbstractFrontend
{
public:
    virtual ~SMBAbstractFrontend() = default;
    virtual bool checkCachedAuthentication(KIO::AuthInfo &info) = 0;
};

// Base class for SMBC management + basic authentication
class SMBAuthenticator
{
public:
    SMBAuthenticator(SMBAbstractFrontend &frontend);

    QString defaultWorkgroup() const;
    void setDefaultWorkgroup(const QString &workGroup);

    // (Re)loads default values from configuration
    void loadConfiguration();

    // Callback for authentication requests.
    void auth(const char *server, const char *share,
              char *workgroup, int wgmaxlen,
              char *username, int unmaxlen,
              char *password, int pwmaxlen);

private:
    // Frontend for authentication requests.
    SMBAbstractFrontend &m_frontend;

    QString m_defaultUser;
    QString m_defaultPassword;
    QString m_defaultEncoding;
    QString m_defaultWorkgroup = QStringLiteral("WORKGROUP"); // overwritten with value from smbc

    Q_DISABLE_COPY(SMBAuthenticator)
};
