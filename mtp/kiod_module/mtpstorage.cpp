/*
    This file is part of the MTP KIOD module, part of the KDE project.

    SPDX-FileCopyrightText: 2018 Andreas Krutzler <andreas.krutzler@gmx.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "mtpstorage.h"

#include <QDateTime>
#include <qplatformdefs.h>

#include "kiod_kmtpd_debug.h"
#include "mtpdevice.h"
#include "storageadaptor.h"

/**
 * @brief MTPDataPutFunc callback function, "puts" data from the device somewhere else
 */
static uint16_t onDataPut(void *, void *priv, uint32_t sendlen, unsigned char *data, uint32_t *putlen)
{
    MTPStorage *storage = static_cast<MTPStorage *>(priv);
    Q_EMIT storage->dataReady(QByteArray(reinterpret_cast<char *>(data), int(sendlen)));
    *putlen = sendlen;

    return LIBMTP_HANDLER_RETURN_OK;
}

static int onDataProgress(const uint64_t sent, const uint64_t total, const void * const priv)
{
    MTPStorage *storage = const_cast<MTPStorage *>(static_cast<const MTPStorage *>(priv));
    Q_EMIT storage->copyProgress(sent, total);
    return LIBMTP_HANDLER_RETURN_OK;
}

static QString convertToPath(const QStringList &pathItems, const int elements)
{
    QString path;

    for (int i = 0; i < elements && elements <= pathItems.size(); i++) {
        path.append(QLatin1Char('/'));
        path.append(pathItems.at(i));
    }

    return path;
}

/**
 * @brief Converts a mime-type to a LIBMTP_filetype_t
 */
static LIBMTP_filetype_t getFiletype(const QString &filename)
{
    LIBMTP_filetype_t filetype;

    const QString ptype = filename.split(QLatin1Char('.')).last();

    /* This need to be kept constantly updated as new file types arrive. */
    if (ptype == QLatin1String("wav")) {
        filetype = LIBMTP_FILETYPE_WAV;
    } else if (ptype == QLatin1String("mp3")) {
        filetype = LIBMTP_FILETYPE_MP3;
    } else if (ptype == QLatin1String("wma")) {
        filetype = LIBMTP_FILETYPE_WMA;
    } else if (ptype == QLatin1String("ogg")) {
        filetype = LIBMTP_FILETYPE_OGG;
    } else if (ptype == QLatin1String("mp4")) {
        filetype = LIBMTP_FILETYPE_MP4;
    } else if (ptype == QLatin1String("wmv")) {
        filetype = LIBMTP_FILETYPE_WMV;
    } else if (ptype == QLatin1String("avi")) {
        filetype = LIBMTP_FILETYPE_AVI;
    } else if (ptype == QLatin1String("mpeg") ||
               ptype == QLatin1String("mpg")) {
        filetype = LIBMTP_FILETYPE_MPEG;
    } else if (ptype == QLatin1String("asf")) {
        filetype = LIBMTP_FILETYPE_ASF;
    } else if (ptype == QLatin1String("qt") ||
               ptype == QLatin1String("mov")) {
        filetype = LIBMTP_FILETYPE_QT;
    } else if (ptype == QLatin1String("wma")) {
        filetype = LIBMTP_FILETYPE_WMA;
    } else if (ptype == QLatin1String("jpg") ||
               ptype == QLatin1String("jpeg")) {
        filetype = LIBMTP_FILETYPE_JPEG;
    } else if (ptype == QLatin1String("jfif")) {
        filetype = LIBMTP_FILETYPE_JFIF;
    } else if (ptype == QLatin1String("tif") ||
               ptype == QLatin1String("tiff")) {
        filetype = LIBMTP_FILETYPE_TIFF;
    } else if (ptype == QLatin1String("bmp")) {
        filetype = LIBMTP_FILETYPE_BMP;
    } else if (ptype == QLatin1String("gif")) {
        filetype = LIBMTP_FILETYPE_GIF;
    } else if (ptype == QLatin1String("pic") ||
               ptype == QLatin1String("pict")) {
        filetype = LIBMTP_FILETYPE_PICT;
    } else if (ptype == QLatin1String("png")) {
        filetype = LIBMTP_FILETYPE_PNG;
    } else if (ptype == QLatin1String("wmf")) {
        filetype = LIBMTP_FILETYPE_WINDOWSIMAGEFORMAT;
    } else if (ptype == QLatin1String("ics")) {
        filetype = LIBMTP_FILETYPE_VCALENDAR2;
    } else if (ptype == QLatin1String("exe") ||
               ptype == QLatin1String("com") ||
               ptype == QLatin1String("bat") ||
               ptype == QLatin1String("dll") ||
               ptype == QLatin1String("sys")) {
        filetype = LIBMTP_FILETYPE_WINEXEC;
    } else if (ptype == QLatin1String("aac")) {
        filetype = LIBMTP_FILETYPE_AAC;
    } else if (ptype == QLatin1String("mp2")) {
        filetype = LIBMTP_FILETYPE_MP2;
    } else if (ptype == QLatin1String("flac")) {
        filetype = LIBMTP_FILETYPE_FLAC;
    } else if (ptype == QLatin1String("m4a")) {
        filetype = LIBMTP_FILETYPE_M4A;
    } else if (ptype == QLatin1String("doc")) {
        filetype = LIBMTP_FILETYPE_DOC;
    } else if (ptype == QLatin1String("xml")) {
        filetype = LIBMTP_FILETYPE_XML;
    } else if (ptype == QLatin1String("xls")) {
        filetype = LIBMTP_FILETYPE_XLS;
    } else if (ptype == QLatin1String("ppt")) {
        filetype = LIBMTP_FILETYPE_PPT;
    } else if (ptype == QLatin1String("mht")) {
        filetype = LIBMTP_FILETYPE_MHT;
    } else if (ptype == QLatin1String("jp2")) {
        filetype = LIBMTP_FILETYPE_JP2;
    } else if (ptype == QLatin1String("jpx")) {
        filetype = LIBMTP_FILETYPE_JPX;
    } else if (ptype == QLatin1String("bin")) {
        filetype = LIBMTP_FILETYPE_FIRMWARE;
    } else if (ptype == QLatin1String("vcf")) {
        filetype = LIBMTP_FILETYPE_VCARD3;
    } else {
        /* Tagging as unknown file type */
        filetype = LIBMTP_FILETYPE_UNKNOWN;
    }

    return filetype;
}

