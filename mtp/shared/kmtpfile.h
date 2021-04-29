/*
    This file is part of the KMTP framework, part of the KDE project.

    SPDX-FileCopyrightText: 2018 Andreas Krutzler <andreas.krutzler@gmx.net>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef KMTPFILE_H
#define KMTPFILE_H

#include <QDBusArgument>

/**
 * @brief A wrapper class for LIBMTP_file_t which provides streaming operators for D-Bus marshalling
 *
 * LIBMTP_file_t definitions with some exceptions:
 *  item_id: unchanged
 *  parent_id: unchanged
 *  storage_id: unchanged
 *  filename: represented as string
 *  filesize: unchanged
 *  modificationsdate: represented as an int64
 *  filetype: converted to a mime-string
 *  next: omitted as it is not necessary in a list
 */
class KMTPFile
{

public:
    KMTPFile();
    KMTPFile(const KMTPFile &other) = default;
    explicit KMTPFile(quint32 itemId,
                      quint32 parentId,
                      quint32 storageId,
                      const char *filename,
                      quint64 filesize,
                      qint64 modificationdate,
                      const QString &filetype);

    bool isValid() const;
    bool isFolder() const;

    quint32 itemId() const;
    quint32 parentId() const;
    quint32 storageId() const;
    QString filename() const;
    quint64 filesize() const;
    qint64  modificationdate() const;
    QString filetype() const;

private:
    quint32 m_itemId;           /**< Unique item ID */
    quint32 m_parentId;         /**< ID of parent folder */
    quint32 m_storageId;        /**< ID of storage holding this file */
    QString m_filename;         /**< Filename of this file */
    quint64 m_filesize;         /**< Size of file in bytes */
    qint64  m_modificationdate; /**< Date of last alteration of the file */
    QString m_filetype;         /**< Filetype used for the current file */

    friend QDBusArgument& operator<<(QDBusArgument &argument, const KMTPFile &mtpFile);
    friend const QDBusArgument& operator>>(const QDBusArgument &argument, KMTPFile &mtpFile);
};

typedef QList<KMTPFile> KMTPFileList;

Q_DECLARE_METATYPE(KMTPFile)
Q_DECLARE_METATYPE(KMTPFileList)

QDBusArgument& operator<<(QDBusArgument &argument, const KMTPFile &mtpFile);
const QDBusArgument& operator>>(const QDBusArgument &argument, KMTPFile &mtpFile);

QDBusArgument& operator<<(QDBusArgument &argument, const KMTPFileList &list);
const QDBusArgument& operator>>(const QDBusArgument &argument, KMTPFileList &list);

#endif // KMTPFILE_H
