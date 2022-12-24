/*
 * SPDX-FileCopyrightText: 2022 Kai Uwe Broulik <kde@broulik.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kio_afc.h"

#include "afc_debug.h"

#include "afcapp.h"
#include "afcdevice.h"
#include "afcfile.h"
#include "afcfilereader.h"
#include "afcurl.h"
#include "afcutils.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QMimeDatabase>
#include <QMimeType>
#include <QMutexLocker>
#include <QScopeGuard>

#include <KFileUtils>
#include <KLocalizedString>

#include <algorithm>

using namespace KIO;

// Pseudo plugin class to embed meta data
class KIOPluginForMetaData : public QObject
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kde.kio.worker.afc" FILE "afc.json")
};

using namespace KIO;
extern "C"
{
    int Q_DECL_EXPORT kdemain(int argc, char **argv)
    {
        QCoreApplication app(argc, argv);
        app.setApplicationName(QStringLiteral("kio_afc"));

        qCDebug(KIO_AFC_LOG) << "*** Starting kio_afc";

        if (argc != 4) {
            qCDebug(KIO_AFC_LOG) << "Usage: kio_afc protocol domain-socket1 domain-socket2";
            exit(-1);
        }

        AfcWorker worker(argv[2], argv[3]);
        worker.dispatchLoop();
        return 0;
    }
}

AfcWorker::AfcWorker(const QByteArray &poolSocket, const QByteArray &appSocket)
    : WorkerBase(QByteArrayLiteral("kio_afc"), poolSocket, appSocket)
{
    const auto result = init();
    Q_ASSERT(result.success());
}

AfcWorker::~AfcWorker()
{
    idevice_event_unsubscribe();

    qDeleteAll(m_devices);
    m_devices.clear();
}

Result AfcWorker::init()
{
    bool ok;
    const int logLevel = qEnvironmentVariableIntValue("KIO_AFC_LOG_VERBOSITY", &ok);
    if (ok) {
        idevice_set_debug_level(logLevel);
    }

    idevice_event_subscribe([](const idevice_event_t *event, void *user_data) {
        // NOTE this is executed in a different thread!
        static_cast<AfcWorker *>(user_data)->onDeviceEvent(event);
    }, this);

    updateDeviceList();

    return Result::pass();
}

void AfcWorker::onDeviceEvent(const idevice_event_t *event)
{
    // NOTE this is executed in a different thread!

    switch (event->event) {
    case IDEVICE_DEVICE_ADD:
        qCDebug(KIO_AFC_LOG) << "idevice event ADD for" << event->udid;
        addDevice(QString::fromLatin1(event->udid));
        return;
    case IDEVICE_DEVICE_REMOVE:
        qCDebug(KIO_AFC_LOG) << "idevice event REMOVE for" << event->udid;
        removeDevice(QString::fromLatin1(event->udid));
        return;
#if IMOBILEDEVICE_API >= QT_VERSION_CHECK(1, 3, 0)
    case IDEVICE_DEVICE_PAIRED:
        qCDebug(KIO_AFC_LOG) << "idevice event PAIRED for" << event->udid;
        return;
#endif
    }

    qCWarning(KIO_AFC_LOG) << "Unhandled idevice event" << event->event << "for" << event->udid;
}

Result AfcWorker::clientForUrl(const AfcUrl &afcUrl, AfcClient::Ptr &client) const
{
    AfcDevice *device = m_devices.value(deviceIdForFriendlyUrl(afcUrl));
    if (!device) {
        return Result::fail(ERR_DOES_NOT_EXIST, afcUrl.url().toDisplayString());
    }

    return device->client(afcUrl.appId(), client);
}

QString AfcWorker::deviceIdForFriendlyUrl(const AfcUrl &afcUrl) const
{
    QString deviceId = m_friendlyNames.value(afcUrl.device());
    if (deviceId.isEmpty()) {
        deviceId = afcUrl.device();
    }
    return deviceId;
}

QUrl AfcWorker::resolveSolidUrl(const QUrl &url) const
{
    const QString path = url.path();

    const QString prefix = QStringLiteral("udi=/org/kde/solid/imobile/");
    if (!path.startsWith(prefix)) {
        return {};
    }

    QString deviceId = path.mid(prefix.length());
    const int slashIdx = deviceId.indexOf(QLatin1Char('/'));
    if (slashIdx > -1) {
        deviceId = deviceId.left(slashIdx);
    }

    const QString friendlyName = m_friendlyNames.key(deviceId);

    QUrl newUrl;
    newUrl.setScheme(QStringLiteral("afc"));
    newUrl.setHost(!friendlyName.isEmpty() ? friendlyName : deviceId);
    // TODO would be nice to preserve subdirectories
    newUrl.setPath(QStringLiteral("/"));

    return newUrl;
}

bool AfcWorker::redirectIfSolidUrl(const QUrl &url)
{
    const QUrl redirectUrl = resolveSolidUrl(url);
    if (!redirectUrl.isValid()) {
        return false;
    }

    redirection(redirectUrl);
    return true;
}

UDSEntry AfcWorker::overviewEntry(const QString &fileName) const
{
    UDSEntry entry;
    entry.fastInsert(UDSEntry::UDS_NAME, !fileName.isEmpty() ? fileName : i18n("Apple Devices"));
    entry.fastInsert(UDSEntry::UDS_ICON_NAME, QStringLiteral("phone-apple-iphone"));
    entry.fastInsert(UDSEntry::UDS_FILE_TYPE, S_IFDIR);
    entry.fastInsert(UDSEntry::UDS_MIME_TYPE, QStringLiteral("inode/directory"));
    return entry;
}

UDSEntry AfcWorker::deviceEntry(const AfcDevice *device, const QString &fileName, bool asLink) const
{
    QString deviceId = m_friendlyNames.key(device->id());
    if (deviceId.isEmpty()) {
        deviceId = device->id();
    }
    const QString deviceClass = device->deviceClass();

    UDSEntry entry;
    entry.fastInsert(UDSEntry::UDS_NAME, !fileName.isEmpty() ? fileName : deviceId);
    if (!device->name().isEmpty()) {
        entry.fastInsert(UDSEntry::UDS_DISPLAY_NAME, device->name());
    }
    // TODO prettier
    entry.fastInsert(UDSEntry::UDS_DISPLAY_TYPE, deviceClass);
    entry.fastInsert(UDSEntry::UDS_FILE_TYPE, S_IFDIR);
    entry.fastInsert(UDSEntry::UDS_MIME_TYPE, QStringLiteral("inode/directory"));


    QString iconName;
    // We can assume iPod running iOS/supporting imobiledevice is an iPod touch?
    if (deviceClass.contains(QLatin1String("iPad"))) {
        iconName = QStringLiteral("computer-apple-ipad");
    } else if (deviceClass.contains(QLatin1String("iPod"))) {
       iconName = QStringLiteral("multimedia-player-apple-ipod-touch");
    } else {
        iconName = QStringLiteral("phone-apple-iphone");
    }

    entry.fastInsert(UDSEntry::UDS_ICON_NAME, iconName);

    if (asLink) {
        const QString contentsUrl = QStringLiteral("afc://%1/").arg(deviceId);
        entry.fastInsert(UDSEntry::UDS_LINK_DEST, contentsUrl);
        entry.fastInsert(UDSEntry::UDS_TARGET_URL, contentsUrl);
    }

    return entry;
}

UDSEntry AfcWorker::appsOverviewEntry(const AfcDevice *device, const QString &fileName) const
{
    QString deviceId = m_friendlyNames.key(device->id());
    if (deviceId.isEmpty()) {
        deviceId = device->id();
    }

    UDSEntry entry;
    entry.fastInsert(UDSEntry::UDS_NAME, !fileName.isEmpty() ? fileName : QStringLiteral("@apps"));
    entry.fastInsert(UDSEntry::UDS_DISPLAY_NAME, i18nc("Link to folder with files stored inside apps", "Apps"));
    entry.fastInsert(UDSEntry::UDS_ICON_NAME, QStringLiteral("folder-documents"));
    entry.fastInsert(UDSEntry::UDS_FILE_TYPE, S_IFDIR);

    const QString appsUrl = QStringLiteral("afc://%1:%2/").arg(deviceId).arg(static_cast<int>(AfcUrl::BrowseMode::Apps));
    entry.fastInsert(UDSEntry::UDS_LINK_DEST, appsUrl);
    entry.fastInsert(UDSEntry::UDS_TARGET_URL, appsUrl);

    return entry;
}

void AfcWorker::updateDeviceList()
{
    char **devices = nullptr;
    int count = 0;

    idevice_get_device_list(&devices, &count);
    for (int i = 0; i < count; ++i) {
        const QString id = QString::fromLatin1(devices[i]);
        addDevice(id);
    }

    if (devices) {
        idevice_device_list_free(devices);
    }
}

bool AfcWorker::addDevice(const QString &id)
{
    // NOTE this may be executed in a different thread when called from device idevice_event callback
    QMutexLocker locker(&m_mutex);

    if (m_devices.contains(id)) {
        return false;
    }

    auto *device = new AfcDevice(id);
    if (!device->isValid()) {
        delete device;
        return false;
    }

    m_devices.insert(id, device);

    Q_ASSERT(!device->name().isEmpty());

    // HACK URL host cannot contain spaces or non-ascii, and has to be lowercase.
    auto normalizeHost = [](const QString &name) {
        return QString::fromLatin1(name.toLatin1()).toLower().replace(QLatin1Char(' '), QLatin1Char('_'));
    };

    QString friendlyName = normalizeHost(device->name());
    // FIXME FIXME FIXME (my KCoreAddons is too old for KFileUtils::makeSuggestedName :)
    while (m_friendlyNames.contains(friendlyName)) {
        friendlyName = normalizeHost(KFileUtils::makeSuggestedName(friendlyName));
    }

    QUrl checkUrl;
    checkUrl.setHost(friendlyName);

    if (checkUrl.host().isEmpty()) {
        qCCritical(KIO_AFC_LOG) << "Failed to normalize" << device->name() << "into a valid URL host, this is a bug!";
    } else {
        m_friendlyNames.insert(friendlyName, id);
    }

    return true;
}

void AfcWorker::removeDevice(const QString &id)
{
    // NOTE this may be executed in a different thread when called from device idevice_event callback
    QMutexLocker locker(&m_mutex);

    auto *device = m_devices.take(id);
    if (device) {
        if (m_openFile && m_openFile->client()->device() == device) {
            m_openFile.reset();
        }
        delete device;

        auto it = std::find_if(m_friendlyNames.begin(), m_friendlyNames.end(), [&id](const QString &deviceId) {
            return deviceId == id;
        });
        if (it != m_friendlyNames.end()) {
            m_friendlyNames.erase(it);
        }
    }
}

Result AfcWorker::listDir(const QUrl &url)
{
    qCDebug(KIO_AFC_LOG) << "list directory:" << url;

    if (redirectIfSolidUrl(url)) {
        return Result::pass();
    }

    const AfcUrl afcUrl(url);
    if (!afcUrl.isValid()) {
        return Result::fail(ERR_MALFORMED_URL, url.toDisplayString());
    }

    // Don't have an empty path.
    // Otherwise we get "invalid URL" errors when trying to enter a subfolder.
    if (!url.host().isEmpty() && url.path().isEmpty()) {
        QUrl newUrl = url;
        newUrl.setPath(QStringLiteral("/"));
        redirection(newUrl);
        return Result::pass();
    }

    if (afcUrl.device().isEmpty()) {
        updateDeviceList();

        for (AfcDevice *device : m_devices) {
            UDSEntry entry = deviceEntry(device, QString(), true /*asLink*/);

            // When there is only one device, redirect to it right away
            if (m_devices.count() == 1) {
                redirection(QUrl(entry.stringValue(UDSEntry::UDS_TARGET_URL)));
                return Result::pass();
            }

            listEntry(entry);
        }

        // We cannot just list that at the beginning because we might do a redirect
        listEntry(overviewEntry(QStringLiteral(".")));

        return Result::pass();
    }

    AfcDevice *device = m_devices.value(deviceIdForFriendlyUrl(afcUrl));
    if (!device) {
        return Result::fail(ERR_DOES_NOT_EXIST, afcUrl.device());
    }

    const QString appId = afcUrl.appId();
    if (afcUrl.browseMode() == AfcUrl::BrowseMode::Apps && appId.isEmpty()) {
        QVector<AfcApp> apps;
        const auto result = device->apps(apps);
        if (!result.success()) {
            return result;
        }

        // Cannot browse apps without sharing enabled, don't list them.
        apps.erase(std::remove_if(apps.begin(),
                                  apps.end(),
                                  [](const AfcApp &app) {
                                      return !app.sharingEnabled();
                                  }),
                   apps.end());

        device->fetchAppIcons(apps);

        for (const auto &app : apps) {
            listEntry(app.entry());
        }

        listEntry(appsOverviewEntry(device, QStringLiteral(".")));
        return Result::pass();
    }

    AfcClient::Ptr client;
    Result result = device->client(appId, client);
    if (!result.success()) {
        return result;
    }

    // Ourself must be "."
    UDSEntry rootEntry;
    result = client->entry(afcUrl.path(), rootEntry);
    if (!result.success()) {
        return result;
    }

    // NOTE this must not be "fastInsert" as AfcDevice::entry already sets a name
    rootEntry.replace(UDSEntry::UDS_NAME, QStringLiteral("."));
    listEntry(rootEntry);

    QStringList files;
    result = client->entryList(afcUrl.path(), files);
    if (!result.success()) {
        // One can only access the "Documents" folder within an app, redirect to it if applicable
        if (result.error() == KIO::ERR_ACCESS_DENIED && !afcUrl.appId().isEmpty() && afcUrl.path().isEmpty()) {
            QUrl newUrl = url;
            newUrl.setPath(newUrl.path() + QLatin1String("/Documents"));
            qCDebug(KIO_AFC_LOG) << "Got access denied on app root folder, redirecting to Documents folder";

            redirection(newUrl);
            return Result::pass();
        }

        return result;
    }

    for (const QString &file : files) {
        QString absolutePath = afcUrl.path();
        if (!absolutePath.endsWith(QLatin1Char('/'))
                && !file.startsWith(QLatin1Char('/'))) {
            absolutePath.append(QLatin1Char('/'));
        }
        absolutePath.append(file);

        UDSEntry entry;
        result = client->entry(absolutePath, entry);
        if (!result.success()) {
            qCWarning(KIO_AFC_LOG) << "Failed to list" << absolutePath << result.error() << result.errorString();
            continue;
        }

        listEntry(entry);
    }

    // Add link to "Apps documents" to device root folder
    if (afcUrl.path().isEmpty()) {
        listEntry(appsOverviewEntry(device));
    }

    return Result::pass();
}

