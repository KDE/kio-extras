/*  This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2003 Fredrik Höglund <fredrik@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef _CURSORCREATOR_H_
#define _CURSORCREATOR_H_

#include <KIO/ThumbnailCreator>

class CursorCreator : public KIO::ThumbnailCreator
{
    Q_OBJECT
public:
    CursorCreator(QObject *parent, const QVariantList &args);
    KIO::ThumbnailResult create(const KIO::ThumbnailRequest &request) override;
    int run(const KIO::ThumbnailRequest &request, const QString &outputLocation);
};

#endif
