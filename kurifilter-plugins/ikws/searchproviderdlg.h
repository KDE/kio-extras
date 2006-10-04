/*
 * Copyright (c) 2000 Malte Starostik <malte@kde.org>
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


#ifndef __SEARCHPROVIDERDLG_H___
#define __SEARCHPROVIDERDLG_H___

#include <kdialog.h>
#include "ui_searchproviderdlg_ui.h"
class SearchProvider;

class SearchProviderDlgUI : public QWidget, public Ui::SearchProviderDlgUI
{
public:
  SearchProviderDlgUI( QWidget *parent ) : QWidget( parent ) {
    setupUi( this );
  }
};


class SearchProviderDialog : public KDialog
{
    Q_OBJECT

public:
    SearchProviderDialog(SearchProvider *provider, QWidget *parent = 0, const char *name = 0);

    SearchProvider *provider() { return m_provider; }

protected Q_SLOTS:
    void slotChanged();
    void slotOk();

private:
    SearchProvider *m_provider;
    SearchProviderDlgUI *m_dlg;
};

#endif
