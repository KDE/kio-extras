/*
 * SPDX-FileCopyrightText: 2022 Kai Uwe Broulik <kde@broulik.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "afcspringboard.h"
#include "afcutils.h"

#include "afc_debug.h"

#include <QScopeGuard>

#include <libimobiledevice/sbservices.h>

using namespace KIO;

AfcSpringBoard::AfcSpringBoard(idevice_t device, lockdownd_client_t lockdowndClient)
{
    auto lockdownRet = lockdownd_start_service(lockdowndClient, SBSERVICES_SERVICE_NAME, &m_springBoardService);
    if (lockdownRet != LOCKDOWN_E_SUCCESS) {
        qCWarning(KIO_AFC_LOG) << "Failed to start sbservices for getting app icons" << lockdownRet;
        m_result = AfcUtils::Result::from(lockdownRet);
        return;
    }

    auto springBoardRet = sbservices_client_new(device, m_springBoardService, &m_springBoardClient);
    if (springBoardRet != SBSERVICES_E_SUCCESS) {
        qCWarning(KIO_AFC_LOG) << "Failed to create sbservices instance for getting app icons" << springBoardRet;
        m_result = AfcUtils::Result::from(springBoardRet);
        return;
    }

    m_result = WorkerResult::pass();
}

AfcSpringBoard::~AfcSpringBoard()
{
    if (m_springBoardClient) {
        sbservices_client_free(m_springBoardClient);
    }
    if (m_springBoardService) {
        lockdownd_service_descriptor_free(m_springBoardService);
    }
}

WorkerResult AfcSpringBoard::result() const
{
    return m_result;
}

WorkerResult AfcSpringBoard::fetchAppIconData(const QString &bundleId, QByteArray &data)
{
    Q_ASSERT(m_springBoardClient);

    char *rawData = nullptr;
    uint64_t len = 0;

    auto dataCleanup = qScopeGuard([rawData] {
        free(rawData);
    });

    auto ret = sbservices_get_icon_pngdata(m_springBoardClient, bundleId.toUtf8().constData(), &rawData, &len);
    if (ret != SBSERVICES_E_SUCCESS || !rawData || len == 0) {
        qCWarning(KIO_AFC_LOG) << "Failed to get pngdata from" << bundleId << ret;
        return AfcUtils::Result::from(ret);
    }

    data = QByteArray::fromRawData(rawData, len);
    return WorkerResult::pass();
}
