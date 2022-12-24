/*
 * SPDX-FileCopyrightText: 2022 Kai Uwe Broulik <kde@broulik.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <KIO/WorkerBase>

#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>
#include <libimobiledevice/sbservices.h>

class AfcSpringBoard
{
public:
    explicit AfcSpringBoard(idevice_t device, lockdownd_client_t lockdowndClient);
    ~AfcSpringBoard();

    KIO::WorkerResult result() const;
    KIO::WorkerResult fetchAppIconData(const QString &bundleId, QByteArray &data);

private:
    KIO::WorkerResult m_result = KIO::WorkerResult::fail();

    lockdownd_service_descriptor_t m_springBoardService = nullptr;
    sbservices_client_t m_springBoardClient = nullptr;
};