/**
 * @brief Converts a LIBMTP_filetype_t to a mime-type
 */
static QString getMimetype(LIBMTP_filetype_t filetype)
{
    switch (filetype) {
    case LIBMTP_FILETYPE_FOLDER:
        return QStringLiteral("inode/directory");

    case LIBMTP_FILETYPE_WAV:
        return QStringLiteral("audio/wav");
    case LIBMTP_FILETYPE_MP3:
        return QStringLiteral("audio/x-mp3");
    case LIBMTP_FILETYPE_WMA:
        return QStringLiteral("audio/x-ms-wma");
    case LIBMTP_FILETYPE_OGG:
        return QStringLiteral("audio/x-vorbis+ogg");
    case LIBMTP_FILETYPE_AUDIBLE:
        return QString();
    case LIBMTP_FILETYPE_MP4:
        return QStringLiteral("audio/mp4");
    case LIBMTP_FILETYPE_UNDEF_AUDIO:
        return QString();
    case LIBMTP_FILETYPE_WMV:
        return QStringLiteral("video/x-ms-wmv");
    case LIBMTP_FILETYPE_AVI:
        return QStringLiteral("video/x-msvideo");
    case LIBMTP_FILETYPE_MPEG:
        return QStringLiteral("video/mpeg");
    case LIBMTP_FILETYPE_ASF:
        return QStringLiteral("video/x-ms-asf");
    case LIBMTP_FILETYPE_QT:
        return QStringLiteral("video/quicktime");
    case LIBMTP_FILETYPE_UNDEF_VIDEO:
        return QString();
    case LIBMTP_FILETYPE_JPEG:
        return QStringLiteral("image/jpeg");
    case LIBMTP_FILETYPE_JFIF:
        return QString();
    case LIBMTP_FILETYPE_TIFF:
        return QStringLiteral("image/tiff");
    case LIBMTP_FILETYPE_BMP:
        return QStringLiteral("image/bmp");
    case LIBMTP_FILETYPE_GIF:
        return QStringLiteral("image/gif");
    case LIBMTP_FILETYPE_PICT:
        return QStringLiteral("image/x-pict");
    case LIBMTP_FILETYPE_PNG:
        return QStringLiteral("image/png");
    case LIBMTP_FILETYPE_VCALENDAR1:
        return QStringLiteral("text/x-vcalendar");
    case LIBMTP_FILETYPE_VCALENDAR2:
        return QStringLiteral("text/x-vcalendar");
    case LIBMTP_FILETYPE_VCARD2:
        return QStringLiteral("text/x-vcard");
    case LIBMTP_FILETYPE_VCARD3:
        return QStringLiteral("text/x-vcard");
    case LIBMTP_FILETYPE_WINDOWSIMAGEFORMAT:
        return QStringLiteral("image/x-wmf");
    case LIBMTP_FILETYPE_WINEXEC:
        return QStringLiteral("application/x-ms-dos-executable");
    case LIBMTP_FILETYPE_TEXT:
        return QStringLiteral("text/plain");
    case LIBMTP_FILETYPE_HTML:
        return QStringLiteral("text/html");
    case LIBMTP_FILETYPE_FIRMWARE:
        return QString();
    case LIBMTP_FILETYPE_AAC:
        return QStringLiteral("audio/aac");
    case LIBMTP_FILETYPE_MEDIACARD:
        return QString();
    case LIBMTP_FILETYPE_FLAC:
        return QStringLiteral("audio/flac");
    case LIBMTP_FILETYPE_MP2:
        return QStringLiteral("video/mpeg");
    case LIBMTP_FILETYPE_M4A:
        return QStringLiteral("audio/mp4");
    case LIBMTP_FILETYPE_DOC:
        return QStringLiteral("application/msword");
    case LIBMTP_FILETYPE_XML:
        return QStringLiteral("text/xml");
    case LIBMTP_FILETYPE_XLS:
        return QStringLiteral("application/vnd.ms-excel");
    case LIBMTP_FILETYPE_PPT:
        return QStringLiteral("application/vnd.ms-powerpoint");
    case LIBMTP_FILETYPE_MHT:
        return QString();
    case LIBMTP_FILETYPE_JP2:
        return QStringLiteral("image/jpeg2000");
    case LIBMTP_FILETYPE_JPX:
        return QStringLiteral("application/x-jbuilder-project");
    case LIBMTP_FILETYPE_UNKNOWN:
    default:
        return QString();
    }
}

