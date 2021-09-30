/*
    SPDX-License-Identifier: GPL-2.0-or-later
    SPDX-FileCopyrightText: 2000 Caldera Systems Inc.
    SPDX-FileCopyrightText: 2021 Harald Sitter <sitter@kde.org>
    SPDX-FileContributor: Matthew Peterson <mpeterson@caldera.com>
*/

#include "kio_smb.h"
#include "smburl.h"

#include <QFile>
#include <QFileInfo>

#include <kconfiggroup.h>
#include <kio/ioslave_defaults.h>

#include <future>

#include "transfer.h"
#include "transfer_resume.h"

void SMBSlave::copy(const QUrl &src, const QUrl &dst, int permissions, KIO::JobFlags flags)
{
    const bool isSourceLocal = src.isLocalFile();
    const bool isDestinationLocal = dst.isLocalFile();

    if (!isSourceLocal && isDestinationLocal) {
        smbCopyGet(src, dst, permissions, flags);
    } else if (isSourceLocal && !isDestinationLocal) {
        smbCopyPut(src, dst, permissions, flags);
    } else {
        smbCopy(src, dst, permissions, flags);
    }
}

void SMBSlave::smbCopy(const QUrl &ksrc, const QUrl &kdst, int permissions, KIO::JobFlags flags)
{
    qCDebug(KIO_SMB_LOG) << "SMBSlave::copy with src = " << ksrc << "and dest = " << kdst << flags;

    // setup urls
    SMBUrl src = ksrc;
    SMBUrl dst = kdst;

    // Obtain information about source
    int errNum = cache_stat(src, &st);
    if (errNum != 0) {
        if (errNum == EACCES) {
            error(KIO::ERR_ACCESS_DENIED, src.toDisplayString());
        } else {
            error(KIO::ERR_DOES_NOT_EXIST, src.toDisplayString());
        }
        return;
    }
    if (S_ISDIR(st.st_mode)) {
        error(KIO::ERR_IS_DIRECTORY, src.toDisplayString());
        return;
    }
    const auto srcSize = st.st_size;
    totalSize(srcSize);

    // Check to se if the destination exists
    errNum = cache_stat(dst, &st);
    if (errNum == 0) {
        if (S_ISDIR(st.st_mode)) {
            error(KIO::ERR_DIR_ALREADY_EXIST, dst.toDisplayString());
            return;
        }
        if (!(flags & KIO::Overwrite)) {
            error(KIO::ERR_FILE_ALREADY_EXIST, dst.toDisplayString());
            return;
        }
    }

    // Open the source file
    int srcfd = smbc_open(src.toSmbcUrl(), O_RDONLY, 0);
    if (srcfd < 0) {
        errNum = errno;
    } else {
        errNum = 0;
    }

    if (srcfd < 0) {
        if (errNum == EACCES) {
            error(KIO::ERR_ACCESS_DENIED, src.toDisplayString());
        } else {
            error(KIO::ERR_DOES_NOT_EXIST, src.toDisplayString());
        }
        return;
    }


    mode_t initialmode = 0;
    // Determine initial creation mode
    if (permissions != -1) {
        initialmode = permissions | S_IWUSR;
    } else {
        initialmode = 0 | S_IWUSR; // 0666;
    }

    // Open the destination file
    int dstflags = O_CREAT | O_TRUNC | O_WRONLY;
    if (!(flags & KIO::Overwrite)) {
        dstflags |= O_EXCL;
    }
    int dstfd = smbc_open(dst.toSmbcUrl(), dstflags, initialmode);
    if (dstfd < 0) {
        errNum = errno;
    } else {
        errNum = 0;
    }

    if (dstfd < 0) {
        if (errNum == EACCES) {
            error(KIO::ERR_WRITE_ACCESS_DENIED, dst.toDisplayString());
        } else {
            error(KIO::ERR_CANNOT_OPEN_FOR_READING, dst.toDisplayString());
        }

        if (srcfd >= 0) {
            smbc_close(srcfd);
        }
        return;
    }

    // Perform copy
    // TODO: if and when smb_context becomes thread-safe, use two contexts connected with
    //   a ring buffer to optimize transfer speed (also see smbCopyGet)
    //   https://bugzilla.samba.org/show_bug.cgi?id=11413
    KIO::filesize_t processed_size = 0;
    TransferSegment segment(srcSize);
    while (true) {
        ssize_t n = smbc_read(srcfd, segment.buf.data(), segment.buf.size());
        if (n > 0) {
            n = smbc_write(dstfd, segment.buf.data(), n);
            if (n == -1) {
                qCDebug(KIO_SMB_LOG) << "SMBSlave::copy copy now KIO::ERR_CANNOT_WRITE";
                error(KIO::ERR_CANNOT_WRITE, dst.toDisplayString());
                break;
            }

            processed_size += n;
            processedSize(processed_size);
        } else if (n == 0) {
            break; // finished
        } else {
            error(KIO::ERR_CANNOT_READ, src.toDisplayString());
            break;
        }
    }

    //    FINISHED:

    if (srcfd >= 0) {
        smbc_close(srcfd);
    }

    if (dstfd >= 0) {
        if (smbc_close(dstfd) == 0) {
            // TODO: set final permissions
        } else {
            error(KIO::ERR_CANNOT_WRITE, dst.toDisplayString());
            return;
        }
    }

    applyMTimeSMBC(dst);

    finished();
}

