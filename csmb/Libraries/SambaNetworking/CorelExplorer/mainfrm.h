/* Name: mainfrm.h

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

#ifndef __INC_MAINFRM_H__
#define __INC_MAINFRM_H__

#include <qapplication.h>
#include "mainwindow.h"

#ifdef QT_20
#include <kparts/mainwindow.h>
#include <kparts/browserextension.h>
#include <kparts/partmanager.h>
#include <ktrader.h>
#include <history.h>
#endif


#include <flattoolbar.h>
#include <qtoolbutton.h>
#include <qiconset.h>
#include <qlist.h>
#include <qstrlist.h>

#include <qmenubar.h>
#include "common.h"
#include "nfsroot.h"

class CRightPanel;
class QSplitter;
class CTreeToolTip;
class CListViewItem;
class QComboBox;
class CMSWindowsNetworkItem;
class CPrinterRootItem;
class CMyComputerItem;
class CLeftTreeView;
class QPopupMenu;
class QStatusBar;
class QFrame;
class CWebRootItem;
class CHomeItem;
class KHTMLView;
class KFM;
class KConfig;
class CFileSystemInfo;

#include "qsplitter.h"

#include <qlabel.h>
#include "autotopcombo.h"

typedef struct
{
	int m_nItemID;
	QPopupMenu *m_pMenu;
} CMenuItemInfo;

typedef CVector<CMenuItemInfo, CMenuItemInfo&> CMenuItemInfoArray;

class CIconText : public QLabel
{
public:
	CIconText(QWidget *parent) : QLabel(parent)
	{
	}

	void paintEvent(QPaintEvent *e);

	void setText(LPCSTR s)
	{
		m_Text = s;
		update();
	}

private:
	QString m_Text;
};

class CMainFrame;

typedef BOOL (CMainFrame::* LPFNCanDo)(CNetworkTreeItem *);

class CMainFrame : public CMainWindow
{
	Q_OBJECT
public:
	CMainFrame(QApplication *pApp);
	~CMainFrame();

	/*virtual bool close(bool forceKill = FALSE);*/

protected:
  bool event(QEvent *e);
	void GoItem(CListViewItem *pItem);
	void GoItem(LPCSTR TargetAddress);

	void CreateMenus();
	void ActivatePart(CHistoryItem* hItem);
	void DeactivatePart(CHistoryItem* hItem);
	void UpdateComboAndToolBar(CListViewItem *pItem);
	void OnSelectDeselect();
	void CreateToolBar();
	void CreateAddressBar();
	void CreateStatusBar();
	void CreateViews();

	void ActivateActions();
  void OpenWithActions( const KTrader::OfferList &services );
	void toggleBar( const char *name, const char *className );

	void EnableMenuItem(int nID, BOOL bEnable);
	void CheckMenuItem(int nID, BOOL bIsChecked);

	void OnRightClicked(const QPoint& Point);

private:
	CFlatToolBar *m_pToolBar;
	CFlatToolBar *m_pAddressBar;
	QSplitter *m_pSplitter;
	QStatusBar *m_pStatusBar;

	QLabel *m_pObjectsLabel;
	QLabel *m_pSizeLabel;

	CIconText *m_pZoneLabel;

	CLeftTreeView *m_pTreeView;
	CRightPanel *m_pRightPanel;
	BOOL m_bPartVisible;
	KParts::ReadOnlyPart *m_pPart;
	KParts::PartManager *m_pManager;
	KTrader::OfferList m_PartServices;
	CListViewItem* m_pCurrentItem;


	CTreeToolTip *m_pTreeTip;
	CTreeToolTip *m_pRightTip;
	QToolButton **m_Buttons;
	CAutoTopCombo *m_pAddressCombo;
	CMSWindowsNetworkItem *m_pNetworkRoot;
	CNFSNetworkItem *m_pNFSRoot;
  CPrinterRootItem *m_pPrinterRoot;
	CMyComputerItem *m_pMyComputer;
	CWebRootItem *m_pWebRoot;
	CHomeItem *m_pHomeRoot;


	/* -------- Menus ------------ */

	CMenuItemInfoArray m_MainMenu;

  QPopupMenu *m_pFileRestoreMenuItem;
  QPopupMenu *m_pEmptyDumpsterMenuItem;
  QPopupMenu *m_pDumpsterSeparator;
  QPopupMenu *m_pSortMenu;

	QList<CNetworkTreeItem> ActiveItems;
	QList<KAction> m_SortBy;
	QList<KAction> m_Trash;
	QList<KAction> m_Select;
	QList<KAction> m_openWithActions;
	QList<KAction> m_ActionList;
  int m_nMenuActions;

	QString m_LastMenuParam;

	BOOL m_bNoAdd;

