/* Name: PrinterSelectionDialog.h

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

	File: PrinterSelectionDialog.h
	Last generated: Mon Aug 23 14:48:14 1999

 *********************************************************************/

#ifndef CPrinterSelectionDialog_included
#define CPrinterSelectionDialog_included

#include "PrinterSelectionDialogData.h"
class CMSWindowsNetworkItem;
class CPrinterSelectionDialog : public CPrinterSelectionDialogData
{
    Q_OBJECT

public:

    CPrinterSelectionDialog
    (
        QWidget* parent = NULL,
        const char* name = NULL
    );

    virtual ~CPrinterSelectionDialog();
public slots:
		void OnDoubleClicked(CListViewItem *pItem);
		void OnReturnPressed();
		void done(int r);
public:
	QString m_Path;
	int m_nCredentialsIndex;
private:
	CMSWindowsNetworkItem *m_pRoot;
	void UpdateConnectAs();
	QString m_DefaultConnectAs;
};
#endif // CPrinterSelectionDialog_included
