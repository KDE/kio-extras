/* Name: FileReplaceDialogData.h

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

#ifndef CFileReplaceDialogData_included
#define CFileReplaceDialogData_included

#include <qdialog.h>
#include <qlabel.h>
#include <qpushbutton.h>

class CFileReplaceDialogData : public QDialog
{
    Q_OBJECT

public:

    CFileReplaceDialogData
    (
        QWidget* parent = NULL,
        const char* name = NULL
    );

    virtual ~CFileReplaceDialogData();

public slots:


protected slots:

    virtual void OnYes();
    virtual void OnButton3();
    virtual void OnButton2();

protected:
    QLabel* m_FileReplaceIconLabel;
    QLabel* m_TopText;
    QLabel* m_pLabel2;
    QLabel* m_FileIcon1;
    QLabel* m_File1Text1;
    QLabel* m_File1Text2;
    QLabel* m_FileIcon2;
    QLabel* m_File2Text1;
    QLabel* m_File2Text2;
    QPushButton* m_pYesButton;
    QPushButton* m_Button2;
    QPushButton* m_Button3;
    QPushButton* m_Cancel;
    QLabel* m_pLabel3;

};

#endif // CFileReplaceDialogData_included
