/*
 * SPDX-FileCopyrightText: 2022 Kai Uwe Broulik <kde@broulik.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <KIO/Global>
#include <KIO/WorkerBase>

#include "afcclient.h"

class AfcFile;

class AfcFileReader
{
public:
    KIO::filesize_t size() const;
    void setSize(KIO::filesize_t size);

    KIO::WorkerResult read();
    bool hasMore() const;
    QByteArray data() const;

private:
    friend class AfcFile;
    AfcFileReader(const AfcClient::Ptr &client, uint64_t handle);

    AfcClient::Ptr m_client;
    uint64_t m_handle;
    KIO::filesize_t m_size = 0;
    KIO::filesize_t m_remainingSize = 0;

    QByteArray m_data;
};
