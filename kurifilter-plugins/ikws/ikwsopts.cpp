/*
 * Copyright (c) 2000 Yves Arrouye <yves@realnames.com>
 * Copyright (c) 2001, 2002 Dawit Alemayehu <adawit@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "ikwsopts.h"

#include <QFile>

#include <QtDBus/QtDBus>

#include <kbuildsycocaprogressdialog.h>
#include <kstandarddirs.h>
#include <kservicetypetrader.h>

#include "kuriikwsfiltereng.h"
#include "searchprovider.h"
#include "searchproviderdlg.h"

class SearchProviderItem : public QTreeWidgetItem
{
public:
    SearchProviderItem(QTreeWidget *parent, SearchProvider *provider)
    :QTreeWidgetItem(parent, QStringList() << provider->name()), m_provider(provider)
    {
      setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
      update();
    }

    virtual ~SearchProviderItem()
    {
      delete m_provider;
    }

    void update()
    {
      setText(0, m_provider->name());
      setText(1, m_provider->keys().join(","));
    }

    SearchProvider *provider() const { return m_provider; }

private:
    SearchProvider *m_provider;
};

FilterOptions::FilterOptions(const KComponentData &componentData, QWidget *parent)
              :KCModule(componentData, parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout( this );

    m_dlg = new FilterOptionsUI (this);
    mainLayout->addWidget(m_dlg);

    m_dlg->lvSearchProviders->setColumnWidth(0, 200);

    // Connect all the signals/slots...
    connect(m_dlg->cbEnableShortcuts, SIGNAL(toggled(bool)), this,
            SLOT(setWebShortcutState()));
    connect(m_dlg->cbEnableShortcuts, SIGNAL(toggled(bool)), this,
            SLOT(configChanged()));

    connect(m_dlg->lvSearchProviders, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)),
           this, SLOT(updateSearchProvider()));
    connect(m_dlg->lvSearchProviders, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)),
           this, SLOT(changeSearchProvider()));
    connect(m_dlg->lvSearchProviders, SIGNAL(itemChanged(QTreeWidgetItem *, int)),
           this, SLOT(checkFavoritesChanged()));

    connect(m_dlg->cmbDefaultEngine, SIGNAL(currentIndexChanged(int)), this,
            SLOT(configChanged()));
    connect(m_dlg->cmbDelimiter, SIGNAL(currentIndexChanged(int)), this,
            SLOT(configChanged()));

    connect(m_dlg->pbNew, SIGNAL(clicked()), this, SLOT(addSearchProvider()));
    connect(m_dlg->pbChange, SIGNAL(clicked()), this, SLOT(changeSearchProvider()));
    connect(m_dlg->pbDelete, SIGNAL(clicked()), this, SLOT(deleteSearchProvider()));
}

QString FilterOptions::quickHelp() const
{
    return i18n("<p>In this module you can configure the web shortcuts feature. "
                "Web shortcuts allow you to quickly search or lookup words on "
                "the Internet. For example, to search for information about the "
                "KDE project using the Google engine, you simply type <b>gg:KDE</b> "
                "or <b>google:KDE</b>.</p>"
                "<p>If you select a default search engine, normal words or phrases "
                "will be looked up at the specified search engine by simply typing "
                "them into applications, such as Konqueror, that have built-in support "
                "for such a feature.</p>");
}

void FilterOptions::load()
{
    // Clear state first.
    m_dlg->lvSearchProviders->clear();

    KConfig config( KURISearchFilterEngine::self()->name() + "rc", KConfig::NoGlobals );
    KConfigGroup group = config.group("General");

    QString defaultSearchEngine = group.readEntry("DefaultSearchEngine");

    m_favoriteEngines.clear();
    m_favoriteEngines << "google" << "google_groups" << "google_news" << "webster" << "dmoz" << "wikipedia";
    m_favoriteEngines = group.readEntry("FavoriteSearchEngines", m_favoriteEngines );

    const KService::List services = KServiceTypeTrader::self()->query("SearchProvider");

    m_dlg->lvSearchProviders->blockSignals(true); // do not emit itemChanged signals
    for (KService::List::ConstIterator it = services.begin();
         it != services.end(); ++it)
    {
      displaySearchProvider(new SearchProvider(*it),
                            ((*it)->desktopEntryName() == defaultSearchEngine));
    }
    m_dlg->lvSearchProviders->blockSignals(false);

    bool webShortcutsEnabled = group.readEntry("EnableWebShortcuts", true);
    m_dlg->cbEnableShortcuts->setChecked( webShortcutsEnabled );

    QString delimiter = group.readEntry ("KeywordDelimiter", ":");
    setDelimiter( delimiter[0].toLatin1() );

    // Update the GUI to reflect the config options read above...
    setWebShortcutState();

    if (m_dlg->lvSearchProviders->topLevelItemCount() > 0)
      m_dlg->lvSearchProviders->setCurrentItem(m_dlg->lvSearchProviders->topLevelItem(0));
}

char FilterOptions::delimiter ()
{
  switch (m_dlg->cmbDelimiter->currentIndex())
  {
    case 1:
      return ' ';
    case 0:
    default:
      return ':';
  };
}

void FilterOptions::setDelimiter (char sep)
{
  switch (sep)
  {
    case ' ':
      m_dlg->cmbDelimiter->setCurrentIndex (1);
      break;
    case ':':
    default:
      m_dlg->cmbDelimiter->setCurrentIndex (0);
  };
}

void FilterOptions::save()
{
  KConfig config( KURISearchFilterEngine::self()->name() + "rc", KConfig::NoGlobals );

  KConfigGroup group = config.group("General");
  group.writeEntry("EnableWebShortcuts", m_dlg->cbEnableShortcuts->isChecked());
  group.writeEntry("KeywordDelimiter", QString( delimiter() ) );

  QString engine;

  if (m_dlg->cmbDefaultEngine->currentIndex() != 0)
    engine = m_dlg->cmbDefaultEngine->currentText();

  group.writeEntry("DefaultSearchEngine", m_defaultEngineMap[engine]);

  // kDebug () << "Engine: " << m_defaultEngineMap[engine];

  int changedProviderCount = 0;
  QString path = KGlobal::mainComponent().dirs()->saveLocation("services", "searchproviders/");

  m_favoriteEngines.clear();

  QTreeWidgetItemIterator it(m_dlg->lvSearchProviders);
  while (*it) {
    SearchProviderItem *item = dynamic_cast<SearchProviderItem *>(*it);

    Q_ASSERT(item);

    SearchProvider *provider = item->provider();

    QString name = provider->desktopEntryName();

    if (item->checkState(0) == Qt::Checked)
      m_favoriteEngines << name;

    if (provider->isDirty())
    {
      changedProviderCount++;

      if (name.isEmpty())
      {
        // New provider
        // Take the longest search shortcut as filename,
        // if such a file already exists, append a number and increase it
        // until the name is unique
        for (QStringList::ConstIterator it = provider->keys().begin(); it != provider->keys().end(); ++it)
        {
            if ((*it).length() > name.length())
                name = (*it).toLower();
        }
        for (int suffix = 0; ; ++suffix)
        {
            QString located, check = name;
            if (suffix)
                check += QString().setNum(suffix);
            if ((located = KStandardDirs::locate("services", "searchproviders/" + check + ".desktop")).isEmpty())
            {
                name = check;
                break;
            }
            else if (located.left(path.length()) == path)
            {
                // If it's a deleted (hidden) entry, overwrite it
                if (KService(located).isDeleted())
                    break;
            }
        }
      }

      KConfig _service( path + name + ".desktop", KConfig::SimpleConfig );
      KConfigGroup service(&_service, "Desktop Entry");
      service.writeEntry("Type", "Service");
      service.writeEntry("ServiceTypes", "SearchProvider");
      service.writeEntry("Name", provider->name());
      service.writeEntry("Query", provider->query());
      service.writeEntry("Keys", provider->keys());
      service.writeEntry("Charset", provider->charset());

      // we might be overwriting a hidden entry
      service.writeEntry("Hidden", false);
    }
    ++it;
  }

  for (QStringList::ConstIterator it = m_deletedProviders.begin();
      it != m_deletedProviders.end(); ++it)
  {
      QStringList matches = KGlobal::mainComponent().dirs()->findAllResources("services", "searchproviders/" + *it + ".desktop");

      // Shouldn't happen
      if (!matches.count())
          continue;

      if (matches.count() == 1 && matches[0].left(path.length()) == path)
      {
          // If only the local copy existed, unlink it
          // TODO: error handling
          QFile::remove(matches[0]);
          continue;
      }
      KConfig _service( path + *it + ".desktop", KConfig::SimpleConfig );
      KConfigGroup service(&_service, "Desktop Entry");
      service.writeEntry("Type", "Service");
      service.writeEntry("ServiceTypes", "SearchProvider");
      service.writeEntry("Hidden", true);
  }

  group.writeEntry("FavoriteSearchEngines", m_favoriteEngines);
  config.sync();

  emit changed(false);

  // Update filters in running applications...
  QDBusMessage msg = QDBusMessage::createSignal("/", "org.kde.KUriFilterPlugin", "configure");
  QDBusConnection::sessionBus().send(msg);

  // If the providers changed, tell sycoca to rebuild its database...
  if (changedProviderCount)
      KBuildSycocaProgressDialog::rebuildKSycoca(this);
}

void FilterOptions::defaults()
{
  setDelimiter (':');
  m_dlg->cmbDefaultEngine->setCurrentIndex (0);
  m_dlg->cbEnableShortcuts->setChecked( true );
}

void FilterOptions::configChanged()
{
  // kDebug () << "FilterOptions::configChanged: TRUE";
  emit changed(true);
}

void FilterOptions::checkFavoritesChanged()
{
  QStringList currentFavoriteEngines;
  QTreeWidgetItemIterator it(m_dlg->lvSearchProviders);
  while (*it) {
    SearchProviderItem *item = dynamic_cast<SearchProviderItem *>(*it);

    Q_ASSERT(item);

    if (item->checkState(0) == Qt::Checked)
      currentFavoriteEngines << item->provider()->desktopEntryName();

    ++it;
  }

  currentFavoriteEngines.sort(); // make sure that both lists do have the same sorting
  m_favoriteEngines.sort();

  if (!(currentFavoriteEngines==m_favoriteEngines)) {
    m_favoriteEngines=currentFavoriteEngines;
    configChanged();
  }
}

void FilterOptions::setWebShortcutState()
{
  bool use_keywords = m_dlg->cbEnableShortcuts->isChecked();
  m_dlg->lvSearchProviders->setEnabled(use_keywords);
  bool itemActive = m_dlg->lvSearchProviders->currentItem() != 0;
  m_dlg->pbNew->setEnabled(use_keywords);
  m_dlg->pbChange->setEnabled(use_keywords && itemActive);
  m_dlg->pbDelete->setEnabled(use_keywords && itemActive);
  m_dlg->lbDelimiter->setEnabled (use_keywords);
  m_dlg->cmbDelimiter->setEnabled (use_keywords);
  m_dlg->lbDefaultEngine->setEnabled (use_keywords);
  m_dlg->cmbDefaultEngine->setEnabled (use_keywords);
}

void FilterOptions::addSearchProvider()
{
  SearchProviderDialog dlg(0, this);

  // Provider can be 0 if the user chooses accept but has not entered \{...} in the query and chooses cancel then.
  if (dlg.exec() && dlg.provider())
  {
      m_dlg->lvSearchProviders->setCurrentItem(displaySearchProvider(dlg.provider()));
      configChanged();
  }
}

void FilterOptions::changeSearchProvider()
{
  SearchProviderItem *item = dynamic_cast<SearchProviderItem *>(m_dlg->lvSearchProviders->currentItem());
  Q_ASSERT(item);

  SearchProviderDialog dlg(item->provider(), this);

  if (dlg.exec())
  {
    m_dlg->lvSearchProviders->setCurrentItem(displaySearchProvider(dlg.provider()));
    configChanged();
  }
}

void FilterOptions::deleteSearchProvider()
{
  SearchProviderItem *item = dynamic_cast<SearchProviderItem *>(m_dlg->lvSearchProviders->currentItem());
  Q_ASSERT(item);

  // Update the combo box to go to None if the fallback was deleted.
  int current = m_dlg->cmbDefaultEngine->currentIndex();
  for (int i = 1, count = m_dlg->cmbDefaultEngine->count(); i < count; ++i)
  {
    if (m_dlg->cmbDefaultEngine->itemText(i) == item->provider()->name())
    {
      m_dlg->cmbDefaultEngine->removeItem(i);
      if (i == current)
        m_dlg->cmbDefaultEngine->setCurrentIndex(0);
      else if (current > i)
        m_dlg->cmbDefaultEngine->setCurrentIndex(current - 1);

      break;
    }
  }

  if (m_dlg->lvSearchProviders->topLevelItem(current + 1))
      m_dlg->lvSearchProviders->setCurrentItem(m_dlg->lvSearchProviders->topLevelItem(current + 1));
  else if (m_dlg->lvSearchProviders->topLevelItem(current - 1))
      m_dlg->lvSearchProviders->setCurrentItem(m_dlg->lvSearchProviders->topLevelItem(current - 1));

  if (!item->provider()->desktopEntryName().isEmpty())
      m_deletedProviders.append(item->provider()->desktopEntryName());

  delete item;
  updateSearchProvider();
  configChanged();
}

void FilterOptions::updateSearchProvider()
{
  m_dlg->pbChange->setEnabled(m_dlg->lvSearchProviders->currentItem());
  m_dlg->pbDelete->setEnabled(m_dlg->lvSearchProviders->currentItem());
}

SearchProviderItem *FilterOptions::displaySearchProvider(SearchProvider *p, bool fallback)
{
  // Show the provider in the list.
  SearchProviderItem *item = 0L;

  QTreeWidgetItemIterator it(m_dlg->lvSearchProviders);
  while (*it)
  {
    if ((*it)->text(0) == p->name())
    {
      item = dynamic_cast<SearchProviderItem *>(*it);
      Q_ASSERT(item);
      break;
    }
    ++it;
  }

  if (item)
    item->update ();
  else
  {
    // Put the name in the default search engine combo box.
    int itemCount;
    int totalCount = m_dlg->cmbDefaultEngine->count();

    item = new SearchProviderItem(m_dlg->lvSearchProviders, p);

    if (m_favoriteEngines.contains(p->desktopEntryName()))
       item->setCheckState(0, Qt::Checked);
    else
       item->setCheckState(0, Qt::Unchecked);

    for (itemCount = 1; itemCount < totalCount; itemCount++)
    {
      if (m_dlg->cmbDefaultEngine->itemText(itemCount) > p->name())
      {
        int currentItem = m_dlg->cmbDefaultEngine->currentIndex();
        m_dlg->cmbDefaultEngine->insertItem(itemCount, p->name());
        m_defaultEngineMap[p->name ()] = p->desktopEntryName ();
        if (currentItem >= itemCount)
          m_dlg->cmbDefaultEngine->setCurrentIndex(currentItem+1);
        break;
      }
    }

    // Append it to the end of the list...
    if (itemCount == totalCount)
    {
      m_dlg->cmbDefaultEngine->insertItem(itemCount, p->name());
      m_defaultEngineMap[p->name ()] = p->desktopEntryName ();
    }

    if (fallback)
      m_dlg->cmbDefaultEngine->setCurrentIndex(itemCount);

  if (!(*it))
    m_dlg->lvSearchProviders->sortItems(0, Qt::AscendingOrder);
  }

  return item;
}

#include "ikwsopts.moc"
