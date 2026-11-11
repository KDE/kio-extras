/*
    SPDX-FileCopyrightText: 2009 Nick Shaforostoff <shaforostoff@kde.ru>
    SPDX-FileCopyrightText: 2026 Tobias Fella <tobias.fella@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "providersmodel.h"

#include "searchprovider.h"

#include <algorithm>

using namespace Qt::StringLiterals;

ProvidersModel::ProvidersModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

QVariant ProvidersModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid)) {
        return {};
    }

    const auto provider = m_providers.at(index.row());

    switch (role) {
    case NameRole:
        return provider->name();
    case IconNameRole:
        return provider->iconName();
    case DesktopEntryNameRole:
        return provider->desktopEntryName();
    case KeysRole:
        return provider->keys();
    case QueryRole:
        return provider->query();
    case CharsetRole:
        return provider->charset();
    case HostRole: {
        const auto query = provider->query();
        auto start = query.indexOf("://"_L1);
        if (start == -1) {
            return QString();
        }
        start += 3;
        const auto end = query.indexOf(u'/', start);
        return end == -1 ? query.mid(start) : query.mid(start, end - start);
    }
    case PreferredRole:
        return m_favoriteEngines.contains(provider->desktopEntryName());
    }

    return {};
}

bool ProvidersModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != PreferredRole || !checkIndex(index, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid)) {
        return false;
    }

    const auto name = m_providers.at(index.row())->desktopEntryName();
    if (value.toBool() == m_favoriteEngines.contains(name)) {
        return false;
    }

    if (value.toBool()) {
        m_favoriteEngines.insert(name);
    } else {
        m_favoriteEngines.remove(name);
    }
    Q_EMIT dataChanged(index, index, {PreferredRole});
    Q_EMIT dataModified();
    return true;
}

int ProvidersModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_providers.size();
}

QHash<int, QByteArray> ProvidersModel::roleNames() const
{
    return {
        {NameRole, "name"},
        {IconNameRole, "iconName"},
        {DesktopEntryNameRole, "desktopEntryName"},
        {KeysRole, "keys"},
        {QueryRole, "query"},
        {CharsetRole, "charset"},
        {HostRole, "host"},
        {PreferredRole, "preferred"},
    };
}

void ProvidersModel::setProviders(const QList<SearchProvider *> &providers, const QStringList &favoriteEngines)
{
    beginResetModel();
    m_providers = providers;
    m_favoriteEngines = QSet<QString>(favoriteEngines.begin(), favoriteEngines.end());
    endResetModel();
}

void ProvidersModel::setFavoriteProviders(const QStringList &favoriteEngines)
{
    const QSet<QString> favorites(favoriteEngines.begin(), favoriteEngines.end());
    if (favorites == m_favoriteEngines) {
        return;
    }
    m_favoriteEngines = favorites;
    if (!m_providers.isEmpty()) {
        Q_EMIT dataChanged(index(0), index(m_providers.size() - 1), {PreferredRole});
    }
    Q_EMIT dataModified();
}

void ProvidersModel::addProvider(SearchProvider *provider)
{
    beginInsertRows({}, m_providers.size(), m_providers.size());
    m_providers.append(provider);
    endInsertRows();
    Q_EMIT dataModified();
}

void ProvidersModel::deleteProvider(SearchProvider *provider)
{
    const int row = m_providers.indexOf(provider);
    if (row == -1) {
        return;
    }
    beginRemoveRows({}, row, row);
    m_favoriteEngines.remove(m_providers.takeAt(row)->desktopEntryName());
    endRemoveRows();
    Q_EMIT dataModified();
}

void ProvidersModel::changeProvider(SearchProvider *provider)
{
    const int row = m_providers.indexOf(provider);
    if (row == -1) {
        return;
    }
    Q_EMIT dataChanged(index(row), index(row));
    Q_EMIT dataModified();
}

QStringList ProvidersModel::favoriteEngines() const
{
    return QStringList(m_favoriteEngines.cbegin(), m_favoriteEngines.cend());
}

QList<SearchProvider *> ProvidersModel::providers() const
{
    return m_providers;
}

ProvidersProxyModel::ProvidersProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
    setSortCaseSensitivity(Qt::CaseInsensitive);
    setSortLocaleAware(true);
    setSortRole(ProvidersModel::NameRole);
    sort(0);
}

QString ProvidersProxyModel::filterText() const
{
    return m_filterText;
}

void ProvidersProxyModel::setFilterText(const QString &filterText)
{
    if (m_filterText == filterText) {
        return;
    }
    beginFilterChange();
    m_filterText = filterText;
    endFilterChange(QSortFilterProxyModel::Direction::Rows);
    Q_EMIT filterTextChanged();
}

bool ProvidersProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if (m_filterText.isEmpty()) {
        return true;
    }

    const auto index = sourceModel()->index(sourceRow, 0, sourceParent);
    if (index.data(ProvidersModel::NameRole).toString().contains(m_filterText, Qt::CaseInsensitive)) {
        return true;
    }
    const auto keys = index.data(ProvidersModel::KeysRole).toStringList();
    return std::ranges::any_of(keys, [this](const QString &key) {
        return key.contains(m_filterText, Qt::CaseInsensitive);
    });
}

#include "moc_providersmodel.cpp"
