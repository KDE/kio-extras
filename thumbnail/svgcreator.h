/*  This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2006 Pascal Létourneau <pascal.letourneau@kdemail.net>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef _SVGCREATOR_H_
#define _SVGCREATOR_H_

#include <KIO/ThumbnailCreator>

class SvgCreator : public KIO::ThumbnailCreator
{
    Q_OBJECT
public:
    SvgCreator(QObject *parent, const QVariantList &args);
    KIO::ThumbnailResult create(const KIO::ThumbnailRequest &request) override;
};

#endif