void SMBSlave::smbCopyGet(const QUrl &ksrc, const QUrl &kdst, int permissions, KIO::JobFlags flags)
{
    qCDebug(KIO_SMB_LOG) << "src = " << ksrc << ", dest = " << kdst << flags;

    // check if destination is ok ...
    const QString dstFile = kdst.toLocalFile();
    const QFileInfo dstInfo(dstFile);

    if (dstInfo.exists()) {
        if (dstInfo.isDir()) {
            error(ERR_IS_DIRECTORY, kdst.toDisplayString());
            return;
        }

        if (!(flags & KIO::Overwrite)) {
            error(ERR_FILE_ALREADY_EXIST, kdst.toDisplayString());
            return;
        }
    }

    auto optionalResume = Transfer::shouldResume<QFileResumeIO>(kdst, flags, this);
    if (!optionalResume.has_value()) { // had error
        return;
    }
    const auto resume = optionalResume.value();

    // open the output file...
    const QFile::OpenMode mode = resume.resuming ? (QFile::WriteOnly | QFile::Append) : (QFile::WriteOnly | QFile::Truncate);

    QFile file(resume.destination.path());
    if (!resume.resuming) {
        QFile::Permissions perms;
        if (permissions == -1) {
            perms = QFile::ReadOwner | QFile::WriteOwner;
        } else {
            perms = KIO::convertPermissions(permissions | QFile::WriteOwner);
        }
        file.setPermissions(perms);
    }

    if (!file.open(mode)) {
        qCDebug(KIO_SMB_LOG) << "could not write to" << dstFile;
        switch (file.error()) {
        case QFile::OpenError:
            if (resume.resuming) {
                error(ERR_CANNOT_RESUME, kdst.toDisplayString());
            } else {
                error(ERR_CANNOT_OPEN_FOR_WRITING, kdst.toDisplayString());
            }
            break;
        case QFile::PermissionsError:
            error(ERR_WRITE_ACCESS_DENIED, kdst.toDisplayString());
            break;
        default:
            error(ERR_CANNOT_OPEN_FOR_WRITING, kdst.toDisplayString());
            break;
        }
        return;
    }

    // setup the source urls
    const SMBUrl src(ksrc);

    // Obtain information about source
    int errNum = cache_stat(src, &st);
    if (errNum != 0) {
        if (errNum == EACCES) {
            error(KIO::ERR_ACCESS_DENIED, src.toDisplayString());
        } else {
            error(KIO::ERR_DOES_NOT_EXIST, src.toDisplayString());
        }
        return;
    }

    if (S_ISDIR(st.st_mode)) {
        error(KIO::ERR_IS_DIRECTORY, src.toDisplayString());
        return;
    }
    totalSize(st.st_size);

    // Open the source file
    KIO::filesize_t processed_size = 0;
    int srcfd = smbc_open(src.toSmbcUrl(), O_RDONLY, 0);
    if (srcfd < 0) {
        errNum = errno;
    } else {
        errNum = 0;
        if (resume.resuming) {
            qCDebug(KIO_SMB_LOG) << "seeking to size" << resume.destinationOffset;
            off_t offset = smbc_lseek(srcfd, resume.destinationOffset, SEEK_SET);
            if (offset == -1) {
                error(KIO::ERR_CANNOT_SEEK, src.toDisplayString());
                smbc_close(srcfd);
                return;
            } else {
                processed_size += offset;
            }
        }
    }

    if (srcfd < 0) {
        if (errNum == EACCES) {
            error(KIO::ERR_ACCESS_DENIED, src.toDisplayString());
        } else {
            error(KIO::ERR_DOES_NOT_EXIST, src.toDisplayString());
        }
        return;
    }

    std::atomic<bool> isErr(false);
    TransferRingBuffer buffer(st.st_size);
    auto future = std::async(std::launch::async, [&buffer, &srcfd, &isErr]() -> int {
        while (!isErr) {
            TransferSegment *segment = buffer.nextFree();
            segment->size = smbc_read(srcfd, segment->buf.data(), segment->buf.capacity());
            if (segment->size <= 0) {
                buffer.push();
                buffer.done();
                if (segment->size < 0) {
                    return KIO::ERR_CANNOT_READ;
                }
                break;
            }
            buffer.push();
        }
        return KJob::NoError;
    });

    while (true) {
        TransferSegment *segment = buffer.pop();
        if (!segment) { // done, no more segments pending
            break;
        }

        const qint64 bytesWritten = file.write(segment->buf.data(), segment->size);
        if (bytesWritten == -1) {
            qCDebug(KIO_SMB_LOG) << "copy now KIO::ERR_CANNOT_WRITE";
            error(KIO::ERR_CANNOT_WRITE, kdst.toDisplayString());
            isErr = true;
            buffer.unpop();
            break;
        }

        processed_size += bytesWritten;
        processedSize(processed_size);
        buffer.unpop();
    }
    if (isErr) { // writing failed
        future.wait();
    } else if (future.get() != KJob::NoError) { // check if read had an error
        error(future.get(), ksrc.toDisplayString());
        isErr = true;
    }

    // FINISHED
    smbc_close(srcfd);

    // Handle error condition.

    if (Transfer::concludeResumeHasError<QFileResumeIO>(isErr, resume, this)) {
        return; // NB: error() called inside if applicable
    }

    // set modification time (if applicable)
    applyMTime([dstFile](struct utimbuf utbuf) {
        utbuf.actime = QFileInfo(dstFile).lastRead().toSecsSinceEpoch(); // access time, unchanged
        utime(QFile::encodeName(dstFile).constData(), &utbuf);
    });

    finished();
}

