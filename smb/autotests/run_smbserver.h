/*
    SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>
    SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#pragma once

#include <QProcess>
#include <QString>
#include <QTemporaryDir>
#include <QUrl>

// A Samba server for the tests, run by the current user on a free port of 127.0.0.1. It shares one
// folder, and every state file of the server stays in a temporary folder.
class SmbServer
{
public:
    SmbServer();
    ~SmbServer();

    bool start();
    void stop();

    // smb://user:password@127.0.0.1:port/share
    QUrl baseUrl() const
    {
        return m_url;
    }
    QString serverLocalFilePath(const QString &relativePath) const;
    QString lastError() const
    {
        return m_lastError;
    }

private:
    bool addUser(const QString &config, const QString &userName);

    // smbd puts sockets in its state folders, so their paths have to stay short.
    QTemporaryDir m_dir = QTemporaryDir(QStringLiteral("/tmp/kio-smb-XXXXXX"));
    QProcess m_serverProc;
    QUrl m_url;
    QString m_lastError;
};
