/* Name: mainfrm.cpp

   Description: This file is a part of the Corel File Manager application.

   Author:	Oleg Noskov (olegn@corel.com)
   Modifications: Jasmin Blanchette, Milind Changuire

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

#include <iostream.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <qstring.h>
#include "common.h"
#include "array.h"
#include "smbutil.h"
#include "clientparser.h"
#include "qapplication.h"
#include <listview.h>
#include "qsplitter.h"
#include "msroot.h"
#include "nfsroot.h"
#include "printerroot.h"
#include "tips.h"
#include "mainfrm.h"
#include "rightpanel.h"
#include <flattoolbar.h>
#include <qcombobox.h>
#include "corellistboxitem.h"
#include <qlist.h>
#include <qlabel.h>
#include "lefttree.h"
#include <qpopupmenu.h>
#ifdef QT_20
#include <kmenubar.h>
#include <kglobal.h>
#include <kinstance.h>
#include <kaction.h>
#include <kdebug.h>
#include <krun.h>
#include <klocale.h>
#define  QMenuBar KMenuBar
#else
#include <qmenubar.h>
#endif
#include "explres.h"
#include "propres.h"
#include <qstatusbar.h>
#include <qpainter.h>
#include "corellogo.h"
#include "mycomputer.h"
#include "filesystem.h"
#include "PropDialog.h"
#include <qmessagebox.h>
#include "DisconnectDlg.h"
#include "ftpsite.h"
#include "ftpsession.h"

#ifdef QT_20
#include <kmimetype.h>
#include <qfile.h>
#include <ktrader.h>
#include <klibloader.h>
#include "qclipboard.h"
#define CCorelLineEdit QLineEdit
#include "qlineedit.h"
#else
#include "corelclipboard.h"
#include "coreldragobject.h"
#endif
#include "copymove.h"
#include "unistd.h"
#include "cuturldrag.h"
#include "qlineedit.h"
#include "qtooltip.h"
#include "webroot.h"
#include "webpage.h"
#include "home.h"
#include "kapp.h"

#if (QT_VERSION < 200)
#include "kbind.h"
#include "kfmprops.h"
#include "htmlview.h"
#else
#include "kglobal.h"
#include "kstddirs.h"
#include "khtml_part.h"
#define KHTMLView KHTMLPart
#endif
#include "kconfig.h"

#include <qbitmap.h>
#include <ctype.h> // for toupper()
#include <errno.h>
#include "qstrlist.h"
#include "kapp.h"
#include "X11/Xlib.h"
#include "X11/Xutil.h"
#include "inifile.h"
#include "vdrive.h"
#include "trashentry.h"
#include "device.h"
#include "automount.h"
#include "sys/sysmacros.h"
#include "header.h"
#include "history.h"
#include "expcommon.h"
#include "printer.h" // for PurgePrinter
#include "printjob.h"
#include <sys/param.h> // for MAXPATHLEN
#include <sys/types.h>
#include <dirent.h>

#define ksEXPLORER_ICON_LINK "http://product.corel.com/en/linux/redirects/explorer/corelcity.htm"

#define ksMENU_GO_LINUX "http://product.corel.com/en/linux/redirects/explorer/go/linux.htm"
#define ksMENU_GO_CORELCITY "http://product.corel.com/en/linux/redirects/explorer/go/corelcity.htm"

#define ksMENU_LINKS_CORELCITY "http://product.corel.com/en/linux/redirects/explorer/links/corelcity.htm"
#define ksMENU_LINKS_LINUX "http://product.corel.com/en/linux/redirects/explorer/links/linux.htm"
#define ksMENU_LINKS_DESIGNER "http://product.corel.com/en/linux/redirects/explorer/links/designer.htm"
#define ksMENU_LINKS_OFFICECOMMUNITY "http://product.corel.com/en/linux/redirects/explorer/links/officecommunity.htm"
#define ksMENU_LINKS_WEBSERVICES_CLIPART_AND_PHOTOS "http://product.corel.com/en/linux/redirects/explorer/links/services/clipartcity.htm"
#define ksMENU_LINKS_WEBSERVICES_EMAIL "http://product.corel.com/en/linux/redirects/explorer/links/services/email.htm"
#define ksMENU_LINKS_WEBSERVICES_FILE_HOSTING "http://product.corel.com/en/linux/redirects/explorer/links/services/vdrive.htm"
#define ksMENU_LINKS_WEBSERVICES_DESIGNER_FX "http://product.corel.com/en/linux/redirects/explorer/links/designerfx.htm"
#define ksMENU_LINKS_WEBSERVICES_PRINTING "http://product.corel.com/en/linux/redirects/explorer/links/virtualprinter.htm"
#define ksMENU_LINKS_WEBSERVICES_TEXT_FX "http://product.corel.com/en/linux/redirects/explorer/links/textfx.htm"
#define ksMENU_TOOLS_FIND_ON_INTERNET "http://product.corel.com/en/linux/redirects/explorer/tools/search.htm"

////////////////////////////////////////////////////////////////////////////

/* History stuff */

QList<CHistoryItem> gHistory;

int gnCanForward = 0;

int gnGoCount = 0;

extern BOOL gbShowTree; // in main.cpp
extern BOOL gbNeedSaveConfigSettings;

extern QString gsStartAddress;
int gnWebOperationsCount=0;
extern int gbUseBigIcons;
extern BOOL gbShowAddressBar;
extern BOOL gbShowToolBar;
extern BOOL gbShowStatusBar;
extern BOOL gbShowMyComputer;
int gnDesiredTreeWidth = -1;
bool bInsideMenu = false;

////////////////////////////////////////////////////////////////////////////

typedef QToolButton *LPToolButton;

////////////////////////////////////////////////////////////////////////////
/* Main window constructor.
   Does:
		- create and attach toolbar, populate is with buttons
		- create central widget (splitter)
		- create tree view (left panel), populate it
		  with root (zero-level) and any first-level items
		- create list view (right panel) and populate it with columns
*/

CMainFrame::CMainFrame(QApplication *pApp) : CMainWindow(0, 0, 0, true), m_bRightClickedOnTree(FALSE)
{
  //setInstance(KGlobal::instance());
	setXMLFile("CorelExplorer.rc");

	m_pManager = new KParts::PartManager(this);
	connect(m_pManager, SIGNAL(activePartChanged(KParts::Part*)), this, SLOT(createGUI(KParts::Part *)));

	m_bNoAdd = FALSE;
	m_bPartVisible = FALSE;

	setRightJustification(TRUE);

	pApp->setMainWidget(this);

	//((KApplication *)pApp)->setTopWidget(this); --- setTopWidget() removed because setCaption() doesn't work after it...

#if (QT_VERSION < 200)
  LoadConfigSettings(pKApp->isRestored() ?
    pKApp->getSessionConfig() : pKApp->getConfig());
#else
  LoadConfigSettings(KGlobal::config());
#endif

  connect(qApp, SIGNAL(saveYourself()), this, SLOT(OnSaveSession()));
	connect(pApp, SIGNAL(lastWindowClosed()), SLOT(OnLastWindowClosed()));

	setGeometry(-1000,-1000,5,5); //QApplication::desktop()->width()-1, QApplication::desktop()->height()-1,1,1);
	//show();

  if (-1 == gnDesiredAppWidth)
		gnDesiredAppWidth = (gnScreenWidth * 2) / 3;

  if (-1 == gnDesiredTreeWidth)
    gnDesiredTreeWidth = gnDesiredAppWidth / 3;

	if (-1 == gnDesiredAppHeight)
		gnDesiredAppHeight = (gnScreenHeight * 2) / 3;

	if (-1 == gnDesiredAppX)
		gnDesiredAppX = (gnScreenWidth - gnDesiredAppWidth)/2;

	if (-1 == gnDesiredAppY)
		gnDesiredAppY = (gnScreenHeight - gnDesiredAppHeight)/2;

	//show();
	//printf("%d %d %d %d\n", gnDesiredAppX, gnDesiredAppY,gnDesiredAppWidth,gnDesiredAppHeight);

	//move(QApplication::desktop()->width()-1, QApplication::desktop()->height()-1);


	//hide(); // this has a side-effect of calling OnLastWindowClosed(),

	//gbStopping = FALSE; // so we repait the damage here.

	ExpInitResources();
	PropInitResources();

	CreateMenus();
	ActivateActions();
	cout<<"before createGUI"<<endl;
	//createGUI( 0L );
	//OnSelectDeselect();
	CheckMenuItem(knMENU_VIEW_LARGE_ICONS, gbUseBigIcons);
	CheckMenuItem(knMENU_SHOW_HIDDEN_FILES, gbShowHiddenFiles);
	cout<<"before create StatusBar"<<endl;
	CreateStatusBar();

	CreateToolBar();
	cout<<"before create Views"<<endl;
  CreateViews();
	createGUI( 0L );
	cout<<"after create Views"<<endl;

#ifdef NEWCCQT
  m_pRightPanel->alternateRowColors(TRUE);
#endif

	CheckMenuItem(knMENU_VIEW_TOOLBARS, gbShowToolBar);
  cout<<"before address bar"<<endl;

	CreateAddressBar();
	CheckMenuItem(knMENU_VIEW_ADDRESS_BAR, gbShowAddressBar);

	/* ------------------------------------------ */

	CCorelLogo *pLogo = new CCorelLogo(m_pToolBar, 35, 34);
	pLogo->setFocusPolicy(QWidget::ClickFocus);

	cout<<"before connect"<<endl;
	connect(pLogo, SIGNAL(clicked()), SLOT(OnLogoClicked()));

	connect(&gTreeExpansionNotifier, SIGNAL(StartWorking()), pLogo, SLOT(UnPause()));
	connect(&gTreeExpansionNotifier, SIGNAL(EndWorking()), pLogo, SLOT(Pause()));
	connect(&gTreeExpansionNotifier, SIGNAL(ItemRenamed(CListViewItem*)), SLOT(OnItemRenamed(CListViewItem*)));
	connect(&gTreeExpansionNotifier, SIGNAL(ItemDestroyed(CListViewItem*)), SLOT(OnItemDestroyed(CListViewItem*)));
	connect(&gTreeExpansionNotifier, SIGNAL(StartRescanItem(CListViewItem*)), SLOT(OnStartRescanItem(CListViewItem*)));
	connect(&gTreeExpansionNotifier, SIGNAL(EndRescanItem(CListViewItem*)), SLOT(OnEndRescanItem(CListViewItem*)));
	connect(&gTreeExpansionNotifier, SIGNAL(SambaConfigurationChanged()), SLOT(OnSambaConfigurationChanged()));
	connect(&gTreeExpansionNotifier, SIGNAL(ErrorAccessingURL(const char*)), SLOT(OnErrorAccessingURL(const char*)));
  connect(&gTreeExpansionNotifier, SIGNAL(MountListChanged()), this, SLOT(UpdateButtons()));

	cout<<"after connect"<<endl;
	pLogo->show();

	m_BookmarkList.setAutoDelete(true);
	LoadBookmarks();
	//setView(m_pSplitter); //alexandrm
/*
  show();
  //qApp->processEvents();
	QTimer::singleShot(50, this, SLOT(SizeInit()));

<<<<< mainfrm.cpp
	m_pTreeView->resize(gnDesiredTreeWidth,m_pTreeView->height());

	//m_pTreeView->resize(width()/3,height());
=======
*/
	cout<<"before move"<<endl;
	move(gnDesiredAppX, gnDesiredAppY);
	cout<<"after move"<<endl;
	resize(gnDesiredAppWidth, gnDesiredAppHeight);
	cout<<"before SizeInit"<<endl;
	//SizeInit();
	cout<<"after SizeInit"<<endl;
  
	QValueList<int> list;
	list.append(gnDesiredTreeWidth);
	list.append(m_pSplitter->width() - gnDesiredTreeWidth - 6);
	cout<<"before splitter "<<endl;
	m_pSplitter->setSizes(list);
  
	cout<<"before global show"<<endl;
	//createGUI(0L);
	show();
//	OnSelectDeselect();

	//m_pAddressCombo->setFocus();
	cout<<"after global show"<<endl;
  /*
	m_pTreeView->setCurrentItem(m_pTreeView->firstChild());
	m_pTreeView->setSelected(m_pTreeView->firstChild(), TRUE);
  m_pTreeView->viewport()->setFocus();
  */
	m_pWebRoot = NULL;
	m_bBrowserCopy = FALSE;
}

////////////////////////////////////////////////////////////////////////////
/* Main window destructor */

CMainFrame::~CMainFrame()
{
	delete m_pAddressCombo;
	delete m_pTreeView;
	delete m_pRightPanel;
	delete m_pToolBar;
	delete m_pPart;

	delete m_pSplitter;

	if (NULL != m_pTreeTip)
		delete m_pTreeTip;

	delete m_pRightTip;

	delete []m_Buttons;

	/* buttons themselves are auto-deleted by the toolbar */

	gTasks.PrepareForExit();
}


void CMainFrame::OnSelectDeselect()
{
	cout<<"Inside of selectDeselect"<<endl;
	KActionCollection *coll = actionCollection();

	unplugActionList("select");
	m_Select.clear();
	
	KActionSeparator* separator = new KActionSeparator();
	m_Select.append(separator);
	KAction *action = new KAction(LoadString(knSELECT__ALL), 0, this, SLOT(OnSelectAll()), coll);
	m_Select.append(action);
	action = new KAction(LoadString(kn_INVERT_SELECTION), 0, this, SLOT(OnInvertSelection()), coll);
	m_Select.append(action);
	plugActionList("select",m_Select);
	cout<<"In the end of selectDeselect"<<endl;

	return;
	

}

void CMainFrame::PopulateSortMenu(QPopupMenu &SortMenu)
{
	KActionCollection *coll = actionCollection();
  LPCSTR Slots[] = {
    SLOT(OnSortColumn0()),
    SLOT(OnSortColumn1()),
    SLOT(OnSortColumn2()),
    SLOT(OnSortColumn3()),
    SLOT(OnSortColumn4()),
    SLOT(OnSortColumn5())
  };

  unplugActionList("sort");
  m_SortBy.clear();
  SortMenu.clear();
  CHeader *pHead = m_pRightPanel->header();



  for (int i=0; i < pHead->count(); i++)
  {
    QString s;// = LoadString(knSORT_BY_X) + pHead->label(i);
    QString tmp = LoadString(knSORT_BY_X);
    s.sprintf(tmp, (LPCSTR)pHead->label(i));
    int nID = SortMenu.insertItem(s, this, Slots[i]);

    KToggleAction *action = new KToggleAction(s, 0, this, Slots[i], coll);
    if (m_pRightPanel->sortColumn() == i)
    	action->setChecked(true);

    if (m_pRightPanel->sortColumn() == i)
      SortMenu.setItemChecked(nID, TRUE);

    m_SortBy.append( action );
  }

  plugActionList("sort", m_SortBy);
}

void CMainFrame::OnMenuHighlighted(int nID)
{
  if (-18 == nID) // File Menu
  { // Check is "Empty Trash" Item is there
    HandleDumpsterMenu();
  }

	kdDebug(1000)<<"nID = "<<nID<<endl;
  if (-35 == nID)
    PopulateSortMenu(*m_pSortMenu);
}

void CMainFrame::OnSortColumn0()
{
  m_pRightPanel->setSorting(0, 0 == m_pRightPanel->sortColumn() ? !m_pRightPanel->sortAscending() : true);
}

void CMainFrame::OnSortColumn1()
{
  m_pRightPanel->setSorting(1, 1 == m_pRightPanel->sortColumn() ? !m_pRightPanel->sortAscending() : true);
}

void CMainFrame::OnSortColumn2()
{
  m_pRightPanel->setSorting(2, 2 == m_pRightPanel->sortColumn() ? !m_pRightPanel->sortAscending() : true);
}

void CMainFrame::OnSortColumn3()
{
  m_pRightPanel->setSorting(3, 3 == m_pRightPanel->sortColumn() ? !m_pRightPanel->sortAscending() : true);
}

void CMainFrame::OnSortColumn4()
{
  m_pRightPanel->setSorting(4, 4 == m_pRightPanel->sortColumn() ? !m_pRightPanel->sortAscending() : true);
}

void CMainFrame::OnSortColumn5()
{
  m_pRightPanel->setSorting(5, 5 == m_pRightPanel->sortColumn() ? !m_pRightPanel->sortAscending() : true);
}

////////////////////////////////////////////////////////////////////////////
/* This function is called internally when we need to jump to some other
   item in our tree view */

void CMainFrame::GoItem(CListViewItem *pItem)
{
  gnGoCount++;

	if (pItem != NULL)
	{

		cout<<"before setSelected"<<endl;
    m_pTreeView->setSelected(pItem, TRUE);
		cout<<"after setSelected"<<endl;
		m_pTreeView->ensureItemVisible(pItem);
		m_pTreeView->viewport()->setFocus();
		OnTreeSelectionChanged(pItem);
    m_pRightPanel->ActivateEvent();
	}
}

////////////////////////////////////////////////////////////////////////////
/* This function reacts on "Up" toolbar button */

void CMainFrame::OnGoParent()
{
	BOOL bRightActive = (qApp->focusWidget()== m_pRightPanel);

	CListViewItem *pItem = m_pTreeView->currentItem();

	if (NULL != pItem)
  {
    GoItem(pItem->parent());
  }

	if (bRightActive)
		m_pRightPanel->viewport()->setFocus();
}

////////////////////////////////////////////////////////////////////////////
/* This code will enable/disable toolbar buttons
   appropriately, update history list and the combo box in the toolbar */

