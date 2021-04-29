/*  This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2004 Brad Hards <bradh@frogmouth.net>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef _EXRCREATOR_H_
#define _EXRCREATOR_H_

#include <kio/thumbcreator.h>

class EXRCreator : public ThumbCreator
{
public:
    EXRCreator() {};
    bool create(const QString &path, int, int, QImage &img) override;
};

#endif
