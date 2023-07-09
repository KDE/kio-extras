/*
    windowsimagecreator.cpp - Thumbnail Creator for Microsoft Windows Images

    SPDX-FileCopyrightText: 2009 Pali Rohár <pali.rohar@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "windowsimagecreator.h"
#include "icoutils.h"

#include <QString>
#include <QImage>
#include <QImageReader>
#include <QMimeDatabase>

#include <KPluginFactory>

K_PLUGIN_CLASS_WITH_JSON(WindowsImageCreator, "windowsimagethumbnail.json")

WindowsImageCreator::WindowsImageCreator(QObject *parent, const QVariantList &args)
    : KIO::ThumbnailCreator(parent, args)
{
}

KIO::ThumbnailResult WindowsImageCreator::create(const KIO::ThumbnailRequest &request)
{
    const QString path = request.url().toLocalFile();
    QImage img;
    if (IcoUtils::loadIcoImage(path, img, request.targetSize().width(), request.targetSize().height())) {
        return KIO::ThumbnailResult::pass(img);
    }

    // Maybe it's an animated cursor
    if (QMimeDatabase().mimeTypeForFile(path).name() == QLatin1String("application/x-navi-animation")) {
        QImageReader reader(path, "ani");
        reader.read(&img);
        return KIO::ThumbnailResult::pass(img);
    }

    return KIO::ThumbnailResult::fail();
}

#include "moc_windowsimagecreator.cpp"
#include "windowsimagecreator.moc"