Result AfcWorker::stat(const QUrl &url)
{
    if (redirectIfSolidUrl(url)) {
        return Result::pass();
    }

    const AfcUrl afcUrl(url);
    if (!afcUrl.isValid()) {
        return Result::fail(ERR_MALFORMED_URL, url.toDisplayString());
    }

    // Device overview page afc:/
    if (afcUrl.device().isEmpty()) {
        statEntry(overviewEntry());
        return Result::pass();
    }

    AfcDevice *device = m_devices.value(deviceIdForFriendlyUrl(afcUrl));
    if (!device) {
        return Result::fail(ERR_DOES_NOT_EXIST, url.toDisplayString());
    }

    if (afcUrl.path().isEmpty()) {
        // Device file system or device app overview
        if (afcUrl.appId().isEmpty()) {
            UDSEntry rootEntry = deviceEntry(device);
            if (afcUrl.browseMode() == AfcUrl::BrowseMode::Apps) {
                rootEntry.replace(UDSEntry::UDS_DISPLAY_NAME, i18nc("Placeholder is device name", "%1 (Apps)", device->name()));
            }
            statEntry(rootEntry);
            return Result::pass();
        }

        // App folder
        AfcApp app = device->app(afcUrl.appId());
        if (!app.isValid()) {
            return Result::fail(KIO::ERR_DOES_NOT_EXIST, afcUrl.appId());
        }

        device->fetchAppIcon(app);

        UDSEntry appEntry = app.entry();
        statEntry(appEntry);
        return Result::pass();
    }

    AfcClient::Ptr client;
    auto result = device->client(afcUrl.appId(), client);
    if (!result.success()) {
        return result;
    }

    UDSEntry entry;
    result = client->entry(afcUrl.path(), entry);
    if (!result.success()) {
        return result;
    }

    statEntry(entry);
    return Result::pass();
}

