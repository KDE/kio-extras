/* Name: TrashEntryPropGeneral.cpp

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

	File: TrashEntryPropGeneral.cpp
	Last generated: Wed Oct 20 12:12:40 1999

 *********************************************************************/

#include "common.h"
#include "TrashEntryPropGeneral.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include "trashentry.h"
#include "propres.h"

#define Inherited CTrashEntryPropGeneralData

BOOL CountFolderContents(LPCSTR Path, long& NumFiles, long& NumFolders);

CTrashEntryPropGeneral::CTrashEntryPropGeneral
(
	CTrashEntryItem *pItem,
  QWidget* parent,
	const char* name
)
	:
	Inherited( parent, name )
{
  m_pOriginalLocationLabel->setText(LoadString(knORIGINAL_LOCATION_COLON));
	m_pSizeLabel->setText(LoadString(knSIZE_COLON));
	m_pDeletedLabel->setText(LoadString(knDELETED_COLON));
	m_pOriginalCreatedLabel->setText(LoadString(knORIGINAL_CREATED_COLON));
	m_pOriginalModifiedLabel->setText(LoadString(knORIGINAL_MODIFIED_COLON));
	m_pOwnerLabel->setText(LoadString(knOWNER_COLON));
	m_pOwnerGroupLabel->setText(LoadString(knOWNER_GROUP_COLON));
	m_pPermissionsLabel->setText(LoadString(knPERMISSIONS_COLON));

	m_OriginalLocation->setText(pItem->m_OriginalLocation);
  QString a;

  if (pItem->IsFolder())
  {
    long NumFiles=0, NumFolders=0;

    if (CountFolderContents((LPCSTR)pItem->m_FileName
#ifdef QT_20
  .latin1()
#endif
    , NumFiles, NumFolders))
      a.sprintf(LoadString(knNUM_FILES_FOLDERS), NumFiles, NumFolders);

    m_pSizeLabel->setText(LoadString(knCONTAINS));
  }
  else
    a.sprintf(LoadString(knX_Y_BYTES),
      (LPCSTR)SizeBytesFormat((double)pItem->m_FileSize)
#ifdef QT_20
  .latin1()
#endif
      ,
      (LPCSTR)NumToCommaString(pItem->m_FileSize)
#ifdef QT_20
  .latin1()
#endif

      );

  m_Size->setText(a);

  a = ctime(&pItem->m_DateDeleted);
  a = a.left(a.length()-1);	   // truncate trailing '\n' character
  m_DateDeleted->setText(a);

  a = ctime(&pItem->m_OriginalCreationDate);
  a = a.left(a.length()-1);	   // truncate trailing '\n' character
  m_OriginalCreated->setText(a);

  a = ctime(&pItem->m_OriginalModifyDate);
  a = a.left(a.length()-1);	   // truncate trailing '\n' character
  m_OriginalModified->setText(a);

	m_FileName->setText(pItem->text(0));

  m_IconLabel->setPixmap(*pItem->Pixmap(TRUE));

  struct stat st;
  lstat((LPCSTR)pItem->m_FileName
#ifdef QT_20
  .latin1()
#endif
  , &st);

  struct passwd *pwd = getpwuid(st.st_uid);

  if (NULL != pwd)
    m_Owner->setText(pwd->pw_name);
  else
    m_Owner->setText("");

  struct group *grp = getgrgid(st.st_gid);

  if (NULL != grp)
    m_OwnerGroup->setText(grp->gr_name);
  else
    m_OwnerGroup->setText("");

  FileModeToString(a, st.st_mode);
  m_Permissions->setText(a);
}


CTrashEntryPropGeneral::~CTrashEntryPropGeneral()
{
}
