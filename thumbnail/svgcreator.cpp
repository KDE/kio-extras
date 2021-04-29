/*  This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2006 Pascal Létourneau <pascal.letourneau@kdemail.net>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "svgcreator.h"

#include <QImage>
#include <QPainter>
#include <QSvgRenderer>

extern "C"
{
    Q_DECL_EXPORT ThumbCreator *new_creator()
    {
        return new SvgCreator;
    }
}

bool SvgCreator::create(const QString &path, int w, int h, QImage &img)
{
    QSvgRenderer r(path);
    if ( !r.isValid() )
        return false;

    // render using the correct ratio
    const double ratio = static_cast<double>(r.defaultSize().height()) /
                         static_cast<double>(r.defaultSize().width());
    if (w < h)
        h = qRound(ratio * w);
    else
        w = qRound(h / ratio);

    QImage i(w, h, QImage::Format_ARGB32_Premultiplied);
    i.fill(0);
    QPainter p(&i);
    r.render(&p);
    img = i;
    return true;
}
