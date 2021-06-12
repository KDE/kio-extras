/*
 *  Main implementation for KIO-MTP
 *  SPDX-FileCopyrightText: 2012 Philipp Schmidt <philschmidt@gmx.net>
 *  SPDX-FileCopyrightText: 2018 Andreas Krutzler <andreas.krutzler@gmx.net>
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

#include <sys/stat.h>
#include <sys/types.h>
#include <utime.h>

#include "kmtpdeviceinterface.h"
#include "kmtpstorageinterface.h"

// Pseudo plugin class to embed meta data
class KIOPluginForMetaData : public QObject
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kde.kio.slave.mtp" FILE "mtp.json")
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

static QString convertPath(const QString &slavePath)
{
    return slavePath.section(QLatin1Char('/'), 3, -1, QString::SectionIncludeLeadingSep);
}

//////////////////////////////////////////////////////////////////////////////
///////////////////////////// Slave Implementation ///////////////////////////
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

    MTPSlave slave(argv[2], argv[3]);

    slave.dispatchLoop();

    qCDebug(LOG_KIO_MTP) << "Slave EventLoop ended";

    return 0;
}

MTPSlave::MTPSlave(const QByteArray &pool, const QByteArray &app)
    : SlaveBase("mtp", pool, app)
{
    qCDebug(LOG_KIO_MTP) << "Slave started";
    qCDebug(LOG_KIO_MTP) << "Connected to kiod5 module:" << m_kmtpDaemon.isValid();
}

MTPSlave::~MTPSlave()
{
    qCDebug(LOG_KIO_MTP) << "Slave destroyed";
}

int MTPSlave::checkUrl(const QUrl &url, bool redirect)
{
    if (url.path().startsWith(QLatin1String("udi=")) && redirect) {
        const QString udi = url.adjusted(QUrl::StripTrailingSlash).path().remove(0, 4);

        qCDebug(LOG_KIO_MTP) << "udi = " << udi;

        const KMTPDeviceInterface *device = m_kmtpDaemon.deviceFromUdi(udi);
        if (device) {
            QUrl newUrl;
            newUrl.setScheme(QStringLiteral("mtp"));
            newUrl.setPath(QLatin1Char('/') + device->friendlyName());
            redirection(newUrl);

            return 1;
        } else {
            return 2;
        }
    } else if (url.path().startsWith(QLatin1Char('/'))) {
        return 0;
    }
    return -1;
}

void MTPSlave::listDir(const QUrl &url)
{
    const int check = checkUrl(url);
    switch (check) {
    case 0:
        break;
    case 1:
        finished();
        return;
    case 2:
        error(ERR_DOES_NOT_EXIST, url.path());
        return;
    default:
        error(ERR_MALFORMED_URL, url.path());
        return;
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

        totalSize(filesize_t(m_kmtpDaemon.devices().size()));

        const auto devices = m_kmtpDaemon.devices();
        for (const KMTPDeviceInterface *device : devices) {
            listEntry(getEntry(device));
        }

        qCDebug(LOG_KIO_MTP) << "[SUCCESS] :: Devices:" << m_kmtpDaemon.devices().size();
        finished();
        return;
    }

    // traverse into device
    const KMTPDeviceInterface *mtpDevice = m_kmtpDaemon.deviceFromName(pathItems.first());
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

                finished();
                qCDebug(LOG_KIO_MTP) << "[SUCCESS] :: Storage media:" << storages.count();
            } else {
                error(ERR_SLAVE_DEFINED, i18n("No storage media found. Make sure your device is unlocked and has MTP enabled in its USB connection settings."));
            }
        } else {
            // list files and folders
            const KMTPStorageInterface *storage = mtpDevice->storageFromDescription(pathItems.at(1));
            if (storage) {
                int result;
                const QString path = convertPath(url.path());
                const KMTPFileList files = storage->getFilesAndFolders(path, result);

                switch (result) {
                case 0:
                    break;
                case 2:
                    error(ERR_IS_FILE, url.path());
                    return;
                default:
                    // path not found
                    error(ERR_CANNOT_ENTER_DIRECTORY, url.path());
                    return;
                }

                for (const KMTPFile &file : files) {
                    listEntry(getEntry(file));
                }

                finished();
                qCDebug(LOG_KIO_MTP) << "[SUCCESS] :: Files:" << files.count();
            } else {
                // storage not found
                error(ERR_CANNOT_ENTER_DIRECTORY, url.path());
                qCDebug(LOG_KIO_MTP) << "[ERROR] :: Storage";
            }
        }
    } else {
        // device not found
        error(ERR_CANNOT_ENTER_DIRECTORY, url.path());
        qCDebug(LOG_KIO_MTP) << "[ERROR] :: Device";
    }
}

void MTPSlave::stat(const QUrl &url)
{
    const int check = checkUrl(url);
    switch (check) {
    case 0:
        break;
    case 1:
        finished();
        return;
    case 2:
        error(ERR_DOES_NOT_EXIST, url.path());
        return;
    default:
        error(ERR_MALFORMED_URL, url.path());
        return;
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
                            error(ERR_DOES_NOT_EXIST, url.path());
                            return;
                        }
                    }
                } else {
                    error(ERR_DOES_NOT_EXIST, url.path());
                    return;
                }
            }
        } else {
            error(ERR_DOES_NOT_EXIST, url.path());
            return;
        }
    }

    statEntry(entry);
    finished();
}

void MTPSlave::mimetype(const QUrl &url)
{
    const int check = checkUrl(url);
    switch (check) {
    case 0:
        break;
    case 1:
        finished();
        return;
    case 2:
        error(ERR_DOES_NOT_EXIST, url.path());
        return;
    default:
        error(ERR_MALFORMED_URL, url.path());
        return;
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
                    return;
                }
            }
        }
    } else {
        mimeType(QStringLiteral("inode/directory"));
        return;
    }

    error(ERR_DOES_NOT_EXIST, url.path());
}

void MTPSlave::get(const QUrl &url)
{
    const int check = checkUrl(url);
    switch (check) {
    case 0:
        break;
    default:
        error(ERR_MALFORMED_URL, url.path());
        return;
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
                    error(KIO::ERR_DOES_NOT_EXIST, url.path());
                    return;
                }

                mimeType(source.filetype());
                totalSize(source.filesize());

                int result = storage->getFileToHandler(path);
                if (result) {
                    error(KIO::ERR_CANNOT_READ, url.path());
                    return;
                }

                QEventLoop loop;
                connect(storage, &KMTPStorageInterface::dataReady, &loop, [this] (const QByteArray &data) {
                    MTPSlave::data(data);
                });
                connect(storage, &KMTPStorageInterface::copyFinished, &loop, &QEventLoop::exit);
                result = loop.exec();

                qCDebug(LOG_KIO_MTP) << "data received";

                if (result) {
                    error(ERR_CANNOT_READ, url.path());
                    return;
                }

                data(QByteArray());
                finished();
                return;
            }
        }
    } else {
        error(ERR_UNSUPPORTED_ACTION, url.path());
        return;
    }
    error(ERR_CANNOT_READ, url.path());
}

void MTPSlave::put(const QUrl &url, int, JobFlags flags)
{
    const int check = checkUrl(url);
    switch (check) {
    case 0:
        break;
    default:
        error(ERR_MALFORMED_URL, url.path());
        return;
    }

    const QStringList destItems = url.path().split(QLatin1Char('/'), Qt::SkipEmptyParts);

    // can't copy to root or device, needs storage
    if (destItems.size() < 2) {
        error(ERR_UNSUPPORTED_ACTION, url.path());
        return;
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
                            error(ERR_CANNOT_DELETE, url.path());
                            return;
                        }
                    } else {
                        error(ERR_FILE_ALREADY_EXIST, url.path());
                        return;
                    }
                }

                totalSize(quint64(temp.size()));

                QDBusUnixFileDescriptor descriptor(temp.handle());
                int result = storage->sendFileFromFileDescriptor(descriptor, destinationPath);
                if (result) {
                    error(KIO::ERR_CANNOT_WRITE, urlFileName(url));
                    return;
                }

                result = waitForCopyOperation(storage);
                processedSize(quint64(temp.size()));
                temp.close();

                switch (result) {
                case 0:
                    break;
                case 2:
                    error(ERR_IS_FILE, urlDirectory(url));
                    return;
                default:
                    error(KIO::ERR_CANNOT_WRITE, urlFileName(url));
                    return;
                }

                qCDebug(LOG_KIO_MTP) << "data sent";
                finished();
                return;
            }
        }
    }

    error(KIO::ERR_CANNOT_WRITE, urlFileName(url));
}

void MTPSlave::copy(const QUrl &src, const QUrl &dest, int, JobFlags flags)
{
    if (src.scheme() == QLatin1String("mtp") && dest.scheme() == QLatin1String("mtp")) {
        qCDebug(LOG_KIO_MTP) << "Copy on device: Not supported";
        // MTP doesn't support moving files directly on the device, so we have to download and then upload...

        error(ERR_UNSUPPORTED_ACTION, i18n("Cannot copy/move files on the device itself"));
        return;
    } else if (src.scheme() == QLatin1String("file") && dest.scheme() == QLatin1String("mtp")) {
        // copy from filesystem to the device

        const int check = checkUrl(dest);
        switch (check) {
        case 0:
            break;
        default:
            error(ERR_MALFORMED_URL, dest.path());
            return;
        }

        QStringList destItems = dest.path().split(QLatin1Char('/'), Qt::SkipEmptyParts);

        // can't copy to root or device, needs storage
        if (destItems.size() < 2) {
            error(ERR_UNSUPPORTED_ACTION, dest.path());
            return;
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
                            error(ERR_CANNOT_DELETE, dest.path());
                            return;
                        }
                    } else {
                        error(ERR_FILE_ALREADY_EXIST, dest.path());
                        return;
                    }
                }

                QFile srcFile(src.path());
                if (!srcFile.open(QIODevice::ReadOnly)) {
                    error(KIO::ERR_CANNOT_OPEN_FOR_READING, src.path());
                    return;
                }

                qCDebug(LOG_KIO_MTP) << "Sending file" << srcFile.fileName() << "with size" << srcFile.size();

                totalSize(quint64(srcFile.size()));

                QDBusUnixFileDescriptor descriptor(srcFile.handle());
                int result = storage->sendFileFromFileDescriptor(descriptor, destinationPath);
                if (result) {
                    error(KIO::ERR_CANNOT_WRITE, urlFileName(dest));
                    return;
                }

                result = waitForCopyOperation(storage);
                processedSize(quint64(srcFile.size()));
                srcFile.close();

                if (result) {
                    error(KIO::ERR_CANNOT_WRITE, urlFileName(dest));
                    return;
                }

                qCDebug(LOG_KIO_MTP) << "Sent file";
                finished();
                return;
            }
        }
        error(KIO::ERR_CANNOT_WRITE, urlFileName(src));

    } else if (src.scheme() == QLatin1String("mtp") && dest.scheme() == QLatin1String("file")) {
        // copy from the device to filesystem

        const int check = checkUrl(src);
        switch (check) {
        case 0:
            break;
        default:
            error(ERR_MALFORMED_URL, src.path());
            return;
        }

        qCDebug(LOG_KIO_MTP) << "Copy file" << urlFileName(src) << "from device to filesystem" << urlDirectory(src, true) << urlDirectory(dest, true);

        QFileInfo destination(dest.path());

        if (!(flags & KIO::Overwrite) && destination.exists()) {
            error(ERR_FILE_ALREADY_EXIST, dest.path());
            return;
        }

        const QStringList srcItems = src.path().split(QLatin1Char('/'), Qt::SkipEmptyParts);

        // can't copy to root or device, needs storage
        if (srcItems.size() < 2) {
            error(ERR_UNSUPPORTED_ACTION, src.path());
            return;
        }

        const KMTPDeviceInterface *mtpDevice = m_kmtpDaemon.deviceFromName(srcItems.first());
        if (mtpDevice) {
            const KMTPStorageInterface *storage = mtpDevice->storageFromDescription(srcItems.at(1));
            if (storage) {

                QFile destFile(dest.path());
                if (!destFile.open(QIODevice::Truncate | QIODevice::WriteOnly)) {
                    error(KIO::ERR_WRITE_ACCESS_DENIED, dest.path());
                    return;
                }

                const KMTPFile source = storage->getFileMetadata(convertPath(src.path()));
                if (!source.isValid()) {
                    error(KIO::ERR_DOES_NOT_EXIST, src.path());
                    return;
                }

                totalSize(source.filesize());

                QDBusUnixFileDescriptor descriptor(destFile.handle());
                int result = storage->getFileToFileDescriptor(descriptor, convertPath(src.path()));
                if (result) {
                    error(KIO::ERR_CANNOT_READ, urlFileName(src));
                    return;
                }

                result = waitForCopyOperation(storage);
                processedSize(quint64(source.filesize()));
                destFile.close();

                if (result) {
                    error(KIO::ERR_CANNOT_READ, urlFileName(src));
                    return;
                }

                // set correct modification time
                struct ::utimbuf times;
                times.actime = QDateTime::currentDateTime().toSecsSinceEpoch();
                times.modtime = source.modificationdate();

                ::utime(dest.path().toUtf8().data(), &times);

                qCDebug(LOG_KIO_MTP) << "Received file";
                finished();
                return;
            }
        }
        error(KIO::ERR_CANNOT_READ, urlFileName(src));
    }
}

void MTPSlave::mkdir(const QUrl &url, int)
{
    const int check = checkUrl(url);
    switch (check) {
    case 0:
        break;
    default:
        error(ERR_MALFORMED_URL, url.path());
        return;
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
                    finished();
                    return;
                }
            }
        }
    }
    error(ERR_CANNOT_MKDIR, url.path());
}

void MTPSlave::del(const QUrl &url, bool)
{
    const int check = checkUrl(url);
    switch (check) {
    case 0:
        break;
    default:
        error(ERR_MALFORMED_URL, url.path());
        return;
    }

    const QStringList pathItems = url.path().split(QLatin1Char('/'), Qt::SkipEmptyParts);
    if (pathItems.size() >= 2) {
        const KMTPDeviceInterface *mtpDevice = m_kmtpDaemon.deviceFromName(pathItems.first());
        if (mtpDevice) {
            const KMTPStorageInterface *storage = mtpDevice->storageFromDescription(pathItems.at(1));
            if (storage) {
                const int result = storage->deleteObject(convertPath(url.path()));
                if (!result) {
                    finished();
                    return;
                }
            }
        }
    }
    error(ERR_CANNOT_DELETE, url.path());
}

void MTPSlave::rename(const QUrl &src, const QUrl &dest, JobFlags flags)
{
    int check = checkUrl(src);
    switch (check) {
    case 0:
        break;
    default:
        error(ERR_MALFORMED_URL, src.path());
        return;
    }
    check = checkUrl(dest);
    switch (check) {
    case 0:
        break;
    default:
        error(ERR_MALFORMED_URL, dest.path());
        return;
    }

    if (src.scheme() != QLatin1String("mtp")) {
        // Kate: when editing files directly on the device and the user wants to save the changes,
        // Kate tries to move the file from /tmp/xxx to the MTP device which is not supported.
        // The ERR_UNSUPPORTED_ACTION error tells Kate to copy the file instead of moving.
        error(ERR_UNSUPPORTED_ACTION, src.path());
        return;
    }

    const QStringList srcItems = src.path().split(QLatin1Char('/'), Qt::SkipEmptyParts);
    KMTPDeviceInterface *mtpDevice = m_kmtpDaemon.deviceFromName(srcItems.first());
    if (mtpDevice) {
        // rename Device
        if (srcItems.size() == 1) {
            const int result = mtpDevice->setFriendlyName(urlFileName(dest));
            if (!result) {
                finished();
                return;
            }
        }
        // rename Storage
        else if (srcItems.size() == 2) {
            error(ERR_CANNOT_RENAME, src.path());
            return;
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
                            error(ERR_CANNOT_DELETE, dest.path());
                            return;
                        }
                    } else {
                        error(ERR_FILE_ALREADY_EXIST, dest.path());
                        return;
                    }
                }

                const int result = storage->setFileName(convertPath(src.path()), dest.fileName());
                if (!result) {
                    finished();
                    return;
                }
            }
        }
    }
    error(ERR_CANNOT_RENAME, src.path());
}

void MTPSlave::virtual_hook(int id, void *data)
{
    switch(id) {
    case SlaveBase::GetFileSystemFreeSpace: {
        QUrl *url = static_cast<QUrl *>(data);
        fileSystemFreeSpace(*url);
    }
    break;
    default:
        SlaveBase::virtual_hook(id, data);
    }
}

void MTPSlave::fileSystemFreeSpace(const QUrl &url)
{
    qCDebug(LOG_KIO_MTP) << "fileSystemFreeSpace:" << url;

    const int check = checkUrl(url);
    switch (check) {
    case 0:
        break;
    case 1:
        finished();
        return;
    case 2:
        error(ERR_DOES_NOT_EXIST, url.toDisplayString());
        return;
    default:
        error(ERR_MALFORMED_URL, url.toDisplayString());
        return;
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
                finished();
                return;
            }
        }
    }
    error(KIO::ERR_CANNOT_STAT, url.toDisplayString());
}

int MTPSlave::waitForCopyOperation(const KMTPStorageInterface *storage)
{
    QEventLoop loop;
    connect(storage, &KMTPStorageInterface::copyProgress, &loop, [this] (qulonglong sent, qulonglong total) {
        Q_UNUSED(total)
        processedSize(sent);
    });

    // any chance to 'miss' the copyFinished signal and dead lock the slave?
    connect(storage, &KMTPStorageInterface::copyFinished, &loop, &QEventLoop::exit);
    return loop.exec();
}

#include "kio_mtp.moc"
