/*
 * SPDX-FileCopyrightText: 2022 Kai Uwe Broulik <kde@broulik.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "afcfile.h"

#include "afc_debug.h"

#include "afcfilereader.h"
#include "afcutils.h"

#include <limits>

using namespace KIO;

AfcFileReader::AfcFileReader(const AfcClient::Ptr &client, uint64_t handle)
    : m_client(client)
    , m_handle(handle)
{
}

filesize_t AfcFileReader::size() const
{
    return m_size;
}

void AfcFileReader::setSize(filesize_t size)
{
    m_size = size;
    m_remainingSize = size;
    m_data.clear();
}

WorkerResult AfcFileReader::read()
{
    m_data.clear();

    if (m_remainingSize == 0) {
        return WorkerResult::pass();
    }

    int bytesToRead = std::numeric_limits<int>::max();
    if (m_remainingSize < static_cast<filesize_t>(bytesToRead)) {
        bytesToRead = m_remainingSize;
    }

    if (m_data.length() < bytesToRead) {
        m_data.resize(bytesToRead);
    }

    uint32_t bytesRead = 0;
    afc_error_t ret = afc_file_read(m_client->internalClient(), m_handle, m_data.data(), bytesToRead, &bytesRead);
    m_data.resize(bytesRead);

    if (ret != AFC_E_SUCCESS && ret != AFC_E_END_OF_DATA) {
        return AfcUtils::Result::from(ret);
    }

    m_remainingSize -= bytesRead;
    return WorkerResult::pass();
}

QByteArray AfcFileReader::data() const
{
    return m_data;
}

bool AfcFileReader::hasMore() const
{
    return m_remainingSize > 0;
}
