/* Name: TrashEntryPropGeneralData.h

   Description: This file is a part of the Corel File Manager application.

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

/**********************************************************************

	--- Qt Architect generated file ---

	File: TrashEntryPropGeneralData.h
	Last generated: Sun Nov 28 22:30:11 1999

	DO NOT EDIT!!!  This file will be automatically
	regenerated by qtarch.  All changes will be lost.

 *********************************************************************/

#ifndef CTrashEntryPropGeneralData_included
#define CTrashEntryPropGeneralData_included

#include <qdialog.h>
#include <qlabel.h>

class CTrashEntryPropGeneralData : public QDialog
{
    Q_OBJECT

public:

    CTrashEntryPropGeneralData
    (
        QWidget* parent = NULL,
        const char* name = NULL
    );

    virtual ~CTrashEntryPropGeneralData();

public slots:


protected slots:


protected:
    QLabel* m_IconLabel;
    QLabel* m_FileName;
    QLabel* m_pOriginalLocationLabel;
    QLabel* m_OriginalLocation;
    QLabel* m_pSizeLabel;
    QLabel* m_Size;
    QLabel* m_pOriginalModifiedLabel;
    QLabel* m_DateDeleted;
    QLabel* m_pOwnerLabel;
    QLabel* m_Owner;
    QLabel* m_pOwnerGroupLabel;
    QLabel* m_OwnerGroup;
    QLabel* m_pPermissionsLabel;
    QLabel* m_Permissions;
    QLabel* m_pOriginalCreatedLabel;
    QLabel* m_pDeletedLabel;
    QLabel* m_OriginalCreated;
    QLabel* m_OriginalModified;

};

#endif // CTrashEntryPropGeneralData_included
