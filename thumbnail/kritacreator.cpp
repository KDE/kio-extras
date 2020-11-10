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

// Qt
#include <QImage>
#include <QIODevice>
#include <QFile>

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

    QImage thumbnail;

    // first check if normal thumbnail is good enough
    // ORA thumbnail?
    const KArchiveEntry *entry = zip.directory()->entry(QLatin1String("Thumbnails/thumbnail.png"));
    // KRA thumbnail
    if (!entry || !entry->isFile()) {
        entry = zip.directory()->entry(QLatin1String("preview.png"));
    }

    if (!entry || !entry->isFile()) {
        return false;
    }

    const KZipFileEntry* fileZipEntry = static_cast<const KZipFileEntry*>(entry);
    bool thumbLoaded = image.loadFromData(fileZipEntry->data(), "PNG");
    // The requested size is a boundingbox, so meeting one size is sufficient
    if (thumbLoaded && ((image.width() >= width) || (image.height() >= height))) {
        return true;
    }

    entry = zip.directory()->entry(QLatin1String("mergedimage.png"));
    if (entry && entry->isFile()) {
        fileZipEntry = static_cast<const KZipFileEntry*>(entry);
        thumbLoaded = thumbnail.loadFromData(fileZipEntry->data(), "PNG");
        if (thumbLoaded) {
            image = thumbnail;
            return true;
        }
    }

    return false;
}
