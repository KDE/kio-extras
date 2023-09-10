/*  This file is part of the Calligra project.
    SPDX-FileCopyrightText: 2015 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kritacreator.h"

#include <KPluginFactory>
#include <KZip>

#include <QIODevice>
#include <QImage>
#include <memory>

K_PLUGIN_CLASS_WITH_JSON(KritaCreator, "kraorathumbnail.json")

KritaCreator::KritaCreator(QObject *parent, const QVariantList &args)
    : KIO::ThumbnailCreator(parent, args)
{
}

KritaCreator::~KritaCreator()
{
}

KIO::ThumbnailResult KritaCreator::create(const KIO::ThumbnailRequest &request)
{
    // for now just rely on the rendered data inside the file,
    // do not load Krita code for rendering ourselves, as that currently (2.9)
    // means loading all plugins, resources etc.
    KZip zip(request.url().toLocalFile());
    if (!zip.open(QIODevice::ReadOnly)) {
        return KIO::ThumbnailResult::fail();
    }

    // first check if normal thumbnail is good enough
    // ORA thumbnail?
    const KArchiveFile *entry = zip.directory()->file(QLatin1String("Thumbnails/thumbnail.png"));
    if (!entry) {
        // KRA thumbnail
        entry = zip.directory()->file(QLatin1String("preview.png"));
    }

    if (!entry) {
        return KIO::ThumbnailResult::fail();
    }

    std::unique_ptr<QIODevice> fileDevice{entry->createDevice()};
    QImage image;
    bool thumbLoaded = image.load(fileDevice.get(), "PNG");
    // The requested size is a boundingbox, so meeting one size is sufficient
    if (thumbLoaded && ((image.width() >= request.targetSize().width()) || (image.height() >= request.targetSize().height()))) {
        return KIO::ThumbnailResult::pass(image);
    }

    entry = zip.directory()->file(QLatin1String("mergedimage.png"));
    if (entry) {
        QImage thumbnail;
        fileDevice.reset(entry->createDevice());
        thumbLoaded = thumbnail.load(fileDevice.get(), "PNG");
        if (thumbLoaded) {
            return KIO::ThumbnailResult::pass(thumbnail);
        }
    }

    return KIO::ThumbnailResult::fail();
}

#include "kritacreator.moc"
#include "moc_kritacreator.cpp"