private slots:
	/* File menu */

	void OnFileNewFolder();
	void NewFolder(); // internal
	void OnFileDelete();
  void FileDelete();   // same as above but without GetActiveItems()
	void OnFileNuke();
  void FileNuke();    // same as above but without GetActiveItems()

	void OnFileRename();
  void FileRename();  // same as above but without GetActiveItems()
	void FileRenameAction(); // internal (invoked by timer)
	void OnFileProperties();
	void OnFileExit();
  void OnFileRestore(); // for Trash only
  void OnEmptyDumpster(); // Nuke everything in Trash

	/* Edit menu */

	void OnUndo();
	void OnFileCut();
  void FileCut(); // same as above but without GetActiveItems()
	void OnFileCopy();
  void FileCopy(); // same as above but without GetActiveItems()
	void OnFilePaste();
	void FilePaste(); // same as above but without GetActiveItems()
	void OnSelectAll();
	void OnOpenWith();
	void slotOpenWith();
	void OnInvertSelection();
	void SelectAll(); // internal
	void InvertSelection(); // internal

  void OnPurge(); // printer purge
  void Purge();   // same as above but without GetActiveItems()
	void OnDeletePrintJob();
	void DeletePrintJob();

  /* View menu */

	void OnViewToolBar();
	void slotShowToolBar();
	void OnViewMenuBar();
	void OnViewAddressBar();
	void OnViewStatusBar();
	void OnViewTree();
	void OnIconView();
	void OnShowHiddenFiles();
  void OnViewMyComputer();

	/* Go menu */

	void OnBack();
	void OnForward();
	void OnGoParent();
	void OnGoMyComputer();
	void OnGoNetworkRoot();

	/* Tools menu */

	void OnFindFiles();
	void OnFindComputer();
	void OnMountNetworkShareHere();
	void OnDisconnectShare();
  void OnOpenConsole();

	/* Help menu */

	void OnHelpTopics();
	void OnAbout();

	/* Other */

  void OnStop();
	void UpdateButtons();
	void OnTreeSelectionChanged(CListViewItem*);
  void OnTreeDoubleClicked(CListViewItem*);

	void OnRightChdir(CListViewItem* pExternalItem, QString Destination);
	void OnRightPopupMenuRequest(const QPoint& p);
  void OnPartRequest(CListViewItem* pItem);

	//void OnHTMLPopupMenuRequest(KHTMLView*, const char*, const QPoint& p);
	void OnSelchangeCombo(int nIndex);
	void ComboReturnPressed();
	void OnRightViewUpdated(int nObjectsCount, double TotalSize, int nSelectedCount);
	void OnRightClickedTree(CListViewItem *pItem, const QPoint& Point, int nColumn);
	void OnDisconnectFileSystem();
	void OnProperties();
	void OnRefresh();
	void OnSharing();
	void OnItemRenamed(CListViewItem *pItem);
	void OnTabRequest(BOOL bIsBacktab);
	void SizeInit();
	void SetSplitterMinSize();
  void OnLastWindowClosed();
	void OnLogoClicked();
	void OnItemDestroyed(CListViewItem *pItem);
	void OnStartRescanItem(CListViewItem*);
	void OnEndRescanItem(CListViewItem*);
	void OnSambaConfigurationChanged();
	void OnErrorAccessingURL(const char *Url);
	void OnBookmarkURL(const char *Title, const char *Url);
	void OnBrowserTextSelected(BOOL bSelected);
	void OnDocumentDone(const char *InitialURL, const char *NewURL, BOOL bIntranet);
  void OnMenuHighlighted(int nID);
  void OnSortColumn0();
  void OnSortColumn1();
  void OnSortColumn2();
  void OnSortColumn3();
  void OnSortColumn4();
  void OnSortColumn5();
  void OnSaveSession();
  void OnStatusMessage(const char *);
	void OnOpen();
	void OnFind();
	void OnCreateNickname();
	void OnEjectCD();
  void OnAddPrinter();

private:

	void DoProperties(BOOL bStartFromSharing);
	void GetActiveItems();
	void RemoveFiles(BOOL bMoveToTrash); // internal, called from OnFileDelete/OnFileNuke
	CListViewItem *GetCurrentItem();

public:
	BOOL CanDoOperation(LPFNCanDo pOperation);
	BOOL CanDoProperties(CNetworkTreeItem *pItem);
	BOOL CanDoCopy(CNetworkTreeItem *pItem);
	BOOL CanDelete(CNetworkTreeItem *pItem);
	BOOL CanMoveToTrash(CNetworkTreeItem *pItem);

	BOOL CanPaste();
	BOOL CanPasteHere();
	BOOL CanRename(CNetworkTreeItem *pItem);
	BOOL CanDoNewFolder(CNetworkTreeItem *pItem);
  BOOL CanRestoreFile(CNetworkTreeItem *pItem);
	BOOL CanDisconnectShare();
private:
	void DisconnectFileSystem(CListViewItem *pItem, CFileSystemInfo *pInfo);
  BOOL HandleDumpsterMenu();
  void AddDeviceItems(CListViewItem *pParent);
	void OpenConsole(CListViewItem *pItem);
  void PopulateSortMenu(QPopupMenu &SortMenu);
  void SaveConfigSettings(KConfig *config, BOOL bSavePath);
  void LoadConfigSettings(KConfig *config);
	BOOL IsLabelEditMode();
	void LoadBookmarks();
	void SaveBookmarks();

private:
	// The bookmarks are simply a title-link pair. They are grouped together in a
	// QList. To load the bookmarks from disk, use LoadBookmarks(). To update the
	// disk bookmark file to the contents of the QList of bookmarks, call
	// SaveBookmarks.

	struct SBookmark
	{
		enum
		{
			TitleMaxLength = 1023,
			LinkMaxLength = 1023
		};

		QString Title;
		QString Link;

		SBookmark(const char *Title0, const char *Link0)
			: Title(Title0), Link(Link0) { }
	};

	QStrList m_SavedChain;
	QStrList m_SavedTreeSelection;
	BOOL m_bSavedTreeItemSelected;
	QList<SBookmark> m_BookmarkList;

	BOOL	m_bRightClickedOnTree;
	BOOL m_bBrowserCopy;
};

#endif /* __INC_MAINFRM_H__ */
