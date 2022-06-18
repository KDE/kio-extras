/*  This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2000 Carsten Pfeiffer <pfeiffer@kde.org>
    SPDX-FileCopyrightText: 2000 Malte Starostik <malte@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "imagecreator.h"
#include "macros.h"

#include <QImageReader>

#include <kcoreaddons_version.h>
#if KCOREADDONS_VERSION >= QT_VERSION_CHECK(5,95,0)
    #include <kmemoryinfo.h>
#endif

EXPORT_THUMBNAILER_WITH_JSON(ImageCreator, "imagethumbnail.json")

#define MiB(bytes) ((bytes) * 1024ll * 1024ll)
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
#ifdef KMEMORYINFO_H
    KMemoryInfo m;
    if (!m.isNull()) {
        freeRam = qint64(m.availablePhysical());
    }
#endif

    /*
     * NOTE 1: a minimal 64MiB image is always guaranteed (this small size should never cause OS thrashing).
     * NOTE 2: the freeRam is divided by 3 for the following reasons:
     *         - the image could be converted (e.g. when depth() != 32)
     *         - we don't want to use all free ram for a thumbnail :)
     */
    return std::max(MINIMUM_GUARANTEED_SIZE, freeRam / RAM_DIVISOR);
}

bool ImageCreator::create(const QString &path, int, int, QImage &img)
{
    // create image preview
    QImageReader ir(path);

    /* The idea is to read the free ram and try to avoid OS trashing when the
     * image is too big:
     * - Qt 6: we can simply limit the maximum size that image reader can handle.
     * - Qt 5: the image plugin that allows big images should help us by implementing
     *         the QImageIOHandler::Size option (TIFF, PSB and XCF already have).
     */
    auto ram = maximumThumbnailRam();
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    if (ir.supportsOption(QImageIOHandler::Size)) {
        auto size = ir.size();
        // euristic way: we always calculate the size supposing a 16-bits RGBA image
        if (size == QSize() || (size.width() * size.height() * 8ll > ram)) {
            return false;
        }
    }
#else
    QImageReader::setAllocationLimit(ram / 1024);
#endif

    ir.setAutoTransform(true);
    ir.setDecideFormatFromContent(true);
    ir.read(&img);
    if (!img.isNull() && img.depth() != 32) {
        img = img.convertToFormat(img.hasAlphaChannel() ? QImage::Format_ARGB32 : QImage::Format_RGB32);
    }
    return !img.isNull();
}

#include "imagecreator.moc"
