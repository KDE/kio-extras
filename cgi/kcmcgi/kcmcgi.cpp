/*
   Copyright (C) 2002 Cornelius Schumacher <schumacher@kde.org>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <KAboutData>
#include <KComponentData>
#include <KConfig>
#include <KFileDialog>
#include <KGenericFactory>
#include <KGlobal>
#include <KHBox>
#include <KLocale>

#include <Q3GroupBox>
#include <QLayout>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

#include "kcmcgi.h"
#include "kcmcgi.moc"

typedef KGenericFactory<KCMCgi> KCMCgiFactory;
K_EXPORT_COMPONENT_FACTORY(cgi, KCMCgiFactory("kcmcgi"))

KCMCgi::KCMCgi(QWidget *parent, const QStringList &)
  : KCModule(KCMCgiFactory::componentData(), parent)
{
  setButtons(Default|Apply);

  QVBoxLayout *topLayout = new QVBoxLayout(this);
  topLayout->setSpacing(KDialog::spacingHint());

  Q3GroupBox *topBox = new Q3GroupBox( 1, Qt::Horizontal, i18n("Paths to Local CGI Programs"), this );
  topLayout->addWidget( topBox );

  mListBox = new QListWidget( topBox );

  KHBox *buttonBox = new KHBox( topBox );
  buttonBox->setSpacing( KDialog::spacingHint() );

  mAddButton = new QPushButton( i18n("Add..."), buttonBox );
  connect( mAddButton, SIGNAL( clicked() ), SLOT( addPath() ) );

  mRemoveButton = new QPushButton( i18n("Remove"), buttonBox );
  connect( mRemoveButton, SIGNAL( clicked() ), SLOT( removePath() ) );
  connect( mListBox, SIGNAL(itemClicked(QListWidgetItem*)),this, SLOT(slotItemSelected(QListWidgetItem*)));

  mConfig = new KConfig("kcmcgirc", KConfig::NoGlobals);

  load();
  updateButton();
  KAboutData *about =
    new KAboutData( I18N_NOOP("kcmcgi"), 0,
                    ki18n("CGI KIO Slave Control Module"),
                    0, KLocalizedString(), KAboutData::License_GPL,
                    ki18n("(c) 2002 Cornelius Schumacher") );

  about->addAuthor( ki18n("Cornelius Schumacher"), KLocalizedString(), "schumacher@kde.org" );
  setAboutData(about);
}

KCMCgi::~KCMCgi()
{
  delete mConfig;
}

void KCMCgi::slotItemSelected( QListWidgetItem * )
{
    updateButton();
}

void KCMCgi::updateButton()
{
    mRemoveButton->setEnabled( !mListBox->selectedItems().isEmpty() );
}

void KCMCgi::defaults()
{
  mListBox->clear();
  updateButton();
}

void KCMCgi::save()
{
  QStringList paths;

  int i;
  for( i = 0; i < mListBox->count(); ++i ) {
    paths.append( mListBox->item(i)->text() );
  }

  mConfig->group("General").writeEntry( "Paths", paths );

  mConfig->sync();
}

void KCMCgi::load()
{
  QStringList paths = mConfig->group("General").readEntry( "Paths" , QStringList() );

  mListBox->addItems( paths );
}

void KCMCgi::addPath()
{
  QString path = KFileDialog::getExistingDirectory( QString(), this );

  if ( !path.isEmpty() ) {
    mListBox->addItem( path );
    emit changed( true );
  }
  updateButton();
}

void KCMCgi::removePath()
{
  int index = mListBox->currentRow();
  if ( index >= 0 ) {
    delete mListBox->takeItem( index );
    emit changed( true );
  }
  updateButton();
}

QString KCMCgi::quickHelp() const
{
  return i18n("<h1>CGI Scripts</h1> The CGI KIO slave lets you execute "
              "local CGI programs without the need to run a web server. "
              "In this control module you can configure the paths that "
              "are searched for CGI scripts.");
}
