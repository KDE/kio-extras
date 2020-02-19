/*
    SPDX-License-Identifier: LGPL-2.0-or-later
    SPDX-FileCopyrightText: 2000 Alexander Neundorf <neundorf@kde.org>
    SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
*/

#include "kio_smb.h"

#include <KLocalizedString>
#include <KProcess>
#include <KShell>

#include <QByteArray>
#include <QDataStream>
#include <QDir>
#include <unistd.h>

WorkerResult SMBWorker::getACE(QDataStream &stream)
{
    QUrl qurl;
    stream >> qurl;
    const SMBUrl url(qurl);

    // Use the same buffer for all properties to reduce the likelihood of having needless allocations. ACL will usually
    // be the largest of the lot and if we try to get that first we'll probably fit all the other properties into the
    // same buffer easy peasy.
    constexpr auto defaultArraySize = 4096; // arbitrary, the ACL is of unknown size
    const int pageSize = static_cast<int>(sysconf(_SC_PAGESIZE));
    Q_ASSERT(pageSize == sysconf(_SC_PAGESIZE)); // ensure conversion accuracy
    QVarLengthArray<char, defaultArraySize> value(pageSize);

    for (auto xattr : {
             "system.nt_sec_desc.acl.*+",
             "system.nt_sec_desc.owner+",
             "system.nt_sec_desc.group+",
         }) {
        while (true) {
            const auto result = smbc_getxattr(url.toSmbcUrl(), xattr, value.data(), value.size());
            if (const auto error = errno; result < 0) {
                // https://bugzilla.samba.org/show_bug.cgi?id=15088
                if (error == ERANGE) {
                    value.resize(value.size() + pageSize);
                    continue;
                }
                qWarning() << xattr << strerror(error);
                return WorkerResult::fail(ERR_INTERNAL, strerror(error));
            }
            qCDebug(KIO_SMB_LOG) << "XATTR" << xattr << value.data();
            if (QLatin1String("system.nt_sec_desc.acl.*+") == QLatin1String(xattr)) {
                setMetaData("ACL", QString::fromUtf8(value.constData()));
            }
            if (QLatin1String("system.nt_sec_desc.owner+") == QLatin1String(xattr)) {
                setMetaData("OWNER", QString::fromUtf8(value.constData()));
            }
            if (QLatin1String("system.nt_sec_desc.group+") == QLatin1String(xattr)) {
                setMetaData("GROUP", QString::fromUtf8(value.constData()));
            }
            break;
        }
    }
    return WorkerResult::pass();
}

KIO::WorkerResult SMBWorker::setACE(QDataStream &stream)
{
    QUrl qurl;
    stream >> qurl;
    const SMBUrl url(qurl);

    QString sid;
    QString aceString;
    stream >> sid >> aceString;

    const QString attr = QLatin1String("system.nt_sec_desc.acl+:") + sid;
    qCDebug(KIO_SMB_LOG) << attr << aceString;

    // https://bugzilla.samba.org/show_bug.cgi?id=15089
    auto flags = SMBC_XATTR_FLAG_REPLACE | SMBC_XATTR_FLAG_CREATE;
    const QByteArray ace = aceString.toUtf8();
    int result = smbc_setxattr(url.toSmbcUrl(), qUtf8Printable(attr), ace.constData(), ace.size(), flags);
    if (const auto error = errno; result < 0) {
        qCDebug(KIO_SMB_LOG) << "smbc_setxattr" << result << strerror(error);
        return WorkerResult::fail(ERR_INTERNAL, strerror(error));
    }
    return WorkerResult::pass();
}

// TODO: rename this file _special instead of _mount. Or better yet: stop having multiple cpp files for the same class.
KIO::WorkerResult SMBWorker::special(const QByteArray &data)
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
                return WorkerResult::fail(KIO::ERR_CANNOT_MKDIR, mountPoint);
            }
        }

        SMBUrl smburl(QUrl("smb:///"));
        smburl.setHost(host);
        smburl.setPath('/' + share);

        const int passwordError = checkPassword(smburl);
        if (passwordError != KJob::NoError && passwordError != KIO::ERR_USER_CANCELED) {
            return WorkerResult::fail(passwordError, smburl.toString());
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
            return WorkerResult::fail(KIO::ERR_CANNOT_LAUNCH_PROCESS, "smbmount" + i18n("\nMake sure that the samba package is installed properly on your system."));
        }

        QString mybuf = QString::fromLocal8Bit(proc.readAllStandardOutput());
        QString mystderr = QString::fromLocal8Bit(proc.readAllStandardError());

        qCDebug(KIO_SMB_LOG) << "mount exit " << proc.exitCode() << "stdout:" << mybuf << "\nstderr:" << mystderr;

        if (proc.exitCode() != 0) {
            return WorkerResult::fail(KIO::ERR_CANNOT_MOUNT, i18n("Mounting of share \"%1\" from host \"%2\" by user \"%3\" failed.\n%4", share, host, user, mybuf + '\n' + mystderr));
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
            return WorkerResult::fail(KIO::ERR_CANNOT_LAUNCH_PROCESS, "smbumount" + i18n("\nMake sure that the samba package is installed properly on your system."));
        }

        QString mybuf = QString::fromLocal8Bit(proc.readAllStandardOutput());
        QString mystderr = QString::fromLocal8Bit(proc.readAllStandardError());

        qCDebug(KIO_SMB_LOG) << "smbumount exit " << proc.exitCode() << "stdout:" << mybuf << "\nstderr:" << mystderr;

        if (proc.exitCode() != 0) {
            return WorkerResult::fail(KIO::ERR_CANNOT_UNMOUNT, i18n("Unmounting of mountpoint \"%1\" failed.\n%2", mountPoint, mybuf + '\n' + mystderr));
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
                return WorkerResult::fail(KIO::ERR_CANNOT_RMDIR, mountPoint);
            }
        }
    } break;
    case 0xAC: { // ACL
        return getACE(stream);
    }
    case 0xACD: { // setACE
        return setACE(stream);
    }
    default:
        break;
    }
    return WorkerResult::pass();
}
