/*
 * SPDX-FileCopyrightText: 2018-2019 Kai Uwe Broulik <kde@broulik.de>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#include "opendocumentcreator.h"

#include <QImage>
#include <QScopedPointer>
#include <QXmlStreamReader>

#include <KPluginFactory>
#include <KZip>

K_PLUGIN_CLASS_WITH_JSON(OpenDocumentCreator, "opendocumentthumbnail.json")

OpenDocumentCreator::OpenDocumentCreator(QObject *parent, const QVariantList &args)
    : KIO::ThumbnailCreator(parent, args)
{
}

OpenDocumentCreator::~OpenDocumentCreator() = default;

KIO::ThumbnailResult OpenDocumentCreator::create(const KIO::ThumbnailRequest &request)
{
    KZip zip(request.url().toLocalFile());
    if (!zip.open(QIODevice::ReadOnly)) {
        return KIO::ThumbnailResult::fail();
    }

    // Open Document
    const KArchiveEntry *entry = zip.directory()->entry(QStringLiteral("Thumbnails/thumbnail.png"));

    if (entry && entry->isFile()) {
        const KZipFileEntry *zipFileEntry = static_cast<const KZipFileEntry *>(entry);
        QImage image;
        if (image.loadFromData(zipFileEntry->data(), "PNG")) {
            return KIO::ThumbnailResult::pass(image);
        }
    }

    // Open Packaging Conventions (e.g. Office "Open" XML)
    const KArchiveEntry *relsEntry = zip.directory()->entry(QStringLiteral("_rels/.rels"));
    if (relsEntry && relsEntry->isFile()) {
        const auto *relsFileEntry = static_cast<const KZipFileEntry *>(relsEntry);

        QScopedPointer<QIODevice> relsDevice(relsFileEntry->createDevice());

        QString thumbnailPath;

        QXmlStreamReader xml(relsDevice.data());
        while (!xml.atEnd() && !xml.hasError()) {
            xml.readNext();
            if (xml.isStartElement() && xml.name() == QLatin1String("Relationship")) {
                const auto attributes = xml.attributes();
                if (attributes.value(QStringLiteral("Type"))
                    == QLatin1String("http://schemas.openxmlformats.org/package/2006/relationships/metadata/thumbnail")) {
                    thumbnailPath = attributes.value(QStringLiteral("Target")).toString();
                    break;
                }
            }
        }

        if (!thumbnailPath.isEmpty()) {
            const auto *thumbnailEntry = zip.directory()->entry(thumbnailPath);
            if (thumbnailEntry && thumbnailEntry->isFile()) {
                QImage image;
                image.loadFromData(static_cast<const KZipFileEntry *>(thumbnailEntry)->data());
                return KIO::ThumbnailResult::pass(image);
            }
        }
    }

    return KIO::ThumbnailResult::fail();
}

#include "moc_opendocumentcreator.cpp"
#include "opendocumentcreator.moc"
