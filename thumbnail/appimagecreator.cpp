/*
 * SPDX-FileCopyrightText: 2018 Kai Uwe Broulik <kde@broulik.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "appimagecreator.h"

#include "macros.h"

#include <QImage>
#include <QScopedPointer>

#include <appimage/appimage.h>

EXPORT_THUMBNAILER_WITH_JSON(AppImageCreator, "appimagethumbnail.json")

AppImageCreator::AppImageCreator() = default;
AppImageCreator::~AppImageCreator() = default;

bool AppImageCreator::create(const QString &path, int width, int height, QImage &image)
{
    // We just load the .DirIcon verbatim and let the PreviewJob figure out scaling to required size if needed
    Q_UNUSED(width);
    Q_UNUSED(height);

    unsigned long size = 0L;
    char *buf = nullptr;

    bool ok = appimage_read_file_into_buffer_following_symlinks(qUtf8Printable(path),
              ".DirIcon",
              &buf,
              &size);

    QScopedPointer<char, QScopedPointerPodDeleter> cleanup(buf);
    Q_UNUSED(cleanup);

    if (!ok) {
        return false;
    }

    return image.loadFromData(reinterpret_cast<uchar*>(buf), size);
}

#include "appimagecreator.moc"
