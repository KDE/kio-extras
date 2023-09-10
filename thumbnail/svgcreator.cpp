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
    const double ratio = static_cast<double>(r.defaultSize().height()) / static_cast<double>(r.defaultSize().width());

    int w = request.targetSize().width();
    int h = request.targetSize().height();

    if (w < h)
        h = qRound(ratio * w);
    else
        w = qRound(h / ratio);

    QImage i(w, h, QImage::Format_ARGB32_Premultiplied);
    i.fill(0);
    QPainter p(&i);
    r.render(&p);

    return KIO::ThumbnailResult::pass(i);
}

#include "moc_svgcreator.cpp"
#include "svgcreator.moc"
