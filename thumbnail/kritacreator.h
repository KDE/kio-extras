/*  This file is part of the Calligra project.
    Copyright 2015 Friedrich W. H. Kossebau <kossebau@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef _KRITACREATOR_H_
#define _KRITACREATOR_H_

#include <QtGlobal>

// KDE
#include <kio/thumbcreator.h>

/**
 * The Krita thumbnail creator can create thumbnails for krita and openraster images
 */
class KritaCreator : public ThumbCreator
{
public:
    KritaCreator();
    ~KritaCreator() override;

    bool create(const QString &path, int width, int height, QImage &image) override;
    Flags flags() const override;
};

#endif
