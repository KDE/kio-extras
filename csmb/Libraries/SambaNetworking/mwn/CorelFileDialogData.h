/* Name: CorelFileDialogData.h

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

#ifndef CCorelFileDialogData_included
#define CCorelFileDialogData_included

#include <qdialog.h>
#include <qlabel.h>
#include "pixmapcombo.h"
#include <qpushbutton.h>
#include <qcombobox.h>
#include <qlineedit.h>

#include "listview.h"

class CCorelFileDialogData : public QDialog
{
    Q_OBJECT

public:

    CCorelFileDialogData
    (
        QWidget* parent = NULL,
        const char* name = NULL
    );

    virtual ~CCorelFileDialogData();

public slots:


protected slots:

    virtual void OnAcceptClicked();

protected:
    QLabel* m_pLookInLabel;
    CPixmapCombo* m_pLookInCombo;
    QPushButton* m_pTopButton1;
    QPushButton* m_pTopButton2;
    QPushButton* m_pTopButton3;
    QPushButton* m_pTopButton4;
    CListView* m_pListView;
    QLabel* m_pFileNameLabel;
    QLineEdit* m_pFileNameEdit;
    QLabel* m_pFileTypeLabel;
    QComboBox* m_pFileTypeCombo;
    QPushButton* m_pAcceptButton;
    QPushButton* m_pCancelButton;

};

#endif // CCorelFileDialogData_included
