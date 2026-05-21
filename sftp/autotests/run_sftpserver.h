/*
 *   SPDX-FileCopyrightText: 2026 Ian Monroe <imonroe@kde.org>
 *   SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#pragma once

#include <QObject>
#include <QProcess>
#include <QString>
#include <QTemporaryDir>
#include <QUrl>

class SftpServer : public QObject
{
    Q_OBJECT
public:
    explicit SftpServer(QObject *parent = nullptr);
    ~SftpServer() override;

    bool start();
    void stop();

    QUrl baseUrl() const
    {
        return m_url;
    }
    QString serverLocalDirPath() const
    {
        return m_remoteDir.path();
    }
    QString serverLocalFilePath(const QString &relativePath) const;
    bool isRunning() const
    {
        return m_serverProc.state() == QProcess::Running;
    }
    QString lastError() const
    {
        return m_lastError;
    }

private:
    static int findFreePort();
    bool setupKnownHosts(const QString &sftpserverDir, int port);

    QTemporaryDir m_remoteDir;
    QProcess m_serverProc;
    QUrl m_url = QUrl(QStringLiteral("sftp://testuser:testpass@127.0.0.1"));
    int m_port = -1;
    QString m_knownHostsFile;
    QString m_lastError;
};
