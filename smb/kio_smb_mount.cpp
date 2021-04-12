/*
    SPDX-License-Identifier: LGPL-2.0-or-later
    SPDX-FileCopyrightText: 2000 Alexander Neundorf <neundorf@kde.org>
*/

#include "kio_smb.h"

#include <KLocalizedString>
#include <KProcess>
#include <QByteArray>
#include <QDataStream>
#include <QDir>
#include <kshell.h>
#include <unistd.h>

void SMBSlave::special(const QByteArray &data)
{
    qCDebug(KIO_SMB_LOG) << "Smb::special()";
    int tmp;
    QDataStream stream(data);
    stream >> tmp;
    // mounting and umounting are both blocking, "guarded" by a SIGALARM in the future
    switch (tmp) {
    case 1:
    case 3: {
        QString remotePath;
        QString mountPoint;
        QString user;
        stream >> remotePath >> mountPoint;

        QStringList sl = remotePath.split('/');
        QString share;
        QString host;
        if (sl.count() >= 2) {
            host = sl.at(0).mid(2);
            share = sl.at(1);
            qCDebug(KIO_SMB_LOG) << "special() host -" << host << "- share -" << share << "-";
        }

        remotePath.replace('\\', '/'); // smbmounterplugin sends \\host/share

        qCDebug(KIO_SMB_LOG) << "mounting: " << remotePath.toLocal8Bit() << " to " << mountPoint.toLocal8Bit();

        if (tmp == 3) {
            if (!QDir().mkpath(mountPoint)) {
                error(KIO::ERR_CANNOT_MKDIR, mountPoint);
                return;
            }
        }

        SMBUrl smburl(QUrl("smb:///"));
        smburl.setHost(host);
        smburl.setPath('/' + share);

        const int passwordError = checkPassword(smburl);
        if (passwordError != KJob::NoError && passwordError != KIO::ERR_USER_CANCELED) {
            error(passwordError, smburl.toString());
            return;
        }

        // using smbmount instead of "mount -t smbfs", because mount does not allow a non-root
        // user to do a mount, but a suid smbmnt does allow this

        KProcess proc;
        proc.setOutputChannelMode(KProcess::SeparateChannels);
        proc << "smbmount";

        QString options;

        if (smburl.userName().isEmpty()) {
            user = "guest";
            options = "guest";
        } else {
            options = "username=" + smburl.userName();
            user = smburl.userName();

            if (!smburl.password().isEmpty())
                options += ",password=" + smburl.password();
        }

        // TODO: check why the control center uses encodings with a blank char, e.g. "cp 1250"
        // if ( ! m_default_encoding.isEmpty() )
        // options += ",codepage=" + KShell::quoteArg(m_default_encoding);

        proc << remotePath;
        proc << mountPoint;
        proc << "-o" << options;

        proc.start();
        if (!proc.waitForFinished()) {
            error(KIO::ERR_CANNOT_LAUNCH_PROCESS, "smbmount" + i18n("\nMake sure that the samba package is installed properly on your system."));
            return;
        }

        QString mybuf = QString::fromLocal8Bit(proc.readAllStandardOutput());
        QString mystderr = QString::fromLocal8Bit(proc.readAllStandardError());

        qCDebug(KIO_SMB_LOG) << "mount exit " << proc.exitCode() << "stdout:" << mybuf << "\nstderr:" << mystderr;

        if (proc.exitCode() != 0) {
            error(KIO::ERR_CANNOT_MOUNT, i18n("Mounting of share \"%1\" from host \"%2\" by user \"%3\" failed.\n%4", share, host, user, mybuf + '\n' + mystderr));
            return;
        }
    }
    break;
    case 2:
    case 4: {
        QString mountPoint;
        stream >> mountPoint;

        KProcess proc;
        proc.setOutputChannelMode(KProcess::SeparateChannels);
        proc << "smbumount";
        proc << mountPoint;

        proc.start();
        if (!proc.waitForFinished()) {
            error(KIO::ERR_CANNOT_LAUNCH_PROCESS, "smbumount" + i18n("\nMake sure that the samba package is installed properly on your system."));
            return;
        }

        QString mybuf = QString::fromLocal8Bit(proc.readAllStandardOutput());
        QString mystderr = QString::fromLocal8Bit(proc.readAllStandardError());

        qCDebug(KIO_SMB_LOG) << "smbumount exit " << proc.exitCode() << "stdout:" << mybuf << "\nstderr:" << mystderr;

        if (proc.exitCode() != 0) {
            error(KIO::ERR_CANNOT_UNMOUNT, i18n("Unmounting of mountpoint \"%1\" failed.\n%2", mountPoint, mybuf + '\n' + mystderr));
            return;
        }

        if (tmp == 4) {
            bool ok;

            QDir dir(mountPoint);
            dir.cdUp();
            ok = dir.rmdir(mountPoint);
            if (ok) {
                QString p = dir.path();
                dir.cdUp();
                ok = dir.rmdir(p);
            }

            if (!ok) {
                error(KIO::ERR_CANNOT_RMDIR, mountPoint);
                return;
            }
        }
    }
    break;
    default:
        break;
    }
    finished();
}
