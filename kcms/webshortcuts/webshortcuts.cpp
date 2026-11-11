/*
    SPDX-FileCopyrightText: 2000 Yves Arrouye <yves@realnames.com>
    SPDX-FileCopyrightText: 2001, 2002 Dawit Alemayehu <adawit@kde.org>
    SPDX-FileCopyrightText: 2009 Nick Shaforostoff <shaforostoff@kde.ru>
    SPDX-FileCopyrightText: 2026 Tobias Fella <tobias.fella@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "webshortcuts.h"

#include "providersmodel.h"
#include "searchprovider.h"

#include <KConfigGroup>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KSharedConfig>

#ifdef WITH_DBUS
#include <QDBusConnection>
#include <QDBusMessage>
#endif

#include <QClipboard>
#include <QCollator>
#include <QFile>
#include <QGuiApplication>
#include <QQmlEngine>
#include <QStandardPaths>
#include <QStringConverter>
#include <QUrl>

#include <algorithm>

using namespace Qt::StringLiterals;

K_PLUGIN_CLASS_WITH_JSON(WebShortcuts, "kcm_webshortcuts.json")

namespace
{
const auto s_defaultProvider = u"duckduckgo"_s;
const auto s_defaultDelimiter = u":"_s;
// Keep in sync with KIO!
const QStringList s_defaultPreferredProviders{
    u"google"_s,
    u"youtube"_s,
    u"yahoo"_s,
    u"wikipedia"_s,
    u"wikit"_s,
};
}

WebShortcuts::WebShortcuts(QObject *parent, const KPluginMetaData &data)
    : KQuickConfigModule(parent, data)
    , m_providersModel(new ProvidersModel(this))
    , m_proxyModel(new ProvidersProxyModel(this))
    , m_keywordDelimiter(s_defaultDelimiter)
{
    setButtons(Apply | Default | Help);

    m_proxyModel->setSourceModel(m_providersModel);

    connect(m_providersModel, &ProvidersModel::dataModified, this, [this] {
        Q_EMIT preferredProvidersChanged();
        markAsChanged();
    });
}

WebShortcuts::~WebShortcuts() = default;

void WebShortcuts::load()
{
    KConfig config(u"kuriikwsfilterrc"_s, KConfig::NoGlobals);
    const KConfigGroup group = config.group(u"General"_s);

    // Drop all references to the providers before the registry deletes them
    m_providersModel->setProviders({}, {});
    m_newProviders.clear();
    m_deletedProviders.clear();
    m_registry.reload();

    QList<SearchProvider *> providers;
    const QList<SearchProvider *> allProviders = m_registry.findAll();
    for (SearchProvider *provider : allProviders) {
        if (!provider->isHidden()) {
            providers << provider;
        }
    }

    m_providersModel->setProviders(providers, group.readEntry("PreferredWebShortcuts", s_defaultPreferredProviders));
    Q_EMIT providersChanged();
    Q_EMIT preferredProvidersChanged();

    m_webShortcutsEnabled = group.readEntry("EnableWebShortcuts", true);
    Q_EMIT webShortcutsEnabledChanged();

    m_usePreferredOnly = group.readEntry("UsePreferredWebShortcutsOnly", false);
    Q_EMIT usePreferredOnlyChanged();

    const QString delimiter = group.readEntry("KeywordDelimiter", s_defaultDelimiter);
    m_keywordDelimiter = delimiter.startsWith(u' ') ? u" "_s : s_defaultDelimiter;
    Q_EMIT keywordDelimiterChanged();

    // An unknown provider means "None"
    const QString defaultProvider = group.readEntry("DefaultWebShortcut", s_defaultProvider);
    const bool defaultProviderExists = std::ranges::any_of(providers, [&defaultProvider](const SearchProvider *provider) {
        return provider->desktopEntryName() == defaultProvider;
    });
    m_defaultProvider = defaultProviderExists ? defaultProvider : QString();
    Q_EMIT defaultProviderChanged();

    setNeedsSave(false);
    updateRepresentsDefaults();
}

void WebShortcuts::save()
{
    KConfig config(u"kuriikwsfilterrc"_s, KConfig::NoGlobals);

    KConfigGroup group = config.group(u"General"_s);
    group.writeEntry("EnableWebShortcuts", m_webShortcutsEnabled);
    group.writeEntry("KeywordDelimiter", m_keywordDelimiter);
    group.writeEntry("DefaultWebShortcut", m_defaultProvider);
    group.writeEntry("PreferredWebShortcuts", m_providersModel->favoriteEngines());
    group.writeEntry("UsePreferredWebShortcutsOnly", m_usePreferredOnly);

    const QList<SearchProvider *> providers = m_providersModel->providers();
    const QString path = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/kf6/searchproviders/"_L1;

    for (SearchProvider *provider : providers) {
        if (!provider->isDirty()) {
            continue;
        }

        KConfig serviceConfig(path + provider->desktopEntryName() + ".desktop"_L1, KConfig::SimpleConfig);
        KConfigGroup service(&serviceConfig, u"Desktop Entry"_s);
        service.writeEntry("Type", "Service");
        service.writeEntry("Name", provider->name());
        service.writeEntry("Query", provider->query());
        service.writeEntry("Keys", provider->keys());
        service.writeEntry("Charset", provider->charset());
        service.writeEntry("Hidden", false); // we might be overwriting a hidden entry
        provider->setDirty(false);
    }

    const QStringList servicesDirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, u"kf6/searchproviders/"_s, QStandardPaths::LocateDirectory);
    for (const QString &providerName : std::as_const(m_deletedProviders)) {
        QStringList matches;
        for (const QString &dir : servicesDirs) {
            const QString current = dir + u'/' + providerName + ".desktop"_L1;
            if (QFile::exists(current)) {
                matches += current;
            }
        }

        // Shouldn't happen
        if (matches.isEmpty()) {
            continue;
        }

        if (matches.size() == 1 && matches.first().startsWith(path)) {
            // If only the local copy existed, unlink it
            QFile::remove(matches.first());
            continue;
        }

        KConfig serviceConfig(path + providerName + ".desktop"_L1, KConfig::SimpleConfig);
        KConfigGroup service(&serviceConfig, u"Desktop Entry"_s);
        service.writeEntry("Type", "Service");
        service.writeEntry("Hidden", true);
    }
    m_deletedProviders.clear();

    config.sync();

    setNeedsSave(false);

#ifdef WITH_DBUS
    // Update filters in running applications...
    const QDBusMessage message = QDBusMessage::createSignal(u"/"_s, u"org.kde.KUriFilterPlugin"_s, u"configure"_s);
    QDBusConnection::sessionBus().send(message);
#endif
}

void WebShortcuts::defaults()
{
    setWebShortcutsEnabled(true);
    setUsePreferredOnly(false);
    setKeywordDelimiter(s_defaultDelimiter);
    setDefaultProvider(defaultDefaultProvider());
    m_providersModel->setFavoriteProviders(s_defaultPreferredProviders);
}

ProvidersProxyModel *WebShortcuts::providersModel() const
{
    return m_proxyModel;
}

bool WebShortcuts::webShortcutsEnabled() const
{
    return m_webShortcutsEnabled;
}

void WebShortcuts::setWebShortcutsEnabled(bool enabled)
{
    if (m_webShortcutsEnabled == enabled) {
        return;
    }
    m_webShortcutsEnabled = enabled;
    Q_EMIT webShortcutsEnabledChanged();
    markAsChanged();
}

bool WebShortcuts::usePreferredOnly() const
{
    return m_usePreferredOnly;
}

void WebShortcuts::setUsePreferredOnly(bool usePreferredOnly)
{
    if (m_usePreferredOnly == usePreferredOnly) {
        return;
    }
    m_usePreferredOnly = usePreferredOnly;
    Q_EMIT usePreferredOnlyChanged();
    markAsChanged();
}

QString WebShortcuts::keywordDelimiter() const
{
    return m_keywordDelimiter;
}

void WebShortcuts::setKeywordDelimiter(const QString &delimiter)
{
    if (m_keywordDelimiter == delimiter || (delimiter != ":"_L1 && delimiter != " "_L1)) {
        return;
    }
    m_keywordDelimiter = delimiter;
    Q_EMIT keywordDelimiterChanged();
    markAsChanged();
}

QString WebShortcuts::defaultProvider() const
{
    return m_defaultProvider;
}

void WebShortcuts::setDefaultProvider(const QString &provider)
{
    if (m_defaultProvider == provider) {
        return;
    }
    m_defaultProvider = provider;
    Q_EMIT defaultProviderChanged();
    markAsChanged();
}

QString WebShortcuts::defaultDefaultProvider() const
{
    const QList<SearchProvider *> providers = m_providersModel->providers();
    const bool found = std::ranges::any_of(providers, [](const SearchProvider *provider) {
        return provider->desktopEntryName() == s_defaultProvider;
    });
    return found ? s_defaultProvider : QString();
}

QVariantList WebShortcuts::defaultProviderChoices() const
{
    QList<SearchProvider *> providers = m_providersModel->providers();
    QCollator collator;
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    std::ranges::sort(providers, [&collator](const SearchProvider *a, const SearchProvider *b) {
        return collator.compare(a->name(), b->name()) < 0;
    });

    QVariantList choices;
    choices.reserve(providers.size() + 1);
    choices.append(QVariantMap{
        {u"text"_s, i18nc("@item:inlistbox No default web search keyword", "None")},
        {u"value"_s, QString()},
    });
    for (const SearchProvider *provider : std::as_const(providers)) {
        choices.append(QVariantMap{
            {u"text"_s, provider->name()},
            {u"value"_s, provider->desktopEntryName()},
        });
    }
    return choices;
}

bool WebShortcuts::preferredProvidersAreDefault() const
{
    const QStringList favorites = m_providersModel->favoriteEngines();
    return QSet<QString>(favorites.cbegin(), favorites.cend()) == QSet<QString>(s_defaultPreferredProviders.cbegin(), s_defaultPreferredProviders.cend());
}

QStringList WebShortcuts::availableCharsets() const
{
    return QStringConverter::availableCodecs();
}

QStringList WebShortcuts::parseKeywords(const QString &keywords)
{
    QString normalized = keywords.trimmed().toLower();
    // A keyword should be a single word. Assume that the user wanted to enter
    // an alternative keyword and hit space instead of the comma key.
    normalized.replace(u' ', u',');
    QStringList keys = normalized.split(u',', Qt::SkipEmptyParts);
    keys.removeDuplicates(); // #169801
    return keys;
}

QString WebShortcuts::keywordConflicts(const QString &keywords, int row) const
{
    const SearchProvider *self = row >= 0 ? providerAt(row) : nullptr;
    const QList<SearchProvider *> providers = m_providersModel->providers();

    // Look at each keyword the user entered and wade through the search
    // provider list in search of a conflicting keyword. Do not continue
    // search after finding one, because keywords should be assigned only once.
    QStringList conflicts;
    QString firstKey;
    QString firstName;
    const QStringList keys = parseKeywords(keywords);
    for (const QString &key : keys) {
        const auto it = std::ranges::find_if(providers, [self, &key](const SearchProvider *provider) {
            return provider != self && provider->keys().contains(key);
        });
        if (it == providers.end()) {
            continue;
        }
        if (conflicts.isEmpty()) {
            firstKey = key;
            firstName = (*it)->name();
        }
        conflicts.append(i18nc("@info - web search keyword (e.g. gg): what it refers to (e.g. Google)", "- %1: \"%2\"", key, (*it)->name()));
    }

    if (conflicts.isEmpty()) {
        return {};
    }
    if (conflicts.size() == 1) {
        return i18nc("@info", "The keyword \"%1\" is already assigned to \"%2\". Please choose a different one.", firstKey, firstName);
    }
    return i18nc("@info %1 is a list of keywords and the providers they are assigned to",
                 "The following keywords are already assigned. Please choose different ones.\n%1",
                 conflicts.join(u'\n'));
}

QString WebShortcuts::clipboardUrl() const
{
    const QString text = QGuiApplication::clipboard()->text().trimmed();
    return QUrl(text).host().isEmpty() ? QString() : text;
}

void WebShortcuts::addProvider(const QString &name, const QString &query, const QString &keywords, const QString &charset)
{
    auto provider = std::make_unique<SearchProvider>();
    provider->setName(name.trimmed());
    provider->setQuery(query.trimmed());
    provider->setKeys(parseKeywords(keywords));
    provider->setCharset(charset.trimmed());
    provider->setDirty(true);

    SearchProvider *ptr = provider.get();
    m_newProviders.push_back(std::move(provider));
    m_deletedProviders.removeAll(ptr->desktopEntryName());
    m_providersModel->addProvider(ptr);
    Q_EMIT providersChanged();
}

void WebShortcuts::changeProvider(int row, const QString &query, const QString &keywords, const QString &charset)
{
    SearchProvider *provider = providerAt(row);
    if (!provider) {
        return;
    }

    const QString newQuery = query.trimmed();
    const QStringList newKeys = parseKeywords(keywords);
    const QString newCharset = charset.trimmed();
    if (newQuery == provider->query() && newKeys == provider->keys() && newCharset == provider->charset()) {
        return;
    }

    provider->setQuery(newQuery);
    provider->setKeys(newKeys);
    provider->setCharset(newCharset);
    provider->setDirty(true);
    m_providersModel->changeProvider(provider);
    Q_EMIT providersChanged();
}

void WebShortcuts::removeProvider(int row)
{
    SearchProvider *provider = providerAt(row);
    if (!provider) {
        return;
    }

    const QString name = provider->desktopEntryName();
    m_deletedProviders.append(name);
    m_providersModel->deleteProvider(provider);
    if (m_defaultProvider == name) {
        setDefaultProvider(QString());
    }
    Q_EMIT providersChanged();
}

SearchProvider *WebShortcuts::providerAt(int row) const
{
    const QModelIndex sourceIndex = m_proxyModel->mapToSource(m_proxyModel->index(row, 0));
    if (!sourceIndex.isValid()) {
        return nullptr;
    }
    return m_providersModel->providers().at(sourceIndex.row());
}

void WebShortcuts::markAsChanged()
{
    setNeedsSave(true);
    updateRepresentsDefaults();
}

void WebShortcuts::updateRepresentsDefaults()
{
    setRepresentsDefaults(m_webShortcutsEnabled && !m_usePreferredOnly && m_keywordDelimiter == s_defaultDelimiter
                          && m_defaultProvider == defaultDefaultProvider() && preferredProvidersAreDefault());
}

#include "webshortcuts.moc"

#include "moc_webshortcuts.cpp"
