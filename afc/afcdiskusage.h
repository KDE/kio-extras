/*
 * SPDX-FileCopyrightText: 2022 Kai Uwe Broulik <kde@broulik.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KIO/Global>

#include "afcclient.h"

class AfcDiskUsage
{
public:
    AfcDiskUsage();
    explicit AfcDiskUsage(const AfcClient::Ptr &client);

    bool isValid() const;

    KIO::filesize_t total() const;
    KIO::filesize_t free() const;

private:
    bool m_valid = false;

    KIO::filesize_t m_total = 0;
    KIO::filesize_t m_free = 0;
};