/**
 * @brief Creates @ref KMTPFile from LIBMTP_file_t
 *
 * @param file  Must not be a nullptr!
 */
static KMTPFile createMTPFile(const LIBMTP_file_t *file)
{
    return KMTPFile(file->item_id,
                    file->parent_id,
                    file->storage_id,
                    file->filename,
                    file->filesize,
                    file->modificationdate,
                    getMimetype(file->filetype));
}

MTPStorage::MTPStorage(const QString &dbusObjectPath, const LIBMTP_devicestorage_t *mtpStorage, MTPDevice *parent)
    : QObject(parent),
      m_dbusObjectPath(dbusObjectPath)
{
    setStorageProperties(mtpStorage);

    qDBusRegisterMetaType<KMTPFile>();
    qDBusRegisterMetaType<KMTPFileList>();

    new StorageAdaptor(this);
    QDBusConnection::sessionBus().registerObject(m_dbusObjectPath, this);
}

QString MTPStorage::dbusObjectPath() const
{
    return m_dbusObjectPath;
}

QString MTPStorage::description() const
{
    return m_description;
}

quint64 MTPStorage::maxCapacity() const
{
    return m_maxCapacity;
}

quint64 MTPStorage::freeSpaceInBytes()
{
    updateStorageInfo();
    return m_freeSpaceInBytes;
}

