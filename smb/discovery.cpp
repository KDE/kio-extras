/*
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
    SPDX-FileCopyrightText: 2019 Harald Sitter <sitter@kde.org>
*/

#include "discovery.h"

Discovery::Discovery()
{
    qRegisterMetaType<Discovery::Ptr>("Discovery::Ptr");
}

Discovery::~Discovery() = default;

Discoverer::Discoverer() = default;
Discoverer::~Discoverer() = default;
