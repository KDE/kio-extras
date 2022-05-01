/*
    SPDX-License-Identifier: LGPL-2.1-or-later OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2020 Stefan Brüns <stefan.bruens@rwth-aachen.de>
*/

#ifndef DJVUCREATOR_H__
#define DJVUCREATOR_H__

#include <KIO/ThumbnailCreator>

class DjVuCreator : public KIO::ThumbnailCreator
{
public:
    DjVuCreator(QObject *parent, const QVariantList &args);

    KIO::ThumbnailResult create(const KIO::ThumbnailRequest &request) override;
};

#endif