void SMBSlave::smbCopyPut(const QUrl &ksrc, const QUrl &kdst, int permissions, KIO::JobFlags flags)
{
    qCDebug(KIO_SMB_LOG) << "src = " << ksrc << ", dest = " << kdst << flags;

    QFile srcFile(ksrc.toLocalFile());
    const QFileInfo srcInfo(srcFile);

    if (srcInfo.exists()) {
        if (srcInfo.isDir()) {
            error(KIO::ERR_IS_DIRECTORY, ksrc.toDisplayString());
            return;
        }
    } else {
        error(KIO::ERR_DOES_NOT_EXIST, ksrc.toDisplayString());
        return;
    }

    if (!srcFile.open(QFile::ReadOnly)) {
        qCDebug(KIO_SMB_LOG) << "could not read from" << ksrc;
        switch (srcFile.error()) {
        case QFile::PermissionsError:
            error(KIO::ERR_WRITE_ACCESS_DENIED, ksrc.toDisplayString());
            break;
        case QFile::OpenError:
        default:
            error(KIO::ERR_CANNOT_OPEN_FOR_READING, ksrc.toDisplayString());
            break;
        }
        return;
    }

    totalSize(static_cast<filesize_t>(srcInfo.size()));

    const SMBUrl dstOrigUrl(kdst);

    const std::optional<TransferContext> resumeOptional = Transfer::shouldResume<SMBResumeIO>(dstOrigUrl, flags, this);
    if (!resumeOptional.has_value()) { // had an error
        return;
    }
    const TransferContext &resume = resumeOptional.value();

    KIO::filesize_t processed_size = 0;
    const SMBUrl dstUrl(resume.destination);

    int dstfd = -1;
    int errNum = 0;
    if (resume.resuming) {
        // append if resuming
        qCDebug(KIO_SMB_LOG) << "resume" << dstUrl;
        dstfd = smbc_open(dstUrl.toSmbcUrl(), O_RDWR, 0);
        if (dstfd < 0) {
            errNum = errno;
        } else {
            const off_t offset = smbc_lseek(dstfd, 0, SEEK_END);
            if (offset == (off_t)-1) {
                error(KIO::ERR_CANNOT_SEEK, dstUrl.toDisplayString());
                smbc_close(dstfd);
                return;
            } else {
                processed_size = offset;
            }
        }
    } else {
        mode_t mode;
        if (permissions == -1) {
            mode = 600;
        } else {
            mode = permissions | S_IRUSR | S_IWUSR;
        }

        qCDebug(KIO_SMB_LOG) << "NO resume" << dstUrl;
        dstfd = smbc_open(dstUrl.toSmbcUrl(), O_CREAT | O_TRUNC | O_WRONLY, mode);
        if (dstfd < 0) {
            errNum = errno;
        }
    }

    if (dstfd < 0) {
        if (errNum == EACCES) {
            qCDebug(KIO_SMB_LOG) << "access denied";
            error(KIO::ERR_WRITE_ACCESS_DENIED, dstUrl.toDisplayString());
        } else {
            qCDebug(KIO_SMB_LOG) << "can not open for writing";
            error(KIO::ERR_CANNOT_OPEN_FOR_WRITING, dstUrl.toDisplayString());
        }
        return;
    }

    bool isErr = false;
    if (processed_size == 0 || srcFile.seek(processed_size)) {
        // Perform the copy
        TransferSegment segment(srcInfo.size());
        while (true) {
            const ssize_t bytesRead = srcFile.read(segment.buf.data(), segment.buf.size());
            if (bytesRead <= 0) {
                if (bytesRead < 0) {
                    error(KIO::ERR_CANNOT_READ, ksrc.toDisplayString());
                    isErr = true;
                }
                break;
            }

            const qint64 bytesWritten = smbc_write(dstfd, segment.buf.data(), bytesRead);
            if (bytesWritten == -1) {
                error(KIO::ERR_CANNOT_WRITE, kdst.toDisplayString());
                isErr = true;
                break;
            }

            processed_size += bytesWritten;
            processedSize(processed_size);
        }
    } else {
        isErr = true;
        error(KIO::ERR_CANNOT_SEEK, ksrc.toDisplayString());
    }

    // FINISHED
    if (smbc_close(dstfd) < 0) {
        qCDebug(KIO_SMB_LOG) << dstUrl << "could not write";
        error(KIO::ERR_CANNOT_WRITE, dstUrl.toDisplayString());
        return;
    }

    if (Transfer::concludeResumeHasError<SMBResumeIO>(isErr, resume, this)) {
        return; // NB: error() called inside if applicable
     }

    applyMTimeSMBC(dstOrigUrl);

    // We have done our job => finish
    finished();
}