void CMainFrame::OnTreeSelectionChanged(CListViewItem* pCurrentItem)
{
  cout<<"(LPCSTR)pCurrentItem->text(0)="<<(LPCSTR)pCurrentItem->text(0)<<endl;
  BOOL bCanDo;

	// Update history

	if (m_bNoAdd)
	{
		m_bNoAdd = FALSE;
	}
	else
	{
		if (pCurrentItem != NULL)
		{
			QStrList *pChain = new QStrList;

			GetChainForItem(m_pTreeView, pCurrentItem, *pChain);
      

			if (!gHistory.count())
			{
				CHistoryItem* hItem = new CHistoryItem;
				hItem->setStrList(*pChain);
				gHistory.append(hItem);
			}
			else
			{
				if (!CompareChains(gHistory.at(0)->getStrList(), *pChain))
				{
					while (gnCanForward > 0)
					{
						gHistory.removeFirst();
						gnCanForward--;
					}

					CHistoryItem* hItem = new CHistoryItem;
					hItem->setStrList(*pChain);
					gHistory.insert(0, hItem);
				}
				else
					delete pChain;
			}
		}
	}

	// Left Tree related-only buttons to be enabled/disabled right there

	CheckMenuItem(knMENU_VIEW_TREE, gbShowTree);
	CheckMenuItem(knMENU_VIEW_STATUS_BAR, gbShowStatusBar);
	CheckMenuItem(knMENU_VIEW_TOOLBARS, gbShowToolBar);

	bCanDo = (pCurrentItem != NULL && pCurrentItem->parent() != NULL);

	m_Buttons[2]->setEnabled(bCanDo);
  m_ActionList.at(m_nMenuActions+2)->setEnabled(bCanDo);
	EnableMenuItem(knMENU_GO_UPONELEVEL, bCanDo);
	//m_paToolBarUpOneLevel->setEnabled((bool)bCanDo);

	gHistory.at(gnCanForward);

	CHistoryItem* hItem = gHistory.next();

  QStrList *pChain;
  if (hItem == NULL)
		pChain = NULL;
	else
		pChain = &hItem->getStrList();

	bCanDo = (pChain != NULL);

	m_Buttons[0]->setEnabled(bCanDo);
	m_ActionList.at(m_nMenuActions+0)->setEnabled(bCanDo);
	EnableMenuItem(knMENU_GO_BACK, bCanDo);
	//m_paToolBarBack->setEnabled((bool)bCanDo);

	if (bCanDo)
	{
		QString a;
		a.sprintf(LoadString(knBACK_TO_X), pChain->first() /*pI->text(0)*/);
		QToolTip::add(m_Buttons[0], a);
    
	}
	else
		QToolTip::remove(m_Buttons[0]);

	bCanDo = (gnCanForward > 0);

	m_Buttons[1]->setEnabled(bCanDo);
 	m_ActionList.at(m_nMenuActions+1)->setEnabled(bCanDo);
	EnableMenuItem(knMENU_GO_FORWARD, bCanDo);
	//m_paToolBarForward->setEnabled((bool)bCanDo);

  if (bCanDo)
  {
  	CHistoryItem* hItem = gHistory.at(gnCanForward-1);
    QStrList *pChain = &hItem->getStrList();

    QString a;
		a.sprintf(LoadString(knFORWARD_TO_X), pChain->first());
		QToolTip::add(m_Buttons[1], a);
  }
	else
    QToolTip::remove(m_Buttons[1]);

	/* Now update toolbar combo */

	QString s = pCurrentItem->text(0);
	QString ComboText;
  

	if (IS_NETWORKTREEITEM(pCurrentItem))
		ComboText = ((CNetworkTreeItem*)pCurrentItem)->FullName(FALSE);

	if (ComboText.isEmpty())
		ComboText = s;

  
	if (strnicmp(ComboText, "file:", 5) == 0)
		ComboText = ComboText.right(ComboText.length() - 5);

  
	m_pAddressCombo->setCurrentItem(0);
  m_pAddressCombo->setEditText(ComboText);

  QString TextUI(m_pAddressCombo->currentTextUI());

  
  QListBox *lb = m_pAddressCombo->listBox();

	/* Remove same item from the list box if it already was there */

	uint i;

	for (i=0; i < lb->count(); i++)
	{
		if (TextUI == lb->text(i))
		{
			lb->removeItem(i);
			break;
		}
	}

	lb->insertItem(
    new CListBoxItem(TextUI , *(pCurrentItem->pixmap(0)), (void*)(m_pAddressCombo->HiddenPrefix())), 0);

	CHistory::Instance()->SetVisited(m_pAddressCombo->currentText());
	m_pAddressCombo->SetPixmap(pCurrentItem->pixmap(0));

	/* Set zone label */

	if (CNetworkTreeItem::IsNetworkTreeItem(pCurrentItem))
	{
		switch (((CNetworkTreeItem *)pCurrentItem)->Kind())
		{
			case keFTPSiteItem:
			case keFTPFileItem:
			case keWebRootItem:
			case keWebPageItem:
				m_pZoneLabel->setText(LoadString(knINTERNET_ZONE));
				m_pZoneLabel->setPixmap(*LoadPixmap(keWorldIcon));
			break;

			default:
				m_pZoneLabel->setText(LoadString(knLOCAL_INTRANET_ZONE));
				m_pZoneLabel->setPixmap(*LoadPixmap(keIntranetIcon));
		}
	}
	else
		if (CNetworkTreeItem::IsMyComputerItem(pCurrentItem))
		{
			m_pZoneLabel->setText(LoadString(knMY_COMPUTER));
			m_pZoneLabel->setPixmap(*LoadPixmap(keMyComputerIcon));
		}
		else
		{
			m_pZoneLabel->setText(LoadString(knMY_LINUX));
			m_pZoneLabel->setPixmap(*LoadPixmap(keDesktopIcon));
		}

  if (m_pRightPanel->m_pExternalItem != pCurrentItem)
  {
    m_pRightPanel->SetExternalItem(pCurrentItem);
	  m_pRightPanel->ResetRefreshTimer(1);
  }
}



////////////////////////////////////////////////////////////////////////////

void CMainFrame::UpdateButtons()
{
	// Hack: Restore focus policies in case they were lost
	m_pRightPanel->setFocusPolicy(QWidget::StrongFocus);
	m_pTreeView->setFocusPolicy(QWidget::StrongFocus);

	///////////////////////////////////////////////////

	BOOL bCanDo;

  GetActiveItems();

	/* File menu.. */

  BOOL bIsTrash = HandleDumpsterMenu();
  m_MainMenu[0].m_pMenu->setItemEnabled(knMENU_FILE_RESTORE, CanDoOperation(&CMainFrame::CanRestoreFile));

	bCanDo = !bIsTrash && CanDoOperation(&CMainFrame::CanDoNewFolder);
	EnableMenuItem(knMENU_FILE_NEW, bCanDo);

	/* Properties */
	bCanDo = CanDoOperation(&CMainFrame::CanDoProperties);

	EnableMenuItem(knMENU_FILE_PROPERTIES, bCanDo);
	//m_paToolBarProperties->setEnabled((bool)bCanDo);
	m_ActionList.at(m_nMenuActions + 7)->setEnabled(bCanDo);
	m_Buttons[9]->setEnabled(bCanDo);

	BOOL bCanCopy = FALSE;
	BOOL bCanDelete = FALSE;
	BOOL bCanTrash = FALSE;

	//m_bBrowserCopy = m_pRightPanel->IsWebBrowserWithFocus();
	if (m_bBrowserCopy)
	{
		bCanCopy = m_pRightPanel->m_bTextSelected;
	}
	else
	{
		bCanCopy = CanDoOperation(&CMainFrame::CanDoCopy);
		bCanDelete = !bIsTrash && CanDoOperation(&CMainFrame::CanDelete);

		if (bCanDelete)
			bCanTrash = CanDoOperation(&CMainFrame::CanMoveToTrash);
	}

	/* Cut */

	bCanDo = bCanCopy && bCanDelete;

	EnableMenuItem(knMENU_EDIT_CUT, bCanDo);
	//m_paToolBarCut->setEnabled((bool)bCanDo);
	m_ActionList.at(m_nMenuActions+3)->setEnabled(bCanDo);
	m_Buttons[4]->setEnabled(bCanDo);

	/* Copy */

	EnableMenuItem(knMENU_EDIT_COPY, bCanCopy);
	//m_paToolBarCopy->setEnabled((bool)bCanDo);
	m_ActionList.at(m_nMenuActions+4)->setEnabled(bCanCopy);
	m_Buttons[5]->setEnabled(bCanCopy);

	/* Paste */

	bCanDo = CanPaste() && CanPasteHere();
	EnableMenuItem(knMENU_EDIT_PASTE, bCanDo);
	//m_paToolBarPaste->setEnabled((bool)bCanDo);
	m_ActionList.at(m_nMenuActions+5)->setEnabled(bCanDo);
	m_Buttons[6]->setEnabled(bCanDo);

	/* Undo */

	/* Delete */

	EnableMenuItem(knMENU_FILE_DELETE, bCanTrash);
	//m_paToolBarMoveToTrash->setEnabled((bool)bCanTrash);

	EnableMenuItem(knMENU_FILE_NUKE, bCanDelete);
	m_ActionList.at(m_nMenuActions+6)->setEnabled(bCanDelete);
	m_Buttons[8]->setEnabled(bCanDelete);

	EnableMenuItem(knMENU_FILE_RENAME,
                 !bIsTrash &&
                 1 == ActiveItems.count() &&
                 CanDoOperation(&CMainFrame::CanRename));

	/* Disconnect share... */
	EnableMenuItem(knMENU_DISCONNECT_SHARE, CanDisconnectShare());

	/* Select All, Invert Selection */

	bCanDo = TRUE; //m_pRightPanel->m_bHasFocus;

	//EnableMenuItem(knMENU_EDIT_SELECT_ALL, bCanDo);
	//EnableMenuItem(knMENU_EDIT_INVERT_SELECTION, bCanDo);
  CheckMenuItem(knMENU_VIEW_MYCOMPUTER, gbShowMyComputer);
  EnableMenuItem(knMENU_GO_MYCOMPUTER, gbShowMyComputer);
}

////////////////////////////////////////////////////////////////////////////
/* These two functions react to the "Back" and "Forward" toolbar buttons */

