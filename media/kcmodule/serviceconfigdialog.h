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

#ifndef _SERVICECONFIGDIALOG_H_
#define _SERVICECONFIGDIALOG_H_

#include <kdialogbase.h>

#include "notifierserviceaction.h"
#include "serviceview.h"

class ServiceConfigDialog : public KDialogBase
{
	Q_OBJECT

public:
	ServiceConfigDialog(NotifierServiceAction *action,
	                    const QStringList &mimetypesList,
	                    QWidget* parent = 0, const char* name = 0);
	
public slots:
	void slotOk();
	void slotIconChanged();
	void slotSet();

private:
	ServiceView *m_view;
	NotifierServiceAction *m_action;
	bool m_iconChanged;
};

#endif
