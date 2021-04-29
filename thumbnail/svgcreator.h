/*  This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2006 Pascal Létourneau <pascal.letourneau@kdemail.net>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef _SVGCREATOR_H_
#define _SVGCREATOR_H_

#include <kio/thumbcreator.h>

class SvgCreator : public ThumbCreator
{
public:
    SvgCreator() {}
    bool create(const QString &path, int w, int h, QImage &img) override;
};

#endif