void MTPStorage::setStorageProperties(const LIBMTP_devicestorage_t *storage)
{
    m_id = storage->id;
    m_maxCapacity = storage->MaxCapacity;
    m_freeSpaceInBytes = storage->FreeSpaceInBytes;
    m_description = QString::fromUtf8(storage->StorageDescription);
}

void MTPStorage::updateStorageInfo()
{
    if (!LIBMTP_Get_Storage(getDevice(), LIBMTP_STORAGE_SORTBY_NOTSORTED)) {
        for (const LIBMTP_devicestorage_t *storage = getDevice()->storage; storage != nullptr; storage = storage->next) {
            if (m_id == storage->id) {
                qCDebug(LOG_KIOD_KMTPD) << "storage info updated";
                setStorageProperties(storage);
                break;
            }
        }
    }
}

LIBMTP_mtpdevice_t *MTPStorage::getDevice() const
{
    return static_cast<MTPDevice *>(parent())->getDevice();
}

KMTPFile MTPStorage::getFileFromPath(const QString &path)
{
    const QStringList pathItems = path.split(QLatin1Char('/'), Qt::SkipEmptyParts);

    // don't handle the root directory
    if (!pathItems.isEmpty()) {

        // 1. check if the file is in the cache
        const quint32 itemId = queryPath(path);
        if (itemId) {
            qCDebug(LOG_KIOD_KMTPD) << "Match found in cache, checking device";

            LIBMTP_file_t *file = LIBMTP_Get_Filemetadata(getDevice(), itemId);
            if (file) {
                qCDebug(LOG_KIOD_KMTPD) << "Found file in cache";
                const KMTPFile mtpFile = createMTPFile(file);
                LIBMTP_destroy_file_t(file);
                return mtpFile;
            }
        }

        // 2. query cache for parent
        else if (pathItems.size() > 1) {
            QString parentPath = convertToPath(pathItems, pathItems.size() - 1);
            quint32 parentId = queryPath(parentPath);

            qCDebug(LOG_KIOD_KMTPD)  << "Match for parent found in cache, checking device. Parent id = " << parentId;

            LIBMTP_file_t *parent = LIBMTP_Get_Filemetadata(getDevice(), parentId);
            if (parent) {
                qCDebug(LOG_KIOD_KMTPD) << "Found parent in cache";

                const KMTPFileList list = getFilesAndFoldersCached(parentPath, parentId);
                const auto it = std::find_if(list.constBegin(), list.constEnd(), [pathItems](const KMTPFile & file) {
                    return file.filename() == pathItems.last();
                });

                LIBMTP_destroy_file_t(parent);
                if (it != list.constEnd()) {
                    qCDebug(LOG_KIOD_KMTPD) << "Found file from cached parent";
                    return *it;
                }
            }
        }
    }

    // 3. traverse further while depth not reached
    QString currentPath;
    quint32 currentParent = LIBMTP_FILES_AND_FOLDERS_ROOT;

    for (const QString &element : pathItems) {
        const KMTPFileList list = getFilesAndFoldersCached(currentPath, currentParent);
        auto it = std::find_if(list.constBegin(), list.constEnd(), [element] (const KMTPFile &file) {
            return file.filename() == element;
        });

        if (it != list.constEnd()) {
            currentParent = it->itemId();
        } else {
            qCDebug(LOG_KIOD_KMTPD) << "File not found!";
            return KMTPFile();
        }
        currentPath.append(QLatin1Char('/') + element);
    }

    LIBMTP_file_t *file = LIBMTP_Get_Filemetadata(getDevice(), currentParent);
    if (file) {
        qCDebug(LOG_KIOD_KMTPD) << "Found file using tree walk";

        const KMTPFile mtpFile = createMTPFile(file);
        LIBMTP_destroy_file_t(file);
        return mtpFile;
    }
    return KMTPFile();
}

