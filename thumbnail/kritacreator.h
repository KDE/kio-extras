/*  This file is part of the Calligra project.
    SPDX-FileCopyrightText: 2015 Friedrich W. H. Kossebau <kossebau@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef _KRITACREATOR_H_
#define _KRITACREATOR_H_

#include <QtGlobal>

// KF
#include <KIO/ThumbnailCreator>

/**
 * The Krita thumbnail creator can create thumbnails for krita and openraster images
 */
class KritaCreator : public KIO::ThumbnailCreator
{
    Q_OBJECT
public:
    KritaCreator(QObject *parent, const QVariantList &args);
    ~KritaCreator() override;

    KIO::ThumbnailResult create(const KIO::ThumbnailRequest &request) override;
};

#endif
