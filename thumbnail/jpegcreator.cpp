/*  This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2008 Andre Gemünd <scroogie@gmail.com>
    SPDX-FileCopyrightText: 2016 Alexander Volkov <a.volkov@rusbitech.ru>
    SPDX-FileCopyrightText: 2022 Kai Uwe Broulik <kde@broulik.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "config-thumbnail.h"

#include "jpegcreator.h"
#include "jpegcreatorsettings5.h"

#include "macros.h"

#include <QImage>
#include <QImageReader>
#include <KLocalizedString>

#if HAVE_KEXIV2
#include <KExiv2/KExiv2>
#endif

EXPORT_THUMBNAILER_WITH_JSON(JpegCreator, "jpegthumbnail.json")

JpegCreator::JpegCreator()
{
}

bool JpegCreator::exifThumbnail(const QString &path, QImage &image) const
{
#if HAVE_KEXIV2
    KExiv2Iface::KExiv2 exiv2Image(path);
    image = exiv2Image.getExifThumbnail(JpegCreatorSettings::self()->rotate());
    return !image.isNull();
#else
    Q_UNUSED(path)
    Q_UNUSED(image)
    return false;
#endif // HAVE_KEXIV2
}

bool JpegCreator::imageReaderThumbnail(const QString &path, int width, int height, QImage &image) const
{
    QImageReader imageReader(path, "jpeg");
    const QSize imageSize = imageReader.size();
    if (imageSize.isValid() && (imageSize.width() > width || imageSize.height() > height)) {
        const QSize thumbnailSize = imageSize.scaled(width, height, Qt::KeepAspectRatio);
        imageReader.setScaledSize(thumbnailSize); // fast downscaling
    }
    imageReader.setQuality(75); // set quality so that the jpeg handler will use a high quality downscaler

    imageReader.setAutoTransform(JpegCreatorSettings::self()->rotate());

    return imageReader.read(&image);
}

bool JpegCreator::create(const QString &path, int width, int height, QImage &image)
{
    JpegCreatorSettings::self()->load();

    if (exifThumbnail(path, image)) {
        return true;
    }

    if (imageReaderThumbnail(path, width, height, image)) {
        return true;
    }

    return false;
}

#include "jpegcreator.moc"
