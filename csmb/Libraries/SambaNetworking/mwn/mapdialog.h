/* Name: mapdialog.h

   Description: This file is a part of the libmwn library.

   Author:	Oleg Noskov (olegn@corel.com)

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.


*/
 

/*
 * NOTE: This source file was created using tab size = 2.
 * Please respect that setting in case of modifications.
 */

/**********************************************************************

	--- Qt Architect generated file ---

	File: mapdialog.h
	Last generated: Wed Nov 4 11:07:25 1998

 *********************************************************************/

#ifndef CMapDialog_included
#define CMapDialog_included

#include "common.h"
#include "mapdialogdata.h"

class CMSWindowsNetworkItem;

class CMapDialog : public CMapDialogData
{
    Q_OBJECT

public:

    CMapDialog
    (
		LPCSTR MountPoint,
		LPCSTR UNCPath,
		BOOL bReconnectAtLogon,
		int nCredentialsIndex,
		CMSWindowsNetworkItem *pOtherTree,
        QWidget* parent = NULL,
        const char* name = NULL
    );

    virtual ~CMapDialog();

	QString m_MountPoint;
	QString m_UNCPath;
	BOOL m_bReconnectAtLogon;
	int m_nCredentialsIndex;
	QString m_DefaultConnectAs;
	BOOL m_bAbort;

public slots:
	void OnBrowse();
	void done(int r);
	void OnDoubleClicked(CListViewItem *pItem);
	void OnUNCEditChanged(const char *);

private:
	void UpdateConnectAs();
protected:
	bool eventFilter(QObject *object, QEvent *event);
};

#endif // CMapDialog_included
