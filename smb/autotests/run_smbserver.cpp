/*
    SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>
    SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "run_smbserver.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>

#include <pwd.h>
#include <signal.h>
#include <unistd.h>

using namespace Qt::Literals::StringLiterals;

namespace
{
constexpr auto shareName = "share"_L1;
constexpr auto password = "testpass"_L1;

int findFreePort()
{
    QTcpServer server;
    if (!server.listen(QHostAddress::LocalHost, 0)) {
        return -1;
    }
    return server.serverPort();
}

QString findSambaProgram(const QString &name)
{
    // Distributions install smbd and pdbedit in sbin, which is not always in PATH.
    QString path = QStandardPaths::findExecutable(name);
    if (path.isEmpty()) {
        path = QStandardPaths::findExecutable(name, {u"/usr/sbin"_s, u"/usr/local/sbin"_s, u"/sbin"_s});
    }
    return path;
}
}

SmbServer::SmbServer() = default;

SmbServer::~SmbServer()
{
    stop();
}

QString SmbServer::serverLocalFilePath(const QString &relativePath) const
{
    return QDir(m_dir.filePath(shareName)).filePath(relativePath);
}

bool SmbServer::addUser(const QString &config, const QString &userName)
{
    // The account in the Samba password database has to be a local user, and pdbedit only adds one
    // without root when it reads the password from stdin.
    QProcess pdbedit;
    pdbedit.setProgram(findSambaProgram(u"pdbedit"_s));
    pdbedit.setArguments({u"-s"_s, config, u"-t"_s, u"-a"_s, u"-u"_s, userName});
    pdbedit.start();
    pdbedit.write(QStringLiteral("%1\n%1\n").arg(password).toUtf8());
    pdbedit.closeWriteChannel();
    if (!pdbedit.waitForFinished(10000) || pdbedit.exitCode() != 0) {
        m_lastError = u"pdbedit could not add the user: "_s + QString::fromLocal8Bit(pdbedit.readAllStandardError());
        return false;
    }
    return true;
}

bool SmbServer::start()
{
    if (!m_dir.isValid()) {
        m_lastError = u"Temporary directory is invalid"_s;
        return false;
    }
    const QString smbd = findSambaProgram(u"smbd"_s);
    if (smbd.isEmpty() || findSambaProgram(u"pdbedit"_s).isEmpty()) {
        m_lastError = u"smbd or pdbedit is not installed"_s;
        return false;
    }

    const int port = findFreePort();
    if (port <= 0) {
        m_lastError = u"Could not find a free port"_s;
        return false;
    }

    const struct passwd *pw = getpwuid(geteuid());
    if (!pw) {
        m_lastError = u"Could not find the name of the current user"_s;
        return false;
    }
    const QString userName = QString::fromLocal8Bit(pw->pw_name);

    const QDir dir(m_dir.path());
    for (const auto &subdir : {shareName, "private"_L1, "lock"_L1, "cache"_L1, "pid"_L1, "ncalrpc"_L1, "log"_L1}) {
        dir.mkpath(subdir);
    }

    const QString config = dir.filePath(u"smb.conf"_s);
    QFile configFile(config);
    if (!configFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_lastError = u"Could not write "_s + config;
        return false;
    }
    configFile.write(QStringLiteral("[global]\n"
                                    "server role = standalone server\n"
                                    "interfaces = 127.0.0.1\n"
                                    "bind interfaces only = yes\n"
                                    "smb ports = %1\n"
                                    "disable netbios = yes\n"
                                    "load printers = no\n"
                                    "printcap name = /dev/null\n"
                                    "disable spoolss = yes\n"
                                    "passdb backend = tdbsam:%2/private/passdb.tdb\n"
                                    "private dir = %2/private\n"
                                    "lock directory = %2/lock\n"
                                    "state directory = %2/lock\n"
                                    "cache directory = %2/cache\n"
                                    "pid directory = %2/pid\n"
                                    "ncalrpc dir = %2/ncalrpc\n"
                                    "log file = %2/log/%m.log\n"
                                    "[%3]\n"
                                    "path = %2/%3\n"
                                    "read only = no\n"
                                    "valid users = %4\n")
                         .arg(QString::number(port), m_dir.path(), shareName, userName)
                         .toUtf8());
    configFile.close();

    if (!addUser(config, userName)) {
        return false;
    }

    // Without --no-process-group, smbd leads its own process group, so stop() reaches the processes
    // that it starts for each connection as well.
    m_serverProc.setProgram(smbd);
    m_serverProc.setArguments({u"--foreground"_s, u"-s"_s, config, u"--log-basename"_s, dir.filePath(u"log"_s)});
    m_serverProc.setProcessChannelMode(QProcess::ForwardedChannels);
    m_serverProc.start();
    if (!m_serverProc.waitForStarted()) {
        m_lastError = u"Failed to start smbd"_s;
        return false;
    }

    bool serverReady = false;
    for (int i = 0; i < 100 && !serverReady && m_serverProc.state() == QProcess::Running; ++i) {
        QTest::qWait(100);
        QTcpSocket testSocket;
        testSocket.connectToHost(QHostAddress::LocalHost, port);
        serverReady = testSocket.waitForConnected(100);
    }
    if (!serverReady) {
        m_lastError = u"smbd did not listen on port "_s + QString::number(port) + u", see the logs in "_s + dir.filePath(u"log"_s);
        stop();
        return false;
    }

    m_url.setScheme(u"smb"_s);
    m_url.setHost(u"127.0.0.1"_s);
    m_url.setPort(port);
    m_url.setUserName(userName);
    m_url.setPassword(password);
    m_url.setPath(u"/"_s + shareName);
    return true;
}

void SmbServer::stop()
{
    if (m_serverProc.state() == QProcess::NotRunning) {
        return;
    }
    ::kill(-m_serverProc.processId(), SIGTERM);
    if (!m_serverProc.waitForFinished(5000)) {
        ::kill(-m_serverProc.processId(), SIGKILL);
        m_serverProc.waitForFinished();
    }
}
