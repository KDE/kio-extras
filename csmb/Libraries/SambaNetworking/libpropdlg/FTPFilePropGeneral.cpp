/* Name: FTPFilePropGeneral.h

   Description: This file is a part of the Corel File Manager application.

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

	File: FilePropGeneral.cpp
	Last generated: Wed Jan 6 08:01:21 1999

 *********************************************************************/

#include "common.h"
#include "FTPFilePropGeneral.h"
#include "ftpfile.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <qdir.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <dirent.h>
#include "propres.h"

#define Inherited CFilePropGeneralData

CFTPFilePropGeneral::CFTPFilePropGeneral
(
	QList<CNetworkTreeItem> &ItemList,
	QWidget* parent,
	const char* name
)
	:
	Inherited( parent, name ),
	m_ItemList(ItemList)
{
	m_FullNameLabel->setText(LoadString(knFULL_NAME_COLON));
	m_SizeLabel->setText(LoadString(knSIZE_COLON));
	m_CreatedLabel->setText(LoadString(knCREATED_COLON));
	m_ModifiedLabel->setText(LoadString(knMODIFIED_COLON));
	m_AccessedLabel->setText(LoadString(knACCESSED_COLON));
	m_OwnerLabel->setText(LoadString(knOWNER_COLON));
	m_OwnerGroupLabel->setText(LoadString(knOWNER_GROUP_COLON));
	m_PermissionsLabel->setText(LoadString(knPERMISSIONS_COLON));

	QPixmap *pixmap;

	CFTPFileItem *pItem = (CFTPFileItem *)ItemList.getFirst();

	if (ItemList.count() == 1)
	{
		pixmap = pItem->Pixmap(TRUE);

		m_FileName->setText(pItem->text(0));

		QString Path(pItem->FullName(FALSE));

		m_FullName->setText(Path);

		size_t FileSize = pItem->GetFileSize();

		QString a;

		if (pItem->IsFolder())
		{
			//long NumFiles=0, NumFolders=0;

			//if (CountFolderContents(Path, NumFiles, NumFolders))
			//	a.sprintf(LoadString(knNUM_FILES_FOLDERS), NumFiles, NumFolders);

			m_SizeLabel->setText(LoadString(knCONTAINS));
		}
		else
			a.sprintf(LoadString(knX_Y_BYTES),
				(LPCSTR)SizeBytesFormat((double)FileSize)
#ifdef QT_20
  .latin1()
#endif
                                                  ,
				(LPCSTR)NumToCommaString(FileSize)
#ifdef QT_20
  .latin1()
#endif
                                          );

		m_Size->setText(a);

		//	a = ctime(&st.st_ctime);
//			a = a.left(a.length()-1);	   // truncate trailing '\n' character
			//m_CreateDate->setText(a);

			m_CreatedLabel->setText(m_ModifiedLabel->text());
			a = ctime(&pItem->m_FileDate);
			a = a.left(a.length()-1);	   // truncate trailing '\n' character
			m_CreateDate->setText(a);

			m_ModifiedLabel->hide();
			m_ModifyDate->hide();
			m_AccessedLabel->hide();
			m_AccessDate->hide();
			//a = ctime(&st.st_atime);
			//a = a.left(a.length()-1);	   // truncate trailing '\n' character
			//m_AccessDate->setText(a);

			m_Owner->setText(pItem->m_Owner);
			m_OwnerGroup->setText(pItem->m_Group);
      m_Permissions->setText(pItem->m_FileAttributes);
	}
	else
	{
		pixmap = LoadPixmap(keManyFilesIconBig);

		QListIterator<CNetworkTreeItem> it(ItemList);

		int nTotalFiles = 0;
		int nTotalFolders = 0;

		int OwnerFlag = -1;
		gid_t OwnerGroupGid = 0;

		BOOL bMultipleOwners = FALSE;

		for (; it.current() != NULL; ++it)
		{
			CFTPFileItem *pItem = (CFTPFileItem*)it.current();
			QString Path(pItem->FullName(FALSE));

			struct stat st;

			if (stat((LPCSTR)Path
#ifdef QT_20
  .latin1()
#endif
                  , &st) >= 0)
			{
				if (OwnerFlag == -1)
				{
					OwnerFlag = st.st_uid;
					OwnerGroupGid = st.st_gid;
				}
				else
				{
					if (st.st_uid != (uid_t)OwnerFlag)
						bMultipleOwners = TRUE;
				}
			}

			if (pItem->IsFolder())
				nTotalFolders++;
			else
				nTotalFiles++;
		}

		QString a;
		a.sprintf(LoadString(knNUM_FILES_FOLDERS), nTotalFiles, nTotalFolders);
		m_FileName->setText(a);

		m_FullNameLabel->setText(LoadString(knLOCATION_COLON));

		a.sprintf(LoadString(knALL_IN_X), (LPCSTR)((CNetworkTreeItem*)pItem->parent())->FullName(FALSE)
#ifdef QT_20
  .latin1()
#endif
    );
		m_FullName->setText(a);

		m_CreatedLabel->hide();
		m_CreateDate->hide();
		m_AccessedLabel->hide();
		m_AccessDate->hide();
		m_ModifiedLabel->hide();
		m_ModifyDate->hide();
		m_PermissionsLabel->hide();
		m_Permissions->hide();
		m_SizeLabel->hide();
		m_Size->hide();

		if (bMultipleOwners)
		{
			m_Owner->setText(LoadString(knMULTIPLE_OWNERS));
			m_OwnerGroupLabel->hide();
			m_OwnerGroup->hide();
		}
		else
		{
			struct passwd *pwd = getpwuid(OwnerFlag);

			m_Owner->setText(pwd == NULL ? "" : pwd->pw_name);

			struct group *grp = getgrgid(OwnerGroupGid);

			m_OwnerGroup->setText(grp == NULL ? "" : grp->gr_name);
		}
	}

	m_IconLabel->setPixmap(*pixmap);
}


CFTPFilePropGeneral::~CFTPFilePropGeneral()
{
}

