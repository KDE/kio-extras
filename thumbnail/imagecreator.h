/*  This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2000 Malte Starostik <malte@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef _IMAGECREATOR_H_
#define _IMAGECREATOR_H_

#include <QtGlobal>

#include <kio/thumbcreator.h>

class ImageCreator : public ThumbCreator
{
public:
    ImageCreator() {}
    bool create(const QString &path, int, int, QImage &img) override;
};

#endif
