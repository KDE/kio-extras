/*  This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2008 Andre Gemünd <scroogie@gmail.com>
    SPDX-FileCopyrightText: 2016 Alexander Volkov <a.volkov@rusbitech.ru>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "jpegcreator.h"
#include "jpegcreatorsettings5.h"

#include <QImage>
#include <QImageReader>
#include <klocalizedstring.h>

extern "C"
{
    Q_DECL_EXPORT ThumbCreator *new_creator()
    {
        return new JpegCreator;
    }
}

JpegCreator::JpegCreator()
{
}

bool JpegCreator::create(const QString &path, int width, int height, QImage &image)
{
    QImageReader imageReader(path, "jpeg");

    const QSize imageSize = imageReader.size();
    if (imageSize.isValid() && (imageSize.width() > width || imageSize.height() > height)) {
        const QSize thumbnailSize = imageSize.scaled(width, height, Qt::KeepAspectRatio);
        imageReader.setScaledSize(thumbnailSize); // fast downscaling
    }
    imageReader.setQuality(75); // set quality so that the jpeg handler will use a high quality downscaler

    JpegCreatorSettings* settings = JpegCreatorSettings::self();
    settings->load();
    imageReader.setAutoTransform(settings->rotate());

    return imageReader.read(&image);
}
