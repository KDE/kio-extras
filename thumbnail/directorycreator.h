/*
    SPDX-FileCopyrightText: 2023 Nicolas Fella <nicolas.fella@gmx.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once
#include <KIO/ThumbnailCreator>
#include <KPluginFactory>

class DirectoryCreator : public KIO::ThumbnailCreator
{
    Q_OBJECT
public:
    DirectoryCreator(QObject *parent, const QVariantList &args);

    KIO::ThumbnailResult create(const KIO::ThumbnailRequest &request) override;
};
