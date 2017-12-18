/*
 *  Helper implementations for KIO-MTP
 *  Copyright (C) 2013  Philipp Schmidt <philschmidt@gmx.net>
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

#include "kio_mtp_helpers.h"

int dataProgress(uint64_t const sent, uint64_t const, void const *const priv)
{
    ((MTPSlave *) priv)->processedSize(sent);

    return 0;
}

/**
 * MTPDataPutFunc callback function, "puts" data from the device somewhere else
 */
uint16_t dataPut(void *, void *priv, uint32_t sendlen, unsigned char *data, uint32_t *putlen)
{
    qCDebug(LOG_KIO_MTP) << "transferring" << sendlen << "bytes to data()";

    ((MTPSlave *) priv)->data(QByteArray((char *) data, (int) sendlen));
    *putlen = sendlen;

    return LIBMTP_HANDLER_RETURN_OK;
}

/**
 * MTPDataGetFunc callback function, "gets" data and puts it on the device
 */
uint16_t dataGet(void *, void *priv, uint32_t, unsigned char *data, uint32_t *gotlen)
{
    ((MTPSlave *) priv)->dataReq();

    QByteArray buffer;
    *gotlen = ((MTPSlave *) priv)->readData(buffer);

    qCDebug(LOG_KIO_MTP) << "transferring" << *gotlen << "bytes to data()";

    data = (unsigned char *) buffer.data();

    return LIBMTP_HANDLER_RETURN_OK;
}

QString convertToPath(const QStringList &pathItems, const int elements)
{
    QString path;

    for (int i = 0; i < elements && elements <= pathItems.size(); i++) {
        path.append(QLatin1Char('/'));
        path.append(pathItems.at(i));
    }

    return path;
}

QString getMimetype(LIBMTP_filetype_t filetype)
{
    switch (filetype) {
    case LIBMTP_FILETYPE_FOLDER:
        return QLatin1String("inode/directory");

    case LIBMTP_FILETYPE_WAV:
        return QLatin1String("audio/wav");
    case LIBMTP_FILETYPE_MP3:
        return QLatin1String("audio/x-mp3");
    case LIBMTP_FILETYPE_WMA:
        return QLatin1String("audio/x-ms-wma");
    case LIBMTP_FILETYPE_OGG:
        return QLatin1String("audio/x-vorbis+ogg");
    case LIBMTP_FILETYPE_AUDIBLE:
        return QLatin1String("");
    case LIBMTP_FILETYPE_MP4:
        return QLatin1String("audio/mp4");
    case LIBMTP_FILETYPE_UNDEF_AUDIO:
        return QLatin1String("");
    case LIBMTP_FILETYPE_WMV:
        return QLatin1String("video/x-ms-wmv");
    case LIBMTP_FILETYPE_AVI:
        return QLatin1String("video/x-msvideo");
    case LIBMTP_FILETYPE_MPEG:
        return QLatin1String("video/mpeg");
    case LIBMTP_FILETYPE_ASF:
        return QLatin1String("video/x-ms-asf");
    case LIBMTP_FILETYPE_QT:
        return QLatin1String("video/quicktime");
    case LIBMTP_FILETYPE_UNDEF_VIDEO:
        return QLatin1String("");
    case LIBMTP_FILETYPE_JPEG:
        return QLatin1String("image/jpeg");
    case LIBMTP_FILETYPE_JFIF:
        return QLatin1String("");
    case LIBMTP_FILETYPE_TIFF:
        return QLatin1String("image/tiff");
    case LIBMTP_FILETYPE_BMP:
        return QLatin1String("image/bmp");
    case LIBMTP_FILETYPE_GIF:
        return QLatin1String("image/gif");
    case LIBMTP_FILETYPE_PICT:
        return QLatin1String("image/x-pict");
    case LIBMTP_FILETYPE_PNG:
        return QLatin1String("image/png");
    case LIBMTP_FILETYPE_VCALENDAR1:
        return QLatin1String("text/x-vcalendar");
    case LIBMTP_FILETYPE_VCALENDAR2:
        return QLatin1String("text/x-vcalendar");
    case LIBMTP_FILETYPE_VCARD2:
        return QLatin1String("text/x-vcard");
    case LIBMTP_FILETYPE_VCARD3:
        return QLatin1String("text/x-vcard");
    case LIBMTP_FILETYPE_WINDOWSIMAGEFORMAT:
        return QLatin1String("image/x-wmf");
    case LIBMTP_FILETYPE_WINEXEC:
        return QLatin1String("application/x-ms-dos-executable");
    case LIBMTP_FILETYPE_TEXT:
        return QLatin1String("text/plain");
    case LIBMTP_FILETYPE_HTML:
        return QLatin1String("text/html");
    case LIBMTP_FILETYPE_FIRMWARE:
        return QLatin1String("");
    case LIBMTP_FILETYPE_AAC:
        return QLatin1String("audio/aac");
    case LIBMTP_FILETYPE_MEDIACARD:
        return QLatin1String("");
    case LIBMTP_FILETYPE_FLAC:
        return QLatin1String("audio/flac");
    case LIBMTP_FILETYPE_MP2:
        return QLatin1String("video/mpeg");
    case LIBMTP_FILETYPE_M4A:
        return QLatin1String("audio/mp4");
    case LIBMTP_FILETYPE_DOC:
        return QLatin1String("application/msword");
    case LIBMTP_FILETYPE_XML:
        return QLatin1String("text/xml");
    case LIBMTP_FILETYPE_XLS:
        return QLatin1String("application/vnd.ms-excel");
    case LIBMTP_FILETYPE_PPT:
        return QLatin1String("application/vnd.ms-powerpoint");
    case LIBMTP_FILETYPE_MHT:
        return QLatin1String("");
    case LIBMTP_FILETYPE_JP2:
        return QLatin1String("image/jpeg2000");
    case LIBMTP_FILETYPE_JPX:
        return QLatin1String("application/x-jbuilder-project");
    case LIBMTP_FILETYPE_UNKNOWN:
        return QLatin1String("");

    default:
        return QLatin1String("");
    }
}

