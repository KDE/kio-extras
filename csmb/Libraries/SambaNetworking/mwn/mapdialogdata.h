/* Name: mapdialogdata.h

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

#ifndef CMapDialogData_included
#define CMapDialogData_included

#include <qdialog.h>
#include <qlabel.h>
#include "listview.h"
#include <qcheckbox.h>
#include <qpushbutton.h>
#include <qlineedit.h>

class CMapDialogData : public QDialog
{
    Q_OBJECT

public:

    CMapDialogData
    (
        QWidget* parent = NULL,
        const char* name = NULL
    );

    virtual ~CMapDialogData();

public slots:


protected slots:

    virtual void OnBrowse();

protected:
    QLabel* m_pShareToMountLabel;
    QLineEdit* m_pUNCPath;
    QLabel* m_pMountPointLabel;
    QLineEdit* m_pMountPoint;
    QPushButton* m_pBrowseButton;
    QLabel* m_pConnectAsLabel;
    QLineEdit* m_pConnectAs;
    QCheckBox* m_pReconnectAtLogon;
    CListView* m_Tree;
    QPushButton* m_OKButton;
    QPushButton* m_pCancelButton;
    QLabel* m_pSharedDirectoriesLabel;

};

#endif // CMapDialogData_included
