/*
    windowsimagecreator.cpp - Thumbnail Creator for Microsoft Windows Images

    SPDX-FileCopyrightText: 2009 Pali Rohár <pali.rohar@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "windowsimagecreator.h"
#include "icoutils.h"
#include "macros.h"

#include <QString>
#include <QImage>
#include <QImageReader>
#include <QMimeDatabase>

EXPORT_THUMBNAILER_WITH_JSON(WindowsImageCreator, "windowsimagethumbnail.json")

bool WindowsImageCreator::create(const QString &path, int width, int height, QImage &img)
{
    if (IcoUtils::loadIcoImage(path, img, width, height)) {
        return true;
    }

    // Maybe it's an animated cursor
    if (QMimeDatabase().mimeTypeForFile(path).name() == QLatin1String("application/x-navi-animation")) {
        QImageReader reader(path, "ani");
        return reader.read(&img);
    }

    return false;

}
#include "windowsimagecreator.moc"
