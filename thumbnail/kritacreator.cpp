/*  This file is part of the Calligra project.
    SPDX-FileCopyrightText: 2015 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kritacreator.h"

#include <kzip.h>

#include <memory>
#include <QImage>
#include <QIODevice>

extern "C"
{
    Q_DECL_EXPORT ThumbCreator *new_creator()
    {
        return new KritaCreator;
    }
}

KritaCreator::KritaCreator()
{
}

KritaCreator::~KritaCreator()
{
}

bool KritaCreator::create(const QString &path, int width, int height, QImage &image)
{
    // for now just rely on the rendered data inside the file,
    // do not load Krita code for rendering ourselves, as that currently (2.9)
    // means loading all plugins, resources etc.
    KZip zip(path);
    if (!zip.open(QIODevice::ReadOnly)) {
        return false;
    }

    // first check if normal thumbnail is good enough
    // ORA thumbnail?
    const KArchiveFile *entry = zip.directory()->file(QLatin1String("Thumbnails/thumbnail.png"));
    if (!entry) {
        // KRA thumbnail
        entry = zip.directory()->file(QLatin1String("preview.png"));
    }

    if (!entry) {
        return false;
    }

    std::unique_ptr<QIODevice> fileDevice{entry->createDevice()};
    bool thumbLoaded = image.load(fileDevice.get(), "PNG");
    // The requested size is a boundingbox, so meeting one size is sufficient
    if (thumbLoaded && ((image.width() >= width) || (image.height() >= height))) {
        return true;
    }

    entry = zip.directory()->file(QLatin1String("mergedimage.png"));
    if (entry) {
        QImage thumbnail;
        fileDevice.reset(entry->createDevice());
        thumbLoaded = thumbnail.load(fileDevice.get(), "PNG");
        if (thumbLoaded) {
            image = thumbnail;
            return true;
        }
    }

    return false;
}
