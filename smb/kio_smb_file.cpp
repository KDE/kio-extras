/*
    SPDX-License-Identifier: GPL-2.0-or-later
    SPDX-FileCopyrightText: 2000 Caldera Systems Inc.
    SPDX-FileCopyrightText: 2018-2021 Harald Sitter <sitter@kde.org>
    SPDX-FileContributor: Matthew Peterson <mpeterson@caldera.com>
*/

#include "kio_smb.h"
#include "smburl.h"

#include <QMimeDatabase>
#include <QMimeType>
#include <QScopeGuard>
#include <QVarLengthArray>

#include <KLocalizedString>

#include <future>

#include "transfer.h"

WorkerResult SMBWorker::get(const QUrl &kurl)
{
    qCDebug(KIO_SMB_LOG) << kurl;

    // check (correct) URL
    QUrl kvurl = checkURL(kurl);
    // if URL is not valid we have to redirect to correct URL
    if (kvurl != kurl) {
        redirection(kvurl);
        return WorkerResult::pass();
    }

    if (!m_context.isValid()) {
        return WorkerResult::fail(ERR_INTERNAL, i18n("libsmbclient failed to create context"));
    }

    // Stat
    SMBUrl url = kurl;
    int errNum = cache_stat(url, &st);
    if (errNum != 0) {
        if (errNum == EACCES) {
            return WorkerResult::fail(KIO::ERR_ACCESS_DENIED, url.toDisplayString());
        }
        return WorkerResult::fail(KIO::ERR_DOES_NOT_EXIST, url.toDisplayString());
    }
    if (S_ISDIR(st.st_mode)) {
        return WorkerResult::fail(KIO::ERR_IS_DIRECTORY, url.toDisplayString());
    }

    // Set the total size
    totalSize(st.st_size);

    // Open and read the file
    int filefd = smbc_open(url.toSmbcUrl(), O_RDONLY, 0);
    if (filefd < 0) {
        return WorkerResult::fail(KIO::ERR_CANNOT_OPEN_FOR_READING, url.toDisplayString());
    }
    auto filefdClose = qScopeGuard([filefd] {
        smbc_close(filefd);
    });

    KIO::filesize_t totalbytesread = 0;
    QByteArray filedata;
    bool isFirstPacket = true;

    TransferRingBuffer buffer(st.st_size);
    auto future = std::async(std::launch::async, [&buffer, &filefd]() -> int {
        while (true) {
            TransferSegment *s = buffer.nextFree();
            s->size = smbc_read(filefd, s->buf.data(), s->buf.capacity());
            if (s->size <= 0) {
                buffer.push();
                buffer.done();
                if (s->size < 0) {
                    return KIO::ERR_CANNOT_READ;
                }
                break;
            }
            buffer.push();
        }
        return KJob::NoError;
    });

    while (true) {
        TransferSegment *s = buffer.pop();
        if (!s) { // done, no more segments pending
            break;
        }

        filedata = QByteArray::fromRawData(s->buf.data(), s->size);
        if (isFirstPacket) {
            QMimeDatabase db;
            QMimeType type = db.mimeTypeForFileNameAndData(url.fileName(), filedata);
            mimeType(type.name());
            isFirstPacket = false;
        }
        data(filedata);
        filedata.clear();

        // increment total bytes read
        totalbytesread += s->size;

        processedSize(totalbytesread);
        buffer.unpop();
    }
    if (future.get() != KJob::NoError) { // check if read had an error
        return WorkerResult::fail(future.get(), url.toDisplayString());
    }

    data(QByteArray());
    if (totalbytesread != static_cast<KIO::filesize_t>(st.st_size)) {
        qCWarning(KIO_SMB_LOG) << "Got" << totalbytesread << "bytes but expected" << st.st_size;
    }
    processedSize(static_cast<KIO::filesize_t>(st.st_size));

    return WorkerResult::pass();
}

