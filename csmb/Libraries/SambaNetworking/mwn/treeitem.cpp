/* Name: treeitem.cpp

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
#include "treeitem.h"
#include "qpainter.h"
#include "qpixmap.h"
#include <qbitmap.h>
#include <qapplication.h>
#include <qstrlist.h>
#include "inifile.h"
#include <sys/types.h>
#include <sys/stat.h>
#include "smbfile.h"
#include "localfile.h"
#include "trashentry.h"
#include "ftpfile.h"

////////////////////////////////////////////////////////////////////////////

BOOL ItemIsDraggable(CListViewItem *pItem)
{
	if (pItem != NULL &&
		(CNetworkTreeItem::IsNetworkTreeItem(pItem) || CNetworkTreeItem::IsMyComputerItem(pItem))
		)
	{
		switch (((CNetworkTreeItem*)pItem)->Kind())
		{
			default:
			break;

			case keShareItem:
			case keFileItem:
			case keLocalFileItem:
			case keFTPFileItem:
				return TRUE;
		}
	}

	return FALSE;
}

////////////////////////////////////////////////////////////////////////////

QPixmap CreateDragPixmap(CListViewItem *pItem)
{
	CListView *pView = pItem->listView();

	QRect r = pView->itemRect(pItem);

	QPixmap pix(r.width(), r.height());
	
	QPainter p(&pix);
	QBitmap mask;

	p.fillRect(0,0,r.width(),r.height(), pView->colorGroup().base());
	
#ifndef QT_20
  if (pItem->listView()->iconView())
	{
		const QPixmap * icon = pItem->iconViewPixmap();

		int marg = 1;
		int w = 0;
		int h = 0;
		QString t = pItem->text(0);

		int maxW = r.width() - 4 - marg*2;
		
		if (!t.isEmpty())
		{
			w = pView->fontMetrics().width(t);
			
			if (w > maxW)
			{
				while (w > maxW && t.length() > 1)
				{
					t = t.left(t.length() - 1);
					w = pView->fontMetrics().width(t + "...");
				}
				
				t = t + "...";
			}
		}
		
		/*if (w > width - marg*2)
		{
			w = width - marg*2;
		}*/
		
		if (w > maxW)
		{
			w = maxW;
		}
		
		h = pView->fontMetrics().height();
		
		QRect IconTextRect((r.width() - w)/2 - 2, 32 + ICON_VIEW_MARGIN*2, w + 4, h + 2);

		p.setPen(pView->colorGroup().text());

		p.drawPixmap(r.width()/2 -16, ICON_VIEW_MARGIN, *icon);
		p.end();
		
    mask = pix.createHeuristicMask();
		
		p.begin(&pix);
		p.setPen(pView->colorGroup().dark());
		p.drawText(IconTextRect.left() + 2, IconTextRect.top() + 1, w, h, Qt::AlignVCenter, t);
		p.end();
		
		p.begin(&mask);
		QBrush br(color0, Dense4Pattern);
		p.setPen(NoPen);
	
		p.setBrush(br);
		p.drawRect(r.width()/2 - 16, ICON_VIEW_MARGIN, icon->width(), icon->height());
		
		p.setPen(color1);
		p.drawText(IconTextRect.left() + 2, IconTextRect.top() + 1, w, h, Qt::AlignVCenter, t);		
	}
	else
#endif
	{
		const QPixmap *icon = pItem->pixmap(0);
	
		int off = pView->itemMargin();
		p.drawPixmap(off, (r.height()-icon->height()) / 2, *icon);
		off += icon->width() + pView->itemMargin();
		
		p.end();
		
		mask = pix.createHeuristicMask();
		
		p.begin(&pix);
		p.setPen(pView->colorGroup().dark());
		p.drawText(off, 0, r.width()-off, r.height(), 
               Qt::AlignVCenter, 
               pItem->text(0));
		p.end();
		
		p.begin(&mask);
		
#if (QT_VERSION < 200)
#define QTCOLOR1 color1
    QBrush br(color0, Dense4Pattern);
#else
    QBrush br(Qt::color1, Qt::Dense4Pattern);
#define QTCOLOR1 Qt::color1
#endif

		p.setPen(Qt::NoPen);

		p.setBrush(br);
		p.drawRect(0, 0, pView->itemMargin() + icon->width(), r.height());
		
		p.setPen(QTCOLOR1);
    p.drawText(off, 0, r.width()-off, r.height(), 
               Qt::AlignVCenter, 
               pItem->text(0));
	}

	p.end();
	
	pix.setMask(mask);
	
	return pix;
}

