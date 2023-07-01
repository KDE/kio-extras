/*
 * SPDX-FileCopyrightText: 2018 Kai Uwe Broulik <kde@broulik.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "appimagecreator.h"

#include <QImage>
#include <QScopedPointer>

#include <appimage/appimage.h>

#include <KPluginFactory>

K_PLUGIN_CLASS_WITH_JSON(AppImageCreator, "appimagethumbnail.json")

AppImageCreator::AppImageCreator(QObject *parent, const QVariantList &args)
    : KIO::ThumbnailCreator(parent, args)
{
}

AppImageCreator::~AppImageCreator() = default;

KIO::ThumbnailResult AppImageCreator::create(const KIO::ThumbnailRequest &request)
{
    unsigned long size = 0L;
    char *buf = nullptr;

    bool ok = appimage_read_file_into_buffer_following_symlinks(qUtf8Printable(request.url().toLocalFile()), ".DirIcon", &buf, &size);

    QScopedPointer<char, QScopedPointerPodDeleter> cleanup(buf);
    Q_UNUSED(cleanup);

    if (!ok) {
        return KIO::ThumbnailResult::fail();
    }

    QImage image;
    image.loadFromData(reinterpret_cast<uchar *>(buf), size);
    return KIO::ThumbnailResult::pass(image);
}

#include "appimagecreator.moc"
#include "moc_appimagecreator.cpp"
