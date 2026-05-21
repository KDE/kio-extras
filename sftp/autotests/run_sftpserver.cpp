/*
 *   SPDX-FileCopyrightText: 2026 Ian Monroe <imonroe@kde.org>
 *   SPDX-License-Identifier: LGPL-2.0-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#include "run_sftpserver.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QTcpServer>
#include <QTest>

SftpServer::SftpServer(QObject *parent)
    : QObject(parent)
{
}

SftpServer::~SftpServer()
{
    stop();
}

QString SftpServer::serverLocalFilePath(const QString &relativePath) const
{
    return QDir(m_remoteDir.path()).filePath(relativePath);
}

int SftpServer::findFreePort()
{
    QTcpServer server;
    if (!server.listen(QHostAddress::LocalHost, 0)) {
        qWarning() << "Failed to find free port:" << server.errorString();
        return -1;
    }
    int port = server.serverPort();
    server.close();
    return port;
}

bool SftpServer::setupKnownHosts(const QString &sftpserverDir, int port)
{
    QString sftpserverSrcDir = sftpserverDir + "/src";
    QString keyFile = sftpserverDir + "/test_server_key";
    QString pubKeyFile = keyFile + ".pub";
    QString knownHostsFile = sftpserverDir + "/known_hosts";

    if (!QFile::exists(keyFile)) {
        qDebug() << "Generating server key...";
        QProcess keygenProc;
        keygenProc.setProgram("python3");
        QString pythonScript = QString(
                                   "import paramiko; key = paramiko.RSAKey.generate(bits=2048); "
                                   "key.write_private_key_file('%1'); "
                                   "print(key.get_name(), key.get_base64())")
                                   .arg(keyFile);
        keygenProc.setArguments({"-c", pythonScript});
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.insert("PYTHONPATH", sftpserverSrcDir);
        keygenProc.setProcessEnvironment(env);
        keygenProc.start();
        if (!keygenProc.waitForFinished(10000)) {
            qWarning() << "Key generation timed out";
            return false;
        }
        QByteArray output = keygenProc.readAllStandardOutput().trimmed();
        if (output.isEmpty()) {
            qWarning() << "Failed to generate key:" << keygenProc.readAllStandardError();
            return false;
        }
        QFile pubKey(pubKeyFile);
        if (pubKey.open(QIODevice::WriteOnly)) {
            pubKey.write(output + "\n");
            pubKey.close();
        }
    }

    QFile pubKey(pubKeyFile);
    if (!pubKey.open(QIODevice::ReadOnly)) {
        qWarning() << "Cannot read public key file:" << pubKeyFile;
        return false;
    }
    QByteArray pubKeyData = pubKey.readAll().trimmed();
    pubKey.close();

    QFile knownHosts(knownHostsFile);
    if (!knownHosts.open(QIODevice::WriteOnly)) {
        qWarning() << "Cannot create known_hosts file:" << knownHostsFile;
        return false;
    }
    QString keyLine = QString("[127.0.0.1]:%1").arg(port);
    knownHosts.write(keyLine.toUtf8());
    knownHosts.write(" ");
    knownHosts.write(pubKeyData);
    knownHosts.write("\n");
    knownHosts.close();

    qDebug() << "Created known_hosts file at:" << knownHostsFile;
    m_knownHostsFile = knownHostsFile;
    return true;
}

bool SftpServer::start()
{
    if (!m_remoteDir.isValid()) {
        m_lastError = "Temporary directory is invalid";
        return false;
    }

    m_port = findFreePort();
    if (m_port <= 0) {
        m_lastError = "Could not find a free port";
        return false;
    }
    m_url.setPort(m_port);

    QDir appDir(QCoreApplication::applicationDirPath());     // the build/bin directory
    QString sftpserverDir = appDir.absoluteFilePath("../sftp/autotests/sftpserver");
    if (!QDir(sftpserverDir).exists()) {
        m_lastError = "Could not find sftpserver library in build directory: " + sftpserverDir;
        return false;
    }
    QString sftpserverSrcDir = sftpserverDir + "/src";

    if (!setupKnownHosts(sftpserverDir, m_port)) {
        m_lastError = "Failed to setup known_hosts file";
        return false;
    }

    // libssh is understandably stubborn about where it reads known-hosts file from
    // to workaround this, we're using mock_getpwuid_r.c to trick libssh into reading
    // out of our fake home, with host checks effectively disabled.
    const QString fakeHome = m_remoteDir.path() + QStringLiteral("/fake_home");
    QDir(fakeHome).mkpath(QStringLiteral(".ssh"));

    QFile sshConfig(fakeHome + QStringLiteral("/.ssh/config"));
    if (sshConfig.open(QIODevice::WriteOnly | QIODevice::Text)) {
        sshConfig.write(QStringLiteral("Host *\n    UserKnownHostsFile %1\n").arg(m_knownHostsFile).toUtf8());
        sshConfig.close();
    }

    qputenv("KIO_SFTP_TEST_HOME", fakeHome.toUtf8());

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("PYTHONPATH", sftpserverSrcDir);
    m_serverProc.setProcessEnvironment(env);

    QString keyFile = sftpserverDir + "/test_server_key";
    m_serverProc.setProgram("python3");
    m_serverProc.setArguments({"-m", "sftpserver", "-p", QString::number(m_port), "-r", m_remoteDir.path(), "-k", keyFile});
    m_serverProc.setProcessChannelMode(QProcess::ForwardedOutputChannel);

    connect(&m_serverProc, &QProcess::readyReadStandardError, this, [this] {
        qDebug() << "sftp_server STDERR:" << m_serverProc.readAllStandardError();
    });

    m_serverProc.start();
    if (!m_serverProc.waitForStarted()) {
        m_lastError = "Failed to start sftp server process";
        return false;
    }

    bool serverReady = false;
    for (int i = 0; i < 30 && !serverReady; ++i) {
        QTest::qWait(100);
        QTcpSocket testSocket;
        testSocket.connectToHost(QHostAddress::LocalHost, m_port);
        serverReady = testSocket.waitForConnected(100);
        if (serverReady) {
            testSocket.write("SSH-2.0-TestProbe\r\n");
            testSocket.waitForBytesWritten(100);
        }
    }
    qDebug() << "Server ready on port:" << m_port << "Ready:" << serverReady;

    if (!serverReady) {
        m_lastError = "SFTP server did not become ready on the expected port";
        return false;
    }

    return isRunning();
}

void SftpServer::stop()
{
    if (m_serverProc.state() != QProcess::NotRunning) {
        m_serverProc.terminate();
        if (!m_serverProc.waitForFinished(5000)) {
            m_serverProc.kill();
            m_serverProc.waitForFinished();
        }
    }
}
