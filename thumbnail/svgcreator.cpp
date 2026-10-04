/*  This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2006 Pascal Létourneau <pascal.letourneau@kdemail.net>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "svgcreator.h"

#include <QImage>
#include <QPainter>
#include <QSvgRenderer>

#include <KPluginFactory>

K_PLUGIN_CLASS_WITH_JSON(SvgCreator, "svgthumbnail.json")

SvgCreator::SvgCreator(QObject *parent, const QVariantList &args)
    : KIO::ThumbnailCreator(parent, args)
{
}

KIO::ThumbnailResult SvgCreator::create(const KIO::ThumbnailRequest &request)
{
    QSvgRenderer r(request.url().toLocalFile());
    if (!r.isValid())
        return KIO::ThumbnailResult::fail();

    const QSize targetSize = request.targetSize() * request.devicePixelRatio();
    QSize renderSize = targetSize;

    const QSize defaultSize = r.defaultSize();
    if (defaultSize.isValid() && !defaultSize.isEmpty()) {
        const QSize scaled = defaultSize.scaled(targetSize, Qt::KeepAspectRatio);
        if (!scaled.isEmpty()) {
            renderSize = scaled;
        }
    }


    QImage img(renderSize, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);

    QPainter p(&img);
    r.render(&p, QRectF(QPointF(0, 0), renderSize));

    return KIO::ThumbnailResult::pass(img);
}

#include "moc_svgcreator.cpp"
#include "svgcreator.moc"
