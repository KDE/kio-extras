/*
 * SPDX-FileCopyrightText: 2022 Kai Uwe Broulik <kde@broulik.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "afcdevice.h"

#include "afc_debug.h"

#include "afcapp.h"
#include "afcutils.h"

#include <QScopeGuard>

#include <libimobiledevice/installation_proxy.h>

using namespace KIO;

static const char s_lockdownLabel[] = "kio_afc";

AfcDevice::AfcDevice(const QString &id)
    : m_id(id)
{
    idevice_new(&m_device, id.toUtf8().constData());
    if (!m_device) {
        qCWarning(KIO_AFC_LOG) << "Failed to create idevice for" << id;
        return;
    }

    lockdownd_client_t lockdowndClient = nullptr;
    auto ret = lockdownd_client_new(m_device, &lockdowndClient, s_lockdownLabel);
    if (ret != LOCKDOWN_E_SUCCESS) {
        qCWarning(KIO_AFC_LOG) << "Failed to create lockdown client for" << id << ret;
        return;
    }

    ScopedLockdowndClientPtr lockdowndClientPtr(lockdowndClient);

    char *name = nullptr;
    auto lockdownRet = lockdownd_get_device_name(lockdowndClientPtr.data(), &name);
    if (lockdownRet != LOCKDOWN_E_SUCCESS) {
        qCWarning(KIO_AFC_LOG) << "Failed to get device name for" << id << lockdownRet;
    } else {
        m_name = QString::fromUtf8(name);
        free(name);
    }

    plist_t deviceClassEntry = nullptr;
    lockdownRet = lockdownd_get_value(lockdowndClientPtr.data(), nullptr /*global domain*/, "DeviceClass", &deviceClassEntry);
    if (lockdownRet != LOCKDOWN_E_SUCCESS) {
        qCWarning(KIO_AFC_LOG) << "Failed to get device class for" << id << lockdownRet;
    } else {
        char *deviceClass = nullptr;
        plist_get_string_val(deviceClassEntry, &deviceClass);
        m_deviceClass = QString::fromUtf8(deviceClass);
        free(deviceClass);
    }
}

AfcDevice::~AfcDevice()
{
    if (m_afcClient) {
        afc_client_free(m_afcClient);
        m_afcClient = nullptr;
    }

    if (m_device) {
        idevice_free(m_device);
        m_device = nullptr;
    }
}

idevice_t AfcDevice::device() const
{
    return m_device;
}

QString AfcDevice::id() const
{
    return m_id;
}

bool AfcDevice::isValid() const
{
    return m_device && !m_name.isEmpty();
}

QString AfcDevice::name() const
{
    return m_name;
}

QString AfcDevice::deviceClass() const
{
    return m_deviceClass;
}

WorkerResult AfcDevice::handshake()
{
    if (!m_handshakeSuccessful) {
        lockdownd_client_t lockdownClient = nullptr;
        // libimobiledevice doesn't properly allow doing a handshake on an existing instance
        // Instead, create a new one, and when it works, swap the one created in the constructor with this one
        auto ret = lockdownd_client_new_with_handshake(m_device, &lockdownClient, s_lockdownLabel);
        if (ret != LOCKDOWN_E_SUCCESS) {
            qCWarning(KIO_AFC_LOG) << "Failed to create lockdownd client with handshake on" << id() << "- make sure the device is unlocked" << ret;
            return AfcUtils::Result::from(ret);
        }

        m_lockdowndClient.reset(lockdownClient);
        m_handshakeSuccessful = true;
    }

    return WorkerResult::pass();
}

WorkerResult AfcDevice::client(const QString &appId, AfcClient::Ptr &client)
{
    auto result = handshake();
    if (!result.success()) {
        return result;
    }

    if (m_lastClient && m_lastClient->appId() == appId) {
        client = m_lastClient;
        return WorkerResult::pass();
    }

    Q_ASSERT(m_lockdowndClient);

    AfcClient::Ptr clientPtr(new AfcClient(this));
    result = clientPtr->init(m_lockdowndClient.data(), appId);
    if (!result.success()) {
        return result;
    }

    m_lastClient = clientPtr;
    client = clientPtr;
    return WorkerResult::pass();
}

AfcApp AfcDevice::app(const QString &bundleId)
{
    auto it = m_apps.constFind(bundleId);
    if (it != m_apps.constEnd()) {
        return *it;
    }

    // Refresh cache
    QVector<AfcApp> appsList;
    if (!apps(appsList).success()) {
        return AfcApp();
    }

    // See if we know it now
    it = m_apps.constFind(bundleId);
    if (it != m_apps.constEnd()) {
        return *it;
    }

    return AfcApp();
}

WorkerResult AfcDevice::apps(QVector<AfcApp> &apps)
{
    auto result = handshake();
    if (!result.success()) {
        return result;
    }

    lockdownd_service_descriptor_t service = nullptr;
    auto ret = lockdownd_start_service(m_lockdowndClient.data(), INSTPROXY_SERVICE_NAME, &service);
    if (ret != LOCKDOWN_E_SUCCESS) {
        qCWarning(KIO_AFC_LOG) << "Failed to start instproxy for getting apps" << ret;
        return AfcUtils::Result::from(ret, m_id);
    }

    auto serviceCleanup = qScopeGuard([service] {
        lockdownd_service_descriptor_free(service);
    });

    instproxy_client_t instProxyClient;
    auto instRet = instproxy_client_new(m_device, service, &instProxyClient);
    if (instRet != INSTPROXY_E_SUCCESS) {
        qCWarning(KIO_AFC_LOG) << "Failed to create instproxy instance" << ret;
        return WorkerResult::fail(ERR_OUT_OF_MEMORY);
    }

    auto opts = instproxy_client_options_new();
    auto optsCleanup = qScopeGuard([opts] {
        instproxy_client_options_free(opts);
    });
    instproxy_client_options_add(opts, "ApplicationType", "User", nullptr);

    plist_t appsPlist = nullptr;
    instRet = instproxy_browse(instProxyClient, opts, &appsPlist);
    if (instRet != INSTPROXY_E_SUCCESS) {
        qCWarning(KIO_AFC_LOG) << "Failed to browse apps via instproxy" << instRet;
        return AfcUtils::Result::from(instRet);
    }

    auto instCleanup = qScopeGuard([appsPlist, instProxyClient] {
        plist_free(appsPlist);
        instproxy_client_free(instProxyClient);
    });

    apps.clear();

    const int count = plist_array_get_size(appsPlist);
    for (int i = 0; i < count; ++i) {
        plist_t appPlist = plist_array_get_item(appsPlist, i);
        AfcApp app(appPlist);
        if (!app.isValid()) {
            continue;
        }
        apps.append(app);
        m_apps.insert(app.bundleId(), app);
    }

    return WorkerResult::pass();
}
