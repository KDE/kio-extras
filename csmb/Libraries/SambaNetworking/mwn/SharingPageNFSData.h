/* Name: SharingPageNFSData.h

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


#ifndef CSharingPageNFSData_included
#define CSharingPageNFSData_included

#include <qdialog.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qcheckbox.h>

#include "listview.h"
#include <qframe.h>

class CSharingPageNFSData : public QDialog
{
    Q_OBJECT

public:

    CSharingPageNFSData
    (
        QWidget* parent = NULL,
        const char* name = NULL
    );

    virtual ~CSharingPageNFSData();

public slots:


protected slots:

    virtual void OnIsShared();
    virtual void OnAdd();
    virtual void OnEdit();
    virtual void OnRemove();

protected:
    QLabel* m_IconLabel;
    QLabel* m_PathLabel;
    QFrame* m_pSeparator1;
    QCheckBox* m_pIsSharedCheckbox;
    CListView* m_pListView;
    QPushButton* m_pAddButton;
    QPushButton* m_pEditButton;
    QPushButton* m_pDeleteButton;

};

#endif // CSharingPageNFSData_included
