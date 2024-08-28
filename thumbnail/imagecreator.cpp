/*  This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2000 Carsten Pfeiffer <pfeiffer@kde.org>
    SPDX-FileCopyrightText: 2000 Malte Starostik <malte@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "imagecreator.h"

#include <QCommandLineParser>
#include <QGuiApplication>
#include <QImageReader>

#include <KMemoryInfo>
#include <KPluginFactory>

ImageCreator::ImageCreator(QObject *parent, const QVariantList &args)
    : KIO::ThumbnailCreator(parent, args)
{
}

#define MiB(bytes) ((bytes)*1024ll * 1024ll)
#define GiB(bytes) (MiB(bytes) * 1024ll)

// When the ram check is disabled or not available, this is the expected default value of free RAM
#define DEFAULT_FREE_RAM GiB(2)

// The maximum usable RAM is the free RAM is divided by this number:
// if the calculated image size is greater than this value, the preview is skipped.
#define RAM_DIVISOR 3

// An image smaller than 64 MiB will be loaded even if the usable RAM check fails.
#define MINIMUM_GUARANTEED_SIZE MiB(64)

/**
 * @brief maximumThumbnailRam
 * Calculates the maximum RAM that can be used to generate the thumbnail.
 *
 * The value returned is a third of the available free RAM.
 */
qint64 maximumThumbnailRam()
{
    // read available RAM (physical free ram only)
    auto freeRam = DEFAULT_FREE_RAM;

    KMemoryInfo m;
    if (!m.isNull()) {
        freeRam = qint64(m.availablePhysical());
    }

    /*
     * NOTE 1: a minimal 64MiB image is always guaranteed (this small size should never cause OS thrashing).
     * NOTE 2: the freeRam is divided by 3 for the following reasons:
     *         - the image could be converted (e.g. when depth() != 32)
     *         - we don't want to use all free ram for a thumbnail :)
     */
    return std::max(MINIMUM_GUARANTEED_SIZE, freeRam / RAM_DIVISOR);
}

KIO::ThumbnailResult ImageCreator::create(const KIO::ThumbnailRequest &request)
{
    // create image preview
    QImageReader ir(request.url().toLocalFile());
    auto thumbSize = ir.size().scaled(request.targetSize().width(), request.targetSize().height(), Qt::AspectRatioMode::KeepAspectRatio);
    ir.setScaledSize(thumbSize);
    ir.setQuality(0);

    /* The idea is to read the free ram and try to avoid OS trashing when the
     * image is too big:
     * - Qt 6: we can simply limit the maximum size that image reader can handle.
     * - Qt 5: the image plugin that allows big images should help us by implementing
     *         the QImageIOHandler::Size option (TIFF, PSB and XCF already have).
     */
    auto ram = maximumThumbnailRam();
    QImageReader::setAllocationLimit(ram / 1024 / 1024);

    ir.setAutoTransform(true);
    ir.setDecideFormatFromContent(true);
    if (ir.format() == QByteArray("raw")) {
        // make preview generation of raw files ~3 times faster (requires setDecideFormatFromContent(true))
        ir.setQuality(1);
    }

    QImage img;
    ir.read(&img);

    if (!img.isNull()) {
        return KIO::ThumbnailResult::pass(img);
    }

    return KIO::ThumbnailResult::fail();
}

int main(int argc, char **argv)
{
    QGuiApplication::setDesktopSettingsAware(false);
    QGuiApplication application{argc, argv};
    application.setApplicationName(u"imagecreator"_qs);

    QCommandLineParser parser;
    parser.setApplicationDescription(u"Generates thumbnails for image files"_qs);
    parser.addHelpOption();
    parser.addOptions({
        {{u"i"_qs, u"input"_qs}, u"The input file"_qs, u"input"_qs},
        {{u"o"_qs, u"output"_qs}, u"The output file"_qs, u"output"_qs},
        {{u"s"_qs, u"size"_qs}, u"The width in pixels"_qs, u"size"_qs},
    });
    parser.process(application);

    auto input = QUrl::fromLocalFile(parser.value(u"input"_qs));
    auto output = QUrl::fromLocalFile(parser.value(u"output"_qs));
    int size = parser.value(u"size"_qs).toInt();
    parser.process(application);
    if (!input.isValid() || !output.isValid() || size <= 0) {
        qWarning() << "Expects an input and an output file";
        return 1;
    }

    auto req = KIO::ThumbnailRequest(input, QSize(size, size), "", 1.0, 1.0);
    ImageCreator creator(nullptr, QVariantList());
    auto c = creator.create(req);
    if (c.isValid()) {
        c.image().save(output.toLocalFile(), "png");
        return 0;
    } else {
        qWarning() << "Failed to generate a thumbnail!";
        return 1;
    }
}

#include "imagecreator.moc"
#include "moc_imagecreator.cpp"
