/* Name: localfile.cpp

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

/*
 * NOTE: This source file was created using tab size = 2.
 * Please respect that setting in case of modifications.
 */

////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "localfile.h"
#include "smbutil.h"
#include "inifile.h"
#include <qdir.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <qmessagebox.h>
//#include "explres.h"
#include "qapplication.h"
#include <time.h> // for mktime and strftime
#include <sys/time.h>
#include <unistd.h>
//#include "expcommon.h" // Explorer-specific common routines
#include "kapp.h"
#include "errno.h"
#include "trashentry.h"

////////////////////////////////////////////////////////////////////////////

void CLocalFileItem::Init()
{
	InitPixmap();

	if (m_FileAttributes[0] == 'b' || m_FileAttributes[0] == 'c')
		setText(1, m_FileSize + "  "); // major, minor are here
	else
		if (m_FileAttributes[0] == '-')
			setText(1, SizeInKilobytes((unsigned long)atol(m_FileSize)) + "  ");

  setText(3, FormatDateTime(m_FileDate));
}

////////////////////////////////////////////////////////////////////////////

QTTEXTTYPE CLocalFileItem::text(int column) const
{
	switch (column)
	{
		case -1:
			return "MyComputer";

		case 0: // Name
		default:
      return (QTTEXTTYPE)m_FileName;
		case 1: // Size
			return CListViewItem::text(1);
		case 2: // Type
			return (QTTEXTTYPE)m_FileAttributes;
		case 3: // Modified
			return CListViewItem::text(3);
	}
}

////////////////////////////////////////////////////////////////////////////

QPixmap *CLocalFileItem::Pixmap(BOOL bIsBig)
{
  return DefaultFilePixmap(bIsBig, m_FileAttributes, m_TargetMode, m_TargetName, IsLink(), IsFolder());
}

////////////////////////////////////////////////////////////////////////////

void CLocalFileContainer::Fill()
{
	CWaitLogo WaitLogo;
	SetExpansionStatus(keExpanding);

	time(&m_LastRefreshTime);

  QString Name(FullName(FALSE));

  if (IsTrashFolder(Name))
    SetExpansionStatus(keExpansionComplete);
  else
  {
    CFileArray list;

		//struct timeval tv1, tv2;
		//struct timezone tz;

		//gettimeofday(&tv1, &tz);
  	BOOL bRet = GetLocalFileList(Name, &list);
		//gettimeofday(&tv2, &tz);
		//double TimeSpent = (double)tv2.tv_sec * 1000000. + tv2.tv_usec - (double)tv1.tv_sec * 1000000. - tv1.tv_usec;
    //printf("Spent : %f\n", TimeSpent / 1000.);

    switch (bRet)
  	{
  		case TRUE:
  		{
  			int i;

        //struct timeval tv1, tv2;
        //struct timezone tz;

        //gettimeofday(&tv1, &tz);

        for (i=0; i < list.count(); i++)
        {
          new CLocalFileItem(this, this, &list[i]);
        }

        //gettimeofday(&tv2, &tz);
        //double TimeSpent = (double)tv2.tv_sec * 1000000. + tv2.tv_usec - (double)tv1.tv_sec * 1000000. - tv1.tv_usec;
        //printf("Spent phase 2: %f\n", TimeSpent / 1000.);
  			SetExpansionStatus(keExpansionComplete);
  		}
  		break;

  		default:
  			//printf("Unable to GetLocalFileList!\n");
  			SetExpansionStatus(keNotExpanded);
  	}
  }

	gTreeExpansionNotifier.Fire(this);
}

////////////////////////////////////////////////////////////////////////////

void CLocalFileContainer::Fill(CFileArray& list)
{
  SetExpansionStatus(keExpanding);

	int i;

	for (i=0; i < list.count(); i++)
	{
		if (list[i].IsFolder())
			new CLocalFileItem(this, this, &list[i]);
	}

	setExpandable(childCount() > 0);
	SetExpansionStatus(keExpansionComplete);

	listView()->repaintItem(this);
}

////////////////////////////////////////////////////////////////////////////