void AfcWorker::guessMimeType(AfcFile &file, const QString &path)
{
    // Determine the mimetype of the file to be retrieved, and emit it.
    // This is mandatory in all workers...

    AfcFileReader reader = file.reader();
    reader.setSize(1024);
    const Result result = reader.read();
    if (result.success()) {
        QMimeDatabase db;
        const QString fileName = path.section(QLatin1Char('/'), -1, -1);
        QMimeType mime = db.mimeTypeForFileNameAndData(fileName, reader.data());
        mimeType(mime.name());
    }

    file.seek(0);
}

Result AfcWorker::get(const QUrl &url)
{
    if (redirectIfSolidUrl(url)) {
        return Result::pass();
    }

    const AfcUrl afcUrl(url);

    AfcClient::Ptr client;
    auto result = clientForUrl(afcUrl, client);
    if (!result.success()) {
        return result;
    }

    UDSEntry entry;
    result = client->entry(afcUrl.path(), entry);
    if (!result.success()) {
        return result;
    }

    AfcFile file(client, afcUrl.path());

    result = file.open(QIODevice::ReadOnly);
    if (!result.success()) {
        return result;
    }

    const auto size = entry.numberValue(UDSEntry::UDS_SIZE, 0);
    totalSize(size);

    guessMimeType(file, afcUrl.path());

    position(0);

    AfcFileReader reader = file.reader();
    reader.setSize(size);

    while (reader.hasMore()) {
        const auto result = reader.read();
        if (!result.success()) {
            return result;
        }
        data(reader.data());
    }

    return Result::pass();
}

