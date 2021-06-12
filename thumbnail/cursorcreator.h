/*  This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2003 Fredrik Höglund <fredrik@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef _CURSORCREATOR_H_
#define _CURSORCREATOR_H_

#include <kio/thumbcreator.h>

class CursorCreator : public ThumbCreator
{
public:
    CursorCreator() {}
    bool create( const QString &path, int, int, QImage &img ) override;
};

#endif