QString CLocalFileContainer::FullName(BOOL)
{
	QString s;

	if (NULL != m_pLogicalParent && IS_NETWORKTREEITEM(m_pLogicalParent))
	{
		CNetworkTreeItem *pParent = (CNetworkTreeItem*)m_pLogicalParent;

		QString MyName = text(0);
    
		if (MyName[0] == '/' || MyName[0] == '\\')
			s = MyName;
		else
		{
			s = pParent->FullName(FALSE);

			if (s.right(1) != "/")
				s.append("/");

			s.append(MyName);
		}
	}
	else
		s = text(0);

	return s;
}

////////////////////////////////////////////////////////////////////////////

void CLocalFileItem::setup()
{
  //setExpandable(IsFolder());
	CNetworkTreeItem::setup();
}

////////////////////////////////////////////////////////////////////////////

QTKEYTYPE CLocalFileItem::key(int nColumn, bool /*ascending*/) const
{
	static QSTRING_WITH_SIZE(s, 1024);

	if (nColumn == 1) /* Size */
	{
		if (IsFolder())
	    s = QString("\1") + text(0);
		else
	    s.sprintf("%.10ld_%s", atol((LPCSTR)m_FileSize), (LPCSTR)text(0));
	}
	else
	{
		if (nColumn == 3)
			s.sprintf("%.10ld_%s", m_FileDate, (LPCSTR)text(0));
		else
		{
			s = IsFolder() ? "\1" : "";
			s += text(nColumn);
		}
	}

	//printf("key(%d) of %s is %s\n", nColumn, text(0), (LPCSTR)s);

	return (QTKEYTYPE)s;
}

////////////////////////////////////////////////////////////////////////////

void CheckChangedFromItem(CListViewItem *pItem)
{
	CLocalFileContainer *pI;

	for (pI = (CLocalFileContainer*)pItem->firstChild(); NULL != pI; pI = (CLocalFileContainer*)pI->nextSibling())
	{
		if (pI->isOpen())
		{
			if (!IsExcludedFromRescan(pI->FullName(FALSE)))
      {
        if (pI->ContentsChanged(0, -1))
  			{
  				//printf("Contents of %s changed!\n", (LPCSTR)pI->text(0));
  				RescanItem(pI);
  				return;
  			}
  			else
  				CheckChangedFromItem(pI);
      }
		}
	}
}

////////////////////////////////////////////////////////////////////////////

