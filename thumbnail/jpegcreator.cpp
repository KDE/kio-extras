/*  This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2008 Andre Gemünd <scroogie@gmail.com>
    SPDX-FileCopyrightText: 2016 Alexander Volkov <a.volkov@rusbitech.ru>
    SPDX-FileCopyrightText: 2022 Kai Uwe Broulik <kde@broulik.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "config-thumbnail.h"

#include "jpegcreator.h"
#include "jpegcreatorsettings5.h"

#include <QImage>
#include <QImageReader>

#include <KLocalizedString>
#include <KPluginFactory>

#if HAVE_KEXIV2
#include <KExiv2/KExiv2>
#endif

K_PLUGIN_CLASS_WITH_JSON(JpegCreator, "jpegthumbnail.json")

JpegCreator::JpegCreator(QObject *parent, const QVariantList &args)
    : KIO::ThumbnailCreator(parent, args)
{
}

KIO::ThumbnailResult JpegCreator::exifThumbnail(const KIO::ThumbnailRequest &request) const
{
#if HAVE_KEXIV2
    KExiv2Iface::KExiv2 exiv2Image(request.url().toLocalFile());
    QImage image = exiv2Image.getExifThumbnail(JpegCreatorSettings::self()->rotate());

    if (image.isNull()) {
        return KIO::ThumbnailResult::fail();
    }

    // skip embedded thumbnail if strictly smaller
    if (image.size().width() < request.targetSize().width()
        && image.size().height() < request.targetSize().height()) {
        return KIO::ThumbnailResult::fail();
    }

    return KIO::ThumbnailResult::pass(image);
#else
    Q_UNUSED(request)
    return KIO::ThumbnailResult::fail();
#endif // HAVE_KEXIV2
}

KIO::ThumbnailResult JpegCreator::imageReaderThumbnail(const KIO::ThumbnailRequest &request) const
{
    QImageReader imageReader(request.url().toLocalFile(), "jpeg");
    const QSize imageSize = imageReader.size();
    if (imageSize.isValid() && (imageSize.width() > request.targetSize().width() || imageSize.height() > request.targetSize().height())) {
        const QSize thumbnailSize = imageSize.scaled(request.targetSize(), Qt::KeepAspectRatio);
        imageReader.setScaledSize(thumbnailSize); // fast downscaling
    }
    imageReader.setQuality(75); // set quality so that the jpeg handler will use a high quality downscaler

    imageReader.setAutoTransform(JpegCreatorSettings::self()->rotate());

    QImage image = imageReader.read();

    if (image.isNull()) {
        return KIO::ThumbnailResult::fail();
    }

    return KIO::ThumbnailResult::pass(image);
}

KIO::ThumbnailResult JpegCreator::create(const KIO::ThumbnailRequest &request)
{
    JpegCreatorSettings::self()->load();

    if (auto result = exifThumbnail(request); result.isValid()) {
        return result;
    }

    if (auto result = imageReaderThumbnail(request); result.isValid()) {
        return result;
    }

    return KIO::ThumbnailResult::fail();
}

#include "jpegcreator.moc"
#include "moc_jpegcreator.cpp"