WorkerResult SMBWorker::open(const QUrl &kurl, QIODevice::OpenMode mode)
{
    int errNum = 0;
    qCDebug(KIO_SMB_LOG) << kurl;

    // check (correct) URL
    QUrl kvurl = checkURL(kurl);

    // if URL is not valid we have to redirect to correct URL
    if (kvurl != kurl) {
        redirection(kvurl);
        return WorkerResult::pass();
    }

    if (!m_context.isValid()) {
        return WorkerResult::fail(KIO::ERR_ACCESS_DENIED, kurl.toDisplayString());
    }

    // Save the URL as a private member
    // FIXME For some reason m_openUrl has be be declared in bottom private
    // section of the class SMBWorker declaration instead of the top section
    // or else this assignment fails
    m_openUrl = kurl;

    // FIXME authentication is missing here. when starting a FileJob without first otherwise accessing the path
    // to cache credentials this results in failure to open the file... should do the auth dance from browse here

    // Stat
    errNum = cache_stat(m_openUrl, &st);
    if (errNum != 0) {
        if (errNum == EACCES) {
            return WorkerResult::fail(KIO::ERR_ACCESS_DENIED, m_openUrl.toDisplayString());
        }
        return WorkerResult::fail(KIO::ERR_DOES_NOT_EXIST, m_openUrl.toDisplayString());
    }
    if (S_ISDIR(st.st_mode)) {
        return WorkerResult::fail(KIO::ERR_IS_DIRECTORY, m_openUrl.toDisplayString());
    }

    // Set the total size
    totalSize(st.st_size);

    // Convert permissions
    int flags = 0;
    if (mode & QIODevice::ReadOnly) {
        if (mode & QIODevice::WriteOnly) {
            flags = O_RDWR | O_CREAT;
        } else {
            flags = O_RDONLY;
        }
    } else if (mode & QIODevice::WriteOnly) {
        flags = O_WRONLY | O_CREAT;
    }

    if (mode & QIODevice::Append) {
        flags |= O_APPEND;
    } else if (mode & QIODevice::Truncate) {
        flags |= O_TRUNC;
    }

    // Open the file
    m_openFd = smbc_open(m_openUrl.toSmbcUrl(), flags, 0);
    if (m_openFd < 0) {
        return WorkerResult::fail(KIO::ERR_CANNOT_OPEN_FOR_READING, m_openUrl.toDisplayString());
    }

    // Determine the mimetype of the file to be retrieved, and emit it.
    // This is mandatory in all workers (for KRun/BrowserRun to work).
    // If we're not opening the file ReadOnly or ReadWrite, don't attempt to
    // read the file and send the mimetype.
    if (mode & QIODevice::ReadOnly) {
        ssize_t bytesRequested = 1024;
        ssize_t bytesRead = 0;
        QVarLengthArray<char> buffer(bytesRequested);
        bytesRead = smbc_read(m_openFd, buffer.data(), bytesRequested);
        if (bytesRead < 0) {
            closeWithoutFinish();
            return WorkerResult::fail(KIO::ERR_CANNOT_READ, m_openUrl.toDisplayString());
        }
        QByteArray fileData = QByteArray::fromRawData(buffer.data(), bytesRead);
        QMimeDatabase db;
        QMimeType type = db.mimeTypeForFileNameAndData(m_openUrl.fileName(), fileData);
        mimeType(type.name());

        off_t res = smbc_lseek(m_openFd, 0, SEEK_SET);
        if (res == (off_t)-1) {
            closeWithoutFinish();
            return WorkerResult::fail(KIO::ERR_CANNOT_SEEK, m_openUrl.path());
        }
    }

    position(0);
    return WorkerResult::pass();
}

WorkerResult SMBWorker::read(KIO::filesize_t bytesRequested)
{
    Q_ASSERT(m_openFd != -1);

    QVarLengthArray<char> buffer(bytesRequested);
    ssize_t bytesRead = 0;

    bytesRead = smbc_read(m_openFd, buffer.data(), bytesRequested);
    Q_ASSERT(bytesRead <= static_cast<ssize_t>(bytesRequested));

    if (bytesRead < 0) {
        qCDebug(KIO_SMB_LOG) << "Could not read " << m_openUrl;
        closeWithoutFinish();
        return WorkerResult::fail(KIO::ERR_CANNOT_READ, m_openUrl.toDisplayString());
    }

    QByteArray fileData = QByteArray::fromRawData(buffer.data(), bytesRead);
    data(fileData);
    return WorkerResult::pass();
}

WorkerResult SMBWorker::write(const QByteArray &fileData)
{
    Q_ASSERT(m_openFd != -1);

    QByteArray buf(fileData);

    ssize_t size = smbc_write(m_openFd, buf.data(), buf.size());
    if (size < 0) {
        qCDebug(KIO_SMB_LOG) << "Could not write to " << m_openUrl;
        closeWithoutFinish();
        return WorkerResult::fail(KIO::ERR_CANNOT_WRITE, m_openUrl.toDisplayString());
    }

    written(size);
    return WorkerResult::pass();
}

WorkerResult SMBWorker::seek(KIO::filesize_t offset)
{
    off_t res = smbc_lseek(m_openFd, static_cast<off_t>(offset), SEEK_SET);
    if (res == (off_t)-1) {
        closeWithoutFinish();
        return WorkerResult::fail(KIO::ERR_CANNOT_SEEK, m_openUrl.path());
    }
    qCDebug(KIO_SMB_LOG) << "res" << res;
    position(res);
    return WorkerResult::pass();
}

WorkerResult SMBWorker::truncate(KIO::filesize_t length)
{
    off_t res = smbc_ftruncate(m_openFd, static_cast<off_t>(length));
    if (res < 0) {
        closeWithoutFinish();
        return WorkerResult::fail(KIO::ERR_CANNOT_TRUNCATE, m_openUrl.path());
    }
    qCDebug(KIO_SMB_LOG) << "res" << res;
    truncated(length);
    return WorkerResult::pass();
}

