/* This file is part of the KDE Project
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

#include "managermodule.h"

#include <config.h>
#include <config-media.h>

#include "managermodule.moc"

#include <KLocale>
#include <QCheckBox>
#include <QtDBus/QtDBus>

#include "managermoduleview.h"
#include "mediamanagersettings.h"

ManagerModule::ManagerModule(const KComponentData &componentData, QWidget* parent)
	: KCModule(componentData, parent)
{
	ManagerModuleView *view = new ManagerModuleView( this );

	addConfig(  MediaManagerSettings::self(), view );

#ifndef COMPILE_HALBACKEND
	QString hal_text = view->kcfg_HalBackendEnabled->text();
	hal_text += " ("+i18n("No support for HAL on this system")+')';
	view->kcfg_HalBackendEnabled->setText( hal_text );
	view->kcfg_HalBackendEnabled->setEnabled( false );
#endif

#ifndef COMPILE_LINUXCDPOLLING
	QString poll_text = view->kcfg_CdPollingEnabled->text();
	poll_text += " ("+i18n("No support for CD polling on this system")+')';
	view->kcfg_CdPollingEnabled->setText( poll_text );
	view->kcfg_CdPollingEnabled->setEnabled( false );
#endif

	load();
}

void ManagerModule::save()
{
	KCModule::save();

	QDBusInterface mediamanager( "org.kde.kded", "/modules/mediamanager", "org.kde.MediaManager" );
	mediamanager.call( "reloadBackends" );
}