void CMainFrame::OnBack()
{
	m_bNoAdd = TRUE;
	CHistoryItem* hItem = gHistory.at(++gnCanForward);

	//kdDebug(1000)<<"item="<<((CNetworkTreeItem*)hItem->getListViewItem())->FullName(FALSE)<<endl;
	if (hItem->getPart() != 0L)
	{
		if (m_bPartVisible == FALSE)
    	{
      	m_pRightPanel->hide();
				ActivatePart(hItem);
				UpdateComboAndToolBar(hItem->getListViewItem());
				return;

      }
		else
    	{
      	ActivatePart(hItem);
				UpdateComboAndToolBar(hItem->getListViewItem());
				return;
      }
     }
	else
	{
		if (m_bPartVisible == TRUE)
		{
			CHistoryItem* hPrevItem = gHistory.at(gnCanForward-1);
			DeactivatePart(hPrevItem);
			//return;
    }
		GoItem(GetItemFromChain(m_pTreeView, hItem->getStrList()));
	}
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnForward()
{
	m_bNoAdd = TRUE;
	CHistoryItem* hItem = gHistory.at(--gnCanForward);
	
	if (hItem->getPart() != 0L)
	{
		if (m_bPartVisible == FALSE)
		{
			m_pRightPanel->hide();
			ActivatePart(hItem);
			UpdateComboAndToolBar(hItem->getListViewItem());
			return;
		}
		else
		{
			ActivatePart(hItem);
			UpdateComboAndToolBar(hItem->getListViewItem());
			return;
		}
	}
	else
	{
		if (m_bPartVisible == TRUE)
		{
			CHistoryItem* hPrevItem = gHistory.at(gnCanForward+1);
			DeactivatePart(hPrevItem);
			//return;
		}
		GoItem(GetItemFromChain(m_pTreeView, hItem->getStrList()));
  }
}

////////////////////////////////////////////////////////////////////////////
/*
	This function is called when user double-clicks on the right panel item.
	Another situation when this function is called is when user hits a hyperlink
	in the browser view.
*/

void CMainFrame::OnRightChdir(CListViewItem* pExternalItem, QString Destination)
{
  if (!strncmp((LPCSTR)Destination
#ifdef QT_20
  .latin1()
#endif
  , "file:", 5))
	{
		GoItem((LPCSTR) Destination
#ifdef QT_20
  .latin1()
#endif
+ 5);
		return;
	}

	// Netscape 4.[567] is completely buggy. If it's already open, it does the
	// Right Thing; if it's not, it crashes on startup.
#if 0
	if (!strncmp(Destination, "mailto:", 7) && NULL == strchr(Destination, '\''))
	{
		QString command = "netscape '";

		command += Destination;
		command += "' &";
		system(command);
		return;
	}
#endif

	if (pExternalItem == NULL)
		return;

	if (CNetworkTreeItem::IsNetworkTreeItem(pExternalItem) &&
			((CNetworkTreeItem*)pExternalItem)->Kind() == keWebPageItem)
	{
		GoItem((LPCSTR)Destination
#ifdef QT_20
  .latin1()
#endif
    );
		return;
	}

	CListViewItem *child = pExternalItem->firstChild();

	while (child != NULL)
	{
		if (Destination == child->text(0))
			break;

		child = child->nextSibling();
	}

	if (child != NULL)
	{
		GoItem(child);
		do
		{
      m_pTreeView->setOpen(child, TRUE);
			child = child->parent();
		}
		while (child != NULL);
	}

	m_pRightPanel->ActivateEvent();
	m_pRightPanel->viewport()->setFocus();
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::GetActiveItems()
{
	ActiveItems.clear();

	// Get any selection from the right panel if right panel has focus now...

	if (m_pRightPanel->m_bHasFocus || bInsideMenu)
	{
		CListViewItem *pI;

		for (pI = m_pRightPanel->firstChild(); pI != NULL; pI = pI->nextSibling())
		{
			if (m_pRightPanel->isSelected(pI) && IS_NETWORKTREEITEM(pI))
			{
				ActiveItems.append((CNetworkTreeItem*)pI);
			}
		}

		if (!ActiveItems.count() && IS_NETWORKTREEITEM(m_pRightPanel->m_pExternalItem))
			ActiveItems.append((CNetworkTreeItem*)m_pRightPanel->m_pExternalItem);
	}
	else
	{
		// Get the active item in the tree control if the tree has focus now...

		CListViewItem *pI = m_pTreeView->currentItem();

		if (pI != NULL && IS_NETWORKTREEITEM(pI))
		{
			ActiveItems.append((CNetworkTreeItem*)pI);
		}
	}
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnRightPopupMenuRequest(const QPoint& p)
{
	CListViewItem *pItem = m_pRightPanel->GetItemAt(p);

	if (NULL == pItem)
	{
    ActiveItems.clear();

		CListViewItem *pI = m_pRightPanel->m_pExternalItem;

    if (NULL == pI)
      return; // oops...

		CNetworkTreeItem *pItem = IS_NETWORKTREEITEM(pI) ? (CNetworkTreeItem*)pI : NULL;

		if (NULL != pItem)
			ActiveItems.append(pItem);

		BOOL bCanPaste = FALSE;

		QPopupMenu Menu;
    int nID;

		BOOL bCanCreateSubfolder = FALSE;
    // "Empty dumpster" menu item - only for Trash folders

    if (NULL != pItem)
    {
      if (IsTrashFolder(pItem->FullName(FALSE)))
      {
        nID = Menu.insertItem(LoadString(knEMPTY_DUMPSTER), this, SLOT(OnEmptyDumpster()));

        Menu.setItemEnabled(nID,
                            NULL != m_pRightPanel->firstChild() &&
                            IsMyTrashFolder(pItem->FullName(FALSE)) &&
                            !IsTrashEmpty());
        Menu.insertSeparator();
      }
      else
        bCanCreateSubfolder = pItem->CanCreateSubfolder();
    }

		nID = Menu.insertItem(LoadString(knSHOW_LARGE_ICONS), this, SLOT(OnIconView()));
		Menu.setItemChecked(nID, m_pRightPanel->iconView());

		nID = Menu.insertItem(LoadString(knSHOW_HIDDEN_FILES), this, SLOT(OnShowHiddenFiles()));
    Menu.setItemChecked(nID, gbShowHiddenFiles);

		Menu.insertSeparator();

    QPopupMenu SortMenu;
    PopulateSortMenu(SortMenu);
    Menu.insertItem(LoadString(knSORT_MENU), &SortMenu);
    Menu.insertSeparator();

		BOOL bCanDoProperties = FALSE;

		if (NULL != pItem)
		{
			bCanDoProperties = CanDoProperties(pItem);

			if (CanPaste())
			{
				if (pItem->Kind() == keFileItem ||
						pItem->Kind() == keFTPFileItem ||
						pItem->Kind() == keShareItem)
					bCanPaste = TRUE;
				else
				{
					if (pItem->Kind() == keFileSystemItem ||
							pItem->Kind() == keLocalFileItem)
					{
						bCanPaste = IsSuperUser() ? TRUE : !access((LPCSTR)pItem->FullName(FALSE)
#ifdef QT_20
  .latin1()
#endif
            , W_OK);
					}
				}
			}
		}

		nID = Menu.insertItem(LoadString(kn_PASTE), this, SLOT(FilePaste()));
		Menu.setItemEnabled(nID, bCanPaste);

		Menu.insertSeparator();

    nID = Menu.insertItem(LoadString(knNEW_FOLDER_MENU), this, SLOT(OnFileNewFolder()));
		Menu.setItemEnabled(nID, bCanCreateSubfolder);

		if (keLocalFileItem == pItem->Kind() && ((CLocalFileItem*)pItem)->IsFolder())
			nID = Menu.insertItem(LoadString(kn_OPEN_CONSOLE_WINDOW), this, SLOT(OnOpenConsole()));

		Menu.insertSeparator();

		nID = Menu.insertItem(LoadString(knP_ROPERTIES), this, SLOT(OnProperties()));
		Menu.setItemEnabled(nID, bCanDoProperties);

		Menu.exec(p);
	}
	else
	{
		GetActiveItems();
		OnRightClicked(p);
	}
}

////////////////////////////////////////////////////////////////////////////

/* This function is called when user selects
   something from the toolbar combo box */

void CMainFrame::OnSelchangeCombo(int nIndex)
{
	if (nIndex > 0) /* an item at index 0 is current one, no need to jump */
	{
		QListBox *lb = m_pAddressCombo->listBox();

		QListBoxItem *item = lb->item(nIndex);

		CListBoxItem *lbitem = (CListBoxItem*)item;

		if (lbitem != NULL)
		{
			QString Target = AttachHiddenPrefix(lbitem->text(), (int)lbitem->GetUserData());

			lb->removeItem(nIndex);
      GoItem(Target);
      //m_pAddressCombo->setEditText(m_pAddressCombo->text(m_pAddressCombo->currentItem()));
			m_pAddressCombo->setFocus();

			CCorelLineEdit *pComboEdit = (CCorelLineEdit*)GetComboEdit(m_pAddressCombo);

			if (NULL != pComboEdit)
				pComboEdit->selectAll();
		}
	}
}

////////////////////////////////////////////////////////////////////////////
/* This function is called when user presses "Enter" in toolbar combo box */

void CMainFrame::ComboReturnPressed()
{
  QString TargetAddress = m_pAddressCombo->currentTextUI();

	
	GoItem(TargetAddress);
	m_pAddressCombo->setFocus();

	CCorelLineEdit *pComboEdit = (CCorelLineEdit*)GetComboEdit(m_pAddressCombo);

	if (NULL != pComboEdit)
		pComboEdit->selectAll();

  CHistory::Instance()->SetVisited(TargetAddress);
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::ActivateActions()
{
	KActionCollection *coll = actionCollection();
  KAction * action; //= new KAction;
  KToggleAction * toggleAction;

	for (int i=0; i < gMainWindowMenuSize; i++)
	{
		if (i<6 && i>0) 
		{
			toggleAction = new KToggleAction(LoadString(gMainWindowMenu[i].m_nItemLabelID), 0, this, gMainWindowMenu[i].m_pSlot, coll, gMainWindowActions[i]);
			toggleAction->setChecked(true);
			m_ActionList.append(toggleAction);
		}
		else
		{
			action = new KAction(LoadString(gMainWindowMenu[i].m_nItemLabelID), 0, this, gMainWindowMenu[i].m_pSlot, coll, gMainWindowActions[i]);
			if (i<9 && i>5)
      	action->setEnabled(false);
			m_ActionList.append(action);
		}
	}

	m_nMenuActions = m_ActionList.count();



	for (i=0; i < gMainWindowButtonInfoSize; i++)
	{
		action = new KAction(LoadString(gMainWindowButtonInfo[i].m_nTextID), 0, this, gMainWindowButtonInfo[i].m_pSignal, coll, gMainWindowToolBarActionsNames[i]);
		action->setEnabled(false);
		action->setIconSet(QIconSet(**gMainWindowButtonInfo[i].m_ppIcon,QIconSet::Large));
		m_ActionList.append(action);
	}


	
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::UpdateComboAndToolBar(CListViewItem *pCurrentItem)
{
	BOOL	bCanDo = (pCurrentItem != NULL && pCurrentItem->parent() != NULL);

	m_Buttons[2]->setEnabled(bCanDo);
  m_ActionList.at(m_nMenuActions+2)->setEnabled(bCanDo);
	EnableMenuItem(knMENU_GO_UPONELEVEL, bCanDo);
	//m_paToolBarUpOneLevel->setEnabled((bool)bCanDo);

	gHistory.at(gnCanForward);

	CHistoryItem* hItem = gHistory.next();

  QStrList *pChain;
  if (hItem == NULL)
		pChain = NULL;
	else
		pChain = &hItem->getStrList();

	bCanDo = (pChain != NULL);

	m_Buttons[0]->setEnabled(bCanDo);
	m_ActionList.at(m_nMenuActions+0)->setEnabled(bCanDo);
	EnableMenuItem(knMENU_GO_BACK, bCanDo);
	//m_paToolBarBack->setEnabled((bool)bCanDo);

	if (bCanDo)
	{
		QString a;
		a.sprintf(LoadString(knBACK_TO_X), pChain->first() /*pI->text(0)*/);
		QToolTip::add(m_Buttons[0], a);
	}
	else
		QToolTip::remove(m_Buttons[0]);

	bCanDo = (gnCanForward > 0);

	m_Buttons[1]->setEnabled(bCanDo);
 	m_ActionList.at(m_nMenuActions+1)->setEnabled(bCanDo);
	EnableMenuItem(knMENU_GO_FORWARD, bCanDo);	

	if (bCanDo)
  {
  	CHistoryItem* hItem = gHistory.at(gnCanForward-1);
    QStrList *pChain = &hItem->getStrList();

    QString a;
		a.sprintf(LoadString(knFORWARD_TO_X), pChain->first());
		QToolTip::add(m_Buttons[1], a);
  }
	else
    QToolTip::remove(m_Buttons[1]);

	QString s = pCurrentItem->text(0);
	QString ComboText;

	if (IS_NETWORKTREEITEM(pCurrentItem))
		ComboText = ((CNetworkTreeItem*)pCurrentItem)->FullName(FALSE);

	if (ComboText.isEmpty())
		ComboText = s;

	if (strnicmp(ComboText, "file:", 5) == 0)
		ComboText = ComboText.right(ComboText.length() - 5);

	m_pAddressCombo->setCurrentItem(0);
	m_pAddressCombo->setEditText(ComboText);

	QString TextUI(m_pAddressCombo->currentTextUI());

	QListBox *lb = m_pAddressCombo->listBox();

	/* Remove same item from the list box if it already was there */

	uint i;

	for (i=0; i < lb->count(); i++)
	{
		if (TextUI == lb->text(i))
		{
			lb->removeItem(i);
			break;
		}
	}

	lb->insertItem(
    new CListBoxItem(TextUI , *(pCurrentItem->pixmap(0)), (void*)(m_pAddressCombo->HiddenPrefix())), 0);

	CHistory::Instance()->SetVisited(m_pAddressCombo->currentText());
	m_pAddressCombo->SetPixmap(pCurrentItem->pixmap(0));

	return;
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnViewMenuBar()
{
  if (menuBar()->isVisible())
    menuBar()->hide();
  else
    menuBar()->show();
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::slotShowToolBar()
{
  toggleBar( "mainToolBar", "KToolBar" );
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::toggleBar( const char *name, const char *className )
{
  KToolBar *bar = static_cast<KToolBar *>( child( name, className ) );
  if ( !bar )
    return;
  if ( bar->isVisible() )
    bar->hide();
  else
    bar->show();
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::GoItem(LPCSTR TargetAddress)
{
  if (NULL == TargetAddress) // safety check
		return;


	int nGoCount = gnGoCount;

	CListViewItem *pTarget = NULL;

	QString AlteredTargetAddress;

	if (strlen(TargetAddress) > strlen(GetHiddenPrefix(knPRINTER_HIDDEN_PREFIX)) &&
      !strnicmp(TargetAddress, GetHiddenPrefix(knPRINTER_HIDDEN_PREFIX), strlen(GetHiddenPrefix(knPRINTER_HIDDEN_PREFIX))))
	{
		pTarget = m_pPrinterRoot->FindAndExpand(TargetAddress);
	}
	else
	if (strlen(TargetAddress) > 6 && !strnicmp(TargetAddress, "ftp:", 4))
	{
		pTarget = gFtpSessions.FindAndExpand(m_pTreeView->firstChild(), TargetAddress);
	}
	else
	if (strlen(TargetAddress) > 6 && !strnicmp(TargetAddress, "nfs:", 4))
	{
		pTarget = m_pNFSRoot->FindAndExpand(TargetAddress);
	}
	else
	{
		if (strlen(TargetAddress) > 6 && !strnicmp(TargetAddress, "http:", 5))
		{

HandleURL:;

			if (NULL == m_pWebRoot)
			{
				m_pWebRoot = new CWebRootItem(((CListViewItem*)m_pHomeRoot)->parent());
			}

			if (!m_pWebRoot->isOpen())
				m_pWebRoot->setOpen(TRUE);

			// search through the list of already recorded URLs

			for (pTarget = m_pWebRoot->firstChild(); NULL != pTarget; pTarget=pTarget->nextSibling())
			{
				if (((CNetworkTreeItem*)pTarget)->FullName(FALSE) == TargetAddress)
					break;
			}

			if (NULL == pTarget)
			{
				pTarget = new CWebPageItem(m_pWebRoot, TargetAddress);
			}
		}
		else
		{
			if (TargetAddress[0] == '/' && !IsUNCPath(TargetAddress)) // Local path
			{
				if (!strcmp((LPCSTR)m_pHomeRoot->FullName(FALSE)
#ifdef QT_20
  .latin1()
#endif
        , TargetAddress))
				{
					pTarget = m_pHomeRoot;

					if (!pTarget->isOpen())
          {
            pTarget->setOpen(TRUE);

          }
				}
				else
        {
          if (!gbShowMyComputer)
            OnViewMyComputer();

					/* The truncated address is the address without the #anchor part. */
					QString TruncatedTargetAddress = TargetAddress;
					const char *p0 = (LPCSTR)TruncatedTargetAddress
#ifdef QT_20
  .latin1()
#endif
          ;
					const char *p = strrchr(p0, '#');
					if (p != NULL)
						TruncatedTargetAddress.truncate((int) (p - p0));

					pTarget = m_pMyComputer->FindAndExpand((LPCSTR)TruncatedTargetAddress
#ifdef QT_20
  .latin1()
#endif
          );
					if (TruncatedTargetAddress != ((CNetworkTreeItem*)pTarget)->FullName(FALSE))
					{
						pTarget = m_pMyComputer->FindItemByPath((LPCSTR)TruncatedTargetAddress);            
					}

					struct stat st;
					int nLen = strlen((LPCSTR)TruncatedTargetAddress
#ifdef QT_20
  .latin1()
#endif
          );

					if (nLen >= 5
							&& (!stricmp((LPCSTR) TruncatedTargetAddress
#ifdef QT_20
  .latin1()
#endif
               + nLen - 5, ".html")
									|| !stricmp((LPCSTR) TruncatedTargetAddress
#ifdef QT_20
  .latin1()
#endif
                   + nLen - 4, ".htm")
									|| !stricmp((LPCSTR) TruncatedTargetAddress
#ifdef QT_20
  .latin1()
#endif
                   + nLen - 4, ".gif")
									|| !stricmp((LPCSTR) TruncatedTargetAddress
#ifdef QT_20
  .latin1()
#endif
                   + nLen - 5, ".jpeg")
									|| !stricmp((LPCSTR) TruncatedTargetAddress
#ifdef QT_20
  .latin1()
#endif
+ nLen - 4, ".jpg")))
					{
						if (!stat((LPCSTR)TruncatedTargetAddress
#ifdef QT_20
  .latin1()
#endif
            , &st) && S_ISREG(st.st_mode))
						{
							/* Add the prefix 'file:' and treat is as just any other URL. */
							AlteredTargetAddress = "file:";
							AlteredTargetAddress += TargetAddress;
							TargetAddress = (LPCSTR) AlteredTargetAddress
#ifdef QT_20
  .latin1()
#endif
              ;
							goto HandleURL;
						}
						else
						{
							QString message;
							message.sprintf(LoadString(knCANNOT_OPEN_FILE_X),
									(LPCSTR) TruncatedTargetAddress
#ifdef QT_20
  .latin1()
#endif
                  , strerror(errno));
							QMessageBox::warning(qApp->mainWidget(), LoadString(knAPP_TITLE), message);
							pTarget = NULL;
						}
					}
        }
			}
			else
			{
        if ((strlen(TargetAddress) >= strlen(GetHiddenPrefix(knWORKGROUPS_HIDDEN_PREFIX)) &&
            !strnicmp(TargetAddress, GetHiddenPrefix(knWORKGROUPS_HIDDEN_PREFIX), strlen(GetHiddenPrefix(knWORKGROUP_HIDDEN_PREFIX)))))
          pTarget = m_pNetworkRoot;
        else
        if ((strlen(TargetAddress) >= strlen(GetHiddenPrefix(knNFSSERVERS_HIDDEN_PREFIX)) &&
            !strnicmp(TargetAddress, GetHiddenPrefix(knNFSSERVERS_HIDDEN_PREFIX), strlen(GetHiddenPrefix(knNFSSERVERS_HIDDEN_PREFIX)))))
          pTarget = m_pNFSRoot;
        else
        if ((strlen(TargetAddress) >= strlen(GetHiddenPrefix(knPRINTERS_HIDDEN_PREFIX)) &&
            !strnicmp(TargetAddress, GetHiddenPrefix(knPRINTERS_HIDDEN_PREFIX), strlen(GetHiddenPrefix(knPRINTERS_HIDDEN_PREFIX)))))
          pTarget = m_pPrinterRoot;
        else

        if (IsUNCPath(TargetAddress) || (strlen(TargetAddress) > strlen(GetHiddenPrefix(knWORKGROUP_HIDDEN_PREFIX)) &&
                                         !strnicmp(TargetAddress, GetHiddenPrefix(knWORKGROUP_HIDDEN_PREFIX), strlen(GetHiddenPrefix(knWORKGROUP_HIDDEN_PREFIX)))))
				{
					if (gbNetworkAvailable)
          {
            pTarget = m_pNetworkRoot->FindAndExpand(TargetAddress);
          }
				}
				else
				{
					CListViewItem *pChild = m_pTreeView->firstChild(); // that's desktop

					if (!stricmp((LPCSTR)pChild->text(0)
#ifdef QT_20
  .latin1()
#endif
          , TargetAddress))
						pTarget = pChild;
					else
					{
						for (pChild = pChild->firstChild(); pChild != NULL; pChild = pChild->nextSibling())
						{
							if (!stricmp((LPCSTR)pChild->text(0)
#ifdef QT_20
  .latin1()
#endif
              , TargetAddress))
							{
								pTarget = pChild;
								break;
							}
						}

						if (NULL == pTarget)
						{
							static QString UrlPrefixed;

							UrlPrefixed.sprintf("http://%s", TargetAddress);
							TargetAddress = (LPCSTR)UrlPrefixed
#ifdef QT_20
  .latin1()
#endif
              ;
							goto HandleURL;
						}
					}
				}
		  }
		}
	}

	if (NULL != pTarget)
	{
		CNetworkTreeItem* pI = (CNetworkTreeItem*)pTarget;
		CSMBFileInfo *pFile = (keLocalFileItem == pI->Kind()) ?
													((CSMBFileInfo *)(CLocalFileItem *)pI) :
													((CSMBFileInfo *)(CFileItem *)pI);
						
		if(!pFile->IsFolder())
		{
			OnPartRequest(pTarget);
			return;
		}
	}

	if (pTarget != NULL && nGoCount == gnGoCount)
		GoItem(pTarget);
	
}

////////////////////////////////////////////////////////////////////////////

/* This function is called in response to the "Stop" toolbar button */

void CMainFrame::OnStop()
{
	if (gnWebOperationsCount > 0)
		m_pRightPanel->OnStop();
	else
	{
		if (gnActiveTaskCount > 0 && !gbStopping)
		{
			gbStopping = TRUE;
			m_pRightPanel->OnStop();
		}
	}
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::CreateToolBar()
{
	m_pToolBar = new CFlatToolBar("ButtonBar", /*this, this,// alexandrm*/ this, NULL, FALSE);

  if (gbShowToolBar)
  {
	  addToolBar(m_pToolBar,"ButtonBar",Top,TRUE);
  }
	else
		m_pToolBar->move(-1000,-1000);

  /* Add some buttons to the toolbar */

  m_Buttons = new LPToolButton[gMainWindowButtonInfoSize];
	int i;

	//m_pToolBar->addSeparator();

	for (i=0; i < gMainWindowButtonInfoSize; i++)
	{
		if (gMainWindowButtonInfo[i].m_ppIcon == NULL)
				m_pToolBar->addSeparator();
		else
		{
			QIconSet is(**gMainWindowButtonInfo[i].m_ppIcon, QIconSet::Large);
			is.setPixmap(**gMainWindowButtonInfo[i].m_ppActiveIcon, QIconSet::Large, QIconSet::Active);

			if (gMainWindowButtonInfo[i].m_ppDisabledIcon != NULL)
				is.setPixmap(**gMainWindowButtonInfo[i].m_ppDisabledIcon, QIconSet::Large, QIconSet::Disabled);

			QToolButton *tb = new QToolButton(is, LoadString(gMainWindowButtonInfo[i].m_nTextID), LoadString(gMainWindowButtonInfo[i].m_nTextID), this, gMainWindowButtonInfo[i].m_pSignal, m_pToolBar);

			tb->setUsesBigPixmap(TRUE);
			//tb->setUsesTextLabel(TRUE);

			m_Buttons[i] = tb;
		}
	}

	/* Add empty widget to the toolbar and make it "stretchable".
	   This ensures the toolbar will fill all the horizontal span and
	   the buttons will not move - just like in Windows. */

	m_pToolBar->setStretchableWidget(new QWidget(m_pToolBar));
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::CreateAddressBar()
{
	m_pAddressBar = new CFlatToolBar("AddressBar", this, this, TRUE);

	if (gbShowAddressBar)
		addToolBar(m_pAddressBar, "AddressBar", Top, TRUE);
	else
		m_pAddressBar->move(-1000,-1000);

	QLabel *pLabel = new QLabel(LoadString(knADDRESS), m_pAddressBar);

	pLabel->setMargin(5);
	pLabel->setFixedWidth(pLabel->sizeHint().width()); /*pLabel->width());*/

	m_pAddressCombo = new CAutoTopCombo(TRUE, m_pAddressBar, "address");

	m_pAddressCombo->setInsertionPolicy(CTopCombo::NoInsertion);
	m_pAddressCombo->setAutoCompletion(TRUE);
	m_pAddressCombo->setFixedHeight(22);
	m_pAddressCombo->setMinimumWidth(200);

	if (gbNetworkAvailable)
		m_pAddressCombo->listBox()->insertItem(new CListBoxItem((LPCSTR)m_pNetworkRoot->text(0)
#ifdef QT_20
  .latin1()
#endif
    , *(m_pNetworkRoot->pixmap(0)), (void*)-1), 0);
	else
	  if (NULL != m_pMyComputer)
	    m_pAddressCombo->listBox()->insertItem(new CListBoxItem((LPCSTR)m_pMyComputer->text(0)
#ifdef QT_20
  .latin1()
#endif
      , *(m_pMyComputer->pixmap(0)), (void*)-1), 0);

	m_pAddressCombo->setCurrentItem(0);
	m_pAddressCombo->setEditText(m_pAddressCombo->text(m_pAddressCombo->currentItem()));

	QObject *pComboEdit = GetComboEdit(m_pAddressCombo);

	if (pComboEdit != NULL)
	  connect(pComboEdit, SIGNAL(returnPressed()), this, SLOT(ComboReturnPressed()));

	connect(m_pAddressCombo, SIGNAL(activated(int)), this, SLOT(OnSelchangeCombo(int)));
	connect(m_pAddressCombo, SIGNAL(TabRequest(BOOL)), SLOT(OnTabRequest(BOOL)));

	/* Adjust address bar so it has Windows look and feel */

	m_pAddressBar->setMaximumHeight(m_pAddressCombo->height()+12);
	m_pAddressBar->setStretchableWidget(m_pAddressCombo);
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::CreateStatusBar()
{
	/* Initialize status bar */

	m_pStatusBar = new QStatusBar(this);

	m_pObjectsLabel = new QLabel(m_pStatusBar);
	m_pObjectsLabel->setFrameStyle(QFrame::NoFrame);// | QFrame::Sunken);
	m_pObjectsLabel->setFixedHeight(20);
	m_pObjectsLabel->setMargin(5);
	m_pObjectsLabel->setText(LoadString(kn0_OBJECTS));

	m_pSizeLabel = new QLabel(m_pStatusBar);
	m_pSizeLabel->setFrameStyle(QFrame::NoFrame); //Panel | QFrame::Sunken);
	m_pSizeLabel->setFixedHeight(20);
	m_pSizeLabel->setMargin(5);
	m_pSizeLabel->setText("");

	m_pZoneLabel = new CIconText(m_pStatusBar);
	m_pZoneLabel->setFrameStyle(QFrame::NoFrame); //Panel | QFrame::Sunken);
	m_pZoneLabel->setFixedHeight(20);
	m_pZoneLabel->setMargin(2);

	m_pZoneLabel->setText(LoadString(knLOCAL_INTRANET_ZONE));
	m_pZoneLabel->setMaximumHeight(20);
	m_pZoneLabel->setPixmap(*LoadPixmap(keIntranetIcon));

//	m_pStatusBar->setMinimumHeight(35);
	m_pStatusBar->setFixedHeight(24);

	m_pStatusBar->addWidget(m_pObjectsLabel,2000);
	m_pStatusBar->addWidget(m_pSizeLabel,2000);
	m_pStatusBar->addWidget(m_pZoneLabel,2000);

  m_pZoneLabel->move(m_pZoneLabel->x(), 1);
  if (!gbShowStatusBar)
    m_pStatusBar->hide();
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::ActivatePart(CHistoryItem* hItem)
{
	unplugActionList("sort");
	unplugActionList("select");
	m_pPart = hItem->getPart();
	m_PartServices = hItem->getPartServices();
	cout<<"before createGUI"<<endl;

	if (0 == m_pSplitter->child(m_pPart->name()))	
  	m_pSplitter->insertChild(m_pPart);
	m_pSplitter->removeChild(m_pRightPanel);
	createGUI(m_pPart);
	m_pPart->widget()->resize(this->width() - m_pTreeView->width() - m_pRightPanel->width(),m_pRightPanel->height());
	cout<<"after createGUI"<<endl;
	cout<<"((CNetworkTreeItem*)hItem->getListViewItem())"<<(hItem->getListViewItem())<<endl;
	m_pPart->openURL(KURL(((CNetworkTreeItem*)(hItem->getListViewItem()))->FullName(FALSE)));
	cout<<"after openURL"<<endl;              
	m_pManager->addPart(m_pPart, true);
	if (m_bPartVisible == FALSE)
  	m_bPartVisible = TRUE;

	unplugActionList("openwith");
	cout<<"before OpenWithActions"<<endl;
	OpenWithActions(m_PartServices);
	cout<<"after OpenWithActions"<<endl;
	if (m_PartServices.count() > 0)
		plugActionList("openwith", m_openWithActions);
	return;
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::DeactivatePart(CHistoryItem* hItem)
{
	m_pPart = hItem->getPart();
  m_pSplitter->removeChild(m_pPart);
	m_pManager->removePart(m_pPart);
	m_pSplitter->insertChild(m_pRightPanel);
	//createGUI(0L);
	m_pPart->widget()->hide();
  //delete m_pPart;
	m_pPart = 0L;
	m_pRightPanel->show();
	m_bPartVisible = FALSE;
	unplugActionList("openwith");
	PopulateSortMenu(*m_pSortMenu);
	//OnSelectDeselect();
	return;
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::CreateMenus()                                                                                                  
{
	QMenuBar *pMenuBar = new QMenuBar(NULL/*this alexandrm*/ , "MenuBar");
  //connect(pMenuBar, SIGNAL(highlighted(int)), this, SLOT(OnMenuHighlighted(int)));
  	connect(menuBar(), SIGNAL(highlighted(int)), this, SLOT(OnMenuHighlighted(int)));

	m_MainMenu.clear();

	m_pSortMenu = new QPopupMenu;

	QPopupMenu *pCurrentMenu = NULL;

	for (int i=0; i < gMainMenuItemInitSize; i++)
	{
		if (gMainMenu[i].m_nItemID == knMENU_SEPARATOR)
			pCurrentMenu->insertSeparator();
		else
		{
			if (gMainMenu[i].m_pSlot == NULL)
			{
        if (gMainMenu[i].m_nItemID == knMENU_VIEW_SORT)
        {
          pCurrentMenu->insertItem(LoadString(knSORT_MENU), m_pSortMenu);
        }
        else
        {
          pCurrentMenu = new QPopupMenu(NULL, "PopupMenu");

          pMenuBar->insertItem(LoadString(gMainMenu[i].m_nItemLabelID), pCurrentMenu);

          if (2 == i)
            pCurrentMenu->setCheckable(TRUE);
        }
			}
			else
			{
				pCurrentMenu->insertItem(LoadString(gMainMenu[i].m_nItemLabelID), this, gMainMenu[i].m_pSlot, 0, gMainMenu[i].m_nItemID);

				CMenuItemInfo mii;
				mii.m_nItemID = gMainMenu[i].m_nItemID;
				mii.m_pMenu = pCurrentMenu;
				m_MainMenu.Add(mii);
			}
		}
	}

  // Special items

  m_pFileRestoreMenuItem = NULL;
  m_pEmptyDumpsterMenuItem = NULL;
  m_pDumpsterSeparator = NULL;
  pMenuBar->hide();
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnViewTree()
{
  if (gbShowTree)
	{
    gnDesiredTreeWidth = m_pTreeView->width();
    m_pTreeView->hide();
		m_pTreeView->recreate(this, 0, QPoint(0,0));
		CheckMenuItem(knMENU_VIEW_TREE, FALSE);
	}
	else
	{
		m_pTreeView->resize(gnDesiredTreeWidth, m_pTreeView->height());
		m_pRightPanel->resize(m_pSplitter->width() - gnDesiredTreeWidth - 6, m_pRightPanel->height());
    m_pTreeView->recreate(m_pSplitter, 0, QPoint(0,0), TRUE);
		m_pSplitter->moveToFirst(m_pTreeView);
		CheckMenuItem(knMENU_VIEW_TREE, TRUE);
	}

	int w = width();
	int h = height();

  qApp->processEvents();
	SetSplitterMinSize();
	resize(w, h);
  gbShowTree = !gbShowTree;
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnIconView()
{
	if (gbUseBigIcons)
	{
    cout<<"Do not use big icons"<<endl;
		m_pRightPanel->setIconView(FALSE);
		CheckMenuItem(knMENU_VIEW_LARGE_ICONS, FALSE);
	}
	else
	{
    cout<<"Use big icons"<<endl;
		m_pRightPanel->setIconView(TRUE);
		CheckMenuItem(knMENU_VIEW_LARGE_ICONS, TRUE);
	}

  gbUseBigIcons = !gbUseBigIcons;
}

////////////////////////////////////////////////////////////////////////////

static const unsigned char cross_gif_data[] =
{
	0x47,0x49,0x46,0x38,0x37,0x61,0x0a,0x00,0x09,0x00,0xf0,0x00,0x00,0x00,
	0x00,0x00,0xc5,0xc5,0xc5,0x2c,0x00,0x00,0x00,0x00,0x0a,0x00,0x09,0x00,
	0x00,0x02,0x11,0x8c,0x8f,0x01,0x80,0x7b,0xae,0x58,0x5b,0x32,0x41,0x4b,
	0xb1,0xb9,0x37,0xa6,0x5f,0x00,0x00,0x3b
};

void CMainFrame::CreateViews()
{
	m_pSplitter = new QSplitter(this);

	//m_pSplitter->setMinimumSize(gnDesiredAppWidth, gnDesiredAppHeight);

	setCentralWidget(m_pSplitter);
	setCaption(LoadString(knAPP_TITLE));

	//if (gbShowTree)
	//{
		//m_pTreeView = new CLeftTreeView(m_pSplitter);
	//}
	//else
	//{
		m_pTreeView = new CLeftTreeView(this);
		m_pTreeView->hide();
	//}

	m_pTreeView->addColumn("");

	static QPixmap Cross;
	Cross.loadFromData(cross_gif_data, sizeof(cross_gif_data) / sizeof(unsigned char));
	Cross.setMask(Cross.createHeuristicMask());

	m_pTreeView->stationaryHeader(true, LoadString(knALL_FOLDERS),
																true, &Cross);

	m_pTreeView->setTreeStepSize(20);
#ifdef QT_20
	m_pTreeView->setFrameStyle(QFrame::NoFrame);
#endif
	m_pTreeTip = new CTreeToolTip(m_pTreeView);
	cout<<"before create rightpanel"<<endl;
	m_pRightPanel = new CRightPanel(m_pSplitter);
	//setXMLFile("RightPanelui.rc");
	cout<<"after create rightpanel"<<endl;
#ifdef QT_20
	m_pRightPanel->setFrameStyle(QFrame::NoFrame);
#endif
	m_pRightTip = new CTreeToolTip(m_pRightPanel);
#ifdef QT_20
	connect(m_pRightPanel, SIGNAL(PartRequest(CListViewItem*)), this, SLOT(OnPartRequest(CListViewItem*)));
#endif
	connect(m_pRightPanel, SIGNAL(ChdirRequest(CListViewItem*,QString)), this, SLOT(OnRightChdir(CListViewItem*,QString)));
	connect(m_pRightPanel, SIGNAL(GoParentRequest()), this, SLOT(OnGoParent()));
	connect(m_pRightPanel, SIGNAL(PropertiesRequest()), this, SLOT(OnFileProperties()));
	connect(m_pRightPanel, SIGNAL(PopupMenuRequest(const QPoint&)), this, SLOT(OnRightPopupMenuRequest(const QPoint&)));
	connect(m_pRightPanel, SIGNAL(CutRequest()), this, SLOT(OnFileCut()));
	connect(m_pRightPanel, SIGNAL(CopyRequest()), this, SLOT(OnFileCopy()));
	connect(m_pRightPanel, SIGNAL(PasteRequest()), this, SLOT(OnFilePaste()));
#ifndef BRIEF_VERSION
	connect(m_pRightPanel, SIGNAL(UndoRequest()), this, SLOT(OnUndo()));
#endif
	connect(m_pRightPanel, SIGNAL(DeleteRequest()), this, SLOT(OnFileDelete()));
	connect(m_pRightPanel, SIGNAL(NukeRequest()), this, SLOT(OnFileNuke()));
	connect(m_pRightPanel, SIGNAL(BackRequest()), this, SLOT(OnBack()));
	connect(m_pRightPanel, SIGNAL(ForwardRequest()), this, SLOT(OnForward()));
	connect(m_pRightPanel, SIGNAL(BookmarkURLRequest(const char *, const char *)), this, SLOT(OnBookmarkURL(const char *, const char *)));
  connect(m_pRightPanel, SIGNAL(ConsoleRequest()), this, SLOT(OnOpenConsole()));

	connect(m_pRightPanel, SIGNAL(UpdateCompleted(int,double,int)), this, SLOT(OnRightViewUpdated(int,double,int)));
	connect(m_pRightPanel, SIGNAL(selectionChanged()), SLOT(UpdateButtons()));
	connect(m_pRightPanel, SIGNAL(TabRequest(BOOL)), SLOT(OnTabRequest(BOOL)));
#ifdef QT_20
  connect(m_pRightPanel, SIGNAL(StatusMessage(const char *)), this, SLOT(OnStatusMessage(const char *)));
#else
  connect(m_pRightPanel, SIGNAL(StatusMessage(const char*)), m_pStatusBar, SLOT(message(const char*)));
#endif
	connect(m_pRightPanel, SIGNAL(DocumentDone(const char *, const char *, BOOL)), SLOT(OnDocumentDone(const char *, const char *, BOOL)));
	connect(m_pRightPanel, SIGNAL(BrowserTextSelected(BOOL)), SLOT(OnBrowserTextSelected(BOOL)));

	m_pRightPanel->setFocusPolicy(QWidget::StrongFocus);
	cout<<"after all signals"<<endl;
	m_pTreeView->setFocusPolicy(QWidget::StrongFocus);

	connect(m_pTreeView, SIGNAL(NavigateRequest(CListViewItem*)), SLOT(OnTreeSelectionChanged(CListViewItem*)));
  connect(m_pTreeView, SIGNAL(doubleClicked(CListViewItem*)), this, SLOT(OnTreeDoubleClicked(CListViewItem*)));

  connect(m_pTreeView, SIGNAL(selectionChanged()), SLOT(UpdateButtons()));
	connect(m_pTreeView, SIGNAL(rightButtonClicked(CListViewItem*, const QPoint&, int)), this, SLOT(OnRightClickedTree(CListViewItem*, const QPoint&, int)));
	connect(m_pTreeView, SIGNAL(TabRequest(BOOL)), SLOT(OnTabRequest(BOOL)));
	connect(m_pTreeView, SIGNAL(GoParentRequest()), this, SLOT(OnGoParent()));
	connect(m_pTreeView, SIGNAL(CutRequest()), this, SLOT(OnFileCut()));
	connect(m_pTreeView, SIGNAL(CopyRequest()), this, SLOT(OnFileCopy()));
	connect(m_pTreeView, SIGNAL(PasteRequest()), this, SLOT(OnFilePaste()));
#ifndef BRIEF_VERSION
	connect(m_pTreeView, SIGNAL(UndoRequest()), this, SLOT(OnUndo()));
#endif
	connect(m_pTreeView, SIGNAL(DeleteRequest()), this, SLOT(OnFileDelete()));
	connect(m_pTreeView, SIGNAL(NukeRequest()), this, SLOT(OnFileNuke()));
	connect(m_pTreeView, SIGNAL(SelectAllRequest()), this, SLOT(SelectAll()));
	connect(m_pTreeView, SIGNAL(stationaryHeaderClosed()), this, SLOT(OnViewTree()));
  connect(m_pTreeView, SIGNAL(ConsoleRequest()), this, SLOT(OnOpenConsole()));

  cout<<"before somewhere"<<endl;	
	CListViewItem *pDesktopItem = new CWindowsTreeItem(m_pTreeView, LoadString(knMY_LINUX));
	cout<<"after creation"<<endl;	
	//m_pTreeView->setCurrentItem(pDesktopItem);
  cout<<"somewhere else"<<endl;	
	if (gbNetworkAvailable)
		m_pNetworkRoot = new CMSWindowsNetworkItem(pDesktopItem);
	else
		m_pNetworkRoot = NULL;

	cout<<"somewhere"<<endl;	
	m_pNFSRoot = new CNFSNetworkItem(pDesktopItem);

  m_pPrinterRoot = new CPrinterRootItem(pDesktopItem);

  if (gbShowMyComputer == -1)
  {
    gbShowMyComputer = !getuid() || !geteuid();
  }

  if (gbShowMyComputer)
	  m_pMyComputer = new CMyComputerItem(pDesktopItem);
  else
    m_pMyComputer = NULL;

	CSMBFileInfo info;
	FillFileInfo(&info, getenv("HOME"), "olegn", NULL, NULL);

	m_pHomeRoot = new CHomeItem(pDesktopItem, NULL, &info);
  //new CVDriveItem(pDesktopItem);

  AddDeviceItems(pDesktopItem);

	pDesktopItem->setExpandable(TRUE);
	pDesktopItem->setPixmap(0, *LoadPixmap(keDesktopIcon));
	pDesktopItem->setOpen(TRUE);
	cout<<"in the end"<<endl;
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::EnableMenuItem(int nID, BOOL bEnable)
{
	for (int i=0; i < m_MainMenu.count(); i++)
	{
		if (m_MainMenu[i].m_nItemID == nID)
    {
			m_MainMenu[i].m_pMenu->setItemEnabled(nID, bEnable);
			if (i<14)
				m_ActionList.at(i)->setEnabled(bEnable);
			else
				m_ActionList.at(i-2)->setEnabled(bEnable);
    }


	}
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::CheckMenuItem(int nID, BOOL bIsChecked)
{
	for (int i=0; i < m_MainMenu.count(); i++)
	{
		if (m_MainMenu[i].m_nItemID == nID)
			m_MainMenu[i].m_pMenu->setItemChecked(nID, bIsChecked);
	}
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnFileNewFolder()
{
	QTimer::singleShot(30, this, SLOT(NewFolder()));
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::NewFolder()
{
	CWindowsTreeItem *pItem = (CWindowsTreeItem *)m_pTreeView->currentItem();

	if (NULL != pItem && pItem->CanCreateSubfolder())
	{
    QString sFolderName(LoadString(knNEW_FOLDER));

		if (pItem->CreateSubfolder(sFolderName))
		{
			RescanItem((CNetworkTreeItem*)pItem);

			m_pRightPanel->ResetRefreshTimer(2);
			m_pRightPanel->ActivateEvent();
			m_pRightPanel->viewport()->setFocus();
			m_pRightPanel->SelectByName((LPCSTR)sFolderName
#ifdef QT_20
  .latin1()
#endif
      );
			OnFileRename();
		}
	}
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnFileDelete()
{
	GetActiveItems();

	// XXX milindc
	// The flag, if TRUE, implies that the files should be moved to Trash!
	RemoveFiles(TRUE);
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::FileDelete()
{
	RemoveFiles(TRUE);
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnFileNuke()
{
	GetActiveItems();
	// The flag, if FALSE, implies that the files should be removed permanently!
	RemoveFiles(FALSE);
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::FileNuke()
{
  RemoveFiles(FALSE);
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::RemoveFiles(BOOL bMoveToTrash)
{
  if (!CanDoOperation(bMoveToTrash ? &CMainFrame::CanMoveToTrash : &CMainFrame::CanDelete))
	{
		printf("Cannot do operation!\n");
		return;
	}

	QListIterator<CNetworkTreeItem> it(ActiveItems);
	QStrList list;
	BOOL bSilentDelete = FALSE;

	for (; it.current() != NULL; ++it)
	{
		QString URL;

		if (it.current()->Kind() == keTrashEntryItem)
		{
      bSilentDelete = TRUE;

      CTrashEntryItem *pTrashEntry = (CTrashEntryItem *)it.current();

      int nIndex = pTrashEntry->m_FileName.findRev('/');

      QString InfoFileName = pTrashEntry->m_FileName.left(nIndex);

		  MakeURL(InfoFileName, NULL, URL);
		  //printf("Adding %s\n", (LPCSTR)URL);
		  list.append(URL);

      URL += ".info";
    }
    else
      URL = MakeItemURL(it.current());

    //printf("Adding %s\n", (LPCSTR)URL);

     list.append(URL);
	}

	StartDelete(list, bMoveToTrash, bSilentDelete);
}

////////////////////////////////////////////////////////////////////////////

static CListViewItem *pRenameItem;

void CMainFrame::FileRenameAction()
{
	CWindowsTreeItem *pItem = (CWindowsTreeItem *)pRenameItem;

  if (NULL != pItem && pItem->CanEditLabel())
	{
		pItem->listView()->viewport()->setFocus();
		pItem->StartLabelEdit();
	}
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnFileRename()
{
  GetActiveItems();
  FileRename();
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnPartRequest(CListViewItem* pItem)
{
	CNetworkTreeItem* pI = (CNetworkTreeItem*)pItem;
	QString Name(((CNetworkTreeItem*)pI)->FullName(FALSE));
	cout<<"Name="<<Name<<endl;

	int nLen = strlen(Name);
  m_pCurrentItem = pItem;

		struct stat buff;
		if ( stat( QFile::encodeName(Name), &buff ) != -1 )
		{
			KMimeType::Ptr mime = KMimeType::findByURL(KURL(Name), buff.st_mode, true);
			KService::Ptr service = 0L;
			if( mime == 0L )
				cout<<"mime is 0L"<<endl;
			else
			{
				cout<<"mime is not 0L"<<endl;
				cout<<"mimeType is "<<mime->name()<<endl;
				KTrader::OfferList offers = KTrader::self()->query(mime->name());

				if(offers.count() != 0 )
				{
					KTrader::OfferList::ConstIterator it = offers.begin();
					KTrader::OfferList::ConstIterator end = offers.end();
					for(;it != end; ++it)
					{
						QVariant prop = (*it)->property( "X-KDE-BrowserView-AllowAsDefault" );
						cout<< (*it)->name() << " : X-KDE-BrowserView-AllowAsDefault is valid : " << prop.isValid() << endl;
						if ( !prop.isValid() || prop.toBool() ) // defaults to true
						{
							service=*it;
							break;
						}
					}

					cout<<"service is "<<service->name()<<endl;
					cout<<"library is "<<service->library()<<endl;
					m_PartServices = offers;
					KLibrary* library = KLibLoader::self()->library(service->library());

					if (!library)
					{
						cout<<"Can not create library"<<endl;
						KURL::List lst;
						lst.append(KURL(Name));
						KRun::run(*service,lst);
						return;
					}

					KLibFactory *factory = 0L;
					factory = KLibLoader::self()->factory(library->name());

					if (!factory)
					{
						cout<<"Can not create factory"<<endl;
						return;
					}

					QStringList args;
					QVariant prop = service->property("X-KDE-BrowserView-Args");
					if(prop.isValid())
					{
						QString argStr = prop.toString();
						args = QStringList::split("",argStr);
					}

					m_pPart = (KParts::ReadOnlyPart *)factory->create(m_pSplitter, "viewer-part", "Browser/View", args);

					if (!m_pPart)
					{
						cout<<"Can not create Part"<<endl;
						return;
					}

					m_pRightPanel->hide();
					CHistoryItem	* hItem = new CHistoryItem;
					hItem->setPart(m_pPart) ;
					hItem->setFile(true);
					hItem->setListViewItem(pItem);
					hItem->setPartServices(m_PartServices);
          while (gnCanForward > 0)
					{
						gHistory.removeFirst();
						gnCanForward--;
					}
					gHistory.insert(0,hItem);
					
					ActivatePart(hItem);
					UpdateComboAndToolBar(hItem->getListViewItem());

				}
			}
		}
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::FileRename()
{
  pRenameItem = ActiveItems.count() > 0 ? ActiveItems.getFirst() : NULL;
  QTimer::singleShot(30, this, SLOT(FileRenameAction()));
}

////////////////////////////////////////////////////////////////////////////

BOOL CMainFrame::CanDoProperties(CNetworkTreeItem *pItem)
{
	switch (pItem->Kind())
	{
		default:
		break;

		case keFileSystemItem:
		case keServerItem:
		case keShareItem:
		case keFileItem:
		//case kePrinterItem: FIXME: what about printer properties?
		case keLocalFileItem:
		case keFTPFileItem:
    case keTrashEntryItem:
    case keDeviceItem:
    case keLocalPrinterItem:
			return TRUE;
	}

	return FALSE;
}

////////////////////////////////////////////////////////////////////////////

BOOL CMainFrame::CanRestoreFile(CNetworkTreeItem *pItem)
{
  return NULL != pItem && pItem->Kind() == keTrashEntryItem;
}

////////////////////////////////////////////////////////////////////////////

BOOL CMainFrame::CanDoNewFolder(CNetworkTreeItem *pItem)
{
	return NULL != pItem && pItem->CanCreateSubfolder();
}

////////////////////////////////////////////////////////////////////////////

BOOL CanDoFileCopy()
{
	CMainFrame *pMF = (CMainFrame *)qApp->mainWidget();

	if (NULL == pMF)
		return FALSE;

  return pMF->CanDoOperation(&CMainFrame::CanDoCopy);
}

BOOL CMainFrame::CanDoOperation(LPFNCanDo pOperation)
{
#ifndef QT_20
	if (m_pRightPanel->IsWebBrowserWithFocus())
		return FALSE;
#endif
	if (!ActiveItems.count())
		return FALSE;

	QListIterator<CNetworkTreeItem> it(ActiveItems);

	for (; it.current() != NULL; ++it)
	{
		 if (!(this->*pOperation)(it.current()))
			 return FALSE;
	}

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnFileProperties()
{
	GetActiveItems();

	if (ActiveItems.count() >  0)
		DoProperties(FALSE);
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnFileExit()
{
  if (gbNeedSaveConfigSettings)
  {
#if (QT_VERSION < 200)
    SaveConfigSettings(((KApplication *)qApp)->getConfig(), FALSE);	 // do not save path info
#else
    SaveConfigSettings(KGlobal::config(), FALSE); // do not save path info
#endif
  }

  qApp->quit();
}

////////////////////////////////////////////////////////////////////////////

void CIconText::paintEvent(QPaintEvent *e)
{
	QPainter p;
	QLabel::paintEvent(e);

	p.begin(this);

	QPixmap *pm = pixmap();

	QFontMetrics fm = p.fontMetrics();
	int yPos; // vertical text position

	if (height() < fm.height())
		yPos = fm.ascent() + fm.leading()/2;
	else
		yPos = (height() - fm.height())/2 + fm.ascent();

	p.drawText(pm->width() + 5 + margin(), yPos, (LPCSTR)m_Text
#ifdef QT_20
  .latin1()
#endif
  );

	p.end();
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnRightViewUpdated(int nObjectsCount, double TotalSize, int nSelectedCount)
{
	QString s;

	if (nSelectedCount)
		s.sprintf(LoadString(knNUM_SELECTED_OBJECTS), nSelectedCount);
	else
		s.sprintf(LoadString(knNUM_OBJECTS), nObjectsCount);

	m_pObjectsLabel->setText(s);

	if (TotalSize < 0.)
		s = "";
	else
		s = SizeBytesFormat(TotalSize);

	m_pSizeLabel->setText(s);
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnRightClickedTree(CListViewItem *pItem, const QPoint& Point, int /*nColumn*/)
{
	QStrList MenuItemChain;
	QStrList OldCurrentChain;

	GetChainForItem(pItem->listView(), pItem, MenuItemChain);
	m_pTreeView->viewport()->setFocus();

	CListViewItem *pOldCurrent = m_pTreeView->currentItem();

	if (pOldCurrent != NULL)
	{
		m_pTreeView->setCurrentItem(pOldCurrent);
		GetChainForItem(m_pTreeView, pOldCurrent, OldCurrentChain);
	}

	m_pTreeView->setSelected(pItem, TRUE);

	ActiveItems.clear();

	if (IS_NETWORKTREEITEM(pItem))
		ActiveItems.append((CNetworkTreeItem*)pItem);

	// OnRightClickedTree(...) has been factored into OnRightClicked(...)
	// since the folder items in the TREE can also be seen in the right PANE.
	// Therefore we set a flag in here so that the factor function i.e. OnRightClicked(...)
	// can know what event it is exactly processing.

	m_bRightClickedOnTree = TRUE;

	OnRightClicked(Point);

	m_bRightClickedOnTree = FALSE;

	pItem = GetItemFromChain(m_pTreeView, MenuItemChain);
	pOldCurrent = OldCurrentChain.count() > 0 ? GetItemFromChain(m_pTreeView, OldCurrentChain) : NULL;

	if (NULL != pItem && m_pTreeView->isSelected(pItem))
	{
		m_pTreeView->setSelected(pItem, FALSE);

		if (NULL != pOldCurrent)
		{
			m_pTreeView->setCurrentItem(pOldCurrent);

			if (m_pTreeView->viewport()->hasFocus())
				m_pTreeView->setSelected(pOldCurrent, TRUE);
		}
	}
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnRightClicked(const QPoint& Point)
{
  if (!ActiveItems.count())
		return;

	BOOL bLastSeparator = TRUE;
	BOOL bCanDoSharing = FALSE;
	BOOL bCanCopyCut = FALSE;
	BOOL bCanPaste = FALSE;
	BOOL bHasReadPermissions = TRUE;
	BOOL bHasWritePermissions = FALSE;

  CNetworkTreeItem *pTreeItem = ActiveItems.getFirst();

	QPopupMenu Menu;

	// So here's the menu items that should be in the popup
	// LEFT PANE:                              RIGHT PANE
	//
	// Open                                    Open
	//                                         Open with ...
	// Find ...                                Find ...
	// -------------                           ----------------
	// Mount Network Share ...
	// Disconnect Network Share
	// Sharing ...
	// -------------
	// Cut                                     Cut
	// Copy                                    Copy
	// Paste                                   Paste
	// -------------                           ----------------
	// Create Nickname                         Create Nickname
	// Move to Dumpster                        Move to Dumpster
	// Rename                                  Rename
	// Properties                              Properties
	// -------------
	// Eject CD


	BOOL bFolder = TRUE;
  BOOL bIsTrash = FALSE;
  BOOL bIsMyTrash = FALSE;

	if (pTreeItem->Kind() == keTrashEntryItem)
	{
		Menu.insertItem(LoadString(knRESTORE), this, SLOT(OnFileRestore()));
		Menu.insertSeparator();

    int id = Menu.insertItem(LoadString(knEMPTY_DUMPSTER), this, SLOT(OnEmptyDumpster()));

		Menu.setItemEnabled(id,
                       IsMyTrashFolder((LPCSTR)((CNetworkTreeItem*)pTreeItem->m_pLogicalParent)->FullName(FALSE)
#ifdef QT_20
  .latin1()
#endif
                        ) &&
                        !IsTrashEmpty());
  }
  else
  {
    if (pTreeItem->Kind() == kePrinterRootItem)
    {
      int id = Menu.insertItem(LoadString(knADD_PRINTER_DOTDOTDOT), this, SLOT(OnAddPrinter()));
			Menu.setItemEnabled(id, IsSuperUser());
    }

    if (pTreeItem->Kind() == keLocalPrinterItem)
    {
      int id = Menu.insertItem(LoadString(knPURGE), this, SLOT(Purge()));
			Menu.setItemEnabled(id, IsSuperUser());

			Menu.insertSeparator();

			bCanPaste = TRUE;
			bHasWritePermissions = TRUE;
    }

    if (pTreeItem->Kind() == kePrintJobItem)
  	{
      Menu.insertItem(LoadString(knDELETE), this, SLOT(DeletePrintJob()));
    }

    // Open
		{
	  	int	id = Menu.insertItem(LoadString(kn_OPEN), this, SLOT(OnOpen()));

			if (pTreeItem->isOpen() && (pTreeItem->ExpansionStatus() == keExpansionComplete))
				Menu.setItemEnabled(id, FALSE);
		}

  	// Add "Open With ..." only if clicked in the right panel.
  	if (!m_bRightClickedOnTree)
  	{
  		int	kind = pTreeItem->Kind();

  		// ...and add it only for leaf items i.e. actual files

      if (kind == keFileItem ||
  				kind == keFTPFileItem ||
  				kind == keLocalFileItem)
  		{
  			if (!ItemIsFolder(pTreeItem))
  			{
  				bFolder = FALSE;
  				Menu.insertItem(LoadString(knOPEN_WITH), this, SLOT(OnOpenWith()));
  			}
  		}
  	}

		if (keLocalFileItem == pTreeItem->Kind() && ItemIsFolder(pTreeItem))
		{
	  	Menu.insertItem(LoadString(kn_OPEN_CONSOLE_WINDOW), this, SLOT(OnOpenConsole()));
		}

  	// Find ...
  	{
  		int	id = Menu.insertItem(LoadString(knFIND), this, SLOT(OnFind()));

  		// If the current item is not a Folder then disable it
  		// since we can find files only in a "folder" and
  		// we can find computers only on a *network* which is
  		// assumed to be a folder of computers!! :)

			if (!bFolder)
  			Menu.setItemEnabled(id, FALSE);
  	}
  }

	Menu.insertSeparator();

	if (pTreeItem->Kind() == keFileSystemItem)
	{
		CFileSystemItem *pFileSystem = (CFileSystemItem *)pTreeItem;

		if (pFileSystem->m_DriveType != keNetworkDrive)
      bCanDoSharing = TRUE;

    if (pFileSystem->m_MountedOn != "/")
    {
			int nID = Menu.insertItem(LoadString(knD_ISCONNECT), this, SLOT(OnDisconnectFileSystem()));
      bLastSeparator = FALSE;

			m_LastMenuParam = pFileSystem->m_MountedOn;

			Menu.setItemEnabled(nID,
								(pFileSystem->m_Type != "smbfs") ||
							CanUmountSMB((LPCSTR)pFileSystem->m_MountedOn
#ifdef QT_20
  .latin1()
#endif
                ));
		}

		QString Path(pTreeItem->FullName(FALSE));

		bHasReadPermissions = CanDoCopy(pTreeItem);
		bHasWritePermissions = !access((LPCSTR)Path
#ifdef QT_20
  .latin1()
#endif
    , W_OK);
		bCanPaste = TRUE;
	}
	else
		if (pTreeItem->Kind() == keLocalFileItem) // && CanMountAt(pTreeItem->FullName(FALSE)))
		{
			QString Path(pTreeItem->FullName(FALSE));

      CFileSystemInfo *pInfo = FindMountedFileSystem((LPCSTR)Path
#ifdef QT_20
  .latin1()
#endif
      );

      if (NULL != pInfo)
      {
        int nID = Menu.insertItem(LoadString(knD_ISCONNECT), this, SLOT(OnDisconnectFileSystem()));

        Menu.setItemEnabled(nID,
                  (pInfo->m_Type == "nfs") ||
                  (pInfo->m_Type == "smbfs" && CanUmountSMB((LPCSTR)Path
#ifdef QT_20
  .latin1()
#endif
                  )));

        Menu.insertSeparator();
      }

      bIsTrash = IsTrashFolder(Path);

      if (bIsTrash)
      {
        bIsMyTrash = IsMyTrashFolder(Path);
      }

      if (gbNetworkAvailable && ((CLocalFileItem*)pTreeItem)->IsFolder())
			{
				Menu.insertItem(LoadString(knMOUNT_NETWORK_SHARE_2), this, SLOT(OnMountNetworkShareHere()));
				bLastSeparator = FALSE;

				bCanDoSharing = TRUE;
			}

			bCanCopyCut = bIsTrash ? FALSE : TRUE;
			bCanPaste = TRUE;

			bHasReadPermissions = CanDoCopy(pTreeItem);
			bHasWritePermissions = bIsTrash ? FALSE : !access((LPCSTR)Path
#ifdef QT_20
  .latin1()
#endif
      , W_OK);
		}
		else
			if (pTreeItem->Kind() == keFTPFileItem ||
					pTreeItem->Kind() == keFileItem)
			{
				bCanCopyCut = TRUE;
				bCanPaste = TRUE;
				bHasWritePermissions = TRUE; // so we could [try to] cut/delete
			}
      else
        if (pTreeItem->Kind() == keTrashEntryItem)
        {
    			QString Path(pTreeItem->FullName(FALSE));
    			bHasReadPermissions = !access((LPCSTR)Path
#ifdef QT_20
  .latin1()
#endif
          , R_OK);
    			bHasWritePermissions = !access((LPCSTR)Path
#ifdef QT_20
  .latin1()
#endif
          , W_OK);
        }
		//	return;

	if (bCanDoSharing && ActiveItems.count() == 1)
	{
		//if (!bLastSeparator)
		//	Menu.insertSeparator();
		int id = Menu.insertItem(LoadString(knSHARING), this, SLOT(OnSharing()));
    Menu.setItemEnabled(id, gbNetworkAvailable);
		bLastSeparator = FALSE;
	}

	if (!bLastSeparator)
	{
		Menu.insertSeparator();
		bLastSeparator = TRUE;
	}

	if (bCanCopyCut)
	{
		int id = Menu.insertItem(LoadString(knCU_T), this, SLOT(FileCut()));

		if (!bHasReadPermissions || !bHasWritePermissions)
			Menu.setItemEnabled(id, FALSE);

		id = Menu.insertItem(LoadString(kn_COPY), this, SLOT(FileCopy()));

		Menu.setItemEnabled(id, CanDoOperation(&CMainFrame::CanDoCopy));

		bLastSeparator = FALSE;
	}

	if (bCanPaste)
	{
		int PasteID = Menu.insertItem(LoadString(kn_PASTE), this, SLOT(FilePaste()));

		if (!bHasWritePermissions || !CanPaste())
			Menu.setItemEnabled(PasteID, FALSE);

		bLastSeparator = FALSE;
	}

	// XXX milindc
	if (!bLastSeparator)
	{
		Menu.insertSeparator();
		Menu.insertItem(LoadString(knCREATE_NICKNAME), this, SLOT(OnCreateNickname()));

		bLastSeparator = TRUE;
	}

	if (!bLastSeparator)
		Menu.insertSeparator();

	int id;

	if (pTreeItem->Kind() == keTrashEntryItem)
	{
		id = Menu.insertItem(LoadString(knDELETE), this, SLOT(FileNuke()));
		Menu.setItemEnabled(id, bHasWritePermissions);
	}
	else
	{
		if (bIsTrash)
		{
			id = Menu.insertItem(LoadString(knEMPTY_DUMPSTER), this, SLOT(OnEmptyDumpster()));
			Menu.setItemEnabled(id, bIsMyTrash && !IsTrashEmpty());
		}
		else
		{
			if (pTreeItem->Kind() != keLocalPrinterItem)
			{
				id = Menu.insertItem(LoadString(knMOVE_TO_DUMPSTER), this, SLOT(FileDelete()));
				Menu.setItemEnabled(id, CanDoOperation(&CMainFrame::CanMoveToTrash));
			}

			id = Menu.insertItem(LoadString(knDELETE), this, SLOT(FileNuke()));
			Menu.setItemEnabled(id, CanDoOperation(&CMainFrame::CanDelete));
		}

		int RenameID = Menu.insertItem(LoadString(knRENAME), this, SLOT(FileRename()));

		if (bIsTrash ||
				1 != ActiveItems.count() ||
				!pTreeItem->CanEditLabel())
			Menu.setItemEnabled(RenameID, FALSE);
	}

	bLastSeparator = FALSE;

	if (CanDoOperation(&CMainFrame::CanDoProperties))
	{
		if (!bLastSeparator)
			Menu.insertSeparator();

		Menu.insertItem(LoadString(knP_ROPERTIES), this, SLOT(OnProperties()));
		bLastSeparator = FALSE;
	}

	// Floppy unmount

	if (
      (
       (pTreeItem->Kind() == keDeviceItem) &&
			 (((CDeviceItem*)pTreeItem)->m_DriveType == keFloppyDrive)
      )
     )
	{
		if (!bLastSeparator)
			Menu.insertSeparator();

		int id = Menu.insertItem(LoadString(knUNMOUNT), this, SLOT(OnDisconnectFileSystem()));

		if (((CDeviceItem*)pTreeItem)->m_MountedOn.isEmpty())
		{
			Menu.setItemEnabled(id, FALSE);
		}
	}

	// Eject CD

	BOOL bIsDeviceItem = (pTreeItem->Kind() == keDeviceItem);

	if (((pTreeItem->Kind() == keFileSystemItem) &&
			((CFileSystemItem*)pTreeItem)->m_DriveType == keCdromDrive) ||
			(bIsDeviceItem &&
			(((CDeviceItem*)pTreeItem)->m_DriveType == keCdromDrive) ||
       ((CDeviceItem*)pTreeItem)->m_DriveType == keZIPDrive))
	{
		if (!bLastSeparator)
			Menu.insertSeparator();

		int nStringID = (bIsDeviceItem && ((CDeviceItem*)pTreeItem)->m_DriveType == keZIPDrive) ? knEJECT : knEJECT_CD;

		int id = Menu.insertItem(LoadString(nStringID), this, SLOT(OnEjectCD()));

		// OK, we disable this item if the CDROM drive has not been mounted.

		if (bIsDeviceItem &&
				((CDeviceItem*)pTreeItem)->m_MountedOn.isEmpty())
			Menu.setItemEnabled(id, FALSE);
	}

	if (Menu.count() > 0)
  {
    if (pTreeItem->listView() != m_pTreeView)
      bInsideMenu = true;

		Menu.exec(Point);

		bInsideMenu = false;
  }
}

////////////////////////////////////////////////////////////////////////////
// called from pop-up menu constructed on Right Mouse Button Click

void CMainFrame::OnOpen()
{
  if (!ActiveItems.count())
		return;

  if (m_bRightClickedOnTree)
	{
		CNetworkTreeItem	*pTreeItem = ActiveItems.getFirst();
    QString URL = pTreeItem->FullName(TRUE);
		printf("Launch URL = <%s>\n", (LPCSTR)URL);
    LaunchURL((LPCSTR)URL
#ifdef QT_20
  .latin1()
#endif
    );
	}
	else
	{
//		if (1 == ActiveItems.count())
    //{
//      m_pRightPanel->OnDoubleClicked(ActiveItems.getFirst());
    //}
    //else
    {
      QListIterator<CNetworkTreeItem> it(ActiveItems);

      for (; it.current() != NULL; ++it)
      {
        CNetworkTreeItem *pItem = it.current();

        if (keFTPFileItem == pItem->Kind() && !((CFTPFileItem*)pItem)->IsFolder())
        {
          QString URL;
          URL = MakeItemURL(pItem);

#if (QT_VERSION < 200)
          KMimeType::InitStatic();
          KMimeType::init();

          KMimeType *t = KMimeType::findByName("text/html");
          RunMimeBinding(FALSE, URL, NULL == t ? NULL : (LPCSTR)t->getDefaultBinding());
#else
          printf("TODO: implement RunMimeBinding!\n");
#endif
        }
        else
        {
          if ((keLocalFileItem == pItem->Kind() &&
              !((CLocalFileItem*)pItem)->IsFolder()) ||
              (keFileItem == pItem->Kind() &&
              !((CFileItem*)pItem)->IsFolder()))
          {
            QString URL;
            URL = MakeItemURL(pItem);
            //QString FileName(pItem->FullName(FALSE));
            RunMimeBinding(FALSE, URL);
          }
          else
          {
            QString URL;
            URL = MakeItemURL(pItem);
            LaunchURL((LPCSTR)URL
#ifdef QT_20
  .latin1()
#endif
            );
          }
        }
      }
    }
	}
}

////////////////////////////////////////////////////////////////////////////
// called from pop-up menu constructed on Right Mouse Button Click

void CMainFrame::OnOpenWith()
{
	if (!ActiveItems.count())
		return;

	QListIterator<CNetworkTreeItem> it(ActiveItems);
  QString Documents;

	for (; it.current() != NULL; ++it)
	{
    if (!Documents.isEmpty())
      Documents += "\n";

    Documents += it.current()->FullName(FALSE);
  }

  if (!Documents.isEmpty())
    RunMimeBinding(TRUE, Documents);
}

////////////////////////////////////////////////////////////////////////////
// TODO find computer or find files ???  functions are already present...
// just invoke them appropriately.
// called from pop-up menu constructed on Right Mouse Button Click

void CMainFrame::OnFind()
{
	int	kind = ActiveItems.getFirst()->Kind();

	if (kind == keMSRootItem || kind == keWorkgroupItem || kind == keNFSRootItem)
	{
		// redirection to already available functions...
		OnFindComputer();
	}
	else
	{
		OnFindFiles();
	}
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnCreateNickname()
{
	if (!ActiveItems.count())
		return; // safety check

	CNetworkTreeItem*	pTreeItem = ActiveItems.getFirst();
	QString	filename = pTreeItem->FullName(FALSE);

	// XXX Usage: nickname <file-name> -path <shortcut-destination-name>

	QString command("nickname -path \"");

	if (ItemIsFolder(pTreeItem))
	{
		command += filename;
	}
	else
	{
		command += getenv("HOME");
		command += "/Desktop\" -nickname \"";
		command += pTreeItem->text(0);
		command += "\" \"";
		command += filename;
	}

	command += "\" &";

	system((LPCSTR)command
#ifdef QT_20
  .latin1()
#endif
  );
}


////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnEjectCD()
{
	if (!ActiveItems.count())
		return; // safety check

	CNetworkTreeItem*	pItem = ActiveItems.getFirst();

  QString Location;
	QString Device;

	if (pItem->Kind() == keFileSystemItem)
	{
		Location = ((CFileSystemItem*)pItem)->m_MountedOn;
		Device = ((CFileSystemItem*)pItem)->m_Name;
	}
	else
		if (pItem->Kind() == keDeviceItem)
		{
			Location = ((CDeviceItem*)pItem)->m_MountedOn;
			Device = ((CDeviceItem*)pItem)->m_Name;
		}
		else
			return; // wrong kind...

	if (m_pTreeView->currentItem() == pItem)
	{
		pItem->setOpen(FALSE);
		GoItem(pItem->m_pLogicalParent);
	}

	QString	command;
	command = "eject ";

	URLEncode(Location);
	URLEncode(Device);

	command += Location;
	command += " ";
  command += Device;

	ServerExecute((LPCSTR)command
#ifdef QT_20
  .latin1()
#endif
  );
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnMountNetworkShareHere()
{
	QString MountPointPath = "";

	int nCredentialsIndex = 0;

	if (ActiveItems.count() > 0)
	{
		CNetworkTreeItem *pItem = ActiveItems.getFirst();

		if (pItem->Kind() == keFileItem)
		{
			while (NULL != pItem && pItem->Kind() != keShareItem)
			{
        pItem = (CNetworkTreeItem *)pItem->m_pLogicalParent;
			}

      if (NULL == pItem)
        pItem = ActiveItems.getFirst();
		}

		if ((pItem->Kind() == keLocalFileItem && ((CLocalFileItem*)pItem)->IsFolder()) ||
				pItem->Kind() == keShareItem ||
				pItem->Kind() == keNFSShareItem)
		{
			MountPointPath = pItem->FullName(FALSE);
			nCredentialsIndex = pItem->CredentialsIndex();
		}
	}

	::OnMountNetworkShare(MountPointPath, m_pNetworkRoot, nCredentialsIndex, this);
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnDisconnectFileSystem()
{
	if (ActiveItems.count() == 1)
		DisconnectFileSystem(ActiveItems.getFirst(), NULL);
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::DisconnectFileSystem(CListViewItem *pItem, CFileSystemInfo *pInfo)
{
	QString MountedOn;
	QString Device;
	BOOL bIsNFS = FALSE;
  BOOL bIsSMBFS = FALSE;

	if (NULL == pItem)
	{
		if (NULL == pInfo)
			return;

		MountedOn = pInfo->m_MountedOn;
		Device = pInfo->m_Name;
		bIsNFS = (pInfo->m_Type == "nfs");
		bIsSMBFS = (pInfo->m_Type == "smbfs");
	}
	else
	{
    CNetworkTreeItem *pI = (CNetworkTreeItem *)pItem;

		if (keFileSystemItem == pI->Kind())
		{
			CFileSystemItem *pFS = (CFileSystemItem *)pItem;
			MountedOn = pItem->text(0);
			Device = pItem->text(1);
			bIsNFS = (pFS->m_Type == "nfs");
			bIsSMBFS = (pFS->m_Type == "smbfs");
		}
		else
			if (keLocalFileItem == pI->Kind())
			{
				QString Path(pI->FullName(FALSE));

				int i;

				for (i=0; i < gFileSystemList.count(); i++)
				{
					if (gFileSystemList[i].m_MountedOn == Path)
					{
						MountedOn = Path;
						Device = gFileSystemList[i].m_Name;
						bIsNFS = (gFileSystemList[i].m_Type == "nfs");
						bIsSMBFS = (gFileSystemList[i].m_Type == "smbfs");

						break;
					}
				}

				if (i == gFileSystemList.count())
					return;
			}
			else
			{
				if (m_pTreeView->currentItem() == pI)
				{
					pI->setOpen(FALSE);
					GoItem(pI->m_pLogicalParent);
				}

				MountedOn = pI->FullName(FALSE);

				QString	command;
				command = "fdumount ";

				URLEncode(MountedOn);
				command += MountedOn;

				ServerExecute((LPCSTR)command
#ifdef QT_20
  .latin1()
#endif
        );

				return;
			}
	}

  if (FindAutoMountEntry((LPCSTR)Device
#ifdef QT_20
  .latin1()
#endif
  , (LPCSTR)MountedOn
#ifdef QT_20
  .latin1()
#endif
  ))
	{
		QSTRING_WITH_SIZE(s, 256+Device.length());

		s.sprintf(LoadString(knDO_YOU_WANT_TO_MOUNT), (LPCSTR)Device
#ifdef QT_20
  .latin1()
#endif
    );

		if (1 == QMessageBox::warning(qApp->mainWidget(), LoadString(knDISCONNECT), (LPCSTR)s
#ifdef QT_20
  .latin1()
#endif
    , LoadString(knYES), LoadString(knNO)))
		{
			RemoveAutoMountEntry((LPCSTR)MountedOn
#ifdef QT_20
  .latin1()
#endif
      );
		}
	}

	QString Error;

	if (bIsSMBFS)
	{
    if (!UmountSMBShare((LPCSTR)MountedOn
#ifdef QT_20
  .latin1()
#endif
    ))
		{
			QSTRING_WITH_SIZE(msg, 1024+Device.length() + MountedOn.length());
			msg.sprintf(LoadString(knUNABLE_TO_DISCONNECT_X_FROM_Y), (LPCSTR)Device
#ifdef QT_20
  .latin1()
#endif
      , (LPCSTR)MountedOn
#ifdef QT_20
  .latin1()
#endif
      );
			QMessageBox::critical(qApp->mainWidget(), LoadString(knDISCONNECT), (LPCSTR)msg
#ifdef QT_20
  .latin1()
#endif
      , LoadString(knOK));
		}
	}
	else
	{
		QString ErrorDescription;

		if (keErrorAccessDenied == UmountFilesystem((LPCSTR)MountedOn
#ifdef QT_20
  .latin1()
#endif
    , bIsNFS, ErrorDescription))
		{
			QSTRING_WITH_SIZE(msg, 1024+MountedOn.length());
			msg.sprintf(LoadString(knUNABLE_TO_UMOUNT_X_Y), (LPCSTR)MountedOn
#ifdef QT_20
  .latin1()
#endif
      , (LPCSTR)ErrorDescription
#ifdef QT_20
  .latin1()
#endif
      );
      QMessageBox::critical(qApp->mainWidget(), LoadString(knDISCONNECT), (LPCSTR)msg
#ifdef QT_20
  .latin1()
#endif
      , LoadString(knOK));
		}
	}
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnItemDestroyed(CListViewItem *pItem)
{
	QListIterator<CNetworkTreeItem> it(ActiveItems);

	for (; it.current() != NULL; ++it)
	{
		if (it.current() == pItem)
		{
			ActiveItems.remove((CNetworkTreeItem*)pItem);
			break;
		}
	}

	CListViewItem *pNew;

	for (pNew=m_pTreeView->currentItem(); NULL != pNew && pNew != pItem; pNew=pNew->parent());

	if (NULL != pNew)
	{
		pNew = NULL;
		pNew = pItem->nextSibling();

		if (NULL == pNew)
		{
			pNew = pItem->parent();

			if (pNew != NULL)
				pNew = pNew->nextSibling();

			if (NULL == pNew)
				pNew = pItem->itemAbove();
		}
	}

	if (pNew != NULL)
	{
		m_pTreeView->setSelected(pNew, TRUE);
	}

	// Now see if right panel is displaying this item or one of its children

	if (NULL != m_pRightPanel->m_pExternalItem)
	{
		CListViewItem *pI = m_pRightPanel->m_pExternalItem;

		while (NULL != pI && pI != pItem)
			pI=pI->parent();

		if (pI == pItem)
		{
			m_pRightPanel->SetExternalItem((pNew == NULL) ? m_pTreeView->firstChild() : pNew);
			m_pRightPanel->ResetRefreshTimer(3);
		}
	}
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnProperties()
{
	DoProperties(FALSE);
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::DoProperties(BOOL bStartFromSharing)
{
	if (ActiveItems.count() > 0)
	{
		CNetworkTreeItem *pTreeItem = ActiveItems.getFirst();

		if (pTreeItem->Kind() == keFileSystemItem ||
			  pTreeItem->Kind() == keLocalFileItem ||
			  pTreeItem->Kind() == keServerItem ||
			  pTreeItem->Kind() == keShareItem ||
			  pTreeItem->Kind() == keFileItem ||
			  pTreeItem->Kind() == keFTPFileItem ||
			  pTreeItem->Kind() == keTrashEntryItem ||
			  pTreeItem->Kind() == keLocalPrinterItem ||
        pTreeItem->Kind() == keDeviceItem)
		{

			if (pTreeItem->Kind() == keLocalFileItem)
			{
				QString FileName = pTreeItem->FullName(FALSE);
				if (FileName.right(7) == ".kdelnk")
				{
#if (QT_VERSION < 200)
          KMimeType::InitStatic();
					KMimeType::init();
					Properties *pProp = new Properties (QString("file:") + FileName, TRUE, FALSE, TRUE);

					pProp->show();
#else
          printf("TODO: implement .kdelnk file properties!\n");
#endif
					return;
				}
			}


			CPropDialog dlg(ActiveItems, bStartFromSharing, this);
			dlg.exec();

      m_pRightPanel->RefreshIcons();
		}
	}
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnRefresh()
{
	// Terminate label editing process if applicable.

  if (IsLabelEditMode())
	{
		m_pAddressBar->setFocus();

		// Sometimes label editor refuses to go away (validation failed, cannot rename etc.).
    // Abort refresh altogether then...

    if (IsLabelEditMode())
			return;
	}

	// Temporarily disable "Refresh" toolbar button and corresponding menu item.

  m_Buttons[12]->setEnabled(0);
	EnableMenuItem(knMENU_VIEW_REFRESH, 0);
	m_ActionList.at(m_nMenuActions+9)->setEnabled(0);
  //m_paViewRefresh->setEnabled(0);

  // Save old tree scrolling position.

  int ContentsX = m_pTreeView->contentsX(); // save scroll offset
	int ContentsY = m_pTreeView->contentsY();

  // Remember whether current tree item was visible before refresh.

  bool bTreeCurrentItemVisible = false;

  if (NULL != m_pTreeView->currentItem())
    bTreeCurrentItemVisible = m_pTreeView->itemRect(m_pTreeView->currentItem()).isValid();

	// Web browser refresh.

  if (m_pRightPanel->IsWebBrowser())
	{
		m_pRightPanel->OnRefreshWebBrowser();

		if (qApp->focusWidget() == m_pRightPanel)
		{
			m_Buttons[12]->setEnabled(1);
      EnableMenuItem(knMENU_VIEW_REFRESH, 1);
			m_ActionList.at(m_nMenuActions+9)->setEnabled(1);
      //m_paViewRefresh->setEnabled(1);
			return;
		}
	}

  // *** Danger area :).  Beginning of the actual refresh code. ***

	void ReadConfiguration();
	ReadConfiguration(); // will also re-read smb.conf
  ReadAutoMountList(gAutoMountList);

	if (gbNetworkAvailable)
		RescanItem(m_pNetworkRoot);

	if (gbShowMyComputer)
    RescanItem(m_pMyComputer);

  RescanItem(m_pHomeRoot);
  RescanItem(m_pNFSRoot);

  if (NULL != m_pMyComputer)
  {
    CListViewItem *pI = m_pMyComputer->listView()->firstChild()->firstChild();

  	for (; NULL != pI; pI = pI->nextSibling())
  	{
  		if (IS_NETWORKTREEITEM(pI))
  		{
  			CNetworkTreeItem *pItem = (CNetworkTreeItem*)pI;

  			if (pItem->Kind() == keFTPSiteItem)
  				RescanItem(pItem);
  		}
  	}
  }

	RescanItem(m_pPrinterRoot);

  // *** Refresh done, try to restore everything back... ***

  // Re-enable toolbar button and menu item.

  m_Buttons[12]->setEnabled(1);
	EnableMenuItem(knMENU_VIEW_REFRESH, 1);
	m_ActionList.at(m_nMenuActions+9)->setEnabled(1);
  //m_paViewRefresh->setEnabled(1);

  // Restore old scrolling position

  m_pTreeView->setContentsPos(ContentsX, ContentsY);

  // If some drastic changes occured in the tree because of refresh, we may have got
  // our current tree item out of sight.
  // If previously our current item was scrolled into view, make sure it is still visible after refresh...

  if (bTreeCurrentItemVisible &&
      NULL != m_pTreeView->currentItem() &&
      !m_pTreeView->itemRect(m_pTreeView->currentItem()).isValid())
  {
    m_pTreeView->ensureItemVisible(m_pTreeView->currentItem());
  }
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnFindFiles()
{
	QString	command = "kfind";

	if (ActiveItems.count() > 0)
	{
		CNetworkTreeItem	*pItem = ActiveItems.getFirst();

		command += " \"";
		command += pItem->FullName( FALSE ); // full name without double-slashes
		command += "\" &";
	}

	system((const char *)command
#ifdef QT_20
  .latin1()
#endif
);
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnFindComputer()
{
	system("kfindcomputer &");
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnSharing()
{
	DoProperties(TRUE);
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnFileCopy()
{
	GetActiveItems();
  FileCopy();
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::FileCopy()
{
	if (m_bBrowserCopy)
	{
		m_pRightPanel->OnCopyBrowserSelection();
		return;
	}

	if (!CanDoOperation(&CMainFrame::CanDoCopy))
		return;

	QListIterator<CNetworkTreeItem> it(ActiveItems);
	QStrList list;

	for (; it.current() != NULL; ++it)
	{
		QString URL = MakeItemURL(it.current());

		list.append((LPCSTR)URL
#ifdef QT_20
  .latin1()
#endif
    );
	}

	CCorelUrlDrag *d = new CCorelUrlDrag(list, this);

#ifdef QT_20
	QApplication::clipboard()->setData(d);
#else
  CCorelClipboard *cb = GetCorelClipboard();
	if (NULL != cb)
		cb->setData(d);
#endif
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnFileCut()
{
	GetActiveItems();
  FileCut();
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::FileCut()
{
  if (!CanDoOperation(&CMainFrame::CanDoCopy) || !CanDoOperation(&CMainFrame::CanDelete))
		return;

	QListIterator<CNetworkTreeItem> it(ActiveItems);
	QStrList list;

	for (; it.current() != NULL; ++it)
	{
		QString URL;
		MakeURL((LPCSTR)it.current()->FullName(FALSE)
#ifdef QT_20
  .latin1()
#endif
    , NULL, URL);

		if (it.current()->Kind() == keFTPFileItem && it.current()->CredentialsIndex() != 1)
		{
			int nCredentialsIndex = it.current()->CredentialsIndex();

			QString a;
			a.sprintf("ftp://%s:%s@%s", (LPCSTR)gCredentials[nCredentialsIndex].m_UserName
#ifdef QT_20
  .latin1()
#endif
      , (LPCSTR)gCredentials[nCredentialsIndex].m_Password
#ifdef QT_20
  .latin1()
#endif
      , ((LPCSTR)URL
#ifdef QT_20
  .latin1()
#endif
      ) + 6);
			URL = a;
		}

		list.append((LPCSTR)URL
#ifdef QT_20
  .latin1()
#endif
    );
	}

	CCorelCutUrlDrag *d = new CCorelCutUrlDrag(list, this);

#ifdef QT_20
  QApplication::clipboard()->setData(d);
#else
  CCorelClipboard *cb = GetCorelClipboard();

	if (NULL != cb)
		cb->setData(d);
#endif
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnFilePaste()
{
	if (!CanPaste())
		return;

	GetActiveItems();

	if (ActiveItems.count() != 1 || !CanPasteHere())
		return;

	FilePaste();
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::FilePaste()
{
	CNetworkTreeItem *pItem = ActiveItems.at(0);

#ifdef QT_20
  QClipboard *cb = QApplication::clipboard();
#else
  CCorelClipboard *cb = GetCorelClipboard();

	if (NULL == cb)
		return; // Oops, no clipboard...
#endif
	CCorelMimeSource *pS = cb->data();

  BOOL bMove = FALSE;

	if (NULL != pS)
	{
		QStrList list;
		QString URL;

		URL = MakeItemURL(pItem);

    if (CCorelUrlDrag::decode(pS, list))
			StartCopyMove(list, (LPCSTR)URL
#ifdef QT_20
  .latin1()
#endif
      ); // copy
    else
		{
			if (CCorelCutUrlDrag::decode(pS, list))
			{
				StartCopyMove(list,(LPCSTR)URL
#ifdef QT_20
  .latin1()
#endif
        , TRUE);	// move
				bMove = TRUE;
			}
			else
			{
        QString s;

        if (CCorelTextDrag::decode(pS, s))
        {
          list.append((LPCSTR)s
#ifdef QT_20
  .latin1()
#endif
          );
          StartCopyMove(list,(LPCSTR) URL
#ifdef QT_20
  .latin1()
#endif
          );	// copy
        }
      }
		}
	}

	if (bMove)
		cb->clear();
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnGoMyComputer()
{
	GoItem(m_pMyComputer);
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnGoNetworkRoot()
{
	if (NULL != m_pNetworkRoot) // FIXME: should we disable this if there's no network?
		GoItem(m_pNetworkRoot);
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnDisconnectShare()
{
	CDisconnectDlg dlg;

	if (QDialog::Accepted == dlg.exec() && dlg.m_pInfo != NULL)
	{
		DisconnectFileSystem(NULL, dlg.m_pInfo);

		// May need to disable menu item now...

		EnableMenuItem(knMENU_DISCONNECT_SHARE, CanDisconnectShare());
	}
}

////////////////////////////////////////////////////////////////////////////

BOOL CMainFrame::CanDisconnectShare()
{
	int i;

	for (i=0; i < gFileSystemList.count(); i++)
	{
		if (gFileSystemList[i].m_Type == "nfs" ||
					gFileSystemList[i].m_Type == "smbfs")
				return TRUE;
	}

	return FALSE;
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnAbout()
{
	QMessageBox::information(this, LoadString(knABOUT_TITLE), LoadString(knABOUT_BODY), LoadString(knOK));
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnHelpTopics()
{
#if (QT_VERSION < 200)
  showClientHelp(kHELP_LOCATION, "");
#else
  printf("TODO: add client help functionality!\n");
#endif
}

////////////////////////////////////////////////////////////////////////////

BOOL CMainFrame::CanDoCopy(CNetworkTreeItem *pItem)
{
	switch (pItem->Kind())
	{
		default:
		break;

    case keLocalFileItem:
    {
      LPCSTR attr = (LPCSTR)pItem->text(2)
#ifdef QT_20
  .latin1()
#endif
      ;

      if (attr[0] == 'b' || attr[0] == 'c')
      {
        return TRUE;
      }

      return attr[0] != 'p' && !access((LPCSTR)pItem->FullName(FALSE)
#ifdef QT_20
  .latin1()
#endif
, R_OK);
		}

		case keFTPFileItem:
		{
			return pItem->text(2)[0] != 'p'; // no named pipes please...
		}

    case keFileItem:
			return TRUE;
	}

	return FALSE;
}

////////////////////////////////////////////////////////////////////////////

BOOL CMainFrame::CanRename(CNetworkTreeItem *pItem)
{
	return pItem != NULL && pItem->CanEditLabel();
}

////////////////////////////////////////////////////////////////////////////

BOOL CMainFrame::CanDelete(CNetworkTreeItem *pItem)
{
	if (pItem->Kind() == keLocalFileItem ||
      pItem->Kind() == keTrashEntryItem)
	{
		QString Path = pItem->FullName(FALSE);

		return CanDeleteLocalPath(Path) && !IsTrashFolder(Path);
	}

	if (pItem->Kind() == keFTPFileItem ||
      pItem->Kind() == keFileItem ||
      (pItem->Kind() == keLocalPrinterItem && IsSuperUser()))
		return TRUE;

	return FALSE;
}

////////////////////////////////////////////////////////////////////////////

BOOL CMainFrame::CanMoveToTrash(CNetworkTreeItem *pItem)
{
	if (pItem->Kind() == keLocalFileItem ||
      pItem->Kind() == keTrashEntryItem)
	{
		QString Path = pItem->FullName(FALSE);

		return CanDeleteLocalPath(Path) && !IsTrashFolder(Path);
	}

	return FALSE;
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnUndo()
{
}

////////////////////////////////////////////////////////////////////////////
// Next two command handlers (SelectAll and InvertSelection):
//
// We use single timer shot because we want menu to disappear and focus to be
// where it used to be.

void CMainFrame::OnSelectAll()
{
	QTimer::singleShot(30, this, SLOT(SelectAll()));
}


////////////////////////////////////////////////////////////////////////////

void CMainFrame::OpenWithActions( const KTrader::OfferList &services )
{
	m_openWithActions.clear();

	KActionSeparator *separator = new KActionSeparator();
  m_openWithActions.append(separator);

	KTrader::OfferList::ConstIterator it = services.begin();
	KTrader::OfferList::ConstIterator end = services.end();
	for (; it != end; ++it )
	{
		QString name = (*it)->name();

		KAction *action = new KAction(i18n( "Open With %1" ).arg( name ), 0, 0, (*it)->name().latin1());

		connect(action, SIGNAL(activated()),
             this, SLOT(slotOpenWith()));

		m_openWithActions.append(action);
	}

}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::slotOpenWith()
{
  KURL::List lst;
  lst.append(KURL(((CNetworkTreeItem*)m_pCurrentItem)->FullName(FALSE)));

  QString serviceName = sender()->name();

  KTrader::OfferList offers = m_PartServices;
  KTrader::OfferList::ConstIterator it = offers.begin();
  KTrader::OfferList::ConstIterator end = offers.end();
  for (; it != end; ++it )
    if ( (*it)->name() == serviceName )
    {
      KRun::run( **it, lst );
      return;
    }
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::SelectAll()
{
	if (!m_pRightPanel->m_bHasFocus)
		m_pRightPanel->viewport()->setFocus();

	m_pRightPanel->SelectAll();
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnInvertSelection()
{
	QTimer::singleShot(30, this, SLOT(InvertSelection()));
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::InvertSelection()
{
	if (!m_pRightPanel->m_bHasFocus)
		m_pRightPanel->viewport()->setFocus();

	m_pRightPanel->InvertSelection();
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnViewStatusBar()
{
	if (gbShowStatusBar)
	{
		m_pStatusBar->hide();
		CheckMenuItem(knMENU_VIEW_STATUS_BAR, FALSE);
	}
	else
	{
		m_pStatusBar->show();
		CheckMenuItem(knMENU_VIEW_STATUS_BAR, TRUE);
	}

	QTimer::singleShot(100, this, SLOT(SetSplitterMinSize()));
  gbShowStatusBar = !gbShowStatusBar;
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnViewToolBar()
{
  slotShowToolBar();
  /*
  if (gbShowToolBar)
	{
		removeToolBar(m_pToolBar);
		m_pToolBar->lower();
		CheckMenuItem(knMENU_VIEW_TOOLBARS, FALSE);
	}
	else
	{
		if (gbShowAddressBar)
		{
			removeToolBar(m_pAddressBar);
			m_pAddressBar->lower();
		}

		addToolBar(m_pToolBar, "ButtonBar", Top, TRUE);
		m_pToolBar->raise();

		if (gbShowAddressBar)
		{
			addToolBar(m_pAddressBar,"AddressBar",Top,TRUE);
			m_pAddressBar->raise();
		}

		CheckMenuItem(knMENU_VIEW_TOOLBARS, TRUE);
	}

	gbShowToolBar = !gbShowToolBar;
	QTimer::singleShot(100, this, SLOT(SetSplitterMinSize()));
  */
}

void CMainFrame::OnViewAddressBar()
{
	if (gbShowAddressBar)
	{
		removeToolBar(m_pAddressBar);
		m_pAddressBar->lower();
		CheckMenuItem(knMENU_VIEW_ADDRESS_BAR, FALSE);
	}
	else
	{
		addToolBar(m_pAddressBar,"AddressBar",Top,TRUE);
		m_pAddressBar->raise();
		CheckMenuItem(knMENU_VIEW_ADDRESS_BAR, TRUE);
	}

	gbShowAddressBar = !gbShowAddressBar;
	QTimer::singleShot(100, this, SLOT(SetSplitterMinSize()));
}

////////////////////////////////////////////////////////////////////////////

BOOL CMainFrame::CanPaste()
{
#ifdef QT_20
  QClipboard *cb = QApplication::clipboard();
#else
  CCorelClipboard *cb = GetCorelClipboard();
#endif

  if (NULL != cb &&
      NULL != cb->data())
  {
    if (CCorelUrlDrag::canDecode(cb->data()) ||
        CCorelCutUrlDrag::canDecode(cb->data()))
    {
      return TRUE;
    }

    QString s;

    if (CCorelTextDrag::decode(cb->data(), s) &&
        s.length() > 6 &&
        !strnicmp((LPCSTR)s
#ifdef QT_20
  .latin1()
#endif
        , "file:/", 6))
    {
      return TRUE;
    }
  }
  return FALSE;
}

////////////////////////////////////////////////////////////////////////////

BOOL CMainFrame::CanPasteHere()
{
	return TRUE;
	//return FALSE;
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnItemRenamed(CListViewItem *pItem)
{
	if (m_pTreeView->currentItem() == pItem)
	{
		/* Fix toolbar combo */

		LPCSTR s = (LPCSTR)pItem->text(0)
#ifdef QT_20
  .latin1()
#endif
    ;

		QString ComboText(IS_NETWORKTREEITEM(pItem) ? (LPCSTR)((CNetworkTreeItem*)pItem)->FullName(FALSE)
#ifdef QT_20
  .latin1()
#endif
: s);

		if (ComboText.isEmpty())
			ComboText = s;

		m_pAddressCombo->setCurrentItem(0);
		m_pAddressCombo->setEditText((LPCSTR)ComboText
#ifdef QT_20
  .latin1()
#endif
    );

    QListBox *lb = m_pAddressCombo->listBox();

    lb->removeItem(0);
		lb->insertItem(new CListBoxItem(m_pAddressCombo->currentTextUI(), *(pItem->pixmap(0)), (void*)m_pAddressCombo->HiddenPrefix()), 0);

		CHistory::Instance()->SetVisited((LPCSTR)ComboText
#ifdef QT_20
  .latin1()
#endif
    );
		m_pAddressCombo->SetPixmap(pItem->pixmap(0));
	}
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnTabRequest(BOOL bIsBackTab)
{
	if (m_pAddressCombo->hasFocus())
	{
		if (bIsBackTab)
		{
			if (m_pRightPanel->IsWebBrowser())
#ifndef QT_20
				m_pRightPanel->m_pHTMLView->setFocus();
#else
     ;
#endif
			else
				m_pRightPanel->viewport()->setFocus();
		}
		else
			m_pTreeView->viewport()->setFocus();
	}
	else
	{
		if (qApp->focusWidget() == m_pRightPanel)
		{
			if (bIsBackTab)
				m_pTreeView->viewport()->setFocus();
			else
				m_pAddressCombo->setFocus();
		}
		else
		{
			if (qApp->focusWidget() == m_pTreeView)
			{
				if (bIsBackTab)
					m_pAddressCombo->setFocus();
				else
				{
					if (m_pRightPanel->IsWebBrowser())
#ifndef QT_20
						m_pRightPanel->m_pHTMLView->setFocus();
#else
     ;
#endif
					else
						m_pRightPanel->viewport()->setFocus();
				}
			}
			else
				m_pTreeView->viewport()->setFocus();
		}
	}
}

////////////////////////////////////////////////////////////////////////////

CListViewItem *CMainFrame::GetCurrentItem()
{
	if (qApp->focusWidget() == m_pRightPanel && m_pRightPanel->currentItem() != NULL)
		return m_pRightPanel->currentItem();
	else
		return m_pTreeView->currentItem();
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::SizeInit()
{
	// This is a hack workaround for a Qt problem.
	// Qt geometry management routines to not allow main widget to be narrower than the toolbar,
	// as opposed to Microsoft Windows. That will cause major inconvenience to the user as it will
	// be impossible to decrease Corel Explorer window width below huge default size.
	// Unfortunately, Qt is not cooperating here as all the functions which have to be modofied
	// to alter that behavior are private and/or non-virtual.
	// So we resort to the timer-driven solution here.

	setMinimumSize(10,10);

//	move(gnDesiredAppX, gnDesiredAppY);
//	resize(gnDesiredAppWidth, gnDesiredAppHeight);

	if (!gsStartAddress.isEmpty())
	{
		GoItem(m_pHomeRoot->m_pLogicalParent);
		cout<<"After GoItem"<<endl;
		m_pRightPanel->ActivateEvent();

    m_pAddressCombo->setEditText(gsStartAddress);
		ComboReturnPressed();
	}

	cout<<"In the middle of the sizeinit"<<endl;
  if (gbShowTree)
  {
    gbShowTree = 0;
    OnViewTree();
    m_pTreeView->setContentsPos(0,0);
  }

	m_pRightPanel->ActivateEvent();
}

////////////////////////////////////////////////////////////////////////////

#ifdef QT_20
#define Event_ChildRemoved QEvent::ChildRemoved
#define Event_ChildInserted QEvent::ChildInserted
#endif

bool CMainFrame::event(QEvent *e)
{
  if (e->type() == Event_ChildRemoved || e->type() == Event_ChildInserted)
    QTimer::singleShot(100, this, SLOT(SetSplitterMinSize()));

  return QMainWindow::event(e);
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::SetSplitterMinSize()
{
	setMinimumSize(10,10);
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnLastWindowClosed()
{
	disconnect(qApp, SIGNAL(lastWindowClosed()), this, SLOT(OnLastWindowClosed()));

  if (gbNeedSaveConfigSettings)
  {
#if (QT_VERSION < 200)
    SaveConfigSettings(((KApplication *)qApp)->getConfig(), FALSE);
#else
    SaveConfigSettings(KGlobal::config(), FALSE);
#endif
  }

  gbStopping = TRUE;
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnLogoClicked()
{
	GoItem(ksEXPLORER_ICON_LINK);
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnShowHiddenFiles()
{
  gbShowHiddenFiles = !gbShowHiddenFiles;
  gbShowHiddenShares = gbShowHiddenFiles;

  CheckMenuItem(knMENU_SHOW_HIDDEN_FILES, gbShowHiddenFiles);

	OnRefresh();
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnStartRescanItem(CListViewItem *pItem)
{
  m_SavedChain.clear();
	m_SavedTreeSelection.clear();

	if (m_pRightPanel->m_pExternalItem != NULL)
	{
		CListViewItem *pI = m_pRightPanel->m_pExternalItem->parent();

		while (NULL != pI && pI != pItem)
			pI = pI->parent();

		if (NULL != pI)
		{
			GetChainForItem(pItem->listView(), m_pRightPanel->m_pExternalItem, m_SavedChain);

			m_pRightPanel->m_pExternalItem = NULL;
		}
	}

	if (NULL != m_pTreeView->currentItem())
	{
		GetChainForItem(m_pTreeView, m_pTreeView->currentItem(), m_SavedTreeSelection);
		m_bSavedTreeItemSelected = m_pTreeView->currentItem()->isSelected();
	}
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnEndRescanItem(CListViewItem *pItem)
{
	if (m_SavedChain.count() > 0)
	{
		CListViewItem *pI = GetItemFromChain(pItem->listView(), m_SavedChain);

    m_pRightPanel->m_pExternalItem = pI;
    m_pRightPanel->ResetRefreshTimer(1);
    m_pRightPanel->ActivateEvent();

		if (NULL == pI)
		{
			m_pRightPanel->clear();
		}
		else
		{
			CNetworkTreeItem *pI;

			for (pI=(CNetworkTreeItem*)m_pRightPanel->firstChild(); NULL != pI; pI=(CNetworkTreeItem*)pI->nextSibling())
			{
				pI->m_pLogicalParent = m_pRightPanel->m_pExternalItem;
			}
		}

		m_SavedChain.clear();
	}

	if (m_SavedTreeSelection.count() > 0)
	{
		CListViewItem *pI = GetItemFromChain(m_pTreeView, m_SavedTreeSelection);

		if (NULL != pI)
		{
			m_pTreeView->setCurrentItem(pI);
			m_pTreeView->setSelected(pI, m_bSavedTreeItemSelected);
		}

		m_SavedTreeSelection.clear();
	}

	GetActiveItems();
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnSambaConfigurationChanged()
{
	RefreshHandPixmaps(m_pTreeView);

	CNetworkTreeItem *pI;

	for (pI = (CNetworkTreeItem*)m_pRightPanel->firstChild(); NULL != pI; pI = (CNetworkTreeItem*)pI->nextSibling())
	{
		if (pI->Kind() == keLocalFileItem &&
				((CLocalFileItem*)pI)->IsFolder())
		{
			pI->InitPixmap();
		}
	}
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnErrorAccessingURL(const char *Url)
{
  CWebPageItem *pItem;

	BOOL bNeedGoBack = FALSE;

	for (pItem = (CWebPageItem *)m_pWebRoot->firstChild(); NULL != pItem; pItem = (CWebPageItem *)pItem->nextSibling())
	{
    if (pItem->FullName(FALSE) == Url)
		{
			// Remove this thing from history

			QListIterator<CHistoryItem> it(gHistory);

			QStrList Chain;

			GetChainForItem(m_pTreeView, pItem, Chain);

			int n = gHistory.count()-1;

			for (it.toLast(); NULL != it.current(); n--)
			{
				QStrList *pEntry = &it.current()->getStrList();
				--it;

				if (CompareChains(*pEntry, Chain))
				{
					gHistory.remove(it.current());

					if (gnCanForward <= n)
					{
						gnCanForward--;
					}
				}
			}

			// Remove it from the address combo

			QListBox *lb = m_pAddressCombo->listBox();

			/* Remove same item from the list box if it already was there */

			uint i;

			for (i=0; i < lb->count(); i++)
			{
				if (!strcmp((LPCSTR)lb->text(i)
#ifdef QT_20
  .latin1()
#endif
        , Url))
				{
					lb->removeItem(i);
					break;
				}
			}

			// Update right panel

			if (m_pRightPanel->m_pExternalItem == pItem)
			{
				bNeedGoBack = TRUE;
				m_pRightPanel->SetExternalItem(m_pWebRoot);
			}
//			cout<<"before delete in error"<<endl;
			delete pItem;

			m_pWebRoot->setExpandable(m_pWebRoot->childCount() > 0);
			//m_pWebRoot->
			break;
		}
	}

	QString message;
	message.sprintf(LoadString(knERROR_ACCESSING_URL_X), (LPCSTR) Url);
	QMessageBox::warning(qApp->mainWidget(), LoadString(knAPP_TITLE), message);

	if (bNeedGoBack)
		OnBack();
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnBookmarkURL(const char *Title, const char *Url)
{
	LoadBookmarks();
	m_BookmarkList.append(new SBookmark(Title, Url));
	SaveBookmarks();
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnBrowserTextSelected(BOOL /*bSelected*/)
{
	UpdateButtons();
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnDocumentDone(const char *InitialURL, const char *NewURL, BOOL bIntranet)
{
	if (bIntranet)
	{
		m_pZoneLabel->setText(LoadString(knLOCAL_INTRANET_ZONE));
		m_pZoneLabel->setPixmap(*LoadPixmap(keIntranetIcon));
	}

	if (!strnicmp(InitialURL, "file:", 5))
	{
		InitialURL += 5;
		NewURL += 5;
	}

	// If the user hasn't changed the text in the meantime, we overwrite it.
	if (!(stricmp((LPCSTR)m_pAddressCombo->currentText()
#ifdef QT_20
  .latin1()
#endif
  , InitialURL))
			&& stricmp(InitialURL, NewURL))
	{
		CHistory::Instance()->SetVisited(NewURL);
		m_pAddressCombo->setEditText(NewURL);
	}
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnViewMyComputer()
{
  if (gbShowMyComputer)
  {
    CListViewItem *pI = m_pRightPanel->m_pExternalItem;

    while (CNetworkTreeItem::IsMyComputerItem(pI))
    {
      pI = ((CNetworkTreeItem*)pI)->m_pLogicalParent;
    }

    if (pI != m_pRightPanel->m_pExternalItem)
    {
	    ActiveItems.clear();
      m_pTreeView->setCurrentItem(pI);
	    m_pTreeView->setSelected(pI, TRUE);
      m_pRightPanel->SetExternalItem(pI);
      m_pRightPanel->ActivateEvent();
      OnTreeSelectionChanged(pI);
    }

    delete m_pMyComputer;
    m_pMyComputer = NULL;
  }
  else
    m_pMyComputer = new CMyComputerItem(((CListViewItem*)m_pHomeRoot)->parent());

  gbShowMyComputer = !gbShowMyComputer;
  CheckMenuItem(knMENU_VIEW_MYCOMPUTER, gbShowMyComputer);
  EnableMenuItem(knMENU_GO_MYCOMPUTER, gbShowMyComputer);
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnFileRestore()
{
  QString OriginalLocation;
  QString URL;

  while (ActiveItems.count() > 0)
  {
	  QStrList list;
	  QListIterator<CNetworkTreeItem> it(ActiveItems);

DoAgain:;

    for (it.toLast(); it.current() != NULL; --it)
  	{
      CTrashEntryItem *tei = (CTrashEntryItem*)it.current();

      QString thisLocation = tei->m_OriginalLocation;
      int nIndex = thisLocation.findRev('/');

      if (-1 != nIndex)
        thisLocation = thisLocation.left(nIndex);

      if (OriginalLocation.isEmpty())
        OriginalLocation = thisLocation;

      if (thisLocation == OriginalLocation)
      {
        QString URL;

        MakeURL(tei->FullName(FALSE), NULL, URL);
   		  list.append(URL);
        ActiveItems.removeRef(tei);
        goto DoAgain;
      }
  	}

    MakeURL(OriginalLocation, NULL, URL);
    OriginalLocation = "";

    StartCopyMove(list, URL, TRUE); // move, prompt for overwrite
  }
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnEmptyDumpster()
{
  EmptyTrash();
}

////////////////////////////////////////////////////////////////////////////

BOOL CMainFrame::HandleDumpsterMenu() // returns TRUE if we're in dumpster
{
  BOOL bIsTrash = FALSE;
  BOOL bIsMyTrash = FALSE;

  CListViewItem *pItem = m_pTreeView->currentItem();

  if (NULL == pItem)
    return FALSE;

  if (IS_NETWORKTREEITEM(pItem))
  {
    QString Name = ((CNetworkTreeItem*)pItem)->FullName(FALSE);
    kdDebug(1000)<<"Name in HandleDumpsterMenu ="<<Name<<endl;
    bIsTrash = IsTrashFolder(Name);

    if (bIsTrash)
    {
      bIsMyTrash = IsMyTrashFolder(Name);
    }
  }

  QPopupMenu *pMenu = m_MainMenu[0].m_pMenu;
  BOOL bHasItems = (pMenu->indexOf(knMENU_FILE_RESTORE) != -1);

  if (bIsTrash)
  {
    if (!bHasItems)
    {
			unplugActionList("trash");
			m_Trash.clear();
			KAction *action = new KAction(LoadString(knEMPTY_DUMPSTER), 0, this, SLOT(OnEmptyDumpster()));
			m_Trash.append(action);
      KActionSeparator * separator = new KActionSeparator();
			m_Trash.append(separator);
			action = new KAction(LoadString(knRESTORE), 0, this, SLOT(OnFileRestore()));
			m_Trash.append(action);
			m_Trash.append(separator);
			plugActionList("trash",m_Trash);

      pMenu->insertSeparator(0);
      pMenu->insertItem(LoadString(knEMPTY_DUMPSTER), this, SLOT(OnEmptyDumpster()), 0, knMENU_FILE_EMPTY_DUMPSTER, 0);
      pMenu->insertSeparator(0);
      pMenu->insertItem(LoadString(knRESTORE), this, SLOT(OnFileRestore()), 0, knMENU_FILE_RESTORE, 0);
    }

    pMenu->setItemEnabled(knMENU_FILE_DELETE, FALSE);

    pMenu->setItemEnabled(knMENU_FILE_NUKE, FALSE);

    pMenu->setItemEnabled(knMENU_FILE_RENAME, FALSE);

    pMenu->setItemEnabled(knMENU_FILE_EMPTY_DUMPSTER,
                          NULL != m_pRightPanel->firstChild() &&
                          bIsMyTrash &&
                          !IsTrashEmpty());
  }
  else
  {
    if (bHasItems)
    {
			unplugActionList("trash");
      pMenu->removeItem(knMENU_FILE_RESTORE);
      pMenu->removeItem(knMENU_FILE_EMPTY_DUMPSTER);
      pMenu->removeItemAt(0);
      pMenu->removeItemAt(0);
    }
  }

  return bIsTrash;
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::AddDeviceItems(CListViewItem *pParentItem)
{
  // Step1: Read "/etc/devices" and find out what CD-ROM and ZIP drives we have

  CSectionList list;

  if (list.Read("/etc/devices"))
  {
  	QListIterator<CSection> it(list);

    int nCDCount = 0;
    int nZIPCount = 0;

    for (; it.current() != NULL; ++it)
  	{
      CSection *pSection = it.current();

      if (!strncmp((LPCSTR)pSection->m_SectionName
#ifdef QT_20
  .latin1()
#endif
      , "cdrom.", 6))
      {
        nCDCount++;
      }
      if (!strncmp((LPCSTR)pSection->m_SectionName
#ifdef QT_20
  .latin1()
#endif
      , "zip.", 4))
      {
        nZIPCount++;
      }
    }

  	int nCount=0;

    for (it.toFirst(); it.current() != NULL; ++it)
  	{
      CSection *pSection = it.current();

      if (!strncmp((LPCSTR)pSection->m_SectionName
#ifdef QT_20
  .latin1()
#endif
      , "cdrom.", 6))
      {
       	CDeviceInfo info;

        if (nCDCount == 1)
        {
          info.m_DisplayName = LoadString(knCD_ROM);
        }
        else
	{
	  QString strFmt(LoadString(knCD_ROM_X));
          info.m_DisplayName.sprintf(strFmt, ++nCount);
	}

        info.m_Device = pSection->Value("device");
        info.m_Driver = pSection->Value("driver");
        info.m_Model = pSection->Value("model");
        info.m_Manufacturer = pSection->Value("manu");
        info.m_Description = pSection->Value("desc");
        info.m_AutoMountLocation = FindAutoMountEntry(gAutoMountList, (LPCSTR)info.m_Device
#ifdef QT_20
  .latin1()
#endif
        );
	      info.m_DriveType = keCdromDrive;
        //printf("Device: %s, AutoMount = %s\n", (LPCSTR)info.m_Device, (LPCSTR)info.m_AutoMountLocation);
        /*CDeviceItem *pCDROM =*/ new CDeviceItem(pParentItem, &info);
      }
  	}

    nCount = 0;

    for (it.toFirst(); it.current() != NULL; ++it)
  	{
      CSection *pSection = it.current();

      if (!strncmp((LPCSTR)pSection->m_SectionName
#ifdef QT_20
  .latin1()
#endif
      , "zip.", 4))
      {
       	CDeviceInfo info;

        if (nZIPCount == 1)
        {
          info.m_DisplayName = LoadString(knZIP_DRIVE);
        }
        else
	{
	  QString strFmt(LoadString(knZIP_DRIVE_X));
          info.m_DisplayName.sprintf(strFmt, ++nCount);
	}

        info.m_Device = pSection->Value("device");
        info.m_Driver = pSection->Value("driver");
        info.m_Model = pSection->Value("model");
        info.m_Manufacturer = pSection->Value("manu");
        info.m_Description = pSection->Value("desc");
        info.m_AutoMountLocation = FindAutoMountEntry(gAutoMountList, (LPCSTR)info.m_Device
#ifdef QT_20
  .latin1()
#endif
);
	      info.m_DriveType = keZIPDrive;
        new CDeviceItem(pParentItem, &info);
      }
  	}
  }

  // step 2: use automount config file to find out about floppy drives

  QListIterator<CAutoMountEntry> it(gAutoMountList);
  CVector<dev_t, dev_t&> FoundList;

  int nFloppyCount = 0;
  QString strFirstFloppyName;

  for (it.toFirst(); NULL != it.current(); ++it)
  {
    struct stat st;

    if (!stat((LPCSTR)it.current()->m_Device
#ifdef QT_20
  .latin1()
#endif
    , &st) &&
        major(st.st_rdev) == 2)
    {
      int i;

      for (i=FoundList.count()-1; i >=0; i--)
      {
        if (FoundList[i] == st.st_rdev)
          break;
      }

      if (i < 0)
      {
        //printf("New floppy found: %s\n", (LPCSTR)it.current()->m_MountLocation);
        FoundList.Add(st.st_rdev);

        CDeviceInfo info;

        QString strFmt(LoadString(knFLOPPY_X));
	info.m_DisplayName.sprintf(strFmt, ++nFloppyCount);

        if (1 == nFloppyCount)
        {
          strFirstFloppyName = info.m_DisplayName;
        }

        info.m_Device = it.current()->m_Device;
        info.m_Description = "Floppy";
        info.m_AutoMountLocation = it.current()->m_MountLocation;
	      info.m_DriveType = keFloppyDrive;

        //printf("Device: %s, AutoMount = %s\n", (LPCSTR)info.m_Device, (LPCSTR)info.m_AutoMountLocation);

        /*CDeviceItem *pFloppy =*/
				new CDeviceItem(pParentItem, &info);
      }
      //else
//        printf("Duplicate entry\n");
    }
  }

  if (1 == nFloppyCount)
  {
    strFirstFloppyName = LoadString(knFLOPPY);
  }
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnSaveSession()
{
#if (QT_VERSION < 200)
  SaveConfigSettings(((KApplication*)qApp)->getSessionConfig(), TRUE);
#else
  SaveConfigSettings(KGlobal::config(), TRUE); // TODO: is it right?
#endif
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::SaveConfigSettings(KConfig *pConfig, BOOL bSavePath)
{
	QString ConfigPath(GetHomeDir());

  if (ConfigPath.right(1) != "/")
    ConfigPath += "/";

  ConfigPath += ".kde/share/config/CorelExplorerrc";

  struct stat st;

  if (!stat((LPCSTR)ConfigPath, &st) && getuid() != st.st_uid)
    unlink((LPCSTR)ConfigPath);

  pConfig->setGroup("CorelExplorer");
	QRect r = frameGeometry();

  QString s;
  s.sprintf("%d", r.width());
  pConfig->writeEntry("width", (LPCSTR)s
#ifdef QT_20
  .latin1()
#endif
  );

  s.sprintf("%d", r.height());
  pConfig->writeEntry("height", (LPCSTR)s
#ifdef QT_20
  .latin1()
#endif
  );

  s.sprintf("%d", r.left());
  pConfig->writeEntry("x", (LPCSTR)s
#ifdef QT_20
  .latin1()
#endif
  );

  s.sprintf("%d", r.top());
  pConfig->writeEntry("y", (LPCSTR)s
#ifdef QT_20
  .latin1()
#endif
  );

  s.sprintf("%d", m_pTreeView->frameGeometry().width());
  pConfig->writeEntry("treewidth", (LPCSTR)s
#ifdef QT_20
  .latin1()
#endif
  );

  pConfig->writeEntry("bigicons", gbUseBigIcons ? "1" : "0");

  pConfig->writeEntry("showtree", gbShowTree ? "1" : "0");

  pConfig->writeEntry("addressbar", gbShowAddressBar ? "1" : "0");

  pConfig->writeEntry("toolbar", gbShowToolBar ? "1" : "0");

  pConfig->writeEntry("statusbar", gbShowStatusBar ? "1" : "0");

	pConfig->writeEntry("hiddenfiles", gbShowHiddenFiles ? "1" : "0");

  pConfig->writeEntry("showsystem", gbShowMyComputer ? "1" : "0");

	if (bSavePath && NULL != m_pAddressCombo)
	{
		pConfig->writeEntry("path", (LPCSTR)m_pAddressCombo->currentText()
#ifdef QT_20
  .latin1()
#endif
    );
	}

	s.sprintf("%u", getuid());
	pConfig->writeEntry("uid", (LPCSTR)s
#ifdef QT_20
  .latin1()
#endif
  );

	pConfig->sync();
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::LoadConfigSettings(KConfig *pConfig)
{
  pConfig->setGroup("CorelExplorer");

  QString s;

  if (-1 == gnDesiredAppWidth)
  {
    s = pConfig->readEntry("width", "-1");
    sscanf((LPCSTR)s, "%d", &gnDesiredAppWidth);
  }

  if (-1 == gnDesiredAppHeight)
  {
    s = pConfig->readEntry("height", "-1");
    sscanf((LPCSTR)s, "%d", &gnDesiredAppHeight);
  }

  if (-1 == gnDesiredAppX)
  {
    s = pConfig->readEntry("x", "-1");
    sscanf((LPCSTR)s, "%d", &gnDesiredAppX);
  }

  if (-1 == gnDesiredAppY)
  {
    s = pConfig->readEntry("y", "-1");
    sscanf((LPCSTR)s, "%d", &gnDesiredAppY);
  }

  if (-1 == gnDesiredTreeWidth)
  {
    s = pConfig->readEntry("treewidth", "-1");
    sscanf((LPCSTR)s, "%d", &gnDesiredTreeWidth);
  }

  if (-1 == gbUseBigIcons)
  {
    s = pConfig->readEntry("bigicons", "0");
    sscanf((LPCSTR)s, "%d", &gbUseBigIcons);
  }

  if (-1 == gbShowTree)
  {
    s = pConfig->readEntry("showtree", "1");
    sscanf((LPCSTR)s, "%d", &gbShowTree);
  }

  if (-1 == gbShowAddressBar)
  {
    s = pConfig->readEntry("addressbar", "1");
    sscanf((LPCSTR)s, "%d", &gbShowAddressBar);
  }

  if (-1 == gbShowToolBar)
  {
    s = pConfig->readEntry("toolbar", "1");
    sscanf((LPCSTR)s, "%d", &gbShowToolBar);
  }

  if (-1 == gbShowStatusBar)
  {
    s = pConfig->readEntry("statusbar", "1");
    sscanf((LPCSTR)s, "%d", &gbShowStatusBar);
  }

	if (-1 == gbShowHiddenFiles)
  {
    s = pConfig->readEntry("hiddenfiles", "0");
    sscanf((LPCSTR)s, "%d", &gbShowHiddenFiles);
		gbShowHiddenShares = gbShowHiddenFiles;
  }

  if (-1 == gbShowMyComputer)
  {
    s = pConfig->readEntry("showsystem", "-1");
    sscanf((LPCSTR)s, "%d", &gbShowMyComputer);
  }

	s = pConfig->readEntry("uid", "-1");

	if (s != "-1")
	{
		uid_t uid;
		sscanf((LPCSTR)s, "%u", &uid);

		if (!uid &&
        uid != getuid() &&
       ((KApplication *)qApp)->isRestored())
		{
			exit(-1); // don't start if it was in root mode last time...
		}
	}

	if (gsStartAddress.isEmpty())
		gsStartAddress = pConfig->readEntry("path", GetHomeDir());
}

////////////////////////////////////////////////////////////////////////////

BOOL CMainFrame::IsLabelEditMode()
{
	return
		(NULL != FindChildByName(m_pTreeView->viewport(), "LabelEdit")) ||
		(NULL != FindChildByName(m_pRightPanel->viewport(), "LabelEdit"));
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::LoadBookmarks()
{
	char Link[SBookmark::LinkMaxLength + 1];
	char Title[SBookmark::TitleMaxLength + 1];
	QString BookmarkName = getenv("HOME");
	BookmarkName += "/.CorExpBookmarks";

	FILE *BookmarkFile = fopen((LPCSTR)BookmarkName
#ifdef QT_20
  .latin1()
#endif

  , "r");
	if (BookmarkFile == NULL)
		return;

	m_BookmarkList.clear();

	while (fscanf(BookmarkFile, "%s %[^\n]", Link, Title) == 2)
	{
		m_BookmarkList.append(new SBookmark(Title, Link));
	}

	fclose(BookmarkFile);
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::SaveBookmarks()
{
	SBookmark *Current;
	QString BookmarkName = getenv("HOME");
	BookmarkName += "/.CorExpBookmarks";

	FILE *BookmarkFile = fopen((LPCSTR)BookmarkName
#ifdef QT_20
  .latin1()
#endif
  , "w");
	if (BookmarkFile == NULL)
		return;

	Current = m_BookmarkList.first();
	while (Current != NULL)
	{
		if (Current->Link.find(' ') == -1)
		{
			fprintf(BookmarkFile, "%s %s\n", (LPCSTR) Current->Link
#ifdef QT_20
  .latin1()
#endif
      ,
					(LPCSTR) Current->Title
#ifdef QT_20
  .latin1()
#endif
          );
		}
		Current = m_BookmarkList.next();
	}

	fclose(BookmarkFile);
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnTreeDoubleClicked(CListViewItem* pItem)
{
  cout<<"pItem->text(0)="<<pItem->text(0)<<endl;
  OnTreeSelectionChanged(pItem);
  m_pRightPanel->ActivateEvent();
}

////////////////////////////////////////////////////////////////////////////


void CMainFrame::OnStatusMessage(const char *p)
{
#ifdef QT_20
  m_pStatusBar->message(QString(p));
#endif
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnAddPrinter()
{
  LPCSTR Param[3];

  Param[0] = "CopyAgent";
  Param[1]= "addprinter";
  Param[2] = NULL;

  LaunchProgram((char *const *)Param);
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnPurge()
{
	GetActiveItems();
	Purge();
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::Purge()
{
	QListIterator<CNetworkTreeItem> it(ActiveItems);

	for (; it.current() != NULL; ++it)
	{
		/*CSMBErrorCode rc =*/
		PurgePrinter((LPCSTR)it.current()->text(0)
#ifdef QT_20
  .latin1()
#endif
    );
	}
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnDeletePrintJob()
{
	GetActiveItems();
	DeletePrintJob();
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::DeletePrintJob()
{
	QListIterator<CNetworkTreeItem> it(ActiveItems);

	for (; it.current() != NULL; ++it)
	{
		CPrintJobItem *pItem = (CPrintJobItem *)it.current();
    RemovePrintJob((LPCSTR)pItem->m_pLogicalParent->text(0)
#ifdef QT_20
  .latin1()
#endif

    , (CPrintJobInfo *)pItem);
	}
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnOpenConsole()
{
	QListIterator<CNetworkTreeItem> it(ActiveItems);

	for (; it.current() != NULL; ++it)
	{
		OpenConsole(it.current());
	}

	//OpenConsole(m_pRightPanel->m_pExternalItem);
}

////////////////////////////////////////////////////////////////////////////

void CMainFrame::OpenConsole(CListViewItem *pI)
{
  char OldPath[MAXPATHLEN+1];
  getcwd(OldPath, sizeof(OldPath));

//  QString Path = KApplication::kde_bindir();
#if (QT_VERSION >= 200)
    QString Path = KGlobal::dirs()->findResource("exe", "konsole");
#else
    QString Path = KApplication::kde_bindir();

		if (Path.right(1) != "/")
			Path += "/";

		Path += "konsole";
#endif

  if (NULL != pI &&
      IS_NETWORKTREEITEM(pI) &&
      ((CNetworkTreeItem *)pI)->Kind() == keLocalFileItem)
  {
		chdir((LPCSTR)((CNetworkTreeItem*)pI)->FullName(FALSE)
#ifdef QT_20
  .latin1()
#endif
    );
  }

  QString cmd(QByteArray(1024));
  cmd.sprintf("%s &", (LPCSTR)Path
#ifdef QT_20
  .latin1()
#endif
  );
  system((LPCSTR)cmd
#ifdef QT_20
  .latin1()
#endif
  );

  chdir(OldPath);
}