////////////////////////////////////////////////////////////////////////////

void CNetworkTreeItem::setOpen(bool bOpen) 
{
  if (bOpen)
  {
    if (ExpansionStatus() == keNotExpanded && 
  			parent() == m_pLogicalParent)
  	{
  		listView()->viewport()->setCursor(waitCursor);

      Fill();
  		
      listView()->viewport()->setCursor(arrowCursor);
  
  		if (ExpansionStatus() == keNotExpanded)
  			return; // Cancelled by user or unable to expand...
  	}
  
    if (!childCount() && ExpansionStatus() == keExpansionComplete)
    {
      setExpandable(FALSE);
      listView()->repaintItem(this);
    }

  	/*if ((Kind() == keLocalFileItem || Kind() == keFileSystemItem))
  	{
  		CNetworkTreeItem *pI;
  	
  		for (pI=(CNetworkTreeItem*)firstChild(); NULL != pI; pI=(CNetworkTreeItem*)pI->nextSibling())
  		{
  			if (pI->ExpansionStatus() == keNotExpanded)
        {
          //pI->Fill();
          BOOL bHasSubfolders = LocalHasSubfolders(pI->FullName(FALSE));
          
          //if (!bHasSubfolders)
          {
            //printf("%s has no subfolders\n", (LPCSTR)pI->FullName(FALSE));
            pI->setExpandable(bHasSubfolders);
            //listView()->repaintItem(pI);
          }
        }
  		}
  	}
    */
  }

	// If we're collapsing that node and the current item is womewhere inside the subtree we're collapsing,
	// we should make the node we're collapsing current.

	if (!bOpen && !isSelected())
	{
		CListViewItem *pCurrent = listView()->currentItem();
		
		while (NULL != pCurrent)
		{
			pCurrent = pCurrent->parent();
			
			if (pCurrent == this)
			{
				listView()->setSelected(this, TRUE);
				break;
			}
		}		
	}

	CListViewItem::setOpen(bOpen);
}

////////////////////////////////////////////////////////////////////////////

void CNetworkTreeItem::InitPixmap()
{
  m_nPixmapID = 0xffff; // not set
	
  QPixmap *pPixmap = Pixmap();

  int nPixmapID;
  BOOL bIsHand;
  BOOL bIsLink;

  GetPixmapID(nPixmapID, bIsHand, bIsLink);

  if (!listView()->isMultiSelection() && 
      this == listView()->currentItem() && 
      nPixmapID == keClosedFolderIcon)
  {
    pPixmap = LoadPixmap(keOpenFolderIcon, bIsLink, bIsHand);
  }

  setPixmap(0, *pPixmap);
  setIconViewPixmap(*BigPixmap());
}

////////////////////////////////////////////////////////////////////////////

void CNetworkTreeItem::SetPixmapID(int nPixmapID, BOOL bIsHand, BOOL bIsLink)
{
  m_nPixmapID = (nPixmapID & 0x3fff) | ((bIsHand & 1) << 15) | ((bIsLink & 1) << 14);
}

////////////////////////////////////////////////////////////////////////////

void CNetworkTreeItem::GetPixmapID(int& nPixmapID, BOOL& bIsHand, BOOL& bIsLink)
{
	nPixmapID = m_nPixmapID & 0x3fff;
	bIsHand = (m_nPixmapID >> 15) & 1;
	bIsLink = (m_nPixmapID >> 14) & 1;
}

////////////////////////////////////////////////////////////////////////////

CNetworkTreeItem::CNetworkTreeItem(CListView *parent, CListViewItem *pLogicalParent) :
	CWindowsTreeItem(parent)
{
	SetExpansionStatus(keNotExpanded);
	m_pLogicalParent = pLogicalParent;
}

////////////////////////////////////////////////////////////////////////////

CNetworkTreeItem::CNetworkTreeItem(CListViewItem *parent, CListViewItem *pLogicalParent) :
	CWindowsTreeItem(parent)	
{
	SetExpansionStatus(keNotExpanded);
	m_pLogicalParent = (NULL == pLogicalParent) ? parent : pLogicalParent;
}

