/*  This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2003 Fredrik Höglund <fredrik@kde.org>
    SPDX-FileCopyrightText: 2026 Kai Uwe Broulik <kde@broulik.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "cursorcreator.h"

#include <QFile>
#include <QImage>
#include <QImageIOHandler>
#include <QScopeGuard>

#include <KPluginFactory>

#include "../3rdparty/xcursor.h"

K_PLUGIN_CLASS_WITH_JSON(CursorCreator, "cursorthumbnail.json")

CursorCreator::CursorCreator(QObject *parent, const QVariantList &args)
    : KIO::ThumbnailCreator(parent, args)
{
}

KIO::ThumbnailResult CursorCreator::create(const KIO::ThumbnailRequest &request)
{
    const int desiredSize = std::max(request.targetSize().width(), request.targetSize().height());

    QFile file(request.url().toLocalFile());
    if (!file.open(QFile::ReadOnly)) {
        return KIO::ThumbnailResult::fail();
    }

    XcursorFile reader{
        .closure = &file,
        .read = [](XcursorFile *file, uint8_t *buffer, int len) -> int {
            QFile *device = static_cast<QFile *>(file->closure);
            return device->read(reinterpret_cast<char *>(buffer), len);
        },
        .skip = [](XcursorFile *file, long offset) -> XcursorBool {
            QFile *device = static_cast<QFile *>(file->closure);
            return device->skip(offset) != -1;
        },
        .seek = [](XcursorFile *file, long offset) -> XcursorBool {
            QFile *device = static_cast<QFile *>(file->closure);
            return device->seek(offset);
        },
    };

    XcursorImages *images = XcursorXcFileLoadImages(&reader, desiredSize * request.devicePixelRatio());
    if (!images) {
        return KIO::ThumbnailResult::fail();
    }

    auto cleanup = qScopeGuard([images] {
        XcursorImagesDestroy(images);
    });

    if (images->nimage <= 0) {
        return KIO::ThumbnailResult::fail();
    }

    const XcursorImage *nativeCursorImage = images->images[0];

    QImage image;
    if (!QImageIOHandler::allocateImage(QSize(nativeCursorImage->width, nativeCursorImage->height), QImage::Format_ARGB32_Premultiplied, &image)) {
        return KIO::ThumbnailResult::fail();
    }

    image.setDevicePixelRatio(request.devicePixelRatio());

    memcpy(image.bits(), nativeCursorImage->pixels, image.sizeInBytes());

    return KIO::ThumbnailResult::pass(image);
}

#include "cursorcreator.moc"
#include "moc_cursorcreator.cpp"
