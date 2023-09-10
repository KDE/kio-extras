/*
 * SPDX-FileCopyrightText: 2022 Kai Uwe Broulik <kde@broulik.de>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "afcapp.h"

#include "afc_debug.h"

using namespace KIO;

AfcApp::AfcApp() = default;

AfcApp::AfcApp(plist_t app)
{
    auto fetchAppField = [app](const char *key, QString &member) {
        plist_t plistItem = plist_dict_get_item(app, key);
        if (plistItem) {
            char *stringValue = nullptr;
            plist_get_string_val(plistItem, &stringValue);
            member = QString::fromUtf8(stringValue);
            free(stringValue);
        }
    };

    fetchAppField("CFBundleIdentifier", m_bundleId);
    fetchAppField("CFBundleDisplayName", m_displayName);

    plist_t sharingItem = plist_dict_get_item(app, "UIFileSharingEnabled");
    if (sharingItem) {
        const auto type = plist_get_node_type(sharingItem);
        switch (type) {
        case PLIST_BOOLEAN: {
            uint8_t sharingEnabled = 0;
            plist_get_bool_val(sharingItem, &sharingEnabled);
            m_sharingEnabled = sharingEnabled;
            break;
        }
        case PLIST_STRING: {
            char *sharingString = nullptr;
            plist_get_string_val(sharingItem, &sharingString);
            if (sharingString) {
                m_sharingEnabled = (strcmp(sharingString, "YES") == 0 || strcmp(sharingString, "true") == 0);
                free(sharingString);
            }
            break;
        }
        default:
            qCWarning(KIO_AFC_LOG) << "Unhandled plist node type" << type << "for file sharing enabled property";
            break;
        }
    }
}

bool AfcApp::isValid() const
{
    return !m_bundleId.isEmpty();
}

QString AfcApp::bundleId() const
{
    return m_bundleId;
}

QString AfcApp::displayName() const
{
    return m_displayName;
}

bool AfcApp::sharingEnabled() const
{
    return m_sharingEnabled;
}

QString AfcApp::iconPath() const
{
    return m_iconPath;
}

UDSEntry AfcApp::entry(const QString &name) const
{
    UDSEntry entry;
    entry.fastInsert(UDSEntry::UDS_NAME, !name.isEmpty() ? name : m_bundleId);
    entry.fastInsert(UDSEntry::UDS_DISPLAY_NAME, m_displayName);
    entry.fastInsert(UDSEntry::UDS_FILE_TYPE, S_IFDIR);
    if (!m_iconPath.isEmpty()) {
        entry.fastInsert(UDSEntry::UDS_ICON_NAME, m_iconPath);
    }
    return entry;
}
