/* Name: PermissionsDlgData.h

   Description: This file is a part of the libmwn library.

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

#ifndef CPermissionsDlgData_included
#define CPermissionsDlgData_included

#include <qdialog.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qcombobox.h>
#include <qlistbox.h>

class CPermissionsDlgData : public QDialog
{
    Q_OBJECT

public:

    CPermissionsDlgData
    (
        QWidget* parent = NULL,
        const char* name = NULL
    );

    virtual ~CPermissionsDlgData();

public slots:


protected slots:

    virtual void OnAdd();
    virtual void OnListSelChanged(int);
    virtual void OnAccessTypeComboChanged(int);
    virtual void OnRemove();

protected:
    QLabel* m_pAccessThroughShareLabel;
    QLabel* m_ShareName;
    QLabel* m_pNameLabel;
    QListBox* m_List;
    QLabel* m_pAccessTypeLabel;
    QComboBox* m_AccessType;
    QPushButton* m_AddButton;
    QPushButton* m_RemoveButton;
    QPushButton* m_OKButton;
    QPushButton* m_CancelButton;

};

#endif // CPermissionsDlgData_included