void SMBSlave::del(const QUrl &kurl, bool isfile)
{
    qCDebug(KIO_SMB_LOG) << kurl;
    m_current_url = kurl;
    int errNum = 0;
    int retVal = 0;

    if (isfile) {
        // Delete file
        qCDebug(KIO_SMB_LOG) << "Deleting file" << kurl;
        retVal = smbc_unlink(m_current_url.toSmbcUrl());
        if (retVal < 0) {
            errNum = errno;
        } else {
            errNum = 0;
        }
    } else {
        qCDebug(KIO_SMB_LOG) << "Deleting directory" << kurl;
        // Delete directory
        retVal = smbc_rmdir(m_current_url.toSmbcUrl());
        if (retVal < 0) {
            errNum = errno;
        } else {
            errNum = 0;
        }
    }

    if (errNum != 0) {
        reportError(kurl, errNum);
    } else {
        finished();
    }
}

void SMBSlave::mkdir(const QUrl &kurl, int permissions)
{
    qCDebug(KIO_SMB_LOG) << kurl;
    int errNum = 0;
    int retVal = 0;
    m_current_url = kurl;

    retVal = smbc_mkdir(m_current_url.toSmbcUrl(), S_IRWXU | S_IRWXG | S_IRWXO);
    if (retVal < 0) {
        errNum = errno;
    } else {
        errNum = 0;
    }

    if (retVal < 0) {
        if (errNum == EEXIST) {
            errNum = cache_stat(m_current_url, &st);
            if (errNum == 0 && S_ISDIR(st.st_mode)) {
                error(KIO::ERR_DIR_ALREADY_EXIST, m_current_url.toDisplayString());
            } else {
                error(KIO::ERR_FILE_ALREADY_EXIST, m_current_url.toDisplayString());
            }
        } else {
            reportError(kurl, errNum);
        }
        qCDebug(KIO_SMB_LOG) << "exit with error " << kurl;
    } else // success
    {
        if (permissions != -1) {
            // TODO enable the following when complete
            // smbc_chmod( url.toSmbcUrl(), permissions );
        }
        finished();
    }
}