void SMBWorker::closeWithoutFinish()
{
    smbc_close(m_openFd);
}

WorkerResult SMBWorker::close()
{
    closeWithoutFinish();
    return WorkerResult::pass();
}

WorkerResult SMBWorker::put(const QUrl &kurl, int permissions, KIO::JobFlags flags)
{
    void *buf;
    size_t bufsize;

    m_current_url = kurl;

    int filefd;
    bool exists;
    int errNum = 0;
    off_t retValLSeek = 0;
    mode_t mode;
    QByteArray filedata;

    qCDebug(KIO_SMB_LOG) << kurl << flags;

    errNum = cache_stat(m_current_url, &st);
    exists = (errNum == 0);
    if (exists && !(flags & KIO::Overwrite) && !(flags & KIO::Resume)) {
        if (S_ISDIR(st.st_mode)) {
            qCDebug(KIO_SMB_LOG) << kurl << " already isdir !!";
            return WorkerResult::fail(KIO::ERR_DIR_ALREADY_EXIST, m_current_url.toDisplayString());
        }
        qCDebug(KIO_SMB_LOG) << kurl << " already exist !!";
        return WorkerResult::fail(KIO::ERR_FILE_ALREADY_EXIST, m_current_url.toDisplayString());
    }

    if (exists && !(flags & KIO::Resume) && (flags & KIO::Overwrite)) {
        qCDebug(KIO_SMB_LOG) << "exists try to remove " << m_current_url.toSmbcUrl();
        //   remove(m_current_url.url().toLocal8Bit());
    }

    if (flags & KIO::Resume) {
        // append if resuming
        qCDebug(KIO_SMB_LOG) << "resume " << m_current_url.toSmbcUrl();
        filefd = smbc_open(m_current_url.toSmbcUrl(), O_RDWR, 0);
        if (filefd < 0) {
            errNum = errno;
        } else {
            errNum = 0;
        }

        retValLSeek = smbc_lseek(filefd, 0, SEEK_END);
        if (retValLSeek == (off_t)-1) {
            errNum = errno;
        } else {
            errNum = 0;
        }
    } else {
        if (permissions != -1) {
            mode = permissions | S_IWUSR | S_IRUSR;
        } else {
            mode = S_IWUSR | S_IRUSR;
        }

        qCDebug(KIO_SMB_LOG) << "NO resume " << m_current_url.toSmbcUrl();
        filefd = smbc_open(m_current_url.toSmbcUrl(), O_CREAT | O_TRUNC | O_WRONLY, mode);
        if (filefd < 0) {
            errNum = errno;
        } else {
            errNum = 0;
        }
    }

    if (filefd < 0) {
        if (errNum == EACCES) {
            qCDebug(KIO_SMB_LOG) << "error " << kurl << " access denied !!";
            return WorkerResult::fail(KIO::ERR_WRITE_ACCESS_DENIED, m_current_url.toDisplayString());
        }
        qCDebug(KIO_SMB_LOG) << "error " << kurl << " can not open for writing !!";
        return WorkerResult::fail(KIO::ERR_CANNOT_OPEN_FOR_WRITING, m_current_url.toDisplayString());
    }
    auto closeFileFd = qScopeGuard([filefd] {
        smbc_close(filefd);
    });

    // Loop until we got 0 (end of data)
    while (true) {
        qCDebug(KIO_SMB_LOG) << "request data ";
        dataReq(); // Request for data
        qCDebug(KIO_SMB_LOG) << "write " << m_current_url.toSmbcUrl();

        if (readData(filedata) <= 0) {
            qCDebug(KIO_SMB_LOG) << "readData <= 0";
            break;
        }
        qCDebug(KIO_SMB_LOG) << "write " << m_current_url.toSmbcUrl();
        buf = filedata.data();
        bufsize = filedata.size();
        ssize_t size = smbc_write(filefd, buf, bufsize);
        if (size < 0) {
            qCDebug(KIO_SMB_LOG) << "error " << kurl << "could not write !!";
            return WorkerResult::fail(KIO::ERR_CANNOT_WRITE, m_current_url.toDisplayString());
        }
        qCDebug(KIO_SMB_LOG) << "wrote " << size;
    }
    qCDebug(KIO_SMB_LOG) << "close " << m_current_url.toSmbcUrl();

    if (smbc_close(filefd) < 0) {
        qCDebug(KIO_SMB_LOG) << kurl << "could not write !!";
        return WorkerResult::fail(KIO::ERR_CANNOT_WRITE, m_current_url.toDisplayString());
    }

    // set final permissions, if the file was just created
    if (permissions != -1 && !exists) {
        // TODO: did the smbc_chmod fail?
        // TODO: put in call to chmod when it is working!
        // smbc_chmod(url.toSmbcUrl(),permissions);
    }

    applyMTimeSMBC(m_current_url);

    return WorkerResult::pass();
}