Result AfcWorker::put(const QUrl &url, int permissions, JobFlags flags)
{
    Q_UNUSED(permissions);
    const AfcUrl afcUrl(url);

    AfcClient::Ptr client;
    Result result = clientForUrl(afcUrl, client);
    if (!result.success()) {
        return result;
    }

    UDSEntry entry;
    result = client->entry(afcUrl.path(), entry);

    const bool exists = result.error() != ERR_DOES_NOT_EXIST;
    if (exists && !flags.testFlag(Overwrite) && !flags.testFlag(Resume)) {
        if (S_ISDIR(entry.numberValue(UDSEntry::UDS_FILE_TYPE))) {
            return Result::fail(ERR_DIR_ALREADY_EXIST, afcUrl.path());
        }
        return Result::fail(ERR_FILE_ALREADY_EXIST, afcUrl.path());
    }

    AfcFile file(client, afcUrl.path());

    if (flags.testFlag(Resume)) {
        result = file.open(QIODevice::Append);
    } else {
        result = file.open(QIODevice::WriteOnly);
    }

    if (!result.success()) {
        return result;
    }

    int readDataResult = 0;

    do {
        QByteArray buffer;
        dataReq();

        readDataResult = readData(buffer);

        if (readDataResult < 0) {
            return Result::fail(ERR_CANNOT_READ, QStringLiteral("readData result was %1").arg(readDataResult));
        }

        uint32_t bytesWritten = 0;
        const auto result = file.write(buffer, bytesWritten);

        if (!result.success()) {
            return result;
        }
    } while (readDataResult > 0);

    const QString modifiedMeta = metaData(QStringLiteral("modified"));

    if (!modifiedMeta.isEmpty()) {
        const QDateTime mtime = QDateTime::fromString(modifiedMeta, Qt::ISODate);

        if (mtime.isValid() && !client->setModificationTime(afcUrl.path(), mtime).success()) {
            qCWarning(KIO_AFC_LOG) << "Failed to set mtime for" << afcUrl.path() << "in put";
        }
    }

    return Result::pass();
}