KMTPFileList MTPStorage::getFilesAndFoldersCached(const QString &path, quint32 parentId)
{
    KMTPFileList mtpFiles;

    LIBMTP_file_t *tmp = nullptr;
    LIBMTP_file_t *file = LIBMTP_Get_Files_And_Folders(getDevice(), m_id, parentId);
    while (file != nullptr) {
        const KMTPFile mtpFile = createMTPFile(file);
        addPath(path + QLatin1Char('/') + mtpFile.filename(), mtpFile.itemId());
        mtpFiles.append(mtpFile);

        tmp = file;
        file = file->next;
        LIBMTP_destroy_file_t(tmp);
    }
    return mtpFiles;
}

quint32 MTPStorage::queryPath(const QString &path, int timeToLive)
{
    QPair< QDateTime, uint32_t > item = m_cache.value(path);

    if (item.second != 0) {
        QDateTime dateTime = QDateTime::currentDateTimeUtc();

        if (item.first > dateTime) {
            item.first = dateTime.addSecs(timeToLive);
            m_cache.insert(path, item);
            return item.second;
        } else {
            m_cache.remove(path);
            return 0;
        }
    }

    return 0;
}

void MTPStorage::addPath(const QString &path, quint32 id, int timeToLive)
{
    QDateTime dateTime = QDateTime::currentDateTimeUtc();
    dateTime = dateTime.addSecs(timeToLive);

    QPair< QDateTime, uint32_t > item(dateTime, id);

    m_cache.insert(path, item);
}

void MTPStorage::removePath(const QString &path)
{
    m_cache.remove(path);
}

KMTPFileList MTPStorage::getFilesAndFolders(const QString &path, int &result)
{
    result = 0;
    if (path.isEmpty() || path == QLatin1String("/")) {
        // list root directory
        return getFilesAndFoldersCached(path, LIBMTP_FILES_AND_FOLDERS_ROOT);
    }
    const KMTPFile file = getFileFromPath(path);
    if (!file.isValid()) {
        result = 1;     // not existing
        return KMTPFileList();
    } else if (!file.isFolder()) {
        result = 2;     // is file
        return KMTPFileList();
    }
    return getFilesAndFoldersCached(path, file.itemId());
}

KMTPFile MTPStorage::getFileMetadata(const QString &path)
{
    qCDebug(LOG_KIOD_KMTPD) << "getFileMetadata:" << path;
    return getFileFromPath(path);
}

int MTPStorage::getFileToHandler(const QString &path)
{
    qCDebug(LOG_KIOD_KMTPD) << "getFileToHandler:" << path;

    const KMTPFile source = getFileMetadata(path);
    if (source.isValid()) {
        const quint32 itemId = source.itemId();
        QTimer::singleShot(0, this, [this, itemId] {
            const int result = LIBMTP_Get_File_To_Handler(getDevice(), itemId, onDataPut, this, onDataProgress, this);
            if (result) {
                LIBMTP_Dump_Errorstack(getDevice());
                LIBMTP_Clear_Errorstack(getDevice());
            }
            Q_EMIT copyFinished(result);
        });
        return 0;
    }
    return 1;
}

int MTPStorage::getFileToFileDescriptor(const QDBusUnixFileDescriptor &descriptor, const QString &sourcePath)
{
    qCDebug(LOG_KIOD_KMTPD) << "getFileToFileDescriptor:" << sourcePath;

    const KMTPFile source = getFileMetadata(sourcePath);
    if (source.isValid()) {
        const quint32 itemId = source.itemId();

        // big files take some time to copy, and this may lead into D-Bus timeouts.
        // therefore the actual copying is not done within the D-Bus method itself but right after we return to the event loop
        QTimer::singleShot(0, this, [this, itemId, descriptor] {
            const int result = LIBMTP_Get_File_To_File_Descriptor(getDevice(), itemId, descriptor.fileDescriptor(), onDataProgress, this);
            if (result) {
                LIBMTP_Dump_Errorstack(getDevice());
                LIBMTP_Clear_Errorstack(getDevice());
            }
            Q_EMIT copyFinished(result);
        });
        return 0;
    }
    return 1;
}

