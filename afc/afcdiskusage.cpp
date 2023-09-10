/*
 * SPDX-FileCopyrightText: 2022 Kai Uwe Broulik <kde@broulik.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "afcdiskusage.h"

#include "afc_debug.h"

#include <QScopeGuard>

using namespace KIO;

AfcDiskUsage::AfcDiskUsage() = default;

AfcDiskUsage::AfcDiskUsage(const AfcClient::Ptr &client)
{
    char **info = nullptr;
    const auto ret = afc_get_device_info(client->internalClient(), &info);
    if (ret != AFC_E_SUCCESS) {
        qCWarning(KIO_AFC_LOG) << "Failed to get device info for free disk usage" << ret;
        return;
    }

    auto cleanup = qScopeGuard([info] {
        afc_dictionary_free(info);
    });

    bool totalFound = false;
    bool freeFound = false;

    for (int i = 0; info[i]; i += 2) {
        const auto *key = info[i];
        const auto *value = info[i + 1];

        if (!totalFound && strcmp(key, "FSTotalBytes") == 0) {
            m_total = atoll(value);
            totalFound = true;
        } else if (!freeFound && strcmp(key, "FSFreeBytes") == 0) {
            m_free = atoll(value);
            freeFound = true;
        }
    }

    m_valid = totalFound && freeFound;
}

bool AfcDiskUsage::isValid() const
{
    return m_valid;
}

filesize_t AfcDiskUsage::total() const
{
    return m_total;
}

filesize_t AfcDiskUsage::free() const
{
    return m_free;
}
