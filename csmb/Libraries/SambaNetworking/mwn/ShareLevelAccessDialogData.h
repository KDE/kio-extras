/* Name: ShareLevelAccessDialogData.h

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


#ifndef CShareLevelAccessDialogData_included
#define CShareLevelAccessDialogData_included

#include <qdialog.h>
#include <qlabel.h>
#include <qradiobutton.h>
#include <qpushbutton.h>
#include <qcheckbox.h>
#include <qlineedit.h>
#include <qframe.h>

class CShareLevelAccessDialogData : public QDialog
{
    Q_OBJECT

public:

    CShareLevelAccessDialogData
    (
        QWidget* parent = NULL,
        const char* name = NULL
    );

    virtual ~CShareLevelAccessDialogData();

public slots:


protected slots:

    virtual void OnAnonymousAccess();
    virtual void OnPasswordAccess();

protected:
    QLabel* m_pTopLabel;
    QRadioButton* m_pAllowAnonymousAccessRadio;
    QRadioButton* m_pAllowAccessUsingPasswordRadio;
    QLineEdit* m_pPasswordEdit;
    QLabel* m_pConfirmPasswordLabel;
    QLineEdit* m_pConfirmPasswordEdit;
    QFrame* m_pSeparator1;
    QCheckBox* m_pReadOnlyAccessCheckbox;
    QPushButton* m_pOKButton;
    QPushButton* m_pCancelButton;

};

#endif // CShareLevelAccessDialogData_included