BOOL CLocalFileContainer::ContentsChanged(time_t SinceWhen, int nOldChildCount, CListViewItem *pOldFirst)
{
	SinceWhen = m_LastRefreshTime;

	BOOL retval = FALSE;

	struct stat st;
	QString Path = FullName(FALSE);

	// Don't report that contents changed if the file has modification time in the
	// future, otherwize we will end up refrshing constantly...
	time_t timenow = time(NULL);

	m_LastRefreshTime = timenow;

	//printf("%s: From %ld to %ld\n", (LPCSTR)Path, SinceWhen, timenow);

	// if errno returned from lstat == EIO that could mean that
	// network share mounted here lost a connection.
	// We don't consider this as 'contents changed' situation.

  //printf("Checking time of %s\n", (LPCSTR)Path);

	if ((lstat(Path, &st) && errno != EIO) ||
			(st.st_ctime > SinceWhen && st.st_ctime < timenow) ||
			(st.st_mtime > SinceWhen && st.st_mtime < timenow))
	{
		//printf("st.st_mtime = %ld, Since = %ld\n", st.st_mtime, SinceWhen);

    retval = TRUE;
	}
	else
	{
		if (S_ISDIR(st.st_mode))
		{
      if (IsTrashFolder(Path))
      {
        return CheckTrashFolder(Path, SinceWhen, nOldChildCount);
      }
      else
      {
        char *buf = new char[strlen(Path)+2];
  			strcpy(buf, Path);

  			if (buf[strlen(buf)-1] != '/')
  				strcat(buf, "/");

  			DIR *thisDir = opendir(Path);

  			if (thisDir == NULL)
        {
          //printf("thisDir = NULL!\n");
          delete []buf;
  				return TRUE;
        }

  			struct dirent *p;

  			int nItemsFound=0;
  			int nDirsFound=0;

  			while ((p = readdir(thisDir)) != NULL)
  			{
  				if (!strcmp(p->d_name, ".") ||
  					!strcmp(p->d_name, "..") ||
  					(!gbShowHiddenFiles && p->d_name[0] == '.'))
  					continue;

  				struct stat st;
  				char *filename = new char[strlen(buf)+strlen(p->d_name)+1];

  				strcpy(filename, buf);
  				strcat(filename, p->d_name);

          //printf("lstat for %s\n", filename);

          BOOL bStatOK = !lstat(filename, &st);

					if (!bStatOK)
					{
						if (ECONNREFUSED == errno)
						{
							delete []filename;
							continue;
						}
					}

					if (bStatOK || (errno != EIO &&
													errno != ETXTBSY &&
													errno != EACCES &&
													errno != ENOENT))
          {
            if (!bStatOK ||
  						  (st.st_ctime > SinceWhen && st.st_ctime < timenow) ||
  						  (st.st_mtime > SinceWhen && st.st_mtime < timenow))
  				  {
  					  //printf("Time check worked for %s: %ld %ld %ld\n", filename, st.st_ctime, SinceWhen, timenow);
  					  retval = TRUE;
  					  delete []filename;
  					  break;
  				  }
          }

  				nItemsFound++;

  				if (bStatOK && S_ISDIR(st.st_mode))
  				{
  					if (ExpansionStatus() == keExpansionComplete)
  					{
  						CListViewItem *pI;

  						for (pI=firstChild(); NULL != pI; pI=pI->nextSibling())
  						{
  							if (!strcmp(pI->text(0), p->d_name))
  								break;
  						}

  						if (NULL == pI)
  						{
								retval = TRUE;
  							delete []filename;
  							break;
  						}
  					}

  					nDirsFound++;
  				}
  				else
  				{
  					if (bStatOK)
            {
              if (NULL != pOldFirst)
    					{
    						CListViewItem *pI;

    						for (pI=pOldFirst; NULL != pI; pI=pI->nextSibling())
    						{
    							if (!strcmp(pI->text(0), p->d_name))
    							{
    								if (((CNetworkTreeItem*)pI)->Kind() == keLocalFileItem)
    								{
    									CLocalFileItem *lf = (CLocalFileItem*)pI;
    									BOOL bChanged = FALSE;

    									if (S_ISREG(st.st_mode) && atol(lf->m_FileSize) != st.st_size)
    									{
    										lf->m_FileSize.sprintf("%lu", st.st_size);
    										bChanged = TRUE;
    									}

    									QString s;
    									FileModeToString(s, st.st_mode);

    									if (lf->m_FileAttributes != s)
    									{
    										lf->m_FileAttributes = s;
    										bChanged = TRUE;
    									}

    									if (lf->m_FileDate != st.st_mtime)
    									{
    										lf->m_FileDate = st.st_mtime;
    										bChanged = TRUE;
    									}

    									if (bChanged)
    									{
    										lf->Init();
    										lf->listView()->repaintItem(lf);
    									}
    								}

    								break;
    							}
    						}

    						if (NULL == pI)
    						{
                  struct stat st;

                  if (!stat(filename, &st))
                  {
										retval = TRUE;
                    delete []filename;
    							  break;
                  }
                  else
                    nItemsFound--;
    						}
    					}

              if (S_ISLNK(st.st_mode))
    					{
    						if (!stat(filename, &st) && S_ISDIR(st.st_mode))
    						{
    							nDirsFound++;
    						}
    					}
            }
  				}

  				delete []filename;
  			}

  			if (!retval &&
  					(ExpansionStatus() == keExpansionComplete) &&
  					(nOldChildCount == -1 ?
  						(nDirsFound != childCount() - m_nNumExtraItems) :
  						(nItemsFound != nOldChildCount)))
  			{
  				//printf("%s: Item count mismatch %d vs. %d!\n",
  					//		 (LPCSTR)Path,
  						//	 nOldChildCount == -1 ? nDirsFound: nItemsFound,
  							// nOldChildCount == -1 ? childCount() : nOldChildCount);

          retval = TRUE;
  			}

  			closedir(thisDir);

  			delete []buf;
      }
		}
	}

	//if (retval)
  //  printf("%s: contents changed!\n", (LPCSTR)Path);
  return retval;
}

