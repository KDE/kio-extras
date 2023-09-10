/*  This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2003 Fredrik Höglund <fredrik@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "cursorcreator.h"

#include <QFile>
#include <QImage>

#include <KPluginFactory>

#include <X11/Xcursor/Xcursor.h>
#include <X11/Xlib.h>

K_PLUGIN_CLASS_WITH_JSON(CursorCreator, "cursorthumbnail.json")

CursorCreator::CursorCreator(QObject *parent, const QVariantList &args)
    : KIO::ThumbnailCreator(parent, args)
{
}

KIO::ThumbnailResult CursorCreator::create(const KIO::ThumbnailRequest &request)
{
    const int width = request.targetSize().width();
    const int height = request.targetSize().height();

    XcursorImage *cursor = XcursorFilenameLoadImage(QFile::encodeName(request.url().toLocalFile()).data(), width > height ? height : width);

    if (cursor) {
        QImage img(reinterpret_cast<uchar *>(cursor->pixels), cursor->width, cursor->height, QImage::Format_ARGB32_Premultiplied);

        // Create a deep copy of the image so the image data is preserved
        img = img.copy();
        XcursorImageDestroy(cursor);
        return KIO::ThumbnailResult::pass(img);
    }

    return KIO::ThumbnailResult::fail();
}

#include "cursorcreator.moc"
#include "moc_cursorcreator.cpp"
