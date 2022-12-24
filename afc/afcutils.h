/*
 * SPDX-FileCopyrightText: 2022 Kai Uwe Broulik <kde@broulik.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
*/


#pragma once

#include <QString>

#include <KIO/WorkerBase>

#include <libimobiledevice/afc.h>
#include <libimobiledevice/house_arrest.h>
#include <libimobiledevice/installation_proxy.h>
#include <libimobiledevice/lockdown.h>
#include <libimobiledevice/sbservices.h>

namespace AfcUtils
{

namespace Result
{
    KIO::WorkerResult from(const afc_error_t afcError, const QString &errorText = QString());
    KIO::WorkerResult from(const house_arrest_error_t houseArrestError, const QString &errorText = QString());
    KIO::WorkerResult from(const instproxy_error_t instProxyError, const QString &errorText = QString());
    KIO::WorkerResult from(const lockdownd_error_t lockdownError, const QString &errorText = QString());
    KIO::WorkerResult from(const sbservices_error_t springBoardError, const QString &errorText = QString());
} // namespace Result

} // namespace AfcUtils
