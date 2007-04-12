/* This file is part of the KDE Project
   Copyright (c) 2005 Jean-Remy Falleri <jr.falleri@laposte.net>
   Copyright (c) 2005 Kévin Ottens <ervin ipsquad net>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "notificationdialog.h"
#include <QtGui/QLayout>

#include <krun.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kio/global.h>
#include <klistwidget.h>
#include <QLabel>
#include <QCheckBox>
#include <QLinkedList>

#include "actionlistboxitem.h"
#include "notificationdialogview.h"

NotificationDialog::NotificationDialog( KFileItem medium, NotifierSettings *settings,
                                        QWidget* parent, const char* name )
	: KDialog( parent ),
	  m_medium(medium), m_settings( settings )
{
	setCaption( KIO::decodeFileName(m_medium.name()) );
  setObjectName( name );
  setModal( false );
  setButtons( Ok|Cancel|User1 );
  setDefaultButton( Ok );
  showButtonSeparator( true );
	//clearWState(  WState_Polished );

	QWidget *page = new QWidget( this );
	setMainWidget(page);
	QVBoxLayout *topLayout = new QVBoxLayout( page );
	topLayout->setMargin( 0 );
	topLayout->setSpacing( spacingHint() );

	m_view = new NotificationDialogView( page );

	topLayout->addWidget(m_view);
	m_view->iconLabel->setPixmap( m_medium.pixmap(64) );
	m_view->mimetypeLabel->setText( i18n( "<b>Medium Type:</b>" ) + ' '
	                              + m_medium.mimeTypePtr()->comment() );

	updateActionsListBox();

	resize( QSize(400,400).expandedTo( minimumSizeHint() ) );


	m_actionWatcher = new KDirWatch();
	QString services_dir
		= KStandardDirs::locateLocal( "data", "konqueror/servicemenus", true );
	m_actionWatcher->addDir( services_dir );

	setButtonText( User1, i18n("Configure") );

	connect( m_actionWatcher, SIGNAL( dirty( const QString & ) ),
	         this, SLOT( slotActionsChanged( const QString & ) ) );
	connect( this , SIGNAL( okClicked() ),
	         this, SLOT( slotOk() ) );
	connect( this, SIGNAL( user1Clicked() ),
	         this, SLOT( slotConfigure() ) );
	connect( m_view->actionsList, SIGNAL( doubleClicked ( QListWidgetItem*, const QPoint & ) ),
	         this, SLOT( slotOk() ) );

	connect( this, SIGNAL( finished() ),
	         this, SLOT( delayedDestruct() ) );

	m_actionWatcher->startScan();
}

NotificationDialog::~NotificationDialog()
{
	delete m_actionWatcher;
	delete m_settings;
}

void NotificationDialog::updateActionsListBox()
{
	m_view->actionsList->clear();

	QList<NotifierAction*> actions
		= m_settings->actionsForMimetype( m_medium.mimetype() );

	QList<NotifierAction*>::iterator it = actions.begin();
	QList<NotifierAction*>::iterator end = actions.end();

	for ( ; it!=end; ++it )
	{
		new ActionListBoxItem( *it, m_medium.mimetype(),
		                       m_view->actionsList );
	}

	m_view->actionsList->item(0)->setSelected(true);
}


void NotificationDialog::slotActionsChanged(const QString &/*dir*/)
{
	m_settings->reload();
	updateActionsListBox();
}

void NotificationDialog::slotOk()
{
	QListWidgetItem *item = m_view->actionsList->selectedItems().value(0);

	if ( item != 0L )
	{
		ActionListBoxItem *action_item
			= static_cast<ActionListBoxItem*>( item );
		NotifierAction *action = action_item->action();

		launchAction( action );
	}
}

void NotificationDialog::launchAction( NotifierAction *action )
{
	if ( m_view->autoActionCheck->isChecked() )
	{
		m_settings->setAutoAction(  m_medium.mimetype(), action );
		m_settings->save();
	}

	action->execute(m_medium);

	close();
}

void NotificationDialog::slotConfigure()
{
	KRun::runCommand("kcmshell media");
}

#include "notificationdialog.moc"