LIBMTP_filetype_t getFiletype(const QString &filename)
{
    LIBMTP_filetype_t filetype;

    QString ptype = filename.split(QLatin1Char('.')).last();

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

QMap<QString, LIBMTP_devicestorage_t *> getDevicestorages(LIBMTP_mtpdevice_t *&device)
{
    qCDebug(LOG_KIO_MTP) << "[ENTER]" << (device == nullptr);

    QMap<QString, LIBMTP_devicestorage_t *> storages;
    if (device) {
        for (LIBMTP_devicestorage_t *storage = device->storage; storage != nullptr; storage = storage->next) {
            //             char *storageIdentifier = storage->VolumeIdentifier;
            char *storageDescription = storage->StorageDescription;

            QString storagename;
            //             if ( !storageIdentifier )
            storagename = QString::fromUtf8(storageDescription);
            //             else
            //                 storagename = QString::fromUtf8 ( storageIdentifier );

            qCDebug(LOG_KIO_MTP) << "found storage" << storagename;

            storages.insert(storagename, storage);
        }
    }

    qCDebug(LOG_KIO_MTP) << "[EXIT]" << storages.size();

    return storages;
}

QMap<QString, LIBMTP_file_t *> getFiles(LIBMTP_mtpdevice_t *&device, uint32_t storage_id, uint32_t parent_id)
{
    qCDebug(LOG_KIO_MTP) << "getFiles() for parent" << parent_id;

    QMap<QString, LIBMTP_file_t *> fileMap;

    LIBMTP_file_t *files = LIBMTP_Get_Files_And_Folders(device, storage_id, parent_id), *file;
    for (file = files; file != nullptr; file = file->next) {
        fileMap.insert(QString::fromUtf8(file->filename), file);
        //         qCDebug(LOG_KIO_MTP) << "found file" << file->filename;
    }

    qCDebug(LOG_KIO_MTP) << "[EXIT]";

    return fileMap;
}

void getEntry(UDSEntry &entry, LIBMTP_mtpdevice_t *device)
{
    char *charName = LIBMTP_Get_Friendlyname(device);
    char *charModel = LIBMTP_Get_Modelname(device);

    // prefer friendly devicename over model
    QString deviceName;
    if (!charName) {
        deviceName = QString::fromUtf8(charModel);
    } else {
        deviceName = QString::fromUtf8(charName);
    }

    entry.insert(UDSEntry::UDS_NAME, deviceName);
    entry.insert(UDSEntry::UDS_ICON_NAME, QLatin1String("multimedia-player"));
    entry.insert(UDSEntry::UDS_FILE_TYPE, S_IFDIR);
    entry.insert(UDSEntry::UDS_ACCESS, S_IRUSR | S_IRGRP | S_IROTH | S_IXUSR | S_IXGRP | S_IXOTH);
    entry.insert(UDSEntry::UDS_MIME_TYPE, QLatin1String("inode/directory"));
}

void getEntry(UDSEntry &entry, const LIBMTP_devicestorage_t *storage)
{
//     char *charIdentifier = storage->VolumeIdentifier;
    char *charDescription = storage->StorageDescription;

    QString storageName;
//     if ( !charIdentifier )
    storageName = QString::fromUtf8(charDescription);
//     else
//         storageName = QString::fromUtf8 ( charIdentifier );

    entry.insert(UDSEntry::UDS_NAME, storageName);
    entry.insert(UDSEntry::UDS_ICON_NAME, QLatin1String("drive-removable-media"));
    entry.insert(UDSEntry::UDS_FILE_TYPE, S_IFDIR);
    entry.insert(UDSEntry::UDS_ACCESS, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    entry.insert(UDSEntry::UDS_MIME_TYPE, QLatin1String("inode/directory"));
}

void getEntry(UDSEntry &entry, const LIBMTP_file_t *file)
{
    entry.insert(UDSEntry::UDS_NAME, QString::fromUtf8(file->filename));
    if (file->filetype == LIBMTP_FILETYPE_FOLDER) {
        entry.insert(UDSEntry::UDS_FILE_TYPE, S_IFDIR);
        entry.insert(UDSEntry::UDS_ACCESS, S_IRWXU | S_IRWXG | S_IRWXO);
        entry.insert(UDSEntry::UDS_MIME_TYPE, QLatin1String("inode/directory"));
    } else {
        entry.insert(UDSEntry::UDS_FILE_TYPE, S_IFREG);
        entry.insert(UDSEntry::UDS_ACCESS, S_IRUSR | S_IRGRP | S_IROTH | S_IXUSR | S_IXGRP | S_IXOTH);
        entry.insert(UDSEntry::UDS_SIZE, file->filesize);
        entry.insert(UDSEntry::UDS_MIME_TYPE, getMimetype(file->filetype));
    }
    entry.insert(UDSEntry::UDS_INODE, file->item_id);
    entry.insert(UDSEntry::UDS_ACCESS_TIME, file->modificationdate);
    entry.insert(UDSEntry::UDS_MODIFICATION_TIME, file->modificationdate);
    entry.insert(UDSEntry::UDS_CREATION_TIME, file->modificationdate);
}
