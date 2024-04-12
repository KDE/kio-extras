/*
 * SPDX-FileCopyrightText: 2022 Kai Uwe Broulik <kde@broulik.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "afcdevice.h"

#include "afc_debug.h"

#include "afcapp.h"
#include "afcspringboard.h"
#include "afcutils.h"

#include <QDir>
#include <QFileInfo>
#include <QSaveFile>
#include <QScopeGuard>
#include <QStandardPaths>

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

QString AfcDevice::cacheLocation() const
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + QLatin1String("/kio_afc/") + m_id;
}

QString AfcDevice::appIconCachePath(const QString &bundleId) const
{
    return cacheLocation() + QLatin1String("/%1.png").arg(bundleId);
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
    QList<AfcApp> appsList;
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

WorkerResult AfcDevice::apps(QList<AfcApp> &apps)
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

    instproxy_client_t instProxyClient = nullptr;
    auto instRet = instproxy_client_new(m_device, service, &instProxyClient);
    if (instRet != INSTPROXY_E_SUCCESS) {
        qCWarning(KIO_AFC_LOG) << "Failed to create instproxy instance" << instRet;
        return AfcUtils::Result::from(instRet);
    }

    auto instProxyCleanup = qScopeGuard([instProxyClient] {
        instproxy_client_free(instProxyClient);
    });

    auto opts = instproxy_client_options_new();
    auto optsCleanup = qScopeGuard([opts] {
        instproxy_client_options_free(opts);
    });
    instproxy_client_options_add(opts, "ApplicationType", "User", nullptr);

    // Browse apps.
    plist_t appsPlist = nullptr;
    instRet = instproxy_browse(instProxyClient, opts, &appsPlist);
    if (instRet != INSTPROXY_E_SUCCESS) {
        qCWarning(KIO_AFC_LOG) << "Failed to browse apps via instproxy" << instRet;
        return AfcUtils::Result::from(instRet);
    }

    auto appsPlistCleanup = qScopeGuard([appsPlist] {
        plist_free(appsPlist);
    });

    m_apps.clear();
    apps.clear();

    const int count = plist_array_get_size(appsPlist);
    m_apps.reserve(count);
    apps.reserve(count);
    for (int i = 0; i < count; ++i) {
        plist_t appPlist = plist_array_get_item(appsPlist, i);
        AfcApp app(appPlist);
        if (!app.isValid()) {
            continue;
        }

        const QString iconPath = appIconCachePath(app.bundleId());
        if (QFileInfo::exists(iconPath)) {
            app.m_iconPath = iconPath;
        }

        m_apps.insert(app.bundleId(), app);
        apps.append(app);
    }

    return WorkerResult::pass();
}

WorkerResult AfcDevice::fetchAppIcon(AfcApp &app)
{
    QList<AfcApp> apps{app};

    const auto result = fetchAppIcons(apps);
    if (!result.success()) {
        return result;
    }

    app.m_iconPath = apps.first().m_iconPath;
    return result;
}

WorkerResult AfcDevice::fetchAppIcons(QList<AfcApp> &apps)
{
    QStringList appIconsToFetch;

    for (const AfcApp &app : std::as_const(apps)) {
        if (app.iconPath().isEmpty()) {
            appIconsToFetch.append(app.bundleId());
        }
    }

    if (appIconsToFetch.isEmpty()) {
        // Nothing to do.
        return WorkerResult::pass();
    }

    qCDebug(KIO_AFC_LOG) << "About to fetch app icons for" << appIconsToFetch;

    AfcSpringBoard springBoard(m_device, m_lockdowndClient.data());
    if (!springBoard.result().success()) {
        return springBoard.result();
    }

    QDir cacheDir(cacheLocation());
    if (!cacheDir.mkpath(QStringLiteral("."))) { // Returns true if it already exists.
        qCWarning(KIO_AFC_LOG) << "Failed to create icon cache directory" << cacheLocation();
        return WorkerResult::fail(ERR_CANNOT_MKDIR, cacheLocation());
    }

    WorkerResult result = WorkerResult::pass();
    for (const QString &bundleId : appIconsToFetch) {
        QByteArray data;

        const auto fetchIconResult = springBoard.fetchAppIconData(bundleId, data);
        if (!fetchIconResult.success()) {
            result = fetchIconResult;
            continue;
        }

        if (data.isEmpty()) {
            result = WorkerResult::fail(ERR_CANNOT_READ); // NO_CONTENT is "success, but no content"
            continue;
        }

        // Basic sanity check whether we got a PNG file.
        if (!data.startsWith(QByteArrayLiteral("\x89PNG\x0d\x0a\x1a\x0a"))) {
            qCWarning(KIO_AFC_LOG) << "Got bogus app icon data for" << bundleId << data.left(20) << "...";
            result = WorkerResult::fail(ERR_CANNOT_READ);
            continue;
        }

        const QString path = appIconCachePath(bundleId);

        QSaveFile iconFile(path);
        if (!iconFile.open(QIODevice::WriteOnly)) {
            qCWarning(KIO_AFC_LOG) << "Failed to open icon cache file for writing" << path << iconFile.errorString();
            result = WorkerResult::fail(ERR_CANNOT_OPEN_FOR_WRITING, path);
            continue;
        }

        iconFile.write(data);

        if (!iconFile.commit()) {
            qCWarning(KIO_AFC_LOG) << "Failed to save icon cache of size" << data.count() << "to" << path;
            result = WorkerResult::fail(ERR_CANNOT_WRITE, path);
            continue;
        }

        // Update internal cache.
        auto &app = m_apps[bundleId];
        app.m_iconPath = path;

        // Update app list argument.
        auto it = std::find_if(apps.begin(), apps.end(), [&bundleId](const AfcApp &app) {
            return app.bundleId() == bundleId;
        });
        Q_ASSERT(it != apps.end());
        it->m_iconPath = path;
    }

    return result;
}
