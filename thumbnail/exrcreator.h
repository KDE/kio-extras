/*  This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2004 Brad Hards <bradh@frogmouth.net>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef _EXRCREATOR_H_
#define _EXRCREATOR_H_

#include <KIO/ThumbnailCreator>

class EXRCreator : public KIO::ThumbnailCreator
{
    Q_OBJECT
public:
    EXRCreator(QObject *parent, const QVariantList &args);
    KIO::ThumbnailResult create(const KIO::ThumbnailRequest &request) override;
};

#endif