////////////////////////////////////////////////////////////////////////////

void CNetworkTreeItem::setup()
{
  BOOL bCanExpand = TRUE;
	
	if (ExpansionStatus() == keNotExpanded)
	{
		if (Kind() == keShareItem)
		{
			CDetailLevel Level = DetailLevel();
			
			if (Level == keDetail_Shares || 
				Level == keDetail_Printers)
				bCanExpand = FALSE;
		}
		else
			if (Kind() == keWebPageItem)
				bCanExpand = FALSE;
      else
        if ((Kind() == keLocalFileItem || Kind() == keFileSystemItem))
        {
          bCanExpand = LocalHasSubfolders(FullName(FALSE));
        }
	}
	else
		bCanExpand = (childCount() > 0);

	setExpandable(bCanExpand);
	
	CListViewItem::setup();
}

////////////////////////////////////////////////////////////////////////////

// This function returns the desired detalization level in the tree.
// This is the default implementation suitable for most child items.
// It will go back to the most senior parent which knows the answer.
// (CMSWindowsNetworkItem, for instance, knows that for sure, and
// has its own implementation of DetailLevel()).

CDetailLevel CNetworkTreeItem::DetailLevel()
{
	CNetworkTreeItem *pParent = (CNetworkTreeItem*)parent();
	
	return (pParent == NULL) ? keDetail_Folders : pParent->DetailLevel();
}

////////////////////////////////////////////////////////////////////////////

void CNetworkTreeItem::Rescan()
{
	BOOL bWasExpanded = (ExpansionStatus() != keNotExpanded);

  while (NULL != firstChild())
	{
		//gTreeExpansionNotifier.DoItemDestroyed(firstChild());
		delete firstChild();
	}
	
	setOpen(FALSE);
	SetExpansionStatus(keNotExpanded);
  
  if (bWasExpanded)
    Fill();
}

////////////////////////////////////////////////////////////////////////////

int CRemoteFileContainer::CredentialsIndex()
{
	CNetworkTreeItem *pParent = (CNetworkTreeItem*)m_pLogicalParent;
	
	if (m_nCredentialsIndex == -1 && pParent != NULL)
		return pParent->CredentialsIndex();
	else
		return m_nCredentialsIndex;
}

////////////////////////////////////////////////////////////////////////////

BOOL CNetworkTreeItem::ContentsChanged(time_t SinceWhen, int nOldChildCount, CListViewItem *pOldFirst)
{
	return FALSE;
}

////////////////////////////////////////////////////////////////////////////

int CNetworkTreeItem::DesiredRefreshDelay()
{
	return 10000; // 10 seconds
}

////////////////////////////////////////////////////////////////////////////

void GetChainForItem(CListView *pView, CListViewItem *pItem, QStrList& Chain)
{
	while (NULL != pItem)
	{
		Chain.append(pItem->text(0));
		pItem = pItem->parent();
	}
}

////////////////////////////////////////////////////////////////////////////

CListViewItem *GetItemFromChain(CListView *pView, const QStrList& Chain)
{
	QStrListIterator it(Chain);

	CListViewItem *pI = pView->firstChild();
	
	for (it.toLast(); NULL != it.current() && NULL != pI;)
	{
		for (; NULL != pI; pI = pI->nextSibling())
		{
			if (!strcmp(it.current(), pI->text(0)))
				break;
		}

		--it;

		if (NULL != it.current() && NULL != pI)
		{
			pI->setOpen(TRUE);
			pI = pI->firstChild();
		}
	}

	return pI;
}

////////////////////////////////////////////////////////////////////////////

BOOL CompareChains(const QStrList& Chain1, const QStrList& Chain2)
{
	QStrListIterator it1(Chain1);
	QStrListIterator it2(Chain2);
	
  for(it1.toFirst(), it2.toFirst(); NULL != it1.current() && NULL != it2.current(); ++it1, ++it2)
	{
    if (strcmp(it1.current(), it2.current()))
      return FALSE;
	}

  return (NULL == it1.current() && NULL == it2.current());
}

////////////////////////////////////////////////////////////////////////////

