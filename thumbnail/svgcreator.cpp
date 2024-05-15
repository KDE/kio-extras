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

    // render using the correct ratio
    auto defaultSize = r.defaultSize();
    double ratio = 1.0;
    if (defaultSize.height() < request.targetSize().height() && defaultSize.width() < request.targetSize().width()) {
        // scale to output size if size is smaller than output
        ratio = 1.0 * defaultSize.height() / defaultSize.width();
    } else {
        ratio = static_cast<double>(defaultSize.height()) / static_cast<double>(defaultSize.width());
    }

    int width = request.targetSize().width() * request.devicePixelRatio();
    int height = request.targetSize().height() * request.devicePixelRatio();

    if (defaultSize.height() > defaultSize.width())
        height = qRound(width / ratio);
    else
        width = qRound(height / ratio);

    QImage i(width, height, QImage::Format_ARGB32_Premultiplied);
    i.fill(0);
    QPainter p(&i);
    r.render(&p, QRectF(QPointF(0, 0), QSizeF(width, height)));

    return KIO::ThumbnailResult::pass(i);
}

#include "moc_svgcreator.cpp"
#include "svgcreator.moc"
