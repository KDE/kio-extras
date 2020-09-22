/*
    SPDX-License-Identifier: LGPL-2.1-or-later OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2020 Stefan Brüns <stefan.bruens@rwth-aachen.de>
*/

#ifndef DJVUCREATOR_H__
#define DJVUCREATOR_H__

#include <kio/thumbcreator.h>

class DjVuCreator : public ThumbCreator
{
public:
    DjVuCreator() {}
    bool create(const QString &path, int, int, QImage &img) override;
};

#endif