Result AfcWorker::open(const QUrl &url, QIODevice::OpenMode mode)
{
    // TODO fail if already open?

    const AfcUrl afcUrl(url);

    AfcClient::Ptr client;
    Result result = clientForUrl(afcUrl, client);
    if (!result.success()) {
        return result;
    }

    UDSEntry entry;
    result = client->entry(afcUrl.path(), entry);
    if (!result.success()) {
        return result;
    }

    auto file = std::make_unique<AfcFile>(client, afcUrl.path());

    result = file->open(mode);
    if (!result.success()) {
        return result;
    }

    if (mode.testFlag(QIODevice::ReadOnly) && !mode.testFlag(QIODevice::Append)) {
        guessMimeType(*file, afcUrl.path());
    }

    m_openFile = std::move(file);

    totalSize(entry.numberValue(UDSEntry::UDS_SIZE, 0));
    position(0);

    return Result::pass();
}

Result AfcWorker::read(filesize_t bytesRequested)
{
    if (!m_openFile) {
        return Result::fail(ERR_CANNOT_READ, i18n("Cannot read without opening file first"));
    }

    AfcFileReader reader = m_openFile->reader();
    reader.setSize(bytesRequested);

    while (reader.hasMore()) {
        const Result result = reader.read();
        if (!result.success()) {
            return result;
        }
        data(reader.data());
    }

    return Result::pass();
}

