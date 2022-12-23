/*
 * SPDX-FileCopyrightText: 2018-2019 Kai Uwe Broulik <kde@broulik.de>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#include "three_mf.h"

#include "macros.h"

#include <QImage>
#include <QScopedPointer>
#include <QXmlStreamReader>


#include <KZip>

EXPORT_THUMBNAILER_WITH_JSON(ThreemfCreator, "three_mf.json")

ThreemfCreator::ThreemfCreator() = default;

ThreemfCreator::~ThreemfCreator() = default;

bool ThreemfCreator::create(const QString &path, int width, int height, QImage &image)
{
    Q_UNUSED(width);
    Q_UNUSED(height);

    KZip zip(path);
    if (!zip.open(QIODevice::ReadOnly)) {
        return false;
    }

    // Open Document
    const KArchiveEntry *entry = zip.directory()->entry(QStringLiteral("Metadata/thumbnail.png"));

    if (entry && entry->isFile()) {
        const KZipFileEntry *zipFileEntry = static_cast<const KZipFileEntry *>(entry);

        if (image.loadFromData(zipFileEntry->data(), "PNG")) {
            return true;
        }
    }

    return false;
}

#include "three_mf.moc"
