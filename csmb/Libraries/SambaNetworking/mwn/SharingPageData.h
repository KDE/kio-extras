/* Name: SharingPageData.h

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



#ifndef CSharingPageData_included
#define CSharingPageData_included

#include <qdialog.h>
#include <qlabel.h>
#include <qradiobutton.h>
#include <qpushbutton.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qspinbox.h>
#include <qlineedit.h>
#include <qframe.h>

class CSharingPageData : public QDialog
{
    Q_OBJECT

public:

    CSharingPageData
    (
        QWidget* parent = NULL,
        const char* name = NULL
    );

    virtual ~CSharingPageData();

public slots:


protected slots:

    virtual void OnNewShare();
    virtual void OnRemoveShare();
    virtual void OnAllow();
    virtual void OnComboChange(int);
    virtual void OnMaximumAllowed();
    virtual void OnSharedAs();
    virtual void OnPermissions();

protected:
    QLabel* m_IconLabel;
    QLabel* m_PathLabel;
    QCheckBox* m_SharedAs;
    QLabel* m_ShareNameLabel;
    QComboBox* m_ShareName;
    QLineEdit* m_ShareNameEdit;
    QLabel* m_CommentLabel;
    QLineEdit* m_Comment;
    QPushButton* m_NewShareButton;
    QPushButton* m_RemoveShareButton;
    QLabel* m_UserLimitLabel;
    QFrame* m_pSeparator3;
    QRadioButton* m_MaximumAllowed;
    QRadioButton* m_Allow;
    QSpinBox* m_UsersMax;
    QLabel* m_UsersLabel;
    QPushButton* m_PermissionsButton;
    QCheckBox* m_Enabled;

};

#endif // CSharingPageData_included
