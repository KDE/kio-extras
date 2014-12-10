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

#ifndef KIO_MTP_HELPERS_H
#define KIO_MTP_HELPERS_H

#include "kio_mtp.h"

#include <libmtp.h>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(LOG_KIO_MTP)

int dataProgress(uint64_t const sent, uint64_t const, void const *const priv);
uint16_t dataPut(void *, void *priv, uint32_t sendlen, unsigned char *data, uint32_t *putlen);
uint16_t dataGet(void *, void *priv, uint32_t, unsigned char *data, uint32_t *gotlen);

QString convertToPath(const QStringList &pathItems, const int elements);

QString getMimetype(LIBMTP_filetype_t filetype);
LIBMTP_filetype_t getFiletype(const QString &filename);

QMap<QString, LIBMTP_devicestorage_t *> getDevicestorages(LIBMTP_mtpdevice_t *&device);
QMap<QString, LIBMTP_file_t *> getFiles(LIBMTP_mtpdevice_t *&device, uint32_t storage_id, uint32_t parent_id = 0xFFFFFFFF);

void getEntry(UDSEntry &entry, LIBMTP_mtpdevice_t *device);
void getEntry(UDSEntry &entry, const LIBMTP_devicestorage_t *storage);
void getEntry(UDSEntry &entry, const LIBMTP_file_t *file);

#endif // KIO_MTP_HELPERS_H
