/* Name: PermissionsAddDlgData.h

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

#ifndef CPermissionsAddDlgData_included
#define CPermissionsAddDlgData_included

#include <qdialog.h>
#include <qlabel.h>
#include <qradiobutton.h>
#include <qpushbutton.h>
#include <qcombobox.h>
#include <qbuttongroup.h>

class CPermissionsAddDlgData : public QDialog
{
    Q_OBJECT

public:

    CPermissionsAddDlgData
    (
        QWidget* parent = NULL,
        const char* name = NULL
    );

    virtual ~CPermissionsAddDlgData();

public slots:


protected slots:

    virtual void OnUser();
    virtual void OnAccessTypeComboChanged(int);
    virtual void OnGroup();
    virtual void OnEveryone();

protected:
    QButtonGroup* m_pButtonGroup;
    QRadioButton* m_User;
    QComboBox* m_UserCombo;
    QRadioButton* m_Group;
    QComboBox* m_GroupCombo;
    QRadioButton* m_Everyone;
    QLabel* m_pAccessTypeLabel;
    QComboBox* m_AccessType;
    QPushButton* m_pOKButton;
    QPushButton* m_pCancelButton;

};

#endif // CPermissionsAddDlgData_included
