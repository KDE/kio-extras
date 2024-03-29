/*
    SPDX-FileCopyrightText: 2000 Yves Arrouye <yves@realnames.com>
    SPDX-FileCopyrightText: 2002, 2003 Dawit Alemayehu <adawit@kde.org>
    SPDX-FileCopyrightText: 2009 Nick Shaforostoff <shaforostoff@kde.ru>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef IKWSOPTS_H
#define IKWSOPTS_H

#include <QLayout>
#include <QTabWidget>

#include <KCModule>

#include "searchproviderregistry.h"
#include "ui_ikwsopts_ui.h"

class SearchProvider;
class ProvidersModel;

class FilterOptions : public KCModule
{
    Q_OBJECT

public:
    explicit FilterOptions(QObject *parent, const KPluginMetaData &data);

    void load() override;
    void save() override;
    void defaults() override;

private Q_SLOTS:
    void updateSearchProviderEditingButons();
    void addSearchProvider();
    void changeSearchProvider();
    void deleteSearchProvider();

private:
    void setDelimiter(char);
    char delimiter();
    void setDefaultEngine(int);

    // The names of the providers that the user deleted,
    // these are marked as deleted in the user's homedirectory
    // on save if a global service file exists for it.
    QStringList m_deletedProviders;
    ProvidersModel *m_providersModel;
    SearchProviderRegistry m_registry;

    Ui::FilterOptionsUI m_dlg;
    // Keep in sync with KIO!
    const QStringList m_defaultProviders{
        QStringLiteral("google"),
        QStringLiteral("youtube"),
        QStringLiteral("yahoo"),
        QStringLiteral("wikipedia"),
        QStringLiteral("wikit"),
    };
};

#endif
