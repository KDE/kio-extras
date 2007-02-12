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

#include "notifiermodule.h"

#include <QLayout>
#include <QVBoxLayout>
#include <QList>
#include <QBoxLayout>

#include <klocale.h>
#include <kcombobox.h>
#include <kpushbutton.h>
#include <kstandardguiitem.h>

#include "notifiersettings.h"
#include "serviceconfigdialog.h"
#include "actionlistboxitem.h"
#include "mimetypelistboxitem.h"

NotifierModule::NotifierModule(const KComponentData &componentData, QWidget *parent)
	: KCModule(componentData, parent)
{
	QBoxLayout *layout = new QVBoxLayout( this );
	layout->setSpacing( KDialog::spacingHint() );
	layout->setMargin( 0 );

	m_view = new NotifierModuleView( this );
	layout->addWidget( m_view );

	m_view->addButton->setGuiItem( KStandardGuiItem::add() );
	m_view->editButton->setGuiItem( KStandardGuiItem::properties() );
	m_view->deleteButton->setGuiItem( KStandardGuiItem::del() );

	m_view->mimetypesCombo->addItem( i18n("All Mime Types") );

	QStringList mimetypes = m_settings.supportedMimetypes();

	QStringList::iterator it = mimetypes.begin();
	QStringList::iterator end = mimetypes.end();

#ifdef __GNUC__
#warning "Needs porting. listBox() is no longer supplied in Qt4 QComboBox."
#endif
#if 0
	for ( ; it!=end; ++it )
	{
		new MimetypeListBoxItem( *it, m_view->mimetypesCombo->listBox() );
	}
#endif
	updateListBox();

	connect( m_view->mimetypesCombo, SIGNAL( activated(int) ),
	         this, SLOT( slotMimeTypeChanged(int) ) );
	connect( m_view->actionsList, SIGNAL( selectionChanged(QListWidgetItem*) ),
	         this, SLOT( slotActionSelected(QListWidgetItem*) ) );
	connect( m_view->addButton, SIGNAL( clicked() ),
	         this, SLOT( slotAdd() ) );
	connect( m_view->editButton, SIGNAL( clicked() ),
	         this, SLOT( slotEdit() ) );
	connect( m_view->deleteButton, SIGNAL( clicked() ),
	         this, SLOT( slotDelete() ) );
	connect( m_view->toggleAutoButton, SIGNAL( clicked() ),
	         this, SLOT( slotToggleAuto() ) );
}

NotifierModule::~NotifierModule()
{
}

void NotifierModule::load()
{
	m_settings.reload();
	slotMimeTypeChanged( m_view->mimetypesCombo->currentIndex() );
}

void NotifierModule::save()
{
	m_settings.save();
}

void NotifierModule::defaults()
{
	m_settings.clearAutoActions();
	slotMimeTypeChanged( m_view->mimetypesCombo->currentIndex() );
}

void NotifierModule::updateListBox()
{
	m_view->actionsList->clear();
	slotActionSelected( 0L );

	QList<NotifierAction*> services;
	if ( m_mimetype.isEmpty() )
	{
		services = m_settings.actions();
	}
	else
	{
		services = m_settings.actionsForMimetype( m_mimetype );
	}

	QList<NotifierAction*>::iterator it;

	for ( it = services.begin(); it != services.end(); ++it )
	{
		new ActionListBoxItem( *it, m_mimetype, m_view->actionsList );
	}
}

void NotifierModule::slotActionSelected(QListWidgetItem *item)
{
	NotifierAction *action = 0L;

	if ( item!=0L )
	{
		ActionListBoxItem *action_item
			= static_cast<ActionListBoxItem*>(item);
		action = action_item->action();
	}

	bool isWritable = action!=0L && action->isWritable();
	m_view->deleteButton->setEnabled( isWritable );
	m_view->editButton->setEnabled( isWritable );
	m_view->addButton->setEnabled( true );
	m_view->toggleAutoButton->setEnabled( action!=0L && !m_mimetype.isEmpty() );
}

void NotifierModule::slotMimeTypeChanged(int index)
{
	if ( index == 0 )
	{
		m_mimetype.clear();
	}
	else
	{
#ifdef __GNUC__
#warning "Needs porting. listBox() is no longer supplied in Qt4 QComboBox."
#endif
#if 0
		QListWidgetItem *item = m_view->mimetypesCombo->listBox()->item( index );
		MimetypeListBoxItem *mime_item
			= static_cast<MimetypeListBoxItem*>( item );
		m_mimetype = mime_item->mimetype();
#endif
	}

	updateListBox();
}

void NotifierModule::slotAdd()
{
	NotifierServiceAction *action = new NotifierServiceAction();
	ServiceConfigDialog dialog(action, m_settings.supportedMimetypes(), this);

	int value = dialog.exec();

	if ( value == QDialog::Accepted )
	{
		m_settings.addAction( action );
		updateListBox();
		emit changed( true );
	}
	else
	{
		delete action;
	}
}

void NotifierModule::slotEdit()
{
    QList<QListWidgetItem*> selection = m_view->actionsList->selectedItems();

    // code below assumes a single selection
    Q_ASSERT( selection.count() <= 1 );

	ActionListBoxItem *action_item
		= static_cast<ActionListBoxItem*>(selection.value(0));

	NotifierServiceAction * action;
	if ( (action = dynamic_cast<NotifierServiceAction*>( action_item->action() )) )
	{
		ServiceConfigDialog dialog(action, m_settings.supportedMimetypes(), this);

		int value = dialog.exec();

		if ( value == QDialog::Accepted )
		{
			updateListBox();
			emit changed( true );
		}
	}
}

void NotifierModule::slotDelete()
{
    QList<QListWidgetItem*> selection = m_view->actionsList->selectedItems();

    // code below assumes a single selection
    Q_ASSERT( selection.count() <= 1 );

	ActionListBoxItem *action_item
		= static_cast<ActionListBoxItem*>( selection.value(0) );

	NotifierServiceAction *action;
	if ( (action = dynamic_cast<NotifierServiceAction*>( action_item->action() ) ))
	{
		m_settings.deleteAction( action );
		updateListBox();
		emit changed( true );
	}
}

void NotifierModule::slotToggleAuto()
{
    QList<QListWidgetItem*> selection = m_view->actionsList->selectedItems();

    // code below assumes a single selection
    Q_ASSERT( selection.count() <= 1 );

	ActionListBoxItem *action_item
		= static_cast<ActionListBoxItem*>( selection.value(0) ); 
	NotifierAction *action = action_item->action();

	int index = m_view->actionsList->row( action_item );

	if ( action->autoMimetypes().contains( m_mimetype ) )
	{
		m_settings.resetAutoAction( m_mimetype );
	}
	else
	{
		m_settings.setAutoAction( m_mimetype, action );
	}

	updateListBox();
	emit changed( true );

	m_view->actionsList->item(index)->setSelected( true );
}

#include "notifiermodule.moc"