Result AfcWorker::seek(filesize_t offset)
{
    if (!m_openFile) {
        return Result::fail(ERR_CANNOT_SEEK, i18n("Cannot seek without opening file first"));
    }

    const Result result = m_openFile->seek(offset);
    if (result.success()) {
        position(offset);
    }

    return result;
}

Result AfcWorker::truncate(filesize_t length)
{
    if (!m_openFile) {
        return Result::fail(ERR_CANNOT_TRUNCATE, QStringLiteral("Cannot truncate without opening file first"));
    }

    Result result = m_openFile->truncate(length);
    if (result.success()) {
        truncated(length);
    }

    return result;
}

Result AfcWorker::write(const QByteArray &data)
{
    if (!m_openFile) {
        return Result::fail(ERR_CANNOT_WRITE, i18n("Cannot write without opening file first"));
    }

    uint32_t bytesWritten = 0;
    const Result result = m_openFile->write(data, bytesWritten);
    if (result.success()) {
        written(bytesWritten);
    }

    return result;
}

Result AfcWorker::close()
{
    if (!m_openFile) {
        return Result::fail(ERR_INTERNAL, QStringLiteral("Cannot close what is not open"));
    }

    const Result result = m_openFile->close();
    if (result.success()) {
        m_openFile.reset();
    }

    return result;
}

Result AfcWorker::copy(const QUrl &src, const QUrl &dest, int permissions, JobFlags flags)
{
    Q_UNUSED(permissions);

    const AfcUrl srcAfcUrl(src);
    const AfcUrl destAfcUrl(dest);

    if (deviceIdForFriendlyUrl(srcAfcUrl) != deviceIdForFriendlyUrl(destAfcUrl)) {
        // Let KIO handle copying onto, off the, and between devices
        return Result::fail(ERR_UNSUPPORTED_ACTION);
    }

    AfcClient::Ptr client;
    Result result = clientForUrl(srcAfcUrl, client);
    if (!result.success()) {
        return result;
    }

    UDSEntry srcEntry;
    result = client->entry(srcAfcUrl.path(), srcEntry);
    if (!result.success()) {
        return result;
    }

    UDSEntry destEntry;
    result = client->entry(destAfcUrl.path(), destEntry);

    const bool exists = result.error() != ERR_DOES_NOT_EXIST;
    if (exists && !flags.testFlag(Overwrite)) {
        if (S_ISDIR(destEntry.numberValue(UDSEntry::UDS_FILE_TYPE))) {
            return Result::fail(ERR_DIR_ALREADY_EXIST, destAfcUrl.path());
        }
        return Result::fail(ERR_FILE_ALREADY_EXIST, destAfcUrl.path());
    }

    AfcFile srcFile(client, srcAfcUrl.path());
    result = srcFile.open(QIODevice::ReadOnly);
    if (!result.success()) {
        return result;
    }

    AfcFile destFile(client, destAfcUrl.path());

    if (flags.testFlag(Resume)) {
        result = destFile.open(QIODevice::Append);
    } else {
        result = destFile.open(QIODevice::WriteOnly);
    }

    if (!result.success()) {
        return result;
    }

    auto cleanup = qScopeGuard([&client, &destAfcUrl] {
        qCInfo(KIO_AFC_LOG) << "Cleaning up leftover file" << destAfcUrl.path();
        // NOTE cannot emit failure here because emitResult
        // will already have been called before
        auto result = client->del(destAfcUrl.path());
        if (!result.success()) {
            qCWarning(KIO_AFC_LOG) << "Failed to clean up" << result.error() << result.errorString();
        }
    });

    const auto size = srcEntry.numberValue(UDSEntry::UDS_SIZE, 0);
    totalSize(size);

    AfcFileReader reader = srcFile.reader();
    reader.setSize(size);

    KIO::filesize_t copied = 0;

    while (!wasKilled() && reader.hasMore()) {
        auto result = reader.read();
        if (!result.success()) {
            return result;
        }

        const QByteArray chunk = reader.data();

        uint32_t bytesWritten = 0;
        result = destFile.write(chunk, bytesWritten);
        if (!result.success()) {
            return result;
        }

        // TODO check if bytesWritten matches reader.data().size()?

        copied += chunk.size();
        processedSize(copied);
    }

    cleanup.dismiss();
    destFile.close();

    // TODO check if conversion back and forth QDateTime is too expensive when copying many files
    const QDateTime mtime = QDateTime::fromSecsSinceEpoch(srcEntry.numberValue(KIO::UDSEntry::UDS_MODIFICATION_TIME, 0));
    if (mtime.isValid()) {
        client->setModificationTime(destAfcUrl.path(), mtime);
    }

    return Result::pass();
}

