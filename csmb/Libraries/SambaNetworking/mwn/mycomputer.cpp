/* Name: mycomputer.cpp

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
#include "common.h"
#include "mycomputer.h"
#include "filesystem.h"
#include "smbutil.h"
#include "exports.h"
#include <time.h>

////////////////////////////////////////////////////////////////////////////

void CMyComputerItem::UpdateFileSystemInfo()
{
	time(&m_LastUpdateTime);

	int i;
	CListViewItem *child;

	// First pass, see if some new filesystems are there

	for (i=0; i < gFileSystemList.count(); i++)
	{
		CFileSystemInfo *pI = &gFileSystemList[i];

		for (child=firstChild(); NULL != child; child=child->nextSibling())
		{
			if (!strcmp(child->text(0), pI->m_MountedOn))
				break;
		}
		
		if (NULL == child)
			new CFileSystemItem(this, pI, NULL);
	}

	// Second pass, see which filesystems are gone...

	for (child=firstChild(); NULL != child; child=child->nextSibling())
	{
		for (i=gFileSystemList.count()-1; i >=0; i--)
		{
			if (!strcmp(child->text(0), gFileSystemList[i].m_MountedOn))
				break;
		}

		if (-1 == i)
		{
			gTreeExpansionNotifier.DoItemDestroyed(child);
			delete child;
		}
	}
}

////////////////////////////////////////////////////////////////////////////

void CMyComputerItem::CheckRefresh()
{
  if (gFileSystemListTimestamp > m_LastUpdateTime)
	{
		UpdateFileSystemInfo();
		return;
	}

	// Advanced auto-refresh. 
	// Do it only each other time

	static BOOL bOdd = FALSE;

	if (bOdd && !IsScreenSaverRunning())
	{
  	CheckChangedFromItem(this);		
	}
	
	bOdd = !bOdd;
}

////////////////////////////////////////////////////////////////////////////

void CMyComputerItem::Fill()
{
	time(&m_LastUpdateTime);
	
	CFileSystemArray list;
	
	GetFileSystemList(&list);
	
	int i;

	for (i=0; i < list.count(); i++)
		new CFileSystemItem(this, &list[i], NULL);

	SetExpansionStatus(keExpansionComplete);
	gTreeExpansionNotifier.Fire(this);
	
	QObject::connect(&m_Timer, SIGNAL(timeout()), this, SLOT(CheckRefresh()));
	m_Timer.start(500); // 2 times a second
}

////////////////////////////////////////////////////////////////////////////

void CMyComputerItem::AddFileSystem(LPCSTR Name)
{
	CFileSystemArray list;
	
	GetFileSystemList(&list);
	
	int nIndex = list.Find(Name);
	
	if (-1 != nIndex)
	{
		new CFileSystemItem(this, &list[nIndex], NULL);

		/* Rescan affected filesystems */

		CListViewItem *pCurrentItem = listView()->currentItem();
		
		//listView()->setSelected(listView()->firstChild(), TRUE);

		QString SelectedPath;

		if (pCurrentItem != NULL && CNetworkTreeItem::IsMyComputerItem(pCurrentItem))
		  SelectedPath = ((CNetworkTreeItem*)pCurrentItem)->FullName(FALSE);

		CFileSystemItem *pItemF;

		for (pItemF = (CFileSystemItem*)firstChild(); pItemF != NULL; pItemF = (CFileSystemItem*)pItemF->nextSibling())
		{
			int nMin = pItemF->m_MountedOn.length();
			
			if (nMin > (int)strlen(Name))
				nMin = strlen(Name);

			if (!strncmp(pItemF->m_MountedOn, Name, nMin))
				pItemF->Rescan();
		}

		/* Select the old path if possible */

		if (!SelectedPath.isEmpty())
		{
			pCurrentItem = FindAndExpand(SelectedPath);
			
			if (NULL != pCurrentItem)
				listView()->setSelected(pCurrentItem, TRUE);
				
		}

		gTreeExpansionNotifier.Fire(this);
	}
}

////////////////////////////////////////////////////////////////////////////

CListViewItem* CMyComputerItem::FindAndExpand(LPCSTR Path)
{
	CListViewItem *child;
	CListViewItem *pTargetItem;

	if (!isOpen())
		setOpen(TRUE);

	// First try to find the beginning of that path in our filesystem list
	// We also search for longest match, because, for example, 
	// "/" and "/mnt/C" begin with the same substring.

	int nMaxMatch = 0;
	pTargetItem = NULL;
  int nPathLen = strlen(Path);

	for (child = firstChild(); child != NULL; child = child->nextSibling())
	{
		LPCSTR thisText = child->text(0);
		int thisLen = strlen(thisText);

		if (thisLen <= nPathLen &&
        thisLen > nMaxMatch && 
			  !strnicmp(thisText, Path, thisLen) &&
        (thisLen == nPathLen || thisLen == 1 || Path[thisLen] == '/'))
		{
      pTargetItem = child;
		  nMaxMatch = strlen(thisText);
		}
	}

	if (nMaxMatch == 0)
		return NULL;    /* nope */

	pTargetItem->setOpen(TRUE);

	Path +=nMaxMatch;

	while (*Path != '\0')	// This will go deep to last folder
	{
		LPCSTR p = (LPCSTR)Path;

		QString Folder = ExtractWord(p,"\\/");
		
    if (Folder.isEmpty())
      break;
    
		Path = p;
				
		for (child = pTargetItem->firstChild(); NULL != child; child = child->nextSibling())
		{
			if (!stricmp(child->text(0), Folder))
				break;
		}

		if (NULL == child)
		{
			// Deal if hidden file is explicitly requested but not
			// present in our tree
      
			QString LocalPath(((CNetworkTreeItem*)pTargetItem)->FullName(FALSE));
			
			if (LocalPath.right(1) != "/")
				LocalPath += "/";

      LocalPath += Folder;
			
			if (!access(LocalPath, R_OK))
			{
				CSMBFileInfo fi;

				FillFileInfo(&fi, (LPCSTR)LocalPath, (LPCSTR)Folder, NULL, NULL);
				
				if (fi.IsFolder())
				{
					child = new CLocalFileItem(pTargetItem, pTargetItem, &fi);
					((CLocalFileContainer *)pTargetItem)->m_nNumExtraItems++;
				}
			}
		}
		
		if (NULL != child)
		{
			pTargetItem = child;
			pTargetItem->setOpen(TRUE);
		}
		else
			break; // Abort search if not found
	}

	return pTargetItem;
}

////////////////////////////////////////////////////////////////////////////

QTTEXTTYPE CMyComputerItem::text(int column) const
{
  switch (column)
  {
    case -1:
      return "MyComputer";
    
    case 0:
      return LoadString(knMY_COMPUTER);
    
    default:
      return "";
  }
}

////////////////////////////////////////////////////////////////////////////

BOOL CMyComputerItem::ContentsChanged(time_t SinceWhen, 
																			int /*nOldChildCount*/, 
																			CListViewItem */*pOldFirst*/)
{
	return m_LastUpdateTime > SinceWhen;
}


