// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2018 Andreas Krutzler <andreas.krutzler@gmx.net>

#pragma once

#include <kmtpfile.h>

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
        return {};
    case LIBMTP_FILETYPE_MP4:
        return QStringLiteral("video/mp4");
    case LIBMTP_FILETYPE_UNDEF_AUDIO:
        return {};
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
        return {};
    case LIBMTP_FILETYPE_JPEG:
        return QStringLiteral("image/jpeg");
    case LIBMTP_FILETYPE_JFIF:
        return {};
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
        return {};
    case LIBMTP_FILETYPE_AAC:
        return QStringLiteral("audio/aac");
    case LIBMTP_FILETYPE_MEDIACARD:
        return {};
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
        return {};
    case LIBMTP_FILETYPE_JP2:
        return QStringLiteral("image/jpeg2000");
    case LIBMTP_FILETYPE_JPX:
        return QStringLiteral("application/x-jbuilder-project");
    case LIBMTP_FILETYPE_ALBUM:
    case LIBMTP_FILETYPE_PLAYLIST:
    case LIBMTP_FILETYPE_UNKNOWN:
        return {};
    }
    return {};
}

static inline KMTPFile createKMTPFile(const std::unique_ptr<LIBMTP_file_t> &file)
{
    Q_ASSERT(file);
    return KMTPFile(file->item_id, file->parent_id, file->storage_id, file->filename, file->filesize, file->modificationdate, getMimetype(file->filetype));
}
