/*
 * SPDX-FileCopyrightText: 2022 Kai Uwe Broulik <kde@broulik.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "afcutils.h"

#include "afc_debug.h"

#include <KLocalizedString>

using namespace KIO;

WorkerResult AfcUtils::Result::from(const afc_error_t afcError, const QString &errorText)
{
    switch (afcError) {
    case AFC_E_SUCCESS:
    case AFC_E_END_OF_DATA:
        return WorkerResult::pass();

    case AFC_E_UNKNOWN_ERROR:
        return WorkerResult::fail(ERR_UNKNOWN, errorText);

    case AFC_E_NO_RESOURCES:
    case AFC_E_NO_MEM:
        return WorkerResult::fail(ERR_OUT_OF_MEMORY, errorText);

    case AFC_E_READ_ERROR:
        return WorkerResult::fail(ERR_CANNOT_READ, errorText);
    case AFC_E_WRITE_ERROR:
        return WorkerResult::fail(ERR_CANNOT_WRITE, errorText);
    case AFC_E_OBJECT_NOT_FOUND:
        return WorkerResult::fail(ERR_DOES_NOT_EXIST, errorText);
    case AFC_E_OBJECT_IS_DIR:
        return WorkerResult::fail(ERR_IS_DIRECTORY, errorText);
    case AFC_E_PERM_DENIED:
        return WorkerResult::fail(ERR_ACCESS_DENIED, errorText);
    case AFC_E_SERVICE_NOT_CONNECTED :
        return WorkerResult::fail(ERR_CONNECTION_BROKEN, errorText);
    case AFC_E_OP_TIMEOUT:
        return WorkerResult::fail(ERR_SERVER_TIMEOUT, errorText);
    case AFC_E_OP_NOT_SUPPORTED:
        return WorkerResult::fail(ERR_UNSUPPORTED_ACTION, errorText);
    case AFC_E_OBJECT_EXISTS:
        return WorkerResult::fail(ERR_FILE_ALREADY_EXIST, errorText);
    case AFC_E_NO_SPACE_LEFT:
        return WorkerResult::fail(ERR_DISK_FULL, errorText);
    case AFC_E_IO_ERROR:
        return WorkerResult::fail(ERR_CONNECTION_BROKEN, errorText);
    case AFC_E_INTERNAL_ERROR:
        return WorkerResult::fail(ERR_INTERNAL_SERVER, errorText);
    case AFC_E_DIR_NOT_EMPTY:
        return WorkerResult::fail(ERR_CANNOT_RMDIR, errorText);
    case AFC_E_MUX_ERROR:
        return WorkerResult::fail(ERR_CANNOT_CONNECT, errorText);
//    case AFC_E_NOT_ENOUGH_DATA:
//        return WorkerResult::fail(ERR_UNKNOWN_INTERRUPT, errorText);
    default:
        qCWarning(KIO_AFC_LOG) << "Unhandled afc_error_t" << afcError;
        return WorkerResult::fail(ERR_INTERNAL, i18n("Unhandled AFC error code '%1'", afcError));
    }
}

WorkerResult AfcUtils::Result::from(const house_arrest_error_t houseArrestError, const QString &errorText)
{
    switch (houseArrestError) {
    case HOUSE_ARREST_E_SUCCESS:
        return WorkerResult::pass();
    case HOUSE_ARREST_E_CONN_FAILED:
        return WorkerResult::fail(ERR_CANNOT_CONNECT, errorText);
    default:
        qCWarning(KIO_AFC_LOG) << "Unhandled house_arrest_error_t" << houseArrestError;
        return WorkerResult::fail(ERR_INTERNAL, i18n("Unhandled housearrest error code '%1'", houseArrestError));
    }
}

WorkerResult AfcUtils::Result::from(const instproxy_error_t instProxyError, const QString &errorText)
{
    switch (instProxyError) {
    case INSTPROXY_E_SUCCESS:
        return WorkerResult::pass();
    case INSTPROXY_E_CONN_FAILED:
        return WorkerResult::fail(ERR_CANNOT_CONNECT, errorText);
    case INSTPROXY_E_RECEIVE_TIMEOUT:
        return WorkerResult::fail(ERR_SERVER_TIMEOUT, errorText);
    // We don't actually manage any apps (yet?), so most error codes don't apply to us
    default:
        qCWarning(KIO_AFC_LOG) << "Unhandled instproxy_error_t" << instProxyError;
        return WorkerResult::fail(ERR_INTERNAL, i18n("Unhandled instproxy error code '%1'", instProxyError));
    }
}

WorkerResult AfcUtils::Result::from(const lockdownd_error_t lockdownError, const QString &errorText)
{
    switch (lockdownError) {
    case LOCKDOWN_E_SUCCESS:
        return WorkerResult::pass();
    case LOCKDOWN_E_RECEIVE_TIMEOUT:
        return WorkerResult::fail(ERR_SERVER_TIMEOUT, errorText);
    case LOCKDOWN_E_MUX_ERROR:
        return WorkerResult::fail(ERR_CANNOT_CONNECT, errorText);
    case LOCKDOWN_E_PASSWORD_PROTECTED: {
        QString text = errorText;
        if (text.isEmpty()) {
            text = i18n("The device is locked. Please enter the passcode on the device and try again.");
        }
        return WorkerResult::fail(ERR_WORKER_DEFINED, text);
    }
    case LOCKDOWN_E_USER_DENIED_PAIRING: {
        QString text = errorText;
        if (text.isEmpty()) {
            text = i18n("You have denied this computer access to the device.");
        }
        return WorkerResult::fail(ERR_WORKER_DEFINED, text);
    }
    case LOCKDOWN_E_PAIRING_DIALOG_RESPONSE_PENDING: {
        QString text = errorText;
        if (text.isEmpty()) {
            text = i18n("You need to allow this computer to access the device. Please accept the prompt on the device and try again.");
        }
        return WorkerResult::fail(ERR_WORKER_DEFINED, text);
    }
    // lockdownd_client_new_with_handshake returns this when pairing failed rather than any of the errors above
    case LOCKDOWN_E_INVALID_HOST_ID: {
        QString text = errorText;
        if (text.isEmpty()) {
            text = i18n("Cannot acces the device. Make sure it is unlocked and allows this computer to access it.");
        }
        return WorkerResult::fail(ERR_WORKER_DEFINED, text);
    }
    // TODO LOCKDOWN_E_SESSION_INACTIVE
    default:
        qCWarning(KIO_AFC_LOG) << "Unhandled lockdownd_error_t" << lockdownError;
        return WorkerResult::fail(ERR_INTERNAL, i18n("Unhandled lockdownd code '%1'", lockdownError));
    }
}
