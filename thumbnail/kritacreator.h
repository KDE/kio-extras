/*  This file is part of the Calligra project.
    SPDX-FileCopyrightText: 2015 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef _KRITACREATOR_H_
#define _KRITACREATOR_H_

#include <QtGlobal>

// KF
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
};

#endif
