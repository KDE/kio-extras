/*  This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2003 Fredrik Höglund <fredrik@kde.org>
    SPDX-FileCopyrightText: 2026 Kai Uwe Broulik <kde@broulik.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "cursorcreator.h"

#include <QFile>
#include <QImage>

#include <KPluginFactory>

#include <optional>

K_PLUGIN_CLASS_WITH_JSON(CursorCreator, "cursorthumbnail.json")

static constexpr quint32 XCURSOR_MAGIC = 0x72756358; // "Xcur"
static constexpr quint32 XCURSOR_IMAGE_TYPE = 0xfffd0002;

struct XCursorHeader {
    quint32 magic;
    quint32 headerSize;
    quint32 version;
    quint32 ntoc;
};

struct XCursorTocEntry {
    quint32 type;
    quint32 size;
    quint32 position;
};

struct XCursorImageChunk {
    quint32 headerSize;
    quint32 type;
    quint32 subtype;
    quint32 version;
    quint32 width;
    quint32 height;
    quint32 xhot;
    quint32 yhot;
    quint32 delay;
    // pixels
};

QImage loadXCursorImage(const QString &path, int requestedSize)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    XCursorHeader header;
    stream >> header.magic >> header.headerSize >> header.version >> header.ntoc;

    if (header.magic != XCURSOR_MAGIC) {
        return {};
    }

    quint32 position = 0;
    std::optional<quint32> bestDiff;

    for (quint32 i = 0; i < header.ntoc; ++i) {
        XCursorTocEntry toc;
        stream >> toc.type >> toc.size >> toc.position;

        if (toc.type != XCURSOR_IMAGE_TYPE) {
            continue;
        }

        const int sizeDiff = std::abs(int(toc.size - requestedSize));
        if (!bestDiff || sizeDiff < bestDiff) {
            bestDiff = sizeDiff;
            position = toc.position;
        }
    }

    if (!bestDiff) {
        return {};
    }

    if (!file.seek(position)) {
        return {};
    }

    XCursorImageChunk imageChunk;
    stream >> imageChunk.headerSize >> imageChunk.type >> imageChunk.subtype >> imageChunk.version >> imageChunk.width >> imageChunk.height >> imageChunk.xhot
        >> imageChunk.yhot >> imageChunk.delay;

    if (imageChunk.type != XCURSOR_IMAGE_TYPE || imageChunk.width == 0 || imageChunk.height == 0) {
        return {};
    }

    QImage image(imageChunk.width, imageChunk.height, QImage::Format_ARGB32_Premultiplied);

    const qsizetype byteCount = imageChunk.width * imageChunk.height * sizeof(quint32);

    if (stream.readRawData(reinterpret_cast<char *>(image.bits()), byteCount) != byteCount) {
        return {};
    }

    return image;
}

CursorCreator::CursorCreator(QObject *parent, const QVariantList &args)
    : KIO::ThumbnailCreator(parent, args)
{
}

KIO::ThumbnailResult CursorCreator::create(const KIO::ThumbnailRequest &request)
{
    const QImage cursorImage = loadXCursorImage(request.url().toLocalFile(), std::max(request.targetSize().width(), request.targetSize().height()));
    if (cursorImage.isNull()) {
        return KIO::ThumbnailResult::fail();
    } else {
        return KIO::ThumbnailResult::pass(cursorImage);
    }
}

#include "cursorcreator.moc"
#include "moc_cursorcreator.cpp"
