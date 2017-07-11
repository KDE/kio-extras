/*
 *  Main implementation for KIO-MTP
 *  Copyright (C) 2012  Philipp Schmidt <philschmidt@gmx.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "kio_mtp.h"
#include "kio_mtp_helpers.h"

// #include <KComponentData>
#include <QTemporaryFile>
#include <QFileInfo>
#include <QDateTime>
#include <QCoreApplication>
#include <QTimer>

#include <sys/stat.h>
#include <sys/types.h>
#include <utime.h>

#include <solid/device.h>
#include <solid/genericinterface.h>

//////////////////////////////////////////////////////////////////////////////
///////////////////////////// Slave Implementation ///////////////////////////
//////////////////////////////////////////////////////////////////////////////

Q_LOGGING_CATEGORY(LOG_KIO_MTP, "kde.kio-mtp")

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
    LIBMTP_Init();

    qCDebug(LOG_KIO_MTP) << "Slave started";

    deviceCache = new DeviceCache(60000);
    fileCache = new FileCache(this);

    qCDebug(LOG_KIO_MTP) << "Caches created";
}

MTPSlave::~MTPSlave()
{
    qCDebug(LOG_KIO_MTP) << "Slave destroyed";
    delete fileCache;
    delete deviceCache;
}

/**
 * @brief Get's the correct object from the device.
 * @param pathItems A QStringList containing the items of the filepath
 * @return QPair with the object and its device. pair.first is a nullpointer if the object doesn't exist or for root or, depending on the pathItems size device (1), storage (2) or file (>=3)
 */
QPair<void *, LIBMTP_mtpdevice_t *> MTPSlave::getPath(const QString &path)
{
    QStringList pathItems = path.split(QLatin1Char('/'), QString::SkipEmptyParts);

    qCDebug(LOG_KIO_MTP) << path << pathItems.size();

    QPair<void *, LIBMTP_mtpdevice_t *> ret;

    // Don' handle the root directory
    if (pathItems.size() <= 0) {
        return ret;
    }

    if (deviceCache->contains(pathItems.at(0))) {
        LIBMTP_mtpdevice_t *device = deviceCache->get(pathItems.at(0))->getDevice();

        // return specific device
        if (pathItems.size() == 1) {
            ret.first = device;
            ret.second = device;

            qCDebug(LOG_KIO_MTP) << "returning LIBMTP_mtpdevice_t";
        }

        if (pathItems.size() > 2) {
            // Query Cache after we have the device
            uint32_t c_fileID = fileCache->queryPath(path);
            if (c_fileID != 0) {
                qCDebug(LOG_KIO_MTP)  << "Match found in cache, checking device";

                LIBMTP_file_t *file = LIBMTP_Get_Filemetadata(device, c_fileID);
                if (file) {
                    qCDebug(LOG_KIO_MTP) << "Found file in cache";
                    ret.first = file;
                    ret.second = device;

                    qCDebug(LOG_KIO_MTP) << "returning LIBMTP_file_t from cache";

                    return ret;
                }
            }
            // Query cache for parent
            else if (pathItems.size() > 3) {
                QString parentPath = convertToPath(pathItems, pathItems.size() - 1);
                uint32_t c_parentID = fileCache->queryPath(parentPath);

                qCDebug(LOG_KIO_MTP)  << "Match for parent found in cache, checking device. Parent id = " << c_parentID;

                LIBMTP_file_t *parent = LIBMTP_Get_Filemetadata(device, c_parentID);
                if (parent) {
                    qCDebug(LOG_KIO_MTP) << "Found parent in cache";
//                     fileCache->addPath( parentPath, c_parentID );

                    QMap<QString, LIBMTP_file_t *> files = getFiles(device, parent->storage_id, c_parentID);

                    for (QMap<QString, LIBMTP_file_t *>::iterator it = files.begin(); it != files.end(); ++it) {
                        QString filePath = parentPath;
                        filePath.append(QString::fromUtf8("/")).append(it.key());
                        fileCache->addPath(filePath, it.value()->item_id);
                    }

                    if (files.contains(pathItems.last())) {
                        LIBMTP_file_t *file = files.value(pathItems.last());

                        ret.first = file;
                        ret.second = device;

                        qCDebug(LOG_KIO_MTP) << "returning LIBMTP_file_t from cached parent" ;

                        fileCache->addPath(path, file->item_id);
                    }

                    return ret;
                }
            }
        }

        QMap<QString, LIBMTP_devicestorage_t *> storages = getDevicestorages(device);

        if (pathItems.size() > 1 && storages.contains(pathItems.at(1))) {
            LIBMTP_devicestorage_t *storage = storages.value(pathItems.at(1));

            if (pathItems.size() == 2) {
                ret.first = storage;
                ret.second = device;

                qCDebug(LOG_KIO_MTP) << "returning LIBMTP_devicestorage_t";

                return ret;
            }

            int currentLevel = 2, currentParent = 0xFFFFFFFF;

            QMap<QString, LIBMTP_file_t *> files;

            // traverse further while depth not reached
            while (currentLevel < pathItems.size()) {
                files = getFiles(device, storage->id, currentParent);

                if (files.contains(pathItems.at(currentLevel))) {
                    currentParent = files.value(pathItems.at(currentLevel))->item_id;
                } else {
                    qCDebug(LOG_KIO_MTP) << "returning LIBMTP_file_t using tree walk";

                    return ret;
                }
                currentLevel++;
            }

            ret.first = LIBMTP_Get_Filemetadata(device, currentParent);
            ret.second = device;

            fileCache->addPath(path, currentParent);
        }
    }

    return ret;
}