////////////////////////////////////////////////////////////////////////////

BOOL CLocalFileItem::CanEditLabel()
{
	QString Path = FullName(FALSE);
	QString Parent, FileName;

	SplitPath(Path, Parent, FileName);

	return !access(Parent, W_OK);
}

////////////////////////////////////////////////////////////////////////////

CSMBErrorCode CLocalFileItem::Rename(LPCSTR sNewLabel)
{
	QString Path = FullName(FALSE);
	QString Parent, OldName;

	SplitPath(Path, Parent, OldName);

	if (OldName == sNewLabel)
		return keSuccess;

	if (Parent.right(1) != "/")
		Parent += "/";

	Parent += sNewLabel;

	struct stat st;

	if (!stat(Parent, &st))
	{
		QSTRING_WITH_SIZE(a, 1024 + Path.length() + Parent.length());

    a.sprintf(LoadString(knCANNOT_RENAME_X_Y), (LPCSTR)Path, (LPCSTR)Parent);

		QMessageBox::critical(qApp->mainWidget(), LoadString(knERROR_RENAMING_FILE), (LPCSTR)a);
		return keStoppedByUser;
	}

	if (rename(Path, Parent))
	{
		QSTRING_WITH_SIZE(a, 1024 + Path.length());
		a.sprintf(LoadString(knUNABLE_TO_RENAME_FILE_X), (LPCSTR)Path);

		QMessageBox::critical(qApp->mainWidget(), LoadString(knERROR_RENAMING_FILE), (LPCSTR)a);
		return keStoppedByUser;
	}

	RenameLocalShares(Path, Parent);

	m_FileName = sNewLabel;

	return keSuccess;
}

////////////////////////////////////////////////////////////////////////////

QString CLocalFileItem::GetTip(int nType /* = 0 */)
{
  return GetLocalFileTip(nType, IsLink(), IsFolder(), m_FileName, FullName(FALSE), m_TargetName);
}

////////////////////////////////////////////////////////////////////////////

BOOL CLocalFileContainer::CanCreateSubfolder()
{
	return CanMountAt(FullName(FALSE));
}

////////////////////////////////////////////////////////////////////////////

void CLocalFileContainer::Rescan()
{
	m_nNumExtraItems = 0;
	CNetworkTreeItem::Rescan();
}

////////////////////////////////////////////////////////////////////////////

BOOL CLocalFileContainer::CreateSubfolder(QString& sFolderName)
{
	QString Result = FullName(FALSE);

	if (Result.right(1) != "/")
		Result += "/";

	struct stat st;

	if (!stat(Result + sFolderName, &st))
	{
		int i;

		for (i=2; i < 1000; i++)
		{
			QSTRING_WITH_SIZE(x, 256 + sFolderName.length());

			x.sprintf("%s (%d)", (LPCSTR)sFolderName, i);

			if (stat(Result + x, &st))
			{
				sFolderName = (LPCSTR)x;
				break;
			}
		}

		if (i == 1000)
			return FALSE; // unable to create unique name...
	}

	Result += sFolderName;

TryAgain:;

	if (mkdir(Result, 0777))
  {
    if (!ReportCommonFileError(Result, errno, FALSE, knCREATE_NEW_FOLDER, knUNABLE_TO_CREATE_FOLDER))
      goto TryAgain;

    return FALSE;
  }

  return TRUE;
}

////////////////////////////////////////////////////////////////////////////

BOOL CLocalFileContainer::CanEditPermissions()
{
	struct stat st;

	return IsSuperUser() ? TRUE : (!lstat(FullName(FALSE), &st) && st.st_uid == geteuid());
}

////////////////////////////////////////////////////////////////////////////