int MTPStorage::sendFileFromFileDescriptor(const QDBusUnixFileDescriptor &descriptor, const QString &destinationPath)
{
    qCDebug(LOG_KIOD_KMTPD) << "sendFileFromFileDescriptor:" << destinationPath;

    QStringList destItems = destinationPath.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    if (destItems.isEmpty()) {
        return 1;
    }

    const QString filename = destItems.takeLast();
    uint32_t parentId = LIBMTP_FILES_AND_FOLDERS_ROOT;

    if (!destItems.isEmpty()) {
        // not root folder, search for parent folder
        const KMTPFile parent = getFileMetadata(convertToPath(destItems, destItems.size()));
        parentId = parent.itemId();
        if (!parent.isFolder()) {
            return 2;
        }
    }

    QTimer::singleShot(0, this, [this, parentId, descriptor, filename] {
        int result = 1;
        QT_STATBUF srcBuf;
        if (QT_FSTAT(descriptor.fileDescriptor(), &srcBuf) != -1) {
            const QDateTime lastModified = QDateTime::fromSecsSinceEpoch(srcBuf.st_mtim.tv_sec);

            LIBMTP_file_t *file = LIBMTP_new_file_t();
            file->parent_id = parentId;
            file->filename = qstrdup(filename.toUtf8().data());
            file->filetype = getFiletype(filename);
            file->filesize = quint64(srcBuf.st_size);
            file->modificationdate = lastModified.toSecsSinceEpoch();   // no matter what to set here, current time is taken
            file->storage_id = m_id;

            result = LIBMTP_Send_File_From_File_Descriptor(getDevice(), descriptor.fileDescriptor(), file, onDataProgress, this);
            LIBMTP_destroy_file_t(file);

            if (result) {
                LIBMTP_Dump_Errorstack(getDevice());
                LIBMTP_Clear_Errorstack(getDevice());
            }
        }
        Q_EMIT copyFinished(result);
    });

    return 0;
}

int MTPStorage::setFileName(const QString &path, const QString &newName)
{
    qCDebug(LOG_KIOD_KMTPD) << "setFileName:" << path << newName;

    const KMTPFile file = getFileFromPath(path);
    if (file.isValid()) {
        LIBMTP_file_t *source = LIBMTP_Get_Filemetadata(getDevice(), file.itemId());
        if (source) {
            const int result = LIBMTP_Set_File_Name(getDevice(), source, newName.toUtf8().constData());
            if (!result) {
                removePath(path);
                LIBMTP_destroy_file_t(source);
            }
            return result;
        }
    }
    return 1;
}

quint32 MTPStorage::createFolder(const QString &path)
{
    qCDebug(LOG_KIOD_KMTPD) << "createFolder:" << path;

    quint32 folderId = 0;
    const QStringList pathItems = path.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    const quint32 destinationId = queryPath(path);

    if (!pathItems.isEmpty() && !destinationId) {
        QByteArray dirName = pathItems.last().toUtf8();

        if (pathItems.size() == 1) {
            // create folder in device root
            folderId = LIBMTP_Create_Folder(getDevice(), dirName.data(), LIBMTP_FILES_AND_FOLDERS_ROOT, m_id);

        } else {
            const KMTPFile parentFolder = getFileMetadata(path.section(QLatin1Char('/'), 0, -2, QString::SectionIncludeLeadingSep));
            if (parentFolder.isFolder()) {
                folderId = LIBMTP_Create_Folder(getDevice(), dirName.data(), parentFolder.itemId(), m_id);
            }
        }

        if (folderId) {
            LIBMTP_Dump_Errorstack(getDevice());
            LIBMTP_Clear_Errorstack(getDevice());
        } else {
            addPath(path, folderId);
        }
    }
    return folderId;
}

int MTPStorage::deleteObject(const QString &path)
{
    qCDebug(LOG_KIOD_KMTPD) << "deleteObject:" << path;

    const KMTPFile file = getFileMetadata(path);
    const int ret = LIBMTP_Delete_Object(getDevice(), file.itemId());
    if (!ret) {
        removePath(path);
    }
    return ret;
}

#include "moc_mtpstorage.cpp"
