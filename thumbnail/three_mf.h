/*
 * SPDX-FileCopyrightText: 2018 Kai Uwe Broulik <kde@broulik.de>
 *
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#pragma once

#include <KIO/ThumbCreator>

class ThreemfCreator : public ThumbCreator
{
public:
    ThreemfCreator();
    ~ThreemfCreator() override;

    bool create(const QString &path, int width, int height, QImage &image) override;

};
