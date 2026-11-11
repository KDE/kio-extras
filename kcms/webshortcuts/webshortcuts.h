/*
    SPDX-FileCopyrightText: 2000 Yves Arrouye <yves@realnames.com>
    SPDX-FileCopyrightText: 2002, 2003 Dawit Alemayehu <adawit@kde.org>
    SPDX-FileCopyrightText: 2009 Nick Shaforostoff <shaforostoff@kde.ru>
    SPDX-FileCopyrightText: 2026 Tobias Fella <tobias.fella@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KQuickConfigModule>

#include <memory>
#include <vector>

#include "searchproviderregistry.h"

class SearchProvider;
class ProvidersModel;
class ProvidersProxyModel;

Q_MOC_INCLUDE("providersmodel.h")

class WebShortcuts : public KQuickConfigModule
{
    Q_OBJECT

    Q_PROPERTY(ProvidersProxyModel *providersModel READ providersModel CONSTANT)
    Q_PROPERTY(bool webShortcutsEnabled READ webShortcutsEnabled WRITE setWebShortcutsEnabled NOTIFY webShortcutsEnabledChanged)
    Q_PROPERTY(bool usePreferredOnly READ usePreferredOnly WRITE setUsePreferredOnly NOTIFY usePreferredOnlyChanged)
    Q_PROPERTY(QString keywordDelimiter READ keywordDelimiter WRITE setKeywordDelimiter NOTIFY keywordDelimiterChanged)
    Q_PROPERTY(QString defaultProvider READ defaultProvider WRITE setDefaultProvider NOTIFY defaultProviderChanged)
    Q_PROPERTY(QString defaultDefaultProvider READ defaultDefaultProvider NOTIFY providersChanged)
    Q_PROPERTY(QVariantList defaultProviderChoices READ defaultProviderChoices NOTIFY providersChanged)
    Q_PROPERTY(bool preferredProvidersAreDefault READ preferredProvidersAreDefault NOTIFY preferredProvidersChanged)
    Q_PROPERTY(QStringList availableCharsets READ availableCharsets CONSTANT)

public:
    explicit WebShortcuts(QObject *parent, const KPluginMetaData &data);
    ~WebShortcuts() override;

    void load() override;
    void save() override;
    void defaults() override;

    ProvidersProxyModel *providersModel() const;

    bool webShortcutsEnabled() const;
    void setWebShortcutsEnabled(bool enabled);

    bool usePreferredOnly() const;
    void setUsePreferredOnly(bool usePreferredOnly);

    QString keywordDelimiter() const;
    void setKeywordDelimiter(const QString &delimiter);

    QString defaultProvider() const;
    void setDefaultProvider(const QString &provider);

    QString defaultDefaultProvider() const;
    QVariantList defaultProviderChoices() const;
    bool preferredProvidersAreDefault() const;
    QStringList availableCharsets() const;

    /*!
     * Returns a message describing which of the comma-separated \a keywords are
     * already used by another provider than the one at \a row of providersModel,
     * or an empty string if there are no conflicts. Pass -1 for a new provider.
     */
    Q_INVOKABLE QString keywordConflicts(const QString &keywords, int row) const;

    /*!
     * Returns the clipboard text if it is a URL with a host, otherwise an empty string.
     */
    Q_INVOKABLE QString clipboardUrl() const;

    Q_INVOKABLE void addProvider(const QString &name, const QString &query, const QString &keywords, const QString &charset);
    Q_INVOKABLE void changeProvider(int row, const QString &query, const QString &keywords, const QString &charset);
    Q_INVOKABLE void removeProvider(int row);

Q_SIGNALS:
    void webShortcutsEnabledChanged();
    void usePreferredOnlyChanged();
    void keywordDelimiterChanged();
    void defaultProviderChanged();
    void providersChanged();
    void preferredProvidersChanged();

private:
    void markAsChanged();
    void updateRepresentsDefaults();
    SearchProvider *providerAt(int row) const;
    static QStringList parseKeywords(const QString &keywords);

    ProvidersModel *m_providersModel;
    ProvidersProxyModel *m_proxyModel;
    SearchProviderRegistry m_registry;

    // Providers added by the user that are not owned by the registry
    std::vector<std::unique_ptr<SearchProvider>> m_newProviders;

    // The names of the providers that the user deleted,
    // these are marked as deleted in the user's homedirectory
    // on save if a global service file exists for it.
    QStringList m_deletedProviders;

    bool m_webShortcutsEnabled = true;
    bool m_usePreferredOnly = false;
    QString m_keywordDelimiter;
    QString m_defaultProvider;
};