int MTPSlave::checkUrl(const QUrl &url, bool redirect)
{
    qCDebug(LOG_KIO_MTP) << url;

    if (url.path().startsWith(QLatin1String("udi=")) && redirect) {
        QString udi = url.adjusted(QUrl::StripTrailingSlash).path().remove(0, 4);

        qCDebug(LOG_KIO_MTP) << "udi = " << udi;

        if (deviceCache->contains(udi, true)) {
            QUrl newUrl;
            newUrl.setScheme(QLatin1String("mtp"));
            newUrl.setPath(QLatin1Char('/') + deviceCache->get(udi, true)->getName());
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

QString MTPSlave::urlDirectory(const QUrl &url, bool appendTrailingSlash)
{
    if (!appendTrailingSlash) {
        return url.adjusted(QUrl::StripTrailingSlash | QUrl::RemoveFilename).path();
    }
    return url.adjusted(QUrl::RemoveFilename).path();
}

QString MTPSlave::urlFileName(const QUrl &url)
{
    return url.fileName();
}

void MTPSlave::listDir(const QUrl &url)
{
    qCDebug(LOG_KIO_MTP) << url.path();

    int check = checkUrl(url);
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

    QStringList pathItems = url.path().split(QLatin1Char('/'), QString::SkipEmptyParts);

    UDSEntry entry;

    // list devices
    if (pathItems.size() == 0) {
        qCDebug(LOG_KIO_MTP) << "Root directory, listing devices";
        totalSize(deviceCache->size());

        foreach(CachedDevice * cachedDevice, deviceCache->getAll().values()) {
            LIBMTP_mtpdevice_t *device = cachedDevice->getDevice();

            getEntry(entry, device);

            listEntry(entry);
            entry.clear();
        }

        qCDebug(LOG_KIO_MTP) << "[SUCCESS] :: Devices";
        finished();
    }
    // traverse into device
    else if (deviceCache->contains(pathItems.at(0))) {
        QPair<void *, LIBMTP_mtpdevice_t *> pair = getPath(url.path());
        UDSEntry entry;

        if (pair.first) {
            LIBMTP_mtpdevice_t *device = pair.second;

            // Device, list storages
            if (pathItems.size() == 1) {
                QMap<QString, LIBMTP_devicestorage_t *> storages = getDevicestorages(device);

                qCDebug(LOG_KIO_MTP) << "Listing storages for device " << pathItems.at(0);
                totalSize(storages.size());

                if (storages.size() > 0) {
                    foreach(const QString & storageName, storages.keys()) {
                        getEntry(entry, storages.value(storageName));

                        listEntry(entry);
                        entry.clear();
                    }

                    finished();
                    qCDebug(LOG_KIO_MTP) << "[SUCCESS] :: Storages";
                } else {
                    warning(i18n("No Storages found. Make sure your device is unlocked and has MTP enabled in its USB connection settings."));
                }
            }
            // Storage, list files and folders of storage root
            else {
                QMap<QString, LIBMTP_file_t *> files;

                if (pathItems.size() == 2) {
                    qCDebug(LOG_KIO_MTP) << "Getting storage root listing";

                    LIBMTP_devicestorage_t *storage = (LIBMTP_devicestorage_t *)pair.first;

                    qCDebug(LOG_KIO_MTP) << "We have a storage:" << (storage == NULL);

                    files = getFiles(device, storage->id);
                } else {
                    LIBMTP_file_t *parent = (LIBMTP_file_t *)pair.first;

                    files = getFiles(device, parent->storage_id, parent->item_id);
                }

                for (QMap<QString, LIBMTP_file_t *>::iterator it = files.begin(); it != files.end(); ++it) {
                    LIBMTP_file_t *file = it.value();

                    QString filePath;
                    if (url.path().endsWith(QLatin1Char('/'))) {
                        filePath = url.path().append(it.key());
                    } else {
                        filePath = url.path().append(QLatin1Char('/')).append(it.key());
                    }

                    fileCache->addPath(filePath, file->item_id);

                    getEntry(entry, file);

                    listEntry(entry);
                    entry.clear();
                }

                finished();

                qCDebug(LOG_KIO_MTP) << "[SUCCESS] Files";
            }
        } else {
            error(ERR_CANNOT_ENTER_DIRECTORY, url.path());
            qCDebug(LOG_KIO_MTP) << "[ERROR]";
        }
    } else {
        error(ERR_CANNOT_ENTER_DIRECTORY, url.path());
        qCDebug(LOG_KIO_MTP) << "[ERROR]";
    }
}

void MTPSlave::stat(const QUrl &url)
{
    qCDebug(LOG_KIO_MTP) << url.path();

    int check = checkUrl(url);
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

    QStringList pathItems = url.path().split(QLatin1Char('/'), QString::SkipEmptyParts);

    QPair<void *, LIBMTP_mtpdevice_t *> pair = getPath(url.path());
    UDSEntry entry;

    if (pair.first) {
        // Root
        if (pathItems.size() < 1) {
            entry.insert(UDSEntry::UDS_NAME, QLatin1String("mtp:///"));
            entry.insert(UDSEntry::UDS_FILE_TYPE, S_IFDIR);
            entry.insert(UDSEntry::UDS_ACCESS, S_IRUSR | S_IRGRP | S_IROTH | S_IXUSR | S_IXGRP | S_IXOTH);
            entry.insert(UDSEntry::UDS_MIME_TYPE, QLatin1String("inode/directory"));
        }
        // Device
        else if (pathItems.size() < 2) {
            getEntry(entry, pair.second);
        }
        // Storage
        else if (pathItems.size() < 3) {
            getEntry(entry, (LIBMTP_devicestorage_t *) pair.first);
        }
        // Folder/File
        else {
            getEntry(entry, (LIBMTP_file_t *) pair.first);
        }
    }
    statEntry(entry);
    finished();
}

void MTPSlave::mimetype(const QUrl &url)
{
    int check = checkUrl(url);
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

    qCDebug(LOG_KIO_MTP) << url.path();

    QStringList pathItems = url.path().split(QLatin1Char('/'), QString::SkipEmptyParts);

    QPair<void *, LIBMTP_mtpdevice_t *> pair = getPath(url.path());

    if (pair.first) {
//      NOTE the difference between calling mimetype and mimeType
        if (pathItems.size() > 2) {
            mimeType(getMimetype(((LIBMTP_file_t *) pair.first)->filetype));
        } else {
            mimeType(QString::fromLatin1("inode/directory"));
        }
    } else {
        error(ERR_DOES_NOT_EXIST, url.path());
        return;
    }
}

void MTPSlave::put(const QUrl &url, int, JobFlags flags)
{
    int check = checkUrl(url);
    switch (check) {
    case 0:
        break;
    default:
        error(ERR_MALFORMED_URL, url.path());
        return;
    }

    qCDebug(LOG_KIO_MTP) << url.path();

    QStringList destItems = url.path().split(QLatin1Char('/'), QString::SkipEmptyParts);

    // Can't copy to root or device, needs storage
    if (destItems.size() < 2) {
        error(ERR_UNSUPPORTED_ACTION, url.path());
        return;
    }

    if (!(flags & KIO::Overwrite) && getPath(url.path()).first) {
        error(ERR_FILE_ALREADY_EXIST, url.path());
        return;
    }

    destItems.takeLast();

    QPair<void *, LIBMTP_mtpdevice_t *> pair = getPath(urlDirectory(url));

    if (!pair.first) {
        error(ERR_DOES_NOT_EXIST, url.path());
        return;
    }

    LIBMTP_mtpdevice_t *device = pair.second;
    LIBMTP_file_t *parent = (LIBMTP_file_t *) pair.first;
    if (parent->filetype != LIBMTP_FILETYPE_FOLDER) {
        error(ERR_IS_FILE, urlDirectory(url));
        return;
    }

    // We did get a total size from the application
    if (hasMetaData(QLatin1String("sourceSize"))) {
        qCDebug(LOG_KIO_MTP) << "direct put";

        LIBMTP_file_t *file = LIBMTP_new_file_t();
        file->parent_id = parent->item_id;
        file->filename = strdup(urlFileName(url).toUtf8().data());
        file->filetype = getFiletype(urlFileName(url));
        file->filesize = metaData(QLatin1String("sourceSize")).toULongLong();
        file->modificationdate = QDateTime::currentDateTime().toTime_t();
        file->storage_id = parent->storage_id;

        qCDebug(LOG_KIO_MTP) << "Sending file" << file->filename;

        int ret = LIBMTP_Send_File_From_Handler(device, &dataGet, this, file, &dataProgress, this);
        LIBMTP_destroy_file_t(file);

        if (ret != 0) {
            error(KIO::ERR_COULD_NOT_WRITE, urlFileName(url));
            LIBMTP_Dump_Errorstack(device);
            LIBMTP_Clear_Errorstack(device);
            return;
        }
    }
    // We need to get the entire file first, then we can upload
    else {
        qCDebug(LOG_KIO_MTP) << "use temp file";

        QTemporaryFile temp;
        QByteArray buffer;
        int len = 0;

        do {
            dataReq();
            len = readData(buffer);
            temp.write(buffer);
        } while (len > 0);

        QFileInfo info(temp);

        LIBMTP_file_t *file = LIBMTP_new_file_t();
        file->parent_id = parent->item_id;
        file->filename = strdup(urlFileName(url).toUtf8().data());
        file->filetype = getFiletype(urlFileName(url));
        file->filesize = info.size();
        file->modificationdate = QDateTime::currentDateTime().toTime_t();
        file->storage_id = parent->storage_id;

        int ret = LIBMTP_Send_File_From_File_Descriptor(device, temp.handle(), file, NULL, NULL);
        LIBMTP_destroy_file_t(file);

        if (ret != 0) {
            error(KIO::ERR_COULD_NOT_WRITE, urlFileName(url));
            return;
        }
        finished();
    }
}

void MTPSlave::get(const QUrl &url)
{
    int check = checkUrl(url);
    switch (check) {
    case 0:
        break;
    default:
        error(ERR_MALFORMED_URL, url.path());
        return;
    }

    qCDebug(LOG_KIO_MTP) << url.path();

    QStringList pathItems = url.path().split(QLatin1Char('/'), QString::SkipEmptyParts);

    // File
    if (pathItems.size() > 2) {
        QPair<void *, LIBMTP_mtpdevice_t *> pair = getPath(url.path());

        if (pair.first) {
            LIBMTP_file_t *file = (LIBMTP_file_t *) pair.first;

            mimeType(getMimetype(file->filetype));
            totalSize(file->filesize);

            LIBMTP_mtpdevice_t *device = pair.second;

            int ret = LIBMTP_Get_File_To_Handler(device, file->item_id, &dataPut, this, &dataProgress, this);
            if (ret != 0) {
                error(ERR_COULD_NOT_READ, url.path());
                return;
            }
            data(QByteArray());
            finished();
        } else {
            error(ERR_DOES_NOT_EXIST, url.path());
        }
    } else {
        error(ERR_UNSUPPORTED_ACTION, url.path());
    }
}

void MTPSlave::copy(const QUrl &src, const QUrl &dest, int, JobFlags flags)
{
    qCDebug(LOG_KIO_MTP) << src.path() << dest.path();

    if (src.scheme() == QLatin1String("mtp") && dest.scheme() == QLatin1String("mtp")) {
        qCDebug(LOG_KIO_MTP) << "Copy on device: Not supported";
        // MTP doesn't support moving files directly on the device, so we have to download and then upload...

        error(ERR_UNSUPPORTED_ACTION, i18n("Cannot copy/move files on the device itself"));
    } else if (src.scheme() == QLatin1String("file") && dest.scheme() == QLatin1String("mtp")) {
        int check = checkUrl(dest);
        switch (check) {
        case 0:
            break;
        default:
            error(ERR_MALFORMED_URL, dest.path());
            return;
        }

        QStringList destItems = dest.path().split(QLatin1Char('/') , QString::SkipEmptyParts);

        // Can't copy to root or device, needs storage
        if (destItems.size() < 2) {
            error(ERR_UNSUPPORTED_ACTION, dest.path());
            return;
        }

        qCDebug(LOG_KIO_MTP) << "Copy file " << urlFileName(src) << "from filesystem to device" << urlDirectory(src, true) << urlDirectory(dest, true);

        if (!(flags & KIO::Overwrite) && getPath(dest.path()).first) {
            error(ERR_FILE_ALREADY_EXIST, dest.path());
            return;
        }

        destItems.takeLast();

        QPair<void *, LIBMTP_mtpdevice_t *> pair = getPath(urlDirectory(dest));

        if (!pair.first) {
            error(ERR_DOES_NOT_EXIST, urlDirectory(dest, true));
            return;
        }

        LIBMTP_mtpdevice_t *device = pair.second;

        uint32_t parent_id = 0xFFFFFFFF, storage_id = 0;

        if (destItems.size() == 2) {
            LIBMTP_devicestorage_t *storage = (LIBMTP_devicestorage_t *) pair.first;

            storage_id = storage->id;
        } else {
            LIBMTP_file_t *parent = (LIBMTP_file_t *) pair.first;

            storage_id = parent->storage_id;
            parent_id = parent->item_id;

            if (parent->filetype != LIBMTP_FILETYPE_FOLDER) {
                error(ERR_IS_FILE, urlDirectory(dest));
                return;
            }
        }

        QFileInfo source(src.path());

        LIBMTP_file_t *file = LIBMTP_new_file_t();
        file->parent_id = parent_id;
        file->filename = strdup(urlFileName(dest).toUtf8().data());
        file->filetype = getFiletype(urlFileName(dest));
        file->filesize = source.size();
        file->modificationdate = source.lastModified().toTime_t();
        file->storage_id = storage_id;

        qCDebug(LOG_KIO_MTP) << "Sending file" << file->filename << "with size" << file->filesize;

        totalSize(source.size());

        int ret = LIBMTP_Send_File_From_File(device, src.path().toUtf8().data(), file, (LIBMTP_progressfunc_t) &dataProgress, this);
        LIBMTP_destroy_file_t(file);

        if (ret != 0) {
            error(KIO::ERR_COULD_NOT_WRITE, urlFileName(dest));
            LIBMTP_Dump_Errorstack(device);
            LIBMTP_Clear_Errorstack(device);
            return;
        }

        qCDebug(LOG_KIO_MTP) << "Sent file";
    } else if (src.scheme() == QLatin1String("mtp") && dest.scheme() == QLatin1String("file")) {
        int check = checkUrl(src);
        switch (check) {
        case 0:
            break;
        default:
            error(ERR_MALFORMED_URL, src.path());
            return;
        }

        qCDebug(LOG_KIO_MTP) << "Copy file " << urlFileName(src) << "from device to filesystem" << urlDirectory(src, true) << urlDirectory(dest, true);

        QFileInfo destination(dest.path());

        if (!(flags & KIO::Overwrite) && destination.exists()) {
            error(ERR_FILE_ALREADY_EXIST, dest.path());
            return;
        }

        QStringList srcItems = src.path().split(QLatin1Char('/'), QString::SkipEmptyParts);

        // Can't copy to root or device, needs storage
        if (srcItems.size() < 2) {
            error(ERR_UNSUPPORTED_ACTION, src.path());
            return;
        }

        QPair<void *, LIBMTP_mtpdevice_t *> pair = getPath(src.path());
        if (!pair.first) {
            error(ERR_COULD_NOT_READ, src.path());
            return;
        }

        LIBMTP_mtpdevice_t *device = pair.second;
        LIBMTP_file_t *source = (LIBMTP_file_t *) pair.first;
        if (source->filetype == LIBMTP_FILETYPE_FOLDER) {
            error(ERR_IS_DIRECTORY, urlDirectory(src));
            return;
        }

        qCDebug(LOG_KIO_MTP) << "Getting file" << source->filename << urlFileName(dest) << source->filesize;

        totalSize(source->filesize);

        int ret = LIBMTP_Get_File_To_File(device, source->item_id, dest.path().toUtf8().data(), (LIBMTP_progressfunc_t) &dataProgress, this);
        if (ret != 0) {
            error(KIO::ERR_COULD_NOT_WRITE, urlFileName(dest));
            LIBMTP_Dump_Errorstack(device);
            LIBMTP_Clear_Errorstack(device);
            return;
        }

        struct utimbuf *times = (utimbuf *) malloc(sizeof(utimbuf));
        times->actime = QDateTime::currentDateTime().toTime_t();
        times->modtime = source->modificationdate;

        int result = utime(dest.path().toUtf8().data(), times);

        free(times);

        qCDebug(LOG_KIO_MTP) << "Sent file";

    }
    finished();
}

void MTPSlave::mkdir(const QUrl &url, int)
{
    int check = checkUrl(url);
    switch (check) {
    case 0:
        break;
    default:
        error(ERR_MALFORMED_URL, url.path());
        return;
    }

    qCDebug(LOG_KIO_MTP) << url.path();

    QStringList pathItems = url.path().split(QLatin1Char('/') , QString::SkipEmptyParts);
    int pathDepth = pathItems.size();

    if (pathItems.size() > 2 && !getPath(url.path()).first) {
        char *dirName = strdup(pathItems.takeLast().toUtf8().data());

        LIBMTP_mtpdevice_t *device;
        LIBMTP_file_t *file;
        LIBMTP_devicestorage_t *storage;
        int ret = 0;

        QPair<void *, LIBMTP_mtpdevice_t *> pair = getPath(urlDirectory(url));

        if (pathDepth == 3) {
            //the folder need to be created straight to a storage device
            storage = (LIBMTP_devicestorage_t *) pair.first;
            device = pair.second;
            ret = LIBMTP_Create_Folder(device, dirName, 0xFFFFFFFF, storage->id);
        } else if (pair.first) {
            file = (LIBMTP_file_t *) pair.first;
            device = pair.second;

            if (file && file->filetype == LIBMTP_FILETYPE_FOLDER) {
                qCDebug(LOG_KIO_MTP) << "Found parent" << file->item_id << file->filename;
                qCDebug(LOG_KIO_MTP) << "Attempting to create folder" << dirName;

                ret = LIBMTP_Create_Folder(device, dirName, file->item_id, file->storage_id);

            }
        }
        if (ret != 0) {
            fileCache->addPath(url.path(), ret);
            finished();
            return;
        } else {
            LIBMTP_Dump_Errorstack(device);
            LIBMTP_Clear_Errorstack(device);
        }
    } else {
        error(ERR_DIR_ALREADY_EXIST, url.path());
        return;
    }

    error(ERR_COULD_NOT_MKDIR, url.path());
}

void MTPSlave::del(const QUrl &url, bool)
{
    int check = checkUrl(url);
    switch (check) {
    case 0:
        break;
    default:
        error(ERR_MALFORMED_URL, url.path());
        return;
    }

    qCDebug(LOG_KIO_MTP) << url.path();

    QStringList pathItems = url.path().split(QLatin1Char('/'), QString::SkipEmptyParts);

    if (pathItems.size() < 2) {
        error(ERR_CANNOT_DELETE, url.path());
        return;
    }

    QPair<void *, LIBMTP_mtpdevice_t *> pair = getPath(url.path());

    LIBMTP_file_t *file = (LIBMTP_file_t *) pair.first;

    int ret = LIBMTP_Delete_Object(pair.second, file->item_id);

    LIBMTP_destroy_file_t (file);

    if (ret != 0) {
        error(ERR_CANNOT_DELETE, url.path());
        return;
    }

    fileCache->removePath(url.path());
    finished();
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

    qCDebug(LOG_KIO_MTP) << src.path();

    QStringList srcItems = src.path().split(QLatin1Char('/'), QString::SkipEmptyParts);
    QPair<void *, LIBMTP_mtpdevice_t *> pair = getPath(src.path());

    if (pair.first) {
        // Rename Device
        if (srcItems.size() == 1) {
            LIBMTP_Set_Friendlyname(pair.second, urlFileName(dest).toUtf8().data());
        }
        // Rename Storage
        else if (srcItems.size() == 2) {
            error(ERR_CANNOT_RENAME, src.path());
            return;
        } else {
            LIBMTP_file_t *destination = (LIBMTP_file_t *) getPath(dest.path()).first;
            LIBMTP_file_t *source = (LIBMTP_file_t *) pair.first;

            if (!(flags & KIO::Overwrite) && destination) {
                if (destination->filetype == LIBMTP_FILETYPE_FOLDER) {
                    error(ERR_DIR_ALREADY_EXIST, dest.path());
                } else {
                    error(ERR_FILE_ALREADY_EXIST, dest.path());
                }
                return;
            }

            int ret = LIBMTP_Set_File_Name(pair.second, source, urlFileName(dest).toUtf8().data());

            if (ret != 0) {
                error(ERR_CANNOT_RENAME, src.path());
                return;
            } else {
                fileCache->addPath(dest.path(), source->item_id);
                fileCache->removePath(src.path());
            }

            LIBMTP_destroy_file_t (source);
        }

        finished();
    } else {
        error(ERR_DOES_NOT_EXIST, src.path());
    }
}

void MTPSlave::virtual_hook(int id, void *data)
{
    switch(id) {
        case SlaveBase::GetFileSystemFreeSpace: {
            QUrl *url = static_cast<QUrl *>(data);
            fileSystemFreeSpace(*url);
        }   break;
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

    const auto path = url.path();

    const auto storagePath = path.section(QLatin1Char('/'), 0, 2, QString::SectionIncludeLeadingSep);
    if (storagePath.count(QLatin1Char('/')) != 2) { // /{device}/{storage}
        error(KIO::ERR_COULD_NOT_STAT, url.toDisplayString());
        return;
    }

    const auto pair = getPath(storagePath);
    auto storage = (LIBMTP_devicestorage_t *)pair.first;
    if (!storage) {
        error(KIO::ERR_COULD_NOT_STAT, url.toDisplayString());
        return;
    }

    setMetaData(QStringLiteral("total"), QString::number(storage->MaxCapacity));
    setMetaData(QStringLiteral("available"), QString::number(storage->FreeSpaceInBytes));

    finished();
}

#include "kio_mtp.moc"