void RecordOpenFromItem(CListView *pView, CListViewItem *pItem, CChainList &list)
{
	//printf("RecordOpenFromItem %s\n", pItem->text(0));

	CListViewItem *pI;
	
	BOOL bFound = FALSE;

	for (pI = pItem->firstChild(); NULL != pI; pI=pI->nextSibling())
	{
		if (pI->isOpen())
		{
			bFound = TRUE;
			RecordOpenFromItem(pView, pI, list);
		}
	}

	if (!bFound && pItem->isOpen())
	{
		QStrList *pChain = new QStrList;
		GetChainForItem(pView, pItem, *pChain);
		list.append(pChain);
	}
}

////////////////////////////////////////////////////////////////////////////

void RecordOpen(CListView *pView, CChainList &list)
{
	CListViewItem *pItem;

	for (pItem=pView->firstChild(); NULL != pItem; pItem = pItem->nextSibling())
	{
		if (pItem->isOpen())
		{
			RecordOpenFromItem(pView, pItem, list);
		}
	}
}

////////////////////////////////////////////////////////////////////////////

void RescanItem(CNetworkTreeItem *pItem)
{
	CChainList ChainList;
	ChainList.setAutoDelete(TRUE);
	CListView *pView = pItem->listView();

	RecordOpenFromItem(pView, pItem, ChainList);
			
	gTreeExpansionNotifier.DoStartRescanItem(pItem);

	pItem->Rescan();
			
	QListIterator<QStrList> it(ChainList);

	for (it.toFirst(); NULL != it.current(); ++it)
	{
		CListViewItem *pI = GetItemFromChain(pView, *it.current());

		if (NULL != pI)
			pI->setOpen(TRUE);
	}
	
	gTreeExpansionNotifier.DoEndRescanItem(pItem);
}

////////////////////////////////////////////////////////////////////////////

typedef struct
{
	QStrList m_Chain;
	CNetworkTreeItem *m_pItem;
} CHandCacheEntry;

QList<CHandCacheEntry> gHandCache;

////////////////////////////////////////////////////////////////////////////

void LogHandUsage(CNetworkTreeItem *pItem)
{
	gHandCache.setAutoDelete(true);

	QListIterator<CHandCacheEntry> it(gHandCache);

	for (it.toFirst(); NULL != it.current(); ++it)
	{
		if (it.current()->m_pItem == pItem)
			return; // found by pointer
	}

	CHandCacheEntry *pE = new CHandCacheEntry;
	
	GetChainForItem(pItem->listView(), pItem, pE->m_Chain);
  pE->m_pItem = pItem;
	gHandCache.append(pE);
}

////////////////////////////////////////////////////////////////////////////

void RefreshSharedPixmapsFromItem(CListViewItem *pItem)
{
	if (IS_NETWORKTREEITEM(pItem))
	{
		CNetworkTreeItem *pI = (CNetworkTreeItem*)pItem;
		
		if (pI->Kind() != keLocalFileItem && 
				pI->Kind() != keFileSystemItem && 
				pI->Kind() != keMyComputerItem)
		{
			return;
		}

    if (pI->Kind() == keLocalFileItem &&
				IsShared(pI->FullName(FALSE)))
		{
			pI->InitPixmap();
		}

		for (pI=(CNetworkTreeItem*)pI->firstChild(); NULL != pI; pI = (CNetworkTreeItem*)pI->nextSibling())
		{
			RefreshSharedPixmapsFromItem(pI);
		}
	}
}

////////////////////////////////////////////////////////////////////////////

