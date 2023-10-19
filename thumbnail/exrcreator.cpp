/*  This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2004 Brad Hards <bradh@frogmouth.net>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "exrcreator.h"
#include "thumbnail-exr-logsettings.h"

#include <QFile>
#include <QImage>

#include <ImfHeader.h>
#include <ImfInputFile.h>
#include <ImfPreviewImage.h>

#include <KConfigGroup>
#include <KPluginFactory>
#include <KSharedConfig>

#include <limits>

K_PLUGIN_CLASS_WITH_JSON(EXRCreator, "exrthumbnail.json")

EXRCreator::EXRCreator(QObject *parent, const QVariantList &args)
    : KIO::ThumbnailCreator(parent, args)
{
}

KIO::ThumbnailResult EXRCreator::create(const KIO::ThumbnailRequest &request)
{
    try {
        const QByteArray encodedPath = QFile::encodeName(request.url().toLocalFile());
        Imf::InputFile in(encodedPath.constData());
        const Imf::Header &h = in.header();
        if (h.hasPreviewImage()) {
            qCDebug(KIO_THUMBNAIL_EXR_LOG) << "EXRcreator - using preview";
            const Imf::PreviewImage &preview = h.previewImage();
            QImage qpreview(preview.width(), preview.height(), QImage::Format_RGB32);
            for (unsigned int y = 0; y < preview.height(); y++) {
                for (unsigned int x = 0; x < preview.width(); x++) {
                    const Imf::PreviewRgba &q = preview.pixels()[x + (y * preview.width())];
                    qpreview.setPixel(x, y, qRgba(q.r, q.g, q.b, q.a));
                }
            }
            return KIO::ThumbnailResult::pass(qpreview);
        }
    } catch (const std::exception &e) {
        qCDebug(KIO_THUMBNAIL_EXR_LOG) << e.what();
        return KIO::ThumbnailResult::fail();
    }

    // do it the hard way
    // We ignore maximum size when just extracting the thumbnail
    // from the header, but it is very expensive to render large
    // EXR images just to turn it into an icon, so we go back
    // to honoring it in here.
    qCDebug(KIO_THUMBNAIL_EXR_LOG) << "EXRcreator - using original image";
    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    KConfigGroup configGroup(config, "PreviewSettings");
    const qint64 maxSize = configGroup.readEntry("MaximumSize", std::numeric_limits<qint64>::max());
    const qint64 fileSize = QFile(request.url().toLocalFile()).size();
    if ((fileSize > 0) && (fileSize < maxSize)) {
        QImage img;
        if (!img.load(request.url().toLocalFile())) {
            return KIO::ThumbnailResult::fail();
        }
        return KIO::ThumbnailResult::pass(img);
    } else {
        return KIO::ThumbnailResult::fail();
    }
}

#include "exrcreator.moc"
#include "moc_exrcreator.cpp"
