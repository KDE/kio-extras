/* Name: treeitem.h

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

#ifndef __INC_TREEITEM_H__
#define __INC_TREEITEM_H__

#include <listview.h>
#include "wintree.h"
#include <qlist.h>

typedef enum
{
	keMSRootItem,
	keWorkgroupItem,
	keServerItem,
	keShareItem,
	keFileItem,
	kePrinterItem,
	keMyComputerItem,
	keFileSystemItem,
	keLocalFileItem,
	keFTPSiteItem,
	keFTPFileItem,
	keWebRootItem,
	keWebPageItem,
	keDesktopItem,
	keNFSRootItem,
	keNFSHostItem,
	keNFSShareItem,
  keTrashEntryItem,
  keDeviceItem,
  kePrinterRootItem,
  keLocalPrinterItem,
  kePrintJobItem
} CItemKind;

class CNetworkTreeItem;

typedef enum
{
	keNotExpanded,
	keExpanding,
	keExpansionComplete,
  keDeleteRequested
} CExpansionStatus;

typedef enum
{
	keDetail_All,		 // Everything including regular files
	keDetail_Folders,	 // Everything excluding regular files
	keDetail_Shares,	 // Stop at share level, to further detail
	keDetail_Printers	 // same as above but show only printer shares
} CDetailLevel;

class CNetworkTreeItem : public CWindowsTreeItem
{
public:
	CNetworkTreeItem(CListView *Parent, CListViewItem *pLogicalParent);
	CNetworkTreeItem(CListViewItem *Parent, CListViewItem *pLogicalParent);

	virtual QTTEXTTYPE text(int column) const = 0; /* implementation for CListViewItem */

	void setOpen(bool bOpen);
	virtual void Fill() = 0;

	virtual CItemKind Kind() = 0;
	virtual QString FullName(BOOL bDoubleSlashes) = 0;
	virtual int CredentialsIndex() = 0;
	virtual QPixmap *Pixmap() = 0;

	virtual QPixmap *BigPixmap()
	{
		return Pixmap();
	}

	void setup(); /* implementation for CListViewItem */

	virtual BOOL ItemAcceptsDrops()
	{
		return FALSE;
	}

	virtual BOOL ContentsChanged(time_t SinceWhen, int nOldChildCount, CListViewItem *pOldFirst = NULL);
	virtual int DesiredRefreshDelay();

	/*CNetworkTreeItem *m_pParent;*/

	void InitPixmap();

	static BOOL IsNetworkTreeItem(CListViewItem *pItem)
	{
		return pItem != NULL && !strcmp(pItem->text(-1)
#ifdef QT_20
		.latin1()
#endif
                , "Network");
	}

	static BOOL IsMyComputerItem(CListViewItem *pItem)
	{
		return pItem != NULL && !strcmp(pItem->text(-1)
#ifdef QT_20
		.latin1()
#endif
                , "MyComputer");
	}

	// These two functions allow setting and getting "expansion" status.
	// Expansion status set to TRUE means that all the children have been
	// successfully added.
	// Expansion status set to FALSE means that none or some children
	// may be there, but Fill() is required when we expand that node.

	CExpansionStatus ExpansionStatus() const
	{
		return m_ExpansionStatus;
	}

	void SetExpansionStatus(CExpansionStatus Value)
	{
		m_ExpansionStatus = Value;
	}

	virtual CDetailLevel DetailLevel();
	virtual void Rescan();

	void SetPixmapID(int nPixmapID, BOOL bIsHand, BOOL bIsLink);
	void GetPixmapID(int& nPixmapID, BOOL& bIsHand, BOOL& bIsLink);
  QPixmap *DefaultFilePixmap(BOOL bIsBig, LPCSTR FileAttributes, mode_t TargetMode, LPCSTR TargetName, BOOL bIsLink, BOOL bIsFolder);
private:
	CExpansionStatus m_ExpansionStatus;
	unsigned short m_nPixmapID;
public:
	CListViewItem *m_pLogicalParent;
};

class CRemoteFileContainer : public CNetworkTreeItem
{
public:
	CRemoteFileContainer(CListView *Parent, CListViewItem *pLogicalParent) :
		CNetworkTreeItem(Parent, pLogicalParent)
	{
	}

	CRemoteFileContainer(CListViewItem *Parent, CListViewItem *pLogicalParent) :
		CNetworkTreeItem(Parent, pLogicalParent)
	{
	}

	int CredentialsIndex();
	int m_nCredentialsIndex;
};

BOOL ItemAcceptsDrops(CListViewItem *pItem);
BOOL ItemIsDraggable(CListViewItem *pItem);

#define IS_NETWORKTREEITEM(x) (CNetworkTreeItem::IsNetworkTreeItem(x) || CNetworkTreeItem::IsMyComputerItem(x))

QPixmap CreateDragPixmap(CListViewItem *pItem);

typedef QList<QStrList> CChainList;

void GetChainForItem(CListView *pView, CListViewItem *pItem, QStrList& Chain);

CListViewItem *GetItemFromChain(CListView *pView, const QStrList& Chain);

BOOL CompareChains(const QStrList& Chain1, const QStrList& Chain2);
void RecordOpenFromItem(CListView *pView, CListViewItem *pItem, CChainList &list);
void RecordOpen(CListView *pView, CChainList &list);
void RescanItem(CNetworkTreeItem *pItem);
void RefreshHandPixmaps(CListView *pView);
void LogHandUsage(CNetworkTreeItem *pItem);
QString MakeItemURL(CNetworkTreeItem *pI);
BOOL ItemIsFolder(CNetworkTreeItem *pItem);
#endif /* __INC_TREEITEM_H__ */
