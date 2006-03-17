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

#include <kconfig.h>
#include <klocale.h>
#include <kglobal.h>
#include <kinstance.h>
#include <kaboutdata.h>
#include <kfiledialog.h>
#include <khbox.h>

#include <qlayout.h>
#include <q3listbox.h>
#include <qpushbutton.h>
#include <q3groupbox.h>
//Added by qt3to4:
#include <QVBoxLayout>

#include "kcmcgi.h"
#include "kcmcgi.moc"

extern "C"
{
  KDE_EXPORT KCModule *create_cgi( QWidget *parent, const char * )
  {
    return new KCMCgi( parent );
  }
}


KCMCgi::KCMCgi(QWidget *parent)
  : KCModule(new KInstance("kcmcgi"), parent)
{
  setButtons(Default|Apply);

  QVBoxLayout *topLayout = new QVBoxLayout(this);
  topLayout->setSpacing(KDialog::spacingHint());

  Q3GroupBox *topBox = new Q3GroupBox( 1, Qt::Horizontal, i18n("Paths to Local CGI Programs"), this );
  topLayout->addWidget( topBox );

  mListBox = new Q3ListBox( topBox );

  KHBox *buttonBox = new KHBox( topBox );
  buttonBox->setSpacing( KDialog::spacingHint() );

  mAddButton = new QPushButton( i18n("Add..."), buttonBox );
  connect( mAddButton, SIGNAL( clicked() ), SLOT( addPath() ) );

  mRemoveButton = new QPushButton( i18n("Remove"), buttonBox );
  connect( mRemoveButton, SIGNAL( clicked() ), SLOT( removePath() ) );
  connect( mListBox, SIGNAL( clicked ( Q3ListBoxItem * )),this, SLOT( slotItemSelected( Q3ListBoxItem *)));

  mConfig = new KConfig("kcmcgirc");

  load();
  updateButton();
  KAboutData *about =
    new KAboutData( I18N_NOOP("kcmcgi"),
                    I18N_NOOP("CGI KIO Slave Control Module"),
                    0, 0, KAboutData::License_GPL,
                    I18N_NOOP("(c) 2002 Cornelius Schumacher") );

  about->addAuthor( "Cornelius Schumacher", 0, "schumacher@kde.org" );
  setAboutData(about);
}

KCMCgi::~KCMCgi()
{
  delete mConfig;
}

void KCMCgi::slotItemSelected( Q3ListBoxItem * )
{
    updateButton();
}

void KCMCgi::updateButton()
{
    mRemoveButton->setEnabled( mListBox->selectedItem ());
}

void KCMCgi::defaults()
{
  mListBox->clear();
  updateButton();
}

void KCMCgi::save()
{
  QStringList paths;

  uint i;
  for( i = 0; i < mListBox->count(); ++i ) {
    paths.append( mListBox->text( i ) );
  }

  mConfig->setGroup( "General" );
  mConfig->writeEntry( "Paths", paths );

  mConfig->sync();
}

void KCMCgi::load()
{
  mConfig->setGroup( "General" );
  QStringList paths = mConfig->readEntry( "Paths" , QStringList() );

  mListBox->insertStringList( paths );
}

void KCMCgi::addPath()
{
  QString path = KFileDialog::getExistingDirectory( QString(), this );

  if ( !path.isEmpty() ) {
    mListBox->insertItem( path );
    emit changed( true );
  }
  updateButton();
}

void KCMCgi::removePath()
{
  int index = mListBox->currentItem();
  if ( index >= 0 ) {
    mListBox->removeItem( index );
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