Result AfcWorker::del(const QUrl &url, bool isFile)
{
    const AfcUrl afcUrl(url);

    AfcClient::Ptr client;
    Result result = clientForUrl(afcUrl, client);
    if (result.success()) {
        if (isFile) {
            result = client->del(afcUrl.path());
        } else {
            result = client->delRecursively(afcUrl.path());
        }
    }

    return result;
}

Result AfcWorker::rename(const QUrl &url, const QUrl &dest, JobFlags flags)
{
    const AfcUrl srcAfcUrl(url);
    const AfcUrl destAfcUrl(dest);

    if (deviceIdForFriendlyUrl(srcAfcUrl) != deviceIdForFriendlyUrl(destAfcUrl)) {
        return Result::fail(ERR_CANNOT_RENAME, i18n("Cannot rename between devices."));
    }

    AfcClient::Ptr client;
    Result result = clientForUrl(srcAfcUrl, client);
    if (result.success()) {
        result = client->rename(srcAfcUrl.path(), destAfcUrl.path(), flags);
    }

    return result;
}

Result AfcWorker::symlink(const QString &target, const QUrl &dest, JobFlags flags)
{
    const AfcUrl destAfcUrl(dest);

    AfcClient::Ptr client;
    Result result = clientForUrl(destAfcUrl, client);
    if (result.success()) {
        result = client->symlink(target, destAfcUrl.path(), flags);
    }

    return result;
}

Result AfcWorker::mkdir(const QUrl &url, int permissions)
{
    Q_UNUSED(permissions)

    const AfcUrl afcUrl(url);

    AfcClient::Ptr client;
    Result result = clientForUrl(afcUrl, client);
    if (result.success()) {
        result = client->mkdir(afcUrl.path());
    }

    return result;
}

Result AfcWorker::setModificationTime(const QUrl &url, const QDateTime &mtime)
{
    const AfcUrl afcUrl(url);

    AfcClient::Ptr client;
    Result result = clientForUrl(afcUrl, client);
    if (result.success()) {
        result = client->setModificationTime(afcUrl.path(), mtime);
    }

    return result;
}

Result AfcWorker::fileSystemFreeSpace(const QUrl &url)
{
    // TODO FileSystemFreeSpaceJob does not follow redirects!
    const QUrl redirectUrl = resolveSolidUrl(url);
    if (redirectUrl.isValid()) {
        return fileSystemFreeSpace(redirectUrl);
    }

    // TODO FileSystemFreeSpaceJob does not follow redirects!
    const AfcUrl afcUrl(url);
    if (afcUrl.device().isEmpty() && m_devices.count() == 1) {
        return fileSystemFreeSpace(QUrl(QStringLiteral("afc://%1/").arg((*m_devices.constBegin())->id())));
    }

    AfcClient::Ptr client;
    const Result result = clientForUrl(afcUrl, client);
    if (!result.success()) {
        return result;
    }

    const AfcDiskUsage diskUsage(client);
    if (!diskUsage.isValid()) {
        return Result::fail(ERR_CANNOT_STAT, url.toDisplayString());
    }

    setMetaData(QStringLiteral("total"), QString::number(diskUsage.total()));
    setMetaData(QStringLiteral("available"), QString::number(diskUsage.free()));
    return Result::pass();
}

#include "kio_afc.moc"