static bool sameInodeStat(bool hasSrcStat, const struct stat srcStat, const struct stat dstStat)
{
    if (!hasSrcStat) {
        return false;
    }
    const auto badInode = static_cast<ino_t>(-1); // Fun fact: smb actually has code paths that can return -1 cast to ino_t (i.e. unsigned) ... ... ... :(
    if (srcStat.st_ino == badInode || dstStat.st_ino == badInode) {
        // If either returns a bad inode we don't know and assume they aren't the same entity.
        return false;
    }
    const bool equal = (srcStat.st_ino == dstStat.st_ino && srcStat.st_dev == dstStat.st_dev);
    qDebug(KIO_SMB_LOG) << "sameInodeStat"
                        << "equal" << equal << "hasSrcStat" << hasSrcStat << "srcStat.st_ino" << srcStat.st_ino << "dstStat.st_ino" << dstStat.st_ino
                        << "badInode" << badInode << "srcStat.st_dev" << srcStat.st_dev << "dstStat.st_dev" << dstStat.st_dev;
    return equal;
}

void SMBSlave::rename(const QUrl &ksrc, const QUrl &kdest, KIO::JobFlags flags)
{
    SMBUrl src;
    SMBUrl dst;
    int errNum = 0;
    int retVal = 0;

    qCDebug(KIO_SMB_LOG) << "old name = " << ksrc << ", new name = " << kdest << flags;

    src = ksrc;
    dst = kdest;

    // Check to se if the destination exists
    // Samba can be case sensitive or insensitive, depending on server capabilities and configuration. As such if the src and dst have the same case insensitive
    // name we'll stat the src to ascertain its inode and device. On recent samba and windows servers these seem to provide actually valid numbers we can rely
    // on. When not we can still pretend like they aren't the same file. Worst case the user gets an overwrite prompt.
    // https://bugs.kde.org/show_bug.cgi?id=430585
    bool hasSrcStat = false;
    struct stat srcStat {};
    if (src.path().compare(dst.path(), Qt::CaseInsensitive) == 0) {
        qCDebug(KIO_SMB_LOG) << "smbc_rename " << "src and dst insensitive equal, performing inode comparision";
        errNum = cache_stat(dst, &st);
        if (errNum == 0) {
            hasSrcStat = true;
            srcStat = st;
        }
    }
    errNum = cache_stat(dst, &st);
    if (errNum == 0 && !sameInodeStat(hasSrcStat, srcStat, st)) {
        if (S_ISDIR(st.st_mode)) {
            qCDebug(KIO_SMB_LOG) << "KIO::ERR_DIR_ALREADY_EXIST";
            error(KIO::ERR_DIR_ALREADY_EXIST, dst.toDisplayString());
            return;
        }
        if (!(flags & KIO::Overwrite)) {
            qCDebug(KIO_SMB_LOG) << "KIO::ERR_FILE_ALREADY_EXIST";
            error(KIO::ERR_FILE_ALREADY_EXIST, dst.toDisplayString());
            return;
        }
    }

    qCDebug(KIO_SMB_LOG) << "smbc_rename " << src.toSmbcUrl() << " " << dst.toSmbcUrl();
    retVal = smbc_rename(src.toSmbcUrl(), dst.toSmbcUrl());
    if (retVal < 0) {
        errNum = errno;
    } else {
        errNum = 0;
    }

    if (retVal < 0) {
        qCDebug(KIO_SMB_LOG) << "failed ";
        switch (errNum) {
        case ENOENT:
            errNum = cache_stat(src, &st);
            if (errNum != 0) {
                if (errNum == EACCES) {
                    qCDebug(KIO_SMB_LOG) << "KIO::ERR_ACCESS_DENIED";
                    error(KIO::ERR_ACCESS_DENIED, src.toDisplayString());
                } else {
                    qCDebug(KIO_SMB_LOG) << "KIO::ERR_DOES_NOT_EXIST";
                    error(KIO::ERR_DOES_NOT_EXIST, src.toDisplayString());
                }
            }
            break;

        case EACCES:
        case EPERM:
            qCDebug(KIO_SMB_LOG) << "KIO::ERR_ACCESS_DENIED";
            error(KIO::ERR_ACCESS_DENIED, dst.toDisplayString());
            break;

        default:
            qCDebug(KIO_SMB_LOG) << "KIO::ERR_CANNOT_RENAME";
            error(KIO::ERR_CANNOT_RENAME, src.toDisplayString());
        }

        qCDebug(KIO_SMB_LOG) << "exit with error";
        return;
    }

    qCDebug(KIO_SMB_LOG) << "everything fine\n";
    finished();
}
