/*
 * SPDX-FileCopyrightText: 2018 Kai Uwe Broulik <kde@broulik.de>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#pragma once

#include <KIO/ThumbnailCreator>

class OpenDocumentCreator : public KIO::ThumbnailCreator
{
    Q_OBJECT
public:
    OpenDocumentCreator(QObject *parent, const QVariantList &args);
    ~OpenDocumentCreator() override;

    KIO::ThumbnailResult create(const KIO::ThumbnailRequest &request) override;
};
