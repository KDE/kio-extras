/*
 * SPDX-FileCopyrightText: 2018 Kai Uwe Broulik <kde@broulik.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <KIO/ThumbnailCreator>

class AppImageCreator : public KIO::ThumbnailCreator
{
    Q_OBJECT
public:
    AppImageCreator(QObject *parent, const QVariantList &args);
    ~AppImageCreator() override;

    KIO::ThumbnailResult create(const KIO::ThumbnailRequest &request) override;
};