void RefreshHandPixmaps(CListView *pView)
{
	QListIterator<CHandCacheEntry> it(gHandCache);

	for (it.toLast(); NULL != it.current();)
	{
		CHandCacheEntry *pE = it.current();
		--it;

		CNetworkTreeItem *pI = (CNetworkTreeItem*)GetItemFromChain(pView, pE->m_Chain);

		if (NULL != pI)
		{
			pI->InitPixmap();
			
			if (IsShared(pI->FullName(FALSE)))
				gHandCache.remove(pE);
		}
	}

	if (NULL != pView)
	{
		CListViewItem *pItem = pView->firstChild();
	
		if (NULL != pItem)
		{
			for (pItem=pItem->firstChild(); NULL != pItem; pItem=pItem->nextSibling())
			{
				RefreshSharedPixmapsFromItem(pItem);
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////

QPixmap *CNetworkTreeItem::DefaultFilePixmap(BOOL bIsBig, 
                                             LPCSTR FileAttributes, 
                                             mode_t TargetMode, 
                                             LPCSTR TargetName,
                                             BOOL bIsLink,
                                             BOOL bIsFolder) 
{
  if (bIsFolder)
	{
		QString DirFile = FullName(FALSE);
		
		if (DirFile.right(1) != "/")
			DirFile += "/";
		
		DirFile += ".directory";

		QPixmap *pIcon = GetIconFromKDEFile(DirFile, bIsBig);
		
		if (NULL != pIcon)
			return pIcon;

		BOOL bIsHand = IsShared(FullName(FALSE));

		if (bIsHand)
			LogHandUsage(this);

		if (!bIsBig)
			SetPixmapID(keClosedFolderIcon, bIsHand, bIsLink);
		
		return LoadPixmap(bIsBig ? keClosedFolderIconBig : keClosedFolderIcon, bIsLink, bIsHand);
	}
	
	switch (FileAttributes[0])
	{
 		case 's':
			return LoadPixmap(keMSRootIcon);

		case 'c':
		case 'b':
			return LoadPixmap(bIsBig ? keDeviceIconBig : keDeviceIcon);

		case 'p':
			return LoadPixmap(keFIFOIcon);

		case 'l':					 
		{
			if (TargetMode == 0xffffffff)
				return LoadPixmap(bIsBig ? keBrokenLinkBigIcon : keBrokenLinkIcon);
			
			if (S_ISDIR(TargetMode))
				return LoadPixmap(bIsBig ? keClosedFolderIconBig : keClosedFolderIcon, 1);
			else
				if (S_IFCHR == (TargetMode & S_IFMT) || S_IFBLK == (TargetMode & S_IFMT))
					return LoadPixmap(bIsBig ? keDeviceIconBig : keDeviceIcon, 1);
				else
					if (S_ISFIFO(TargetMode))
						return LoadPixmap(keFIFOIcon, 1);
					else
						return GetFilePixmap(TargetName, 
                                 TRUE, 
                                 (TargetMode & S_IXUSR) == S_IXUSR ||	(TargetMode & S_IXGRP) == S_IXGRP || (TargetMode & S_IXOTH) == S_IXOTH, 
                                 bIsBig);
		}
	}
	
	QString FileName(text(0));
  
  if (FileName.right(7) == ".kdelnk")
	{
		QPixmap *pIcon = GetIconFromKDEFile(FullName(FALSE), bIsBig);
		
		if (NULL != pIcon)
			return pIcon;
	}
	
	return GetFilePixmap(FileName, FALSE, NULL != strchr(FileAttributes,'x'), bIsBig);
}

////////////////////////////////////////////////////////////////////////////

QString MakeItemURL(CNetworkTreeItem *pI)
{
  QString s(pI->FullName(FALSE));
  
	if ((pI->Kind() == keFTPFileItem || pI->Kind() == keFTPSiteItem) && pI->CredentialsIndex() != 1)
  {
    QString Hostname, Path;
    int nCredentialsIndex;
    ParseURL(s, Hostname, Path, nCredentialsIndex);
    nCredentialsIndex = pI->CredentialsIndex();
    int nIndex = Hostname.findRev('@');
    
    if (-1 != nIndex)
      Hostname = Hostname.mid(nIndex+1, Hostname.length());
  
    s.sprintf("ftp://%s:%s@%s%s",
      (LPCSTR)gCredentials[nCredentialsIndex].m_UserName,
      (LPCSTR)gCredentials[nCredentialsIndex].m_Password,
      (LPCSTR)Hostname,
      (LPCSTR)Path);
  }
  
  QString URL;

  MakeURL(s, NULL, URL);

  return URL;
}

////////////////////////////////////////////////////////////////////////////

BOOL ItemIsFolder(CNetworkTreeItem *pItem)
{
  switch (pItem->Kind())
  {
    case keFileItem:
      return ((CFileItem*)pItem)->IsFolder();
    
    case keFTPFileItem:
      return ((CFileItem*)pItem)->IsFolder();
    
    case keLocalFileItem:
      return ((CLocalFileItem*)pItem)->IsFolder();
  
    case keTrashEntryItem:
      return ((CTrashEntryItem*)pItem)->IsFolder();
  
    default:
      break;
  }
  
  return FALSE;
}

