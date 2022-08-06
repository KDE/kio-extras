/*
 *  Main implementation for KIO-MTP
 *  SPDX-FileCopyrightText: 2012 Philipp Schmidt <philschmidt@gmx.net>
 *  SPDX-FileCopyrightText: 2018 Andreas Krutzler <andreas.krutzler@gmx.net>
 *  SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <KIO/Global>
#include <KIO/WorkerBase>
#include <KLocalizedString>

#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>


// #include <QCache>

#include <QUrl>

#define MAX_XFER_BUF_SIZE 16348
#define KIO_MTP 7000
#include <kmtpdinterface.h>

class KMTPDeviceInterface;
class KMTPStorageInterface;
class KMTPFile;

using namespace KIO;

class MTPWorker : public QObject, public KIO::WorkerBase
{
    Q_OBJECT

    enum class Url {
        Invalid = -1,
        Valid = 0,
        Redirected = 1,
        NotFound = 2,
    };

public:
    /*
     * Overwritten KIO-functions, see "kio_mtp.cpp"
     */
    MTPWorker(const QByteArray &pool, const QByteArray &app);
    ~MTPWorker() override;

    WorkerResult listDir(const QUrl &url) override;
    WorkerResult stat(const QUrl &url) override;
    WorkerResult mimetype(const QUrl &url) override;
    WorkerResult get(const QUrl &url) override;
    WorkerResult put(const QUrl &url, int, JobFlags flags) override;
    WorkerResult copy(const QUrl &src, const QUrl &dest, int, JobFlags flags) override;
    WorkerResult mkdir(const QUrl &url, int) override;
    WorkerResult del(const QUrl &url, bool) override;
    WorkerResult rename(const QUrl &src, const QUrl &dest, JobFlags flags) override;
    WorkerResult fileSystemFreeSpace(const QUrl &url) override;

private:
    /**
     * Check if it is a valid url or an udi.
     *
     * @param url The url to checkUrl
     * @return enum MTPWorker::Url
     */
    enum MTPWorker::Url checkUrl(const QUrl &url);

    /**
     * @brief Waits for a pending copy operation to finish while updating its progress.
     * @param storage
     * @return The result of the operation, usually 0 if valid
     */
    int waitForCopyOperation(const KMTPStorageInterface *storage);

    KMTPDInterface m_kmtpDaemon;
};
