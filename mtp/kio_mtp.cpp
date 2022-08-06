/*
 *  Main implementation for KIO-MTP
 *  SPDX-FileCopyrightText: 2012 Philipp Schmidt <philschmidt@gmx.net>
 *  SPDX-FileCopyrightText: 2018 Andreas Krutzler <andreas.krutzler@gmx.net>
 *  SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kio_mtp.h"
#include "kio_mtp_debug.h"

// #include <KComponentData>
#include <QTemporaryFile>
#include <QFileInfo>
#include <QDateTime>
#include <QCoreApplication>
#include <QTimer>

#include <sys/types.h>
#include <utime.h>

#include "kmtpdeviceinterface.h"
#include "kmtpstorageinterface.h"

// Pseudo plugin class to embed meta data
class KIOPluginForMetaData : public QObject
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kde.kio.worker.mtp" FILE "mtp.json")
};

static UDSEntry getEntry(const KMTPDeviceInterface *device)
{
    UDSEntry entry;
    entry.reserve(5);
    entry.fastInsert(UDSEntry::UDS_NAME, device->friendlyName());
    entry.fastInsert(UDSEntry::UDS_ICON_NAME, QStringLiteral("multimedia-player"));
    entry.fastInsert(UDSEntry::UDS_FILE_TYPE, S_IFDIR);
    entry.fastInsert(UDSEntry::UDS_ACCESS, S_IRUSR | S_IRGRP | S_IROTH | S_IXUSR | S_IXGRP | S_IXOTH);
    entry.fastInsert(UDSEntry::UDS_MIME_TYPE, QStringLiteral("inode/directory"));
    return entry;
}

static UDSEntry getEntry(const KMTPStorageInterface *storage)
{
    UDSEntry entry;
    entry.reserve(5);
    entry.fastInsert(UDSEntry::UDS_NAME, storage->description());
    entry.fastInsert(UDSEntry::UDS_ICON_NAME, QStringLiteral("drive-removable-media"));
    entry.fastInsert(UDSEntry::UDS_FILE_TYPE, S_IFDIR);
    entry.fastInsert(UDSEntry::UDS_ACCESS, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    entry.fastInsert(UDSEntry::UDS_MIME_TYPE, QStringLiteral("inode/directory"));
    return entry;
}

static UDSEntry getEntry(const KMTPFile &file)
{
    UDSEntry entry;
    entry.reserve(9);
    entry.fastInsert(UDSEntry::UDS_NAME, file.filename());
    if (file.isFolder()) {
        entry.fastInsert(UDSEntry::UDS_FILE_TYPE, S_IFDIR);
        entry.fastInsert(UDSEntry::UDS_ACCESS, S_IRWXU | S_IRWXG | S_IRWXO);
    } else {
        entry.fastInsert(UDSEntry::UDS_FILE_TYPE, S_IFREG);
        entry.fastInsert(UDSEntry::UDS_ACCESS, S_IRUSR | S_IRGRP | S_IROTH | S_IXUSR | S_IXGRP | S_IXOTH);
        entry.fastInsert(UDSEntry::UDS_SIZE, file.filesize());
    }
    entry.fastInsert(UDSEntry::UDS_MIME_TYPE, file.filetype());
    entry.fastInsert(UDSEntry::UDS_INODE, file.itemId());
    entry.fastInsert(UDSEntry::UDS_ACCESS_TIME, file.modificationdate());
    entry.fastInsert(UDSEntry::UDS_MODIFICATION_TIME, file.modificationdate());
    entry.fastInsert(UDSEntry::UDS_CREATION_TIME, file.modificationdate());
    return entry;
}

static QString urlDirectory(const QUrl &url, bool appendTrailingSlash = false)
{
    if (!appendTrailingSlash) {
        return url.adjusted(QUrl::StripTrailingSlash | QUrl::RemoveFilename).path();
    }
    return url.adjusted(QUrl::RemoveFilename).path();
}

static QString urlFileName(const QUrl &url)
{
    return url.fileName();
}

static QString convertPath(const QString &workerPath)
{
    return workerPath.section(QLatin1Char('/'), 3, -1, QString::SectionIncludeLeadingSep);
}

//////////////////////////////////////////////////////////////////////////////
///////////////////////////// Worker Implementation ///////////////////////////
//////////////////////////////////////////////////////////////////////////////

extern "C"
int Q_DECL_EXPORT kdemain(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(QLatin1String("kio_mtp"));

    if (argc != 4) {
        fprintf(stderr, "Usage: kio_mtp protocol domain-socket1 domain-socket2\n");
        exit(-1);
    }

    MTPWorker worker(argv[2], argv[3]);

    worker.dispatchLoop();

    qCDebug(LOG_KIO_MTP) << "Worker EventLoop ended";

    return 0;
}

MTPWorker::MTPWorker(const QByteArray &pool, const QByteArray &app)
    : WorkerBase("mtp", pool, app)
{
    qCDebug(LOG_KIO_MTP) << "Worker started";
    qCDebug(LOG_KIO_MTP) << "Connected to kiod5 module:" << m_kmtpDaemon.isValid();
}

MTPWorker::~MTPWorker()
{
    qCDebug(LOG_KIO_MTP) << "Worker destroyed";
}

enum MTPWorker::Url MTPWorker::checkUrl(const QUrl &url)
{
    if (url.path().startsWith(QLatin1String("udi="))) {
        const QString udi = url.adjusted(QUrl::StripTrailingSlash).path().remove(0, 4);

        qCDebug(LOG_KIO_MTP) << "udi = " << udi;

        const KMTPDeviceInterface *device = m_kmtpDaemon.deviceFromUdi(udi);
        if (!device) {
            return Url::NotFound;
        }

        QUrl newUrl;
        newUrl.setScheme(QStringLiteral("mtp"));
        newUrl.setPath(QLatin1Char('/') + device->friendlyName());
        redirection(newUrl);

        return Url::Redirected;
    }
    if (url.path().startsWith(QLatin1Char('/'))) {
        return Url::Valid;
    } else if (url.scheme() == QLatin1String("mtp") && url.path().isEmpty()) {
        QUrl newUrl = url;
        newUrl.setPath(QLatin1String("/"));
        redirection(newUrl);

        return Url::Redirected;
    }
    return Url::Invalid;
}

WorkerResult MTPWorker::listDir(const QUrl &url)
{
    switch (checkUrl(url)) {
    case Url::Valid:
        break;
    case Url::Redirected:
        return WorkerResult::pass();
    case Url::NotFound:
        return WorkerResult::fail(ERR_DOES_NOT_EXIST, url.path());
    case Url::Invalid:
        return WorkerResult::fail(ERR_MALFORMED_URL, url.path());
    }

    // list '.' entry, otherwise files cannot be pasted to empty folders
    KIO::UDSEntry entry;
    entry.reserve(4);
    entry.fastInsert(KIO::UDSEntry::UDS_NAME, QStringLiteral("."));
    entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
    entry.fastInsert(KIO::UDSEntry::UDS_SIZE, 0);
    entry.fastInsert(KIO::UDSEntry::UDS_ACCESS, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IXOTH);
    listEntry(entry);

    const QStringList pathItems = url.path().split(QLatin1Char('/'), Qt::SkipEmptyParts);
    // list devices
    if (pathItems.isEmpty()) {
        qCDebug(LOG_KIO_MTP) << "Root directory, listing devices";

        const auto devices = m_kmtpDaemon.devices();
        totalSize(filesize_t(devices.size()));

        for (const KMTPDeviceInterface *device : devices) {
            listEntry(getEntry(device));
        }

        qCDebug(LOG_KIO_MTP) << "[SUCCESS] :: Devices:" << devices.size();
        return WorkerResult::pass();
    }

    // traverse into device
    KMTPDeviceInterface *mtpDevice = m_kmtpDaemon.deviceFromName(pathItems.first());
    if (mtpDevice) {
        // list storage media
        if (pathItems.size() == 1) {
            qCDebug(LOG_KIO_MTP) << "Listing storage media for device " << pathItems.first();

            const auto storages = mtpDevice->storages();
            totalSize(filesize_t(storages.size()));

            if (storages.count() > 0) {
                for (KMTPStorageInterface *storage : storages) {
                    listEntry(getEntry(storage));
                }

                qCDebug(LOG_KIO_MTP) << "[SUCCESS] :: Storage media:" << storages.count();
                return WorkerResult::pass();
            }
            return WorkerResult::fail(ERR_WORKER_DEFINED, i18nc("Message shown when attempting to access an MTP device that is not fully accessible yet", "Could not access device. Make sure it is unlocked, and tap \"Allow\" on the popup on its screen. If that does not work, make sure MTP is enabled in its USB connection settings."));
        }
        // list files and folders
        const KMTPStorageInterface *storage = mtpDevice->storageFromDescription(pathItems.at(1));
        if (storage) {
            int result;
            const QString path = convertPath(url.path());
            const KMTPFileList files = storage->getFilesAndFolders(path, result);

            switch (result) {
            case 0:
                for (const KMTPFile &file : files) {
                    listEntry(getEntry(file));
                }

                qCDebug(LOG_KIO_MTP) << "[SUCCESS] :: Files:" << files.count();
                return WorkerResult::pass();
            case 2:
                return WorkerResult::fail(ERR_IS_FILE, url.path());
            }

            // path not found
            return WorkerResult::fail(ERR_CANNOT_ENTER_DIRECTORY, url.path());
        }
        // storage not found
        qCDebug(LOG_KIO_MTP) << "[ERROR] :: Storage";
        return WorkerResult::fail(ERR_CANNOT_ENTER_DIRECTORY, url.path());
    }
    // device not found
    qCDebug(LOG_KIO_MTP) << "[ERROR] :: Device";
    return WorkerResult::fail(ERR_CANNOT_ENTER_DIRECTORY, url.path());
}

WorkerResult MTPWorker::stat(const QUrl &url)
{
    switch (checkUrl(url)) {
    case Url::Valid:
        break;
    case Url::Redirected:
        return WorkerResult::pass();
    case Url::NotFound:
        return WorkerResult::fail(ERR_DOES_NOT_EXIST, url.path());
    case Url::Invalid:
        return WorkerResult::fail(ERR_MALFORMED_URL, url.path());
    }

    const QStringList pathItems = url.path().split(QLatin1Char('/'), Qt::SkipEmptyParts);
    UDSEntry entry;
    // root
    if (pathItems.size() < 1) {
        entry.reserve(4);
        entry.fastInsert(UDSEntry::UDS_NAME, QStringLiteral("mtp:///"));
        entry.fastInsert(UDSEntry::UDS_FILE_TYPE, S_IFDIR);
        entry.fastInsert(UDSEntry::UDS_ACCESS, S_IRUSR | S_IRGRP | S_IROTH | S_IXUSR | S_IXGRP | S_IXOTH);
        entry.fastInsert(UDSEntry::UDS_MIME_TYPE, QStringLiteral("inode/directory"));
    } else {
        const KMTPDeviceInterface *mtpDevice = m_kmtpDaemon.deviceFromName(pathItems.first());
        if (mtpDevice) {

            // device
            if (pathItems.size() < 2) {
                entry = getEntry(mtpDevice);
            } else {
                const KMTPStorageInterface *storage = mtpDevice->storageFromDescription(pathItems.at(1));
                if (storage) {

                    // storage
                    if (pathItems.size() < 3) {
                        entry = getEntry(storage);
                    }
                    // folder/file
                    else {
                        const KMTPFile file = storage->getFileMetadata(convertPath(url.path()));
                        if (file.isValid()) {
                            entry = getEntry(file);
                        } else {
                            return WorkerResult::fail(ERR_DOES_NOT_EXIST, url.path());
                        }
                    }
                } else {
                    return WorkerResult::fail(ERR_DOES_NOT_EXIST, url.path());
                }
            }
        } else {
            return WorkerResult::fail(ERR_DOES_NOT_EXIST, url.path());
        }
    }

    statEntry(entry);
    return WorkerResult::pass();
}

WorkerResult MTPWorker::mimetype(const QUrl &url)
{
    switch (checkUrl(url)) {
    case Url::Valid:
        break;
    case Url::Redirected:
        return WorkerResult::pass();
    case Url::NotFound:
        return WorkerResult::fail(ERR_DOES_NOT_EXIST, url.path());
    case Url::Invalid:
        return WorkerResult::fail(ERR_MALFORMED_URL, url.path());
    }

    const QStringList pathItems = url.path().split(QLatin1Char('/'), Qt::SkipEmptyParts);
    if (pathItems.size() > 2) {
        const KMTPDeviceInterface *mtpDevice = m_kmtpDaemon.deviceFromName(pathItems.first());
        if (mtpDevice) {
            const KMTPStorageInterface *storage = mtpDevice->storageFromDescription(pathItems.at(1));
            if (storage) {
                const KMTPFile file = storage->getFileMetadata(convertPath(url.path()));
                if (file.isValid()) {
                    // NOTE the difference between calling mimetype and mimeType
                    mimeType(file.filetype());
                    return WorkerResult::pass();
                }
            }
        }
    } else {
        mimeType(QStringLiteral("inode/directory"));
        return WorkerResult::pass();
    }

    return WorkerResult::fail(ERR_DOES_NOT_EXIST, url.path());
}

WorkerResult MTPWorker::get(const QUrl &url)
{
    switch (checkUrl(url)) {
    case Url::Valid:
        break;
    case Url::Redirected:
    case Url::NotFound:
    case Url::Invalid:
        return WorkerResult::fail(ERR_MALFORMED_URL, url.path());
    }

    const QStringList pathItems = url.path().split(QLatin1Char('/'), Qt::SkipEmptyParts);

    // file
    if (pathItems.size() > 2) {

        const KMTPDeviceInterface *mtpDevice = m_kmtpDaemon.deviceFromName(pathItems.first());
        if (mtpDevice) {
            const KMTPStorageInterface *storage = mtpDevice->storageFromDescription(pathItems.at(1));
            if (storage) {
                const QString path = convertPath(url.path());
                const KMTPFile source = storage->getFileMetadata(path);
                if (!source.isValid()) {
                    return WorkerResult::fail(KIO::ERR_DOES_NOT_EXIST, url.path());
                }

                mimeType(source.filetype());
                totalSize(source.filesize());

                int result = storage->getFileToHandler(path);
                if (result) {
                    return WorkerResult::fail(KIO::ERR_CANNOT_READ, url.path());
                }

                QEventLoop loop;
                connect(storage, &KMTPStorageInterface::dataReady, &loop, [this] (const QByteArray &data) {
                    MTPWorker::data(data);
                });
                connect(storage, &KMTPStorageInterface::copyFinished, &loop, &QEventLoop::exit);
                result = loop.exec();

                qCDebug(LOG_KIO_MTP) << "data received";

                if (result) {
                    return WorkerResult::fail(ERR_CANNOT_READ, url.path());
                }

                data(QByteArray());
                return WorkerResult::pass();
            }
        }
    } else {
        return WorkerResult::fail(ERR_UNSUPPORTED_ACTION, url.path());
    }
    return WorkerResult::fail(ERR_CANNOT_READ, url.path());
}

WorkerResult MTPWorker::put(const QUrl &url, int, JobFlags flags)
{
    switch (checkUrl(url)) {
    case Url::Valid:
        break;
    case Url::Redirected:
    case Url::NotFound:
    case Url::Invalid:
        return WorkerResult::fail(ERR_MALFORMED_URL, url.path());
    }

    const QStringList destItems = url.path().split(QLatin1Char('/'), Qt::SkipEmptyParts);

    // can't copy to root or device, needs storage
    if (destItems.size() < 2) {
        return WorkerResult::fail(ERR_UNSUPPORTED_ACTION, url.path());
    }

    // we need to get the entire file first, then we can upload
    qCDebug(LOG_KIO_MTP) << "use temp file";

    QTemporaryFile temp;
    if (temp.open()) {
        QByteArray buffer;
        int len = 0;

        do {
            dataReq();
            len = readData(buffer);
            temp.write(buffer);
        } while (len > 0);

        const KMTPDeviceInterface *mtpDevice = m_kmtpDaemon.deviceFromName(destItems.first());
        if (mtpDevice) {
            const KMTPStorageInterface *storage = mtpDevice->storageFromDescription(destItems.at(1));
            if (storage) {
                const QString destinationPath = convertPath(url.path());

                // check if the file already exists on the device
                const KMTPFile destinationFile = storage->getFileMetadata(destinationPath);
                if (destinationFile.isValid()) {
                    if (flags & KIO::Overwrite) {
                        // delete existing file on the device
                        const int result = storage->deleteObject(destinationPath);
                        if (result) {
                            return WorkerResult::fail(ERR_CANNOT_DELETE, url.path());
                        }
                    } else {
                        return WorkerResult::fail(ERR_FILE_ALREADY_EXIST, url.path());
                    }
                }

                totalSize(quint64(temp.size()));

                QDBusUnixFileDescriptor descriptor(temp.handle());
                int result = storage->sendFileFromFileDescriptor(descriptor, destinationPath);
                if (result) {
                    return WorkerResult::fail(KIO::ERR_CANNOT_WRITE, urlFileName(url));
                }

                result = waitForCopyOperation(storage);
                processedSize(quint64(temp.size()));
                temp.close();

                switch (result) {
                case 0:
                    qCDebug(LOG_KIO_MTP) << "data sent";
                    return WorkerResult::pass();
                case 2:
                    return WorkerResult::fail(ERR_IS_FILE, urlDirectory(url));
                }

                return WorkerResult::fail(KIO::ERR_CANNOT_WRITE, urlFileName(url));
            }
        }
    }

    return WorkerResult::fail(KIO::ERR_CANNOT_WRITE, urlFileName(url));
}

WorkerResult MTPWorker::copy(const QUrl &src, const QUrl &dest, int, JobFlags flags)
{
    if (src.scheme() == QLatin1String("mtp") && dest.scheme() == QLatin1String("mtp")) {
        qCDebug(LOG_KIO_MTP) << "Copy on device: Not supported";
        // MTP doesn't support moving files directly on the device, so we have to download and then upload...

        return WorkerResult::fail(ERR_UNSUPPORTED_ACTION, i18n("Cannot copy/move files on the device itself"));
    } else if (src.scheme() == QLatin1String("file") && dest.scheme() == QLatin1String("mtp")) {
        // copy from filesystem to the device

        switch (checkUrl(dest)) {
        case Url::Valid:
            break;
        case Url::NotFound:
            return WorkerResult::fail(ERR_DOES_NOT_EXIST, src.path());
        case Url::Redirected:
        case Url::Invalid:
            return WorkerResult::fail(ERR_MALFORMED_URL, dest.path());
        }

        QStringList destItems = dest.path().split(QLatin1Char('/'), Qt::SkipEmptyParts);

        // can't copy to root or device, needs storage
        if (destItems.size() < 2) {
            return WorkerResult::fail(ERR_UNSUPPORTED_ACTION, dest.path());
        }

        qCDebug(LOG_KIO_MTP) << "Copy file " << urlFileName(src) << "from filesystem to device" << urlDirectory(src, true) << urlDirectory(dest, true);

        const KMTPDeviceInterface *mtpDevice = m_kmtpDaemon.deviceFromName(destItems.first());
        if (mtpDevice) {
            const KMTPStorageInterface *storage = mtpDevice->storageFromDescription(destItems.at(1));
            if (storage) {
                const QString destinationPath = convertPath(dest.path());

                // check if the file already exists on the device
                const KMTPFile destinationFile = storage->getFileMetadata(destinationPath);
                if (destinationFile.isValid()) {
                    if (flags & KIO::Overwrite) {
                        // delete existing file on the device
                        const int result = storage->deleteObject(destinationPath);
                        if (result) {
                            return WorkerResult::fail(ERR_CANNOT_DELETE, dest.path());
                        }
                    } else {
                        return WorkerResult::fail(ERR_FILE_ALREADY_EXIST, dest.path());
                    }
                }

                QFile srcFile(src.path());
                if (!srcFile.open(QIODevice::ReadOnly)) {
                    return WorkerResult::fail(KIO::ERR_CANNOT_OPEN_FOR_READING, src.path());
                }

                qCDebug(LOG_KIO_MTP) << "Sending file" << srcFile.fileName() << "with size" << srcFile.size();

                totalSize(quint64(srcFile.size()));

                QDBusUnixFileDescriptor descriptor(srcFile.handle());
                int result = storage->sendFileFromFileDescriptor(descriptor, destinationPath);
                if (result) {
                    return WorkerResult::fail(KIO::ERR_CANNOT_WRITE, urlFileName(dest));
                }

                result = waitForCopyOperation(storage);
                processedSize(quint64(srcFile.size()));
                srcFile.close();

                if (result) {
                    return WorkerResult::fail(KIO::ERR_CANNOT_WRITE, urlFileName(dest));
                }

                qCDebug(LOG_KIO_MTP) << "Sent file";
                return WorkerResult::pass();
            }
        }
        return WorkerResult::fail(KIO::ERR_CANNOT_WRITE, urlFileName(src));
    } else if (src.scheme() == QLatin1String("mtp") && dest.scheme() == QLatin1String("file")) {
        // copy from the device to filesystem

        switch (checkUrl(src)) {
        case Url::Valid:
            break;
        case Url::NotFound:
            return WorkerResult::fail(ERR_DOES_NOT_EXIST, src.toDisplayString());
        case Url::Redirected:
        case Url::Invalid:
            return WorkerResult::fail(ERR_MALFORMED_URL, src.toDisplayString());
        }

        qCDebug(LOG_KIO_MTP) << "Copy file" << urlFileName(src) << "from device to filesystem" << urlDirectory(src, true) << urlDirectory(dest, true);

        QFileInfo destination(dest.path());

        if (!(flags & KIO::Overwrite) && destination.exists()) {
            return WorkerResult::fail(ERR_FILE_ALREADY_EXIST, dest.path());
        }

        const QStringList srcItems = src.path().split(QLatin1Char('/'), Qt::SkipEmptyParts);

        // can't copy to root or device, needs storage
        if (srcItems.size() < 2) {
            return WorkerResult::fail(ERR_UNSUPPORTED_ACTION, src.path());
        }

        const KMTPDeviceInterface *mtpDevice = m_kmtpDaemon.deviceFromName(srcItems.first());
        if (mtpDevice) {
            const KMTPStorageInterface *storage = mtpDevice->storageFromDescription(srcItems.at(1));
            if (storage) {

                QFile destFile(dest.path());
                if (!destFile.open(QIODevice::Truncate | QIODevice::WriteOnly)) {
                    return WorkerResult::fail(KIO::ERR_WRITE_ACCESS_DENIED, dest.path());
                }

                const KMTPFile source = storage->getFileMetadata(convertPath(src.path()));
                if (!source.isValid()) {
                    return WorkerResult::fail(KIO::ERR_DOES_NOT_EXIST, src.path());
                }

                totalSize(source.filesize());

                QDBusUnixFileDescriptor descriptor(destFile.handle());
                int result = storage->getFileToFileDescriptor(descriptor, convertPath(src.path()));
                if (result) {
                    return WorkerResult::fail(KIO::ERR_CANNOT_READ, urlFileName(src));
                }

                result = waitForCopyOperation(storage);
                processedSize(quint64(source.filesize()));
                destFile.close();

                if (result) {
                    return WorkerResult::fail(KIO::ERR_CANNOT_READ, urlFileName(src));
                }

                // set correct modification time
                struct ::utimbuf times;
                times.actime = QDateTime::currentDateTime().toSecsSinceEpoch();
                times.modtime = source.modificationdate();

                ::utime(dest.path().toUtf8().data(), &times);

                qCDebug(LOG_KIO_MTP) << "Received file";
                return WorkerResult::pass();
            }
        }
        return WorkerResult::fail(KIO::ERR_CANNOT_READ, urlFileName(src));
    }
    return WorkerResult::fail(ERR_UNSUPPORTED_ACTION, {});
}

WorkerResult MTPWorker::mkdir(const QUrl &url, int)
{
    switch (checkUrl(url)) {
    case Url::Valid:
        break;
    case Url::Redirected:
    case Url::NotFound:
    case Url::Invalid:
        return WorkerResult::fail(ERR_MALFORMED_URL, url.path());
    }

    const QStringList pathItems = url.path().split(QLatin1Char('/'), Qt::SkipEmptyParts);
    if (pathItems.size() > 2) {
        const KMTPDeviceInterface *mtpDevice = m_kmtpDaemon.deviceFromName(pathItems.first());
        if (mtpDevice) {
            const KMTPStorageInterface *storage = mtpDevice->storageFromDescription(pathItems.at(1));
            if (storage) {
                // TODO: folder already exists
                const quint32 itemId = storage->createFolder(convertPath(url.path()));
                if (itemId) {
                    return WorkerResult::pass();
                }
            }
        }
    }
    return WorkerResult::fail(ERR_CANNOT_MKDIR, url.path());
}

WorkerResult MTPWorker::del(const QUrl &url, bool)
{
    switch (checkUrl(url)) {
    case Url::Valid:
        break;
    case Url::Redirected:
    case Url::NotFound:
    case Url::Invalid:
        return WorkerResult::fail(ERR_MALFORMED_URL, url.path());
    }

    const QStringList pathItems = url.path().split(QLatin1Char('/'), Qt::SkipEmptyParts);
    if (pathItems.size() >= 2) {
        const KMTPDeviceInterface *mtpDevice = m_kmtpDaemon.deviceFromName(pathItems.first());
        if (mtpDevice) {
            const KMTPStorageInterface *storage = mtpDevice->storageFromDescription(pathItems.at(1));
            if (storage) {
                const int result = storage->deleteObject(convertPath(url.path()));
                if (!result) {
                    return WorkerResult::pass();
                }
            }
        }
    }
    return WorkerResult::fail(ERR_CANNOT_DELETE, url.path());
}

WorkerResult MTPWorker::rename(const QUrl &src, const QUrl &dest, JobFlags flags)
{
    switch (checkUrl(src)) {
    case Url::Valid:
        break;
    case Url::Redirected:
    case Url::NotFound:
    case Url::Invalid:
        return WorkerResult::fail(ERR_MALFORMED_URL, src.path());
    }

    switch (checkUrl(dest)) {
    case Url::Valid:
        break;
    case Url::Redirected:
    case Url::NotFound:
    case Url::Invalid:
        return WorkerResult::fail(ERR_MALFORMED_URL, dest.path());
    }

    if (src.scheme() != QLatin1String("mtp")) {
        // Kate: when editing files directly on the device and the user wants to save the changes,
        // Kate tries to move the file from /tmp/xxx to the MTP device which is not supported.
        // The ERR_UNSUPPORTED_ACTION error tells Kate to copy the file instead of moving.
        return WorkerResult::fail(ERR_UNSUPPORTED_ACTION, src.path());
    }

    const QStringList srcItems = src.path().split(QLatin1Char('/'), Qt::SkipEmptyParts);
    KMTPDeviceInterface *mtpDevice = m_kmtpDaemon.deviceFromName(srcItems.first());
    if (mtpDevice) {
        // rename Device
        if (srcItems.size() == 1) {
            const int result = mtpDevice->setFriendlyName(urlFileName(dest));
            if (!result) {
                return WorkerResult::pass();
            }
        }
        // rename Storage
        else if (srcItems.size() == 2) {
            return WorkerResult::fail(ERR_CANNOT_RENAME, src.path());
        } else {
            // rename file or folder
            const KMTPStorageInterface *storage = mtpDevice->storageFromDescription(srcItems.at(1));
            if (storage) {

                // check if the file already exists on the device
                const QString destinationPath = convertPath(dest.path());
                const KMTPFile destinationFile = storage->getFileMetadata(destinationPath);
                if (destinationFile.isValid()) {
                    if (flags & KIO::Overwrite) {
                        // delete existing file on the device
                        const int result = storage->deleteObject(destinationPath);
                        if (result) {
                            return WorkerResult::fail(ERR_CANNOT_DELETE, dest.path());
                        }
                    } else {
                        return WorkerResult::fail(ERR_FILE_ALREADY_EXIST, dest.path());
                    }
                }

                const int result = storage->setFileName(convertPath(src.path()), dest.fileName());
                if (!result) {
                    return WorkerResult::pass();
                }
            }
        }
    }
    return WorkerResult::fail(ERR_CANNOT_RENAME, src.path());
}

WorkerResult MTPWorker::fileSystemFreeSpace(const QUrl &url)
{
    qCDebug(LOG_KIO_MTP) << "fileSystemFreeSpace:" << url;

    switch (checkUrl(url)) {
    case Url::Valid:
        break;
    case Url::Redirected:
        return WorkerResult::pass();
    case Url::NotFound:
        return WorkerResult::fail(ERR_DOES_NOT_EXIST, url.path());
    case Url::Invalid:
        return WorkerResult::fail(ERR_MALFORMED_URL, url.path());
    }

    const QStringList pathItems = url.path().split(QLatin1Char('/'), Qt::SkipEmptyParts);

    // Storage
    if (pathItems.size() > 1) {
        const KMTPDeviceInterface *mtpDevice = m_kmtpDaemon.deviceFromName(pathItems.first());
        if (mtpDevice) {
            const KMTPStorageInterface *storage = mtpDevice->storageFromDescription(pathItems.at(1));
            if (storage) {
                setMetaData(QStringLiteral("total"), QString::number(storage->maxCapacity()));
                setMetaData(QStringLiteral("available"), QString::number(storage->freeSpaceInBytes()));
                return WorkerResult::pass();
            }
        }
    }
    return WorkerResult::fail(KIO::ERR_CANNOT_STAT, url.toDisplayString());
}

int MTPWorker::waitForCopyOperation(const KMTPStorageInterface *storage)
{
    QEventLoop loop;
    connect(storage, &KMTPStorageInterface::copyProgress, &loop, [this] (qulonglong sent, qulonglong total) {
        Q_UNUSED(total)
        processedSize(sent);
    });

    // any chance to 'miss' the copyFinished signal and dead lock the worker?
    connect(storage, &KMTPStorageInterface::copyFinished, &loop, &QEventLoop::exit);
    return loop.exec();
}

#include "kio_mtp.moc"
