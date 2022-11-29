/*
 * SPDX-FileCopyrightText: 2022 Kai Uwe Broulik <kde@broulik.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "afcclient.h"

#include "afc_debug.h"

#include "afcdevice.h"
#include "afcfile.h"
#include "afcutils.h"

#include <QDateTime>
#include <QScopeGuard>

using namespace KIO;

AfcClient::AfcClient(AfcDevice *device)
    : m_device(device)
{

}

AfcClient::~AfcClient()
{
    if (m_client) {
        afc_client_free(m_client);
        m_client = nullptr;
    }

    if (m_houseArrestClient) {
        house_arrest_client_free(m_houseArrestClient);
        m_houseArrestClient = nullptr;
    }
}

WorkerResult AfcClient::init(lockdownd_client_t lockdowndClient, const QString &appId)
{
    if (m_client) {
        return WorkerResult::pass();
    }

    const char *serviceName = appId.isEmpty() ? AFC_SERVICE_NAME : HOUSE_ARREST_SERVICE_NAME;

    lockdownd_service_descriptor_t service = nullptr;
    auto ret = lockdownd_start_service(lockdowndClient, serviceName, &service);
    if (ret != LOCKDOWN_E_SUCCESS) {
        qCWarning(KIO_AFC_LOG) << "Failed to start" << serviceName << "service through lockdownd" << ret;
        return AfcUtils::Result::from(ret, m_device->id());
    }

    // Regular file system access through AFC client
    if (appId.isEmpty()) {
        auto afcRet = afc_client_new(m_device->device(), service, &m_client);
        if (afcRet != AFC_E_SUCCESS) {
            qCWarning(KIO_AFC_LOG) << "Failed to create AFC client" << afcRet;
            return AfcUtils::Result::from(ret, m_device->id());
        }
        m_appId = appId;
        return WorkerResult::pass();
    }

    // App-specific document folder through specialized House Arrest AFC client
    if (!m_houseArrestClient) {
        auto houseArrestRet = house_arrest_client_new(m_device->device(), service, &m_houseArrestClient);
        if (houseArrestRet != HOUSE_ARREST_E_SUCCESS) {
            qCWarning(KIO_AFC_LOG) << "Failed to create House Arrest client" << houseArrestRet;
            return AfcUtils::Result::from(houseArrestRet);
        }

        auto cleanupHouseArrest = qScopeGuard([this] {
            house_arrest_client_free(m_houseArrestClient);
            m_houseArrestClient = nullptr;
        });

        const char houseArrestCommand[] = "VendDocuments";
        houseArrestRet = house_arrest_send_command(m_houseArrestClient,
                                                   houseArrestCommand,
                                                   appId.toUtf8().constData());
        if (houseArrestRet != HOUSE_ARREST_E_SUCCESS) {
            qCWarning(KIO_AFC_LOG) << "Failed to send House Arrest" << houseArrestCommand << "command" << houseArrestRet;
            return AfcUtils::Result::from(houseArrestRet);
        }

        plist_t resultPlist = nullptr;
        houseArrestRet = house_arrest_get_result(m_houseArrestClient, &resultPlist);
        if (houseArrestRet != HOUSE_ARREST_E_SUCCESS) {
            qCWarning(KIO_AFC_LOG) << "Failed to get House Arrest result for" << houseArrestCommand << houseArrestRet;
            return AfcUtils::Result::from(houseArrestRet);
        }

        auto cleanupPlist = qScopeGuard([resultPlist] {
            plist_free(resultPlist);
        });

        plist_t errorItem = plist_dict_get_item(resultPlist, "Error");
        if (errorItem) {
            char *errorString = nullptr;
            plist_get_string_val(errorItem, &errorString);
            QScopedPointer<char, QScopedPointerPodDeleter> errorStringPtr(errorString);

            // The app does not exist.
            if (strcmp(errorStringPtr.data(), "ApplicationLookupFailed") == 0) {
                return WorkerResult::fail(ERR_DOES_NOT_EXIST, appId);
            // The app does not have UIFileSharingEnabled enabled.
            } else if (strcmp(errorStringPtr.data(), "InstallationLookupFailed") == 0) {
                return WorkerResult::fail(ERR_ACCESS_DENIED, appId);
            }

            qCWarning(KIO_AFC_LOG) << "House Arrest returned error" << errorString;
            return WorkerResult::fail(ERR_INTERNAL, QString::fromUtf8(errorString));
        }

        cleanupHouseArrest.dismiss();
    }

    auto afcRet = afc_client_new_from_house_arrest_client(m_houseArrestClient, &m_client);
    if (afcRet != AFC_E_SUCCESS) {
        qCWarning(KIO_AFC_LOG) << "Failed to create AFC client from House Arrest client" << afcRet;
        return AfcUtils::Result::from(afcRet);
    }

    m_appId = appId;
    return WorkerResult::pass();
}

AfcDevice *AfcClient::device() const
{
    return m_device;
}

afc_client_t AfcClient::internalClient() const
{
    return m_client;
}

QString AfcClient::appId() const
{
    return m_appId;
}

WorkerResult AfcClient::entry(const QString &path, UDSEntry &entry)
{
    char **info = nullptr;
    const auto ret = afc_get_file_info(m_client, path.toUtf8().constData(), &info);
    // may return null https://github.com/libimobiledevice/libimobiledevice/issues/206
    const WorkerResult result = AfcUtils::Result::from(ret, path);
    if (!result.success() || !info) {
        return result;
    }

    auto cleanup = qScopeGuard([info] {
        afc_dictionary_free(info);
    });

    const int lastSlashIdx = path.lastIndexOf(QLatin1Char('/'));
    entry.fastInsert(UDSEntry::UDS_NAME, path.mid(lastSlashIdx + 1));

    // Apply special icons for known locations
    if (m_appId.isEmpty()) {
        static const QHash<QString, QString> s_folderIcons = {
            {QStringLiteral("/DCIM"), QStringLiteral("camera-photo")},
            {QStringLiteral("/Documents"), QStringLiteral("folder-documents")},
            {QStringLiteral("/Downloads"), QStringLiteral("folder-downloads")},
            {QStringLiteral("/Photos"), QStringLiteral("folder-pictures")}
        };
        const QString iconName = s_folderIcons.value(path);
        if (!iconName.isEmpty()) {
            entry.fastInsert(UDSEntry::UDS_ICON_NAME, iconName);
        }
    }

    for (int i = 0; info[i]; i += 2) {
        const auto *key = info[i];
        const auto *value = info[i + 1];

        if (strcmp(key, "st_size") == 0) {
            entry.fastInsert(UDSEntry::UDS_SIZE, atoll(value));
        } else if (strcmp(key, "st_blocks") == 0) {
            // no UDS equivalent
        } else if (strcmp(key, "st_nlink") == 0) {
            // no UDS equivalent
        } else if (strcmp(key, "st_ifmt") == 0) {
            int type = 0;
            if (strcmp(value, "S_IFREG") == 0) {
                type = S_IFREG;
            } else if (strcmp(value, "S_IFDIR") == 0) {
                type = S_IFDIR;
                entry.fastInsert(UDSEntry::UDS_MIME_TYPE, QStringLiteral("inode/directory"));
            } else if (strcmp(value, "S_IFLNK") == 0) {
                type = S_IFLNK;
            } else if (strcmp(value, "S_IFBLK") == 0) {
                type = S_IFBLK;
            } else if (strcmp(value, "S_IFCHR") == 0) {
                type = S_IFCHR;
            } else if (strcmp(value, "S_IFIFO") == 0) {
                type = S_IFIFO;
            } else if (strcmp(value, "S_IFSOCK") == 0) {
                type = S_IFSOCK;
            }

            if (type) {
                entry.fastInsert(UDSEntry::UDS_FILE_TYPE, type);
            } else {
                qCWarning(KIO_AFC_LOG) << "Encountered unknown" << key << "of" << value << "for" << path;
            }
        // is returned in nanoseconds
        } else if (strcmp(key, "st_mtime") == 0) {
            entry.fastInsert(UDSEntry::UDS_MODIFICATION_TIME, atoll(value) / 1000000000);
        } else if (strcmp(key, "st_birthtime") == 0) {
            entry.fastInsert(UDSEntry::UDS_CREATION_TIME, atoll(value) / 1000000000);
        } else if (strcmp(key, "LinkTarget") == 0) {
            // TODO figure out why afc_make_link fails with AFC_E_OP_NOT_SUPPORTED and then implement this
            //entry.fastInsert(UDSEntry::UDS_LINK_DEST, QString::fromUtf8(value));
        } else {
            qCDebug(KIO_AFC_LOG) << "Encountered unrecognized file info key" << key << "for" << path;
        }
    }

    return WorkerResult::pass();
}

WorkerResult AfcClient::entryList(const QString &path, QStringList &entryList)
{
    char **entries = nullptr;
    entryList.clear();

    const auto ret = afc_read_directory(m_client, path.toUtf8().constData(), &entries);
    const WorkerResult result = AfcUtils::Result::from(ret);
    if (!result.success() || !entries) {
        return result;
    }

    auto cleanup = qScopeGuard([entries] {
        afc_dictionary_free(entries);
    });

    char **it = entries;
    while (*it) {
        const QString name = QString::fromUtf8(*it);
        ++it;

        if (name == QLatin1Char('.') || name == QLatin1String("..")) {
            continue;
        }
        entryList.append(name);
    }

    return WorkerResult::pass();
}

WorkerResult AfcClient::del(const QString &path)
{
    const auto ret = afc_remove_path(m_client, path.toUtf8().constData());
    return AfcUtils::Result::from(ret, path);
}

WorkerResult AfcClient::delRecursively(const QString &path)
{
    const auto ret = afc_remove_path_and_contents(m_client, path.toUtf8().constData());
    return AfcUtils::Result::from(ret, path);
}

WorkerResult AfcClient::rename(const QString &src, const QString &dest, JobFlags flags)
{
    UDSEntry srcEntry; // unused
    const WorkerResult srcResult = this->entry(src, srcEntry);
    if (!srcResult.success()) {
        return srcResult;
    }

    UDSEntry destEntry;
    const auto destResult = this->entry(dest, destEntry);

    const bool exists = destResult.error() != ERR_DOES_NOT_EXIST;
    if (exists && !flags.testFlag(Overwrite)) {
        if (S_ISDIR(destEntry.numberValue(UDSEntry::UDS_FILE_TYPE))) {
            return WorkerResult::fail(ERR_DIR_ALREADY_EXIST, dest);
        }
        return WorkerResult::fail(ERR_FILE_ALREADY_EXIST, dest);
    }

    const auto ret = afc_rename_path(m_client,
                                     src.toUtf8().constData(),
                                     dest.toUtf8().constData());
    return AfcUtils::Result::from(ret, dest);
}

WorkerResult AfcClient::symlink(const QString &target, const QString &dest, JobFlags flags)
{
    UDSEntry targetEntry; // unused
    const WorkerResult targetResult = this->entry(target, targetEntry);
    if (!targetResult.success()) {
        return targetResult;
    }

    UDSEntry destEntry;
    const auto destResult = this->entry(dest, destEntry);

    const bool exists = destResult.error() != ERR_DOES_NOT_EXIST;
    if (exists && !flags.testFlag(Overwrite)) {
        if (S_ISDIR(destEntry.numberValue(UDSEntry::UDS_FILE_TYPE))) {
            return WorkerResult::fail(ERR_DIR_ALREADY_EXIST, dest);
        }
        return WorkerResult::fail(ERR_FILE_ALREADY_EXIST, dest);
    }

    // TODO figure out why this always fails with AFC_E_OP_NOT_SUPPORTED
    const auto ret = afc_make_link(m_client,
                                   AFC_SYMLINK,
                                   target.toUtf8().constData(),
                                   dest.toUtf8().constData());
    return AfcUtils::Result::from(ret, dest);
}

WorkerResult AfcClient::mkdir(const QString &path)
{
    UDSEntry entry;
    const WorkerResult getResult = this->entry(path, entry);

    const bool exists = getResult.error() != ERR_DOES_NOT_EXIST;
    if (exists) {
        if (S_ISDIR(entry.numberValue(UDSEntry::UDS_FILE_TYPE))) {
            return WorkerResult::fail(ERR_DIR_ALREADY_EXIST, path);
        }
        return WorkerResult::fail(ERR_FILE_ALREADY_EXIST, path);
    }

    const auto ret = afc_make_directory(m_client, path.toUtf8().constData());
    return AfcUtils::Result::from(ret, path);
}

WorkerResult AfcClient::setModificationTime(const QString &path, const QDateTime &mtime)
{
    const auto ret = afc_set_file_time(m_client,
                                       path.toUtf8().constData(),
                                       mtime.toMSecsSinceEpoch() /*ms*/ * 1000000 /*us*/);
    return AfcUtils::Result::from(ret, path);
}
