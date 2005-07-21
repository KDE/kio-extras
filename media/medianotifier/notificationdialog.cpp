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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "notificationdialog.h"

#include <krun.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kio/global.h>
#include <klistbox.h>
#include <qlabel.h>
#include <qcheckbox.h>

#include "actionlistboxitem.h"
#include "notificationdialogview.h"

NotificationDialog::NotificationDialog( KFileItem &medium, NotifierSettings &settings,
                                        QWidget* parent, const char* name )
	: KDialogBase( parent, name, false, i18n( "Medium Detected" ), Ok|Cancel|User1, Ok, true),
	  m_medium(medium), m_settings( settings )
{
	setCaption( KIO::decodeFileName(m_medium.name()) );
	clearWState( WState_Polished );

	m_view = new NotificationDialogView( this );
	
	m_view->iconLabel->setPixmap( m_medium.pixmap(64) );
	m_view->mimetypeLabel->setText( i18n( "<b>Medium Type:</b>" ) + " "
	                              + m_medium.mimeTypePtr()->comment() );
	
	updateActionsListBox();

	resize( QSize(400,400).expandedTo( minimumSizeHint() ) );

	setMainWidget( m_view );
	
	m_actionWatcher = new KDirWatch();
	QString services_dir
		= locateLocal( "data", "konqueror/servicemenus", true );
	m_actionWatcher->addDir( services_dir );

	setButtonText( User1, i18n("Configure") );

	connect( m_actionWatcher, SIGNAL( dirty( const QString & ) ),
	         this, SLOT( slotActionsChanged( const QString & ) ) );
	connect( this , SIGNAL( okClicked() ),
	         this, SLOT( slotOk() ) );
	connect( this, SIGNAL( user1Clicked() ),
	         this, SLOT( slotConfigure() ) );
	connect( m_view->actionsList, SIGNAL( doubleClicked ( QListBoxItem*, const QPoint & ) ),
	         this, SLOT( slotOk() ) );

	m_actionWatcher->startScan();
}

NotificationDialog::~NotificationDialog()
{
	delete m_actionWatcher;
}

void NotificationDialog::updateActionsListBox()
{
	m_view->actionsList->clear();

	QValueList<NotifierAction*> actions
		= m_settings.actionsForMimetype( m_medium.mimetype() );
	
	QValueList<NotifierAction*>::iterator it = actions.begin();
	QValueList<NotifierAction*>::iterator end = actions.end();
	
	for ( ; it!=end; ++it )
	{
		new ActionListBoxItem( *it, m_medium.mimetype(),
		                       m_view->actionsList );
	}

	m_view->actionsList->setSelected( 0, true );
}


void NotificationDialog::slotActionsChanged(const QString &/*dir*/)
{
	m_settings.reload();
	updateActionsListBox();
}

void NotificationDialog::slotOk()
{
	QListBoxItem *item = m_view->actionsList->selectedItem();
	
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
		m_settings.setAutoAction(  m_medium.mimetype(), action );
		m_settings.save();
	}
	
	action->execute(m_medium);
	
	close();
}

void NotificationDialog::slotConfigure()
{
	KRun::runCommand("kcmshell media");
}

#include "notificationdialog.moc"
