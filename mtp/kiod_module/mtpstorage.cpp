/*
    This file is part of the MTP KIOD module, part of the KDE project.

    SPDX-FileCopyrightText: 2018 Andreas Krutzler <andreas.krutzler@gmx.net>
    SPDX-FileCopyrightText: 2022-2023 Harald Sitter <sitter@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "mtpstorage.h"

#include <memory>
#include <span>

#include <QDateTime>
#include <qplatformdefs.h>

#include "config-mtp.h"
#include "kiod_kmtpd_debug.h"
#include "memory.h"
#include "mtpdevice.h"
#include "mtpfile.h"
#include "mtplister.h"
#include "storageadaptor.h"

#if defined(HAVE_LIBMTP_Get_Children)
static QString dbusErrorNOENT()
{
    return QStringLiteral("org.kde.kmtp.Error.NoEntry");
}

static QString dbusErrorNOTDIR()
{
    return QStringLiteral("org.kde.kmtp.Error.NotDirectory");
}
#else
static QString dbusErrorENOSYS()
{
    return QStringLiteral("org.kde.kmtp.Error.NotImplemented");
}
#endif

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

    // TODO: use QMimeDatabase and query the mimetype from there (still from the extension presumably)
    // TODO: merge the mime mapping tables from this function and the reverse function
    // TODO: map video/* and text/* and audio/* to the generic types (e.g. LIBMTP_FILETYPE_UNDEF_VIDEO) when not otherwise mapped
    //   (NOTE: from glancing at the libmtp code mapping a file type isn't actually all that useful TBH, it only appears used when
    //    no destination folder was given?)
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
    return qobject_cast<MTPDevice *>(parent())->getDevice();
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

            std::unique_ptr<LIBMTP_file_t> file(LIBMTP_Get_Filemetadata(getDevice(), itemId));
            if (file) {
                qCDebug(LOG_KIOD_KMTPD) << "Found file in cache";
                return createKMTPFile(file);
            }
        }

        // 2. query cache for parent
        else if (pathItems.size() > 1) {
            QString parentPath = convertToPath(pathItems, pathItems.size() - 1);
            quint32 parentId = queryPath(parentPath);

            qCDebug(LOG_KIOD_KMTPD)  << "Match for parent found in cache, checking device. Parent id = " << parentId;

            std::unique_ptr<LIBMTP_file_t> parent(LIBMTP_Get_Filemetadata(getDevice(), parentId));
            if (parent) {
                qCDebug(LOG_KIOD_KMTPD) << "Found parent in cache";

                const KMTPFileList list = getFilesAndFoldersCached(parentPath, parentId);
                const auto it = std::find_if(list.constBegin(), list.constEnd(), [pathItems](const KMTPFile &file) {
                    return file.filename() == pathItems.last();
                });

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
        std::optional<KMTPFile> optionalFile = findEntry(element, currentPath, currentParent);
        if (!optionalFile.has_value()) {
            qCDebug(LOG_KIOD_KMTPD) << "File not found!";
            return {};
        }

        currentParent = optionalFile->itemId();
        currentPath.append(QLatin1Char('/') + element);
    }

    std::unique_ptr<LIBMTP_file_t> file(LIBMTP_Get_Filemetadata(getDevice(), currentParent));
    if (file) {
        qCDebug(LOG_KIOD_KMTPD) << "Found file using tree walk";

        return createKMTPFile(file);
    }
    return {};
}

KMTPFileList MTPStorage::getFilesAndFoldersCached(const QString &path, quint32 parentId)
{
    KMTPFileList mtpFiles;

    std::unique_ptr<LIBMTP_file_t> file(LIBMTP_Get_Files_And_Folders(getDevice(), m_id, parentId));
    while (file != nullptr) {
        const KMTPFile mtpFile = createKMTPFile(file);
        addPath(path + QLatin1Char('/') + mtpFile.filename(), mtpFile.itemId());
        mtpFiles.append(mtpFile);

        file.reset(file->next);
    }
    return mtpFiles;
}

#if defined(HAVE_LIBMTP_Get_Children)
std::optional<KMTPFile> MTPStorage::findEntry(const QString &fileNeedle, const QString &parentPath, quint32 parentId)
{
    // Optimized version of finding. Using LIBMTP_Get_Children to get a list of all ids relatively quickly and then
    // iterate them for the needle - leading to an early return when the needle is found, and partial caching!
    uint32_t *children = nullptr;
    const auto childrenSize = LIBMTP_Get_Children(getDevice(), m_id, parentId, &children);
    if (childrenSize == -1) {
        return std::nullopt;
    }

    for (const auto &child : std::span<uint32_t>(children, childrenSize)) {
        const KMTPFile mtpFile = createKMTPFile(std::unique_ptr<LIBMTP_file_t>(LIBMTP_Get_Filemetadata(getDevice(), child)));
        addPath(parentPath + QLatin1Char('/') + mtpFile.filename(), mtpFile.itemId());
        if (mtpFile.filename() == fileNeedle) {
            return mtpFile;
        }
    }

    return std::nullopt;
}
#else
std::optional<KMTPFile> MTPStorage::findEntry(const QString &fileNeedle, const QString &parentPath, quint32 parentId)
{
    // Poor man's search function. This gets all metadata in advance meaning even if the needle is the first of N
    // entries we'll have gotten all N entries.
    const KMTPFileList list = getFilesAndFoldersCached(parentPath, parentId);
    auto it = std::find_if(list.constBegin(), list.constEnd(), [fileNeedle](const KMTPFile &file) {
        return file.filename() == fileNeedle;
    });
    if (it == list.cend()) {
        return std::nullopt;
    }
    return *it;
}
#endif

quint32 MTPStorage::queryPath(const QString &path, int timeToLive)
{
    QPair< QDateTime, uint32_t > item = m_cache.value(path);

    if (item.second != 0) {
        QDateTime dateTime = QDateTime::currentDateTimeUtc();

        if (item.first > dateTime) {
            item.first = dateTime.addSecs(timeToLive);
            m_cache.insert(path, item);
            return item.second;
        }
        m_cache.remove(path);
        return 0;
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
        return {};
    }
    if (!file.isFolder()) {
        result = 2;     // is file
        return {};
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
    if (!source.isValid()) {
        return 1;
    }

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

            std::unique_ptr<LIBMTP_file_t> file(LIBMTP_new_file_t());
            file->parent_id = parentId;
            file->filename = qstrdup(filename.toUtf8().data());
            file->filetype = getFiletype(filename);
            file->filesize = quint64(srcBuf.st_size);
            file->modificationdate = lastModified.toSecsSinceEpoch();   // no matter what to set here, current time is taken
            file->storage_id = m_id;

            result = LIBMTP_Send_File_From_File_Descriptor(getDevice(), descriptor.fileDescriptor(), file.get(), onDataProgress, this);

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
        std::unique_ptr<LIBMTP_file_t> source(LIBMTP_Get_Filemetadata(getDevice(), file.itemId()));
        if (source) {
            const int result = LIBMTP_Set_File_Name(getDevice(), source.get(), newName.toUtf8().constData());
            if (!result) {
                removePath(path);
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

QDBusObjectPath MTPStorage::getFilesAndFolders2(const QString &path)
{
#if defined(HAVE_LIBMTP_Get_Children)
    qint64 idToList = LIBMTP_FILES_AND_FOLDERS_ROOT;
    if (!path.isEmpty() && path != QLatin1String("/")) {
        const KMTPFile file = getFileFromPath(path);
        if (!file.isValid()) {
            sendErrorReply(dbusErrorNOENT(), path);
            return {};
        }
        if (!file.isFolder()) {
            sendErrorReply(dbusErrorNOTDIR(), path);
            return {};
        }
        idToList = file.itemId();
    }

    uint32_t *children = nullptr;
    const auto childrenSize = LIBMTP_Get_Children(getDevice(), m_id, idToList, &children);
    if (childrenSize == -1) {
        sendErrorReply(dbusErrorNOENT(), path);
        return {};
    }

    auto lister = new MTPLister(std::unique_ptr<uint32_t>(children), childrenSize, getDevice(), path, this);
    connect(lister, &MTPLister::entry, this, [this, path](const KMTPFile &file) {
        addPath(path + QLatin1Char('/') + file.filename(), file.itemId()); // add to cache
    });
    static quint64 id = 0;
    const QDBusObjectPath objectPath(QStringLiteral("/modules/kmtpd/Lister/%1").arg(id++));
    connection().registerObject(objectPath.path(), lister);
    return objectPath;
#else
    sendErrorReply(dbusErrorENOSYS(), QString::fromLatin1(Q_FUNC_INFO));
    return {};
#endif
}

#include "moc_mtpstorage.cpp"
