/*
 * Copyright (c) 2018 Kai Uwe Broulik <kde@broulik.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "opendocumentcreator.h"

#include <QImage>

#include <KZip>

extern "C"
{
    Q_DECL_EXPORT ThumbCreator *new_creator()
    {
        return new OpenDocumentCreator;
    }
}

OpenDocumentCreator::OpenDocumentCreator() = default;

OpenDocumentCreator::~OpenDocumentCreator() = default;

bool OpenDocumentCreator::create(const QString &path, int width, int height, QImage &image)
{
    Q_UNUSED(width);
    Q_UNUSED(height);

    KZip zip(path);
    if (!zip.open(QIODevice::ReadOnly)) {
        return false;
    }

    const KArchiveEntry *entry = zip.directory()->entry(QStringLiteral("Thumbnails/thumbnail.png"));

    if (!entry || !entry->isFile()) {
        return false;
    }

    const KZipFileEntry *zipFileEntry = static_cast<const KZipFileEntry *>(entry);
    return image.loadFromData(zipFileEntry->data(), "PNG");
}
