/*  This file is part of the Calligra project.
    Copyright 2015 Friedrich W. H. Kossebau <kossebau@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
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
