/*
    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2021-2022 Harald Sitter <sitter@kde.org>
*/

#pragma once

#include <optional>

#include <QFileInfo>

#include <kio/ioworker_defaults.h>

#include "kio_smb.h"

// Carries the context of a file transfer.
struct TransferContext {
    // When resuming a file. This is false when starting a new .part!
    // To establish if a partial file is used the completeDestination should be compared with the partDestination.
    const bool resuming;
    // The intermediate destination
    const SMBUrl destination;
    // The part destination. This is null when not using a partial file.
    const SMBUrl partDestination;
    // The complete destination i.e. the final destination i.e. the place where the file will be once all is said and done
    const SMBUrl completeDestination;

    // The offest to resume from in the destination. Naturally only should be used when resuming is true.
    const off_t destinationOffset = -1;
};

// Simple encapsulation for SMB resume IO for use with shouldResume.
// This hides the specific IO concern from the resume logic such that it can be used with either SMB IO or local IO.
class SMBResumeIO
{
public:
    explicit SMBResumeIO(const SMBUrl &url)
        : m_url(url)
    {
    }

    bool exists() const
    {
        return statResult() == 0;
    }

    off_t size() const
    {
        statResult();
        return m_stat.st_size;
    }

    bool isDir() const
    {
        statResult();
        return S_ISDIR(m_stat.st_mode);
    }

    bool remove()
    {
        return smbc_unlink(m_url.toSmbcUrl());
    }

    // smbc_rename() replaces a file that already has the new name.
    bool renameTo(const SMBUrl &newUrl)
    {
        if (smbc_rename(m_url.toSmbcUrl(), newUrl.toSmbcUrl()) < 0) {
            qCDebug(KIO_SMB_LOG) << "SMB failed to rename" << m_url << "to" << newUrl << "->" << strerror(errno);
            return false;
        }
        return true;
    }

private:
    // The stat is a request to the server, so it is made on first use: renameTo() and remove() do not need it.
    int statResult() const
    {
        if (!m_statResult) {
            m_statResult = SMBWorker::cache_stat(m_url, &m_stat);
        }
        return *m_statResult;
    }

    const SMBUrl m_url;
    mutable struct stat m_stat = {};
    mutable std::optional<int> m_statResult;
};

// Simple encapsulation for local resume IO for use with shouldResume.
// This hides the specific IO concern from the resume logic such that it can be used with either SMB IO or local IO.
class QFileResumeIO : public QFileInfo
{
public:
    explicit QFileResumeIO(const SMBUrl &url)
        : QFileInfo(url.path())
    {
    }

    bool remove()
    {
        return QFile::remove(filePath());
    }

    bool renameTo(const SMBUrl &newUrl)
    {
        QFile::remove(newUrl.path());
        if (!QFile::rename(filePath(), newUrl.path())) {
            qCDebug(KIO_SMB_LOG) << "failed to rename" << filePath() << "to" << newUrl.path();
            return false;
        }
        return true;
    }

private:
    const SMBUrl m_url;
};

namespace Transfer
{

// Check if we should resume the upload to destination.
// This returns nullopt when an error has ocurred. The error() function is called internally.
// NB: WorkerInterface is intentionally duck-typed so we can unit test with a mock entity that looks like a WorkerBase but isn't one.
//     Similarly ResumeIO is duck-typed so we can use QFileInfo as as base class in one implementation but not the other,
//     allowing us to cut down on boilerplate call-forwarding code.
template<typename ResumeIO, typename WorkerInterface>
Q_REQUIRED_RESULT std::variant<TransferContext, WorkerResult> shouldResume(const SMBUrl &destination, KIO::JobFlags flags, WorkerInterface *worker)
{
    // Resumption has two presentations:
    // a) partial resumption - when a .part file is left behind and we pick up where that part left off
    // b) in-place resumption - when we are expected to append to the actual destination file without
    //   .part temporary in between (FIXME behavior is largely unclear and the below logic is possibly not correct
    //   https://invent.kde.org/frameworks/kio/-/issues/9)
    const bool markPartial = worker->configValue(QStringLiteral("MarkPartial"), true);

    if (const ResumeIO destIO(destination); destIO.exists()) {
        if (const bool resume = static_cast<bool>(flags & KIO::Resume); resume && destIO.exists()) {
            // We are resuming the destination file directly!
            return TransferContext{resume, destination, destination, destination, destIO.size()};
        }

        // Not a resume operation -> if we also were not told to overwrite then we can't process this copy at all
        // because the ultimate destination already exists.
        if (!(flags & KIO::Overwrite)) {
            return WorkerResult::fail(destIO.isDir() ? KIO::ERR_IS_DIRECTORY : KIO::ERR_FILE_ALREADY_EXIST, destination.toDisplayString());
        }
    }

    if (markPartial) {
        const SMBUrl partUrl = destination.partUrl();
        if (ResumeIO partIO(partUrl); partIO.exists() && worker->canResume(partIO.size())) {
            return TransferContext{true, partUrl, partUrl, destination, partIO.size()};
        }

        return TransferContext{false, partUrl, partUrl, destination}; // new part file without offsets or resume
    }

    // The part file is not enabled or present, neither is KIO::Resume enabled and the dest file present -> regular
    // transfer without resuming of anything.
    return TransferContext{false, destination, QUrl(), destination};
}

// Concludes the resuming. This ought to be called after writing to the destination has
// completed. Destination should be closed. isError is the potential error state. When isError is true,
// the partial file may get discarded (depending on it existing and having an insufficient size).
// The return value is true when an error has occurred. When isError was true this can only ever return true.
template<typename ResumeIO, typename WorkerInterface>
Q_REQUIRED_RESULT WorkerResult concludeResumeHasError(const WorkerResult &result, const TransferContext &resume, WorkerInterface *worker)
{
    qCDebug(KIO_SMB_LOG) << "concluding transfer to" << resume.destination << "part file" << resume.partDestination << "final destination"
                         << resume.completeDestination;

    if (resume.destination == resume.completeDestination) {
        return result;
    }

    // Handle error condition.
    if (!result.success()) {
        // TODO should this be partdestination?
        if (ResumeIO destIO(resume.destination); destIO.exists() && destIO.size() < DEFAULT_MINIMUM_KEEP_SIZE) {
            destIO.remove();
        }
        return result;
    }

    // Rename partial file to its original name. The ResumeIO takes care of potential removing of the destination.
    if (ResumeIO partIO(resume.partDestination); !partIO.renameTo(resume.completeDestination)) {
        return WorkerResult::fail(ERR_CANNOT_RENAME_PARTIAL, resume.partDestination.toDisplayString());
    }

    return result;
}

} // namespace Transfer
