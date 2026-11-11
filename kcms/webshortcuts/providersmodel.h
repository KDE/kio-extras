/*
    SPDX-FileCopyrightText: 2009 Nick Shaforostoff <shaforostoff@kde.ru>
    SPDX-FileCopyrightText: 2026 Tobias Fella <tobias.fella@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QAbstractListModel>
#include <QSet>
#include <QSortFilterProxyModel>

class SearchProvider;

class ProvidersModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        NameRole = Qt::DisplayRole,
        IconNameRole = Qt::DecorationRole,
        DesktopEntryNameRole = Qt::UserRole,
        KeysRole,
        QueryRole,
        CharsetRole,
        HostRole,
        PreferredRole,
    };
    Q_ENUM(Roles)

    explicit ProvidersModel(QObject *parent = nullptr);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setProviders(const QList<SearchProvider *> &providers, const QStringList &favoriteEngines);
    void setFavoriteProviders(const QStringList &favoriteEngines);
    void addProvider(SearchProvider *provider);
    void deleteProvider(SearchProvider *provider);
    void changeProvider(SearchProvider *provider);

    QStringList favoriteEngines() const;
    QList<SearchProvider *> providers() const;

Q_SIGNALS:
    void dataModified();

private:
    QSet<QString> m_favoriteEngines;
    QList<SearchProvider *> m_providers;
};

/*!
 * Sorts the providers by name and filters them by name or keyword.
 */
class ProvidersProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(QString filterText READ filterText WRITE setFilterText NOTIFY filterTextChanged)

public:
    explicit ProvidersProxyModel(QObject *parent = nullptr);

    QString filterText() const;
    void setFilterText(const QString &filterText);

Q_SIGNALS:
    void filterTextChanged();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    QString m_filterText;
};
