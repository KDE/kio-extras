/*
    SPDX-License-Identifier: GPL-2.0-or-later
    SPDX-FileCopyrightText: 2000 Caldera Systems Inc.
    SPDX-FileContributor: Matthew Peterson <mpeterson@caldera.com>
*/

#include "kio_smb.h"
#include <QCoreApplication>
#include <QVersionNumber>

// Pseudo plugin class to embed meta data
class KIOPluginForMetaData : public QObject
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kde.kio.worker.smb" FILE "smb.json")
};

bool needsEEXISTWorkaround()
{
    /* There is an issue with some libsmbclient versions that return EEXIST
     * return code from smbc_opendir() instead of EPERM when the user
     * tries to access a resource that requires login authetication.
     * We are working around the issue by treating EEXIST as a special case
     * of "invalid/unavailable credentials" if we detect that we are using
     * the affected versions of libsmbclient
     *
     * Upstream bug report: https://bugzilla.samba.org/show_bug.cgi?id=13050
     */
    static const QVersionNumber firstBrokenVer {4, 7, 0};
    static const QVersionNumber lastBrokenVer {4, 7, 6};

    const QVersionNumber currentVer = QVersionNumber::fromString(smbc_version());
    qCDebug(KIO_SMB_LOG) << "Using libsmbclient library version" << currentVer;

    if (currentVer >= firstBrokenVer && currentVer <= lastBrokenVer) {
        qCDebug(KIO_SMB_LOG) << "Detected broken libsmbclient version" << currentVer;
        return true;
    }

    return false;
}

SMBWorker::SMBWorker(const QByteArray &pool, const QByteArray &app)
    : WorkerBase("smb", pool, app)
    , m_openFd(-1)
    , m_enableEEXISTWorkaround(needsEEXISTWorkaround())
{
}

WorkerFrontend::WorkerFrontend(SMBWorker &worker)
    : m_worker(worker)
{
}

bool WorkerFrontend::checkCachedAuthentication(AuthInfo &info)
{
    return m_worker.checkCachedAuthentication(info);
}

#include "kio_smb.moc"
#include "moc_kio_smb.cpp"
