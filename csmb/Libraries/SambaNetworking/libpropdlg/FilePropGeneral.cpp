/* Name: FilePropGeneral.cpp

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

#include "common.h"
#include "FilePropGeneral.h"
#include "localfile.h"
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
#include <errno.h>
#include "smbutil.h" // for FillFileInfo(...)
#include "treeitem.h" // for DefaultFilePixmap(...)

#define Inherited CFilePropGeneralData

////////////////////////////////////////////////////////////////////////////

BOOL CountFolderContents(LPCSTR Path, long& NumFiles, long& NumFolders)
{
	char *buf = new char[strlen(Path)+2];
	strcpy(buf, Path);

	if (buf[strlen(buf)-1] != '/')
    strcat(buf, "/");

	DIR *thisDir = opendir(Path);

	if (thisDir == NULL)
	    return FALSE;

	struct dirent *p;

	while ((p = readdir(thisDir)) != NULL)
	{
    if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
			continue;

		struct stat st;
		char *filename = new char[strlen(buf)+strlen(p->d_name)+1];

		strcpy(filename, buf);
		strcat(filename, p->d_name);

		if (stat(filename, &st) >= 0 && S_ISDIR(st.st_mode))
		{
			NumFolders++;
			//CountFolderContents(filename, NumFiles, NumFolders);
		}
		else
			NumFiles++;

		delete []filename;
	}

	closedir(thisDir);

	delete []buf;
	return TRUE;
}

////////////////////////////////////////////////////////////////////////////

CFilePropGeneral::CFilePropGeneral
(
	QList<CNetworkTreeItem> &ItemList,
	QWidget* parent,
	const char* name
)
	:
	Inherited( parent, name )
{
	QPixmap *pixmap;

	m_FullNameLabel->setText(LoadString(knFULL_NAME_COLON));
	m_SizeLabel->setText(LoadString(knSIZE_COLON));
	m_CreatedLabel->setText(LoadString(knCREATED_COLON));
	m_ModifiedLabel->setText(LoadString(knMODIFIED_COLON));
	m_AccessedLabel->setText(LoadString(knACCESSED_COLON));
	m_OwnerLabel->setText(LoadString(knOWNER_COLON));
	m_OwnerGroupLabel->setText(LoadString(knOWNER_GROUP_COLON));
	m_PermissionsLabel->setText(LoadString(knPERMISSIONS_COLON));

	CLocalFileItem *pItem = (CLocalFileItem *)ItemList.getFirst();

	if (ItemList.count() == 1)
	{
		pixmap = pItem->Pixmap(TRUE);

		m_FileName->setText(pItem->GetTip(1));

		QString Path(pItem->FullName(FALSE));

		m_FullName->setText(Path);

		struct stat st;

		if (lstat((LPCSTR)Path
#ifdef QT_20
  .latin1()
#endif
    , &st) >= 0)
		{
			QString a;

			if (S_ISDIR(st.st_mode))
			{
				long NumFiles=0, NumFolders=0;

				if (CountFolderContents((LPCSTR)Path
#ifdef QT_20
  .latin1()
#endif
        , NumFiles, NumFolders))
					a.sprintf(LoadString(knNUM_FILES_FOLDERS), NumFiles, NumFolders);

				m_SizeLabel->setText(LoadString(knCONTAINS));
			}
			else
				a.sprintf(LoadString(knX_Y_BYTES),
					(LPCSTR)SizeBytesFormat((double)st.st_size)
#ifdef QT_20
  .latin1()
#endif
          ,
					(LPCSTR)NumToCommaString(st.st_size)
#ifdef QT_20
  .latin1()
#endif
          );

			m_Size->setText(a);

			a = ctime(&st.st_ctime);
			a = a.left(a.length()-1);	   // truncate trailing '\n' character
			m_CreateDate->setText(a);

			a = ctime(&st.st_mtime);
			a = a.left(a.length()-1);	   // truncate trailing '\n' character
			m_ModifyDate->setText(a);

			a = ctime(&st.st_atime);
			a = a.left(a.length()-1);	   // truncate trailing '\n' character
			m_AccessDate->setText(a);

			struct passwd *pwd = getpwuid(st.st_uid);

      if (NULL != pwd)
        m_Owner->setText(pwd->pw_name);
      else
      {
        QString s;
        s.sprintf("%u", st.st_uid);
        m_Owner->setText((LPCSTR)s
#ifdef QT_20
  .latin1()
#endif
                                    );
      }

      struct group *grp = getgrgid(st.st_gid);

      if (NULL != grp)
        m_OwnerGroup->setText(grp->gr_name);
      else
      {
        QString s;
        s.sprintf("%u", st.st_gid);
        m_OwnerGroup->setText((LPCSTR)s
#ifdef QT_20
  .latin1()
#endif
        );
      }

			FileModeToString(a, st.st_mode);
			m_Permissions->setText(a);
		}
    else
      if (errno == ETXTBSY)
      {
        m_SizeLabel->hide();
        m_Size->hide();
        m_OwnerGroupLabel->hide();
        m_OwnerGroup->hide();
        m_OwnerLabel->hide();
        m_Owner->hide();
        m_CreatedLabel->hide();
        m_CreateDate->hide();
        m_ModifiedLabel->hide();
        m_ModifyDate->hide();
        m_AccessedLabel->hide();
        m_AccessDate->hide();
        m_PermissionsLabel->hide();
        m_Permissions->hide();
      }
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
			CLocalFileItem *pItem = (CLocalFileItem*)it.current();
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

		a.sprintf(LoadString(knALL_IN_X), (LPCSTR)((CNetworkTreeItem*)pItem->m_pLogicalParent)->FullName(FALSE)
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

			if (NULL == pwd)
      {
        QString s;
        s.sprintf("%u", OwnerFlag);
        m_Owner->setText((LPCSTR)s
#ifdef QT_20
  .latin1()
#endif
        );
      }
      else
        m_Owner->setText(pwd->pw_name);

			struct group *grp = getgrgid(OwnerGroupGid);

			if (NULL == grp)
      {
        QString s;
        s.sprintf("%u", OwnerGroupGid);
        m_OwnerGroup->setText((LPCSTR)s
#ifdef QT_20
  .latin1()
#endif
        );
      }
      else
        m_OwnerGroup->setText(grp->gr_name);
		}
	}

	m_IconLabel->setPixmap(*pixmap);
}

////////////////////////////////////////////////////////////////////////////

CFilePropGeneral::CFilePropGeneral
(
  LPCSTR URL,
  struct stat &st,
	QWidget* parent,
	const char* name
)
	:
	Inherited( parent, name )
{
	QPixmap *pixmap;

	m_FullNameLabel->setText(LoadString(knFULL_NAME_COLON));
	m_SizeLabel->setText(LoadString(knSIZE_COLON));
	m_CreatedLabel->setText(LoadString(knCREATED_COLON));
	m_ModifiedLabel->setText(LoadString(knMODIFIED_COLON));
	m_AccessedLabel->setText(LoadString(knACCESSED_COLON));
	m_OwnerLabel->setText(LoadString(knOWNER_COLON));
	m_OwnerGroupLabel->setText(LoadString(knOWNER_GROUP_COLON));
	m_PermissionsLabel->setText(LoadString(knPERMISSIONS_COLON));

  struct stat sttarget;

  if (S_ISLNK(st.st_mode))
  {
    if (stat(URL, &sttarget))
      sttarget.st_mode = 0xffffffff;
  }

  QString ParentURL = GetParentURL(URL);
  QString ShortName = URL + ParentURL.length() + 1;

  CSMBFileInfo fi;
  FillFileInfo(&fi, URL, (LPCSTR)ShortName
#ifdef QT_20
  .latin1()
#endif
  , &st, &sttarget);

  m_FileName->setText(GetLocalFileTip(1, fi.IsLink(), fi.IsFolder(), fi.m_FileName, URL, fi.m_TargetName));
	m_FullName->setText(URL);

	QString a;

	if (fi.IsFolder())
  {
    long NumFiles=0, NumFolders=0;

    if (CountFolderContents(URL, NumFiles, NumFolders))
      a.sprintf(LoadString(knNUM_FILES_FOLDERS), NumFiles, NumFolders);

    m_SizeLabel->setText(LoadString(knCONTAINS));
  }
  else
    a.sprintf(LoadString(knX_Y_BYTES),
      (LPCSTR)SizeBytesFormat((double)st.st_size)
#ifdef QT_20
  .latin1()
#endif
      ,
      (LPCSTR)NumToCommaString(st.st_size)
#ifdef QT_20
  .latin1()
#endif
                                          );

  m_Size->setText(a);

  a = ctime(&st.st_ctime);
  a = a.left(a.length()-1);	   // truncate trailing '\n' character
  m_CreateDate->setText(a);

  a = ctime(&st.st_mtime);
  a = a.left(a.length()-1);	   // truncate trailing '\n' character
  m_ModifyDate->setText(a);

  a = ctime(&st.st_atime);
  a = a.left(a.length()-1);	   // truncate trailing '\n' character
  m_AccessDate->setText(a);

  struct passwd *pwd = getpwuid(st.st_uid);

  if (NULL != pwd)
    m_Owner->setText(pwd->pw_name);
  else
  {
    QString s;
    s.sprintf("%u", st.st_uid);
    m_Owner->setText((LPCSTR)s
#ifdef QT_20
  .latin1()
#endif
                                );
  }

  struct group *grp = getgrgid(st.st_gid);

  if (NULL != grp)
    m_OwnerGroup->setText(grp->gr_name);
  else
  {
    QString s;
    s.sprintf("%u", st.st_gid);
    m_OwnerGroup->setText((LPCSTR)s
#ifdef QT_20
  .latin1()
#endif
                                    );
  }

  FileModeToString(a, st.st_mode);
  m_Permissions->setText(a);

	m_pPixmap = DefaultFilePixmap(URL, (LPCSTR)ShortName
#ifdef QT_20
  .latin1()
#endif
  , TRUE, (LPCSTR)fi.m_FileAttributes
#ifdef QT_20
  .latin1()
#endif
  , fi.m_TargetMode, (LPCSTR)fi.m_TargetName
#ifdef QT_20
  .latin1()
#endif
  , fi.IsLink(), fi.IsFolder());
  m_IconLabel->setPixmap(*m_pPixmap);
}

////////////////////////////////////////////////////////////////////////////

CFilePropGeneral::~CFilePropGeneral()
{
}

////////////////////////////////////////////////////////////////////////////

