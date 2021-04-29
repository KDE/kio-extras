/*
 * SPDX-FileCopyrightText: 2018 Kai Uwe Broulik <kde@broulik.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <kio/thumbcreator.h>

class AppImageCreator : public ThumbCreator
{
public:
    AppImageCreator();
    ~AppImageCreator() override;

    bool create(const QString &path, int width, int height, QImage &image) override;
};
