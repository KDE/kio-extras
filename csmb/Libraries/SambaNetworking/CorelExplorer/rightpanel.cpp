/* Name: rightpanel.cpp

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

////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <iostream.h>
#include "common.h"
#include "khtmlview.h"
#include "almosthtmlview.h"
#include "topcombo.h"
#include "history.h"
#include "rightpanel.h"
#include "smbworkgroup.h"
#include "smbserver.h"
#include "smbshare.h"
#include "smbfile.h"
#include "ftpfile.h"
#include "smbutil.h"
#include <stdlib.h> /* for atoi() and system() */
#include <ctype.h>
#include <qmovie.h>
#include "resource.h"
#include "filesystem.h"
#include <qevent.h>
#include "explres.h"
#include "propres.h"
#include "PasswordDlg.h"
#include "qkeycode.h"
#include "qevent.h"
#include <time.h>
#include "copymove.h"
#include "qapplication.h"
#include <qobjcoll.h>
#include "qlineedit.h"
#include "qpainter.h"
#include "qdatetime.h"
#include "expcommon.h"
#include "filerun.h"
#include "header.h"
#include "nfsserver.h"
#include "nfsshare.h"
#include "mainfrm.h"
#include "qbitmap.h"
#include "trashentry.h"
#ifdef QT_20
#include "qclipboard.h"
#include "q1xcompatibility.h"
#include <qvariant.h>
#include <khtml_part.h>
#include <kmimetype.h>
#include <ktrader.h>
#include <klibloader.h>
#include <sys/stat.h>
#include <unistd.h>
#include <qfile.h>
#include "kparts/browserextension.h"
#include <kxmlguiclient.h>
#define findView findFrame
#else
#include "corelclipboard.h"
#include "kbind.h"
#endif
#include <qfiledialog.h>
#if (QT_VERSION < 200)
#include <qmessagebox.h>
#endif
#include <unistd.h> /* for access() */
#include <sys/wait.h>   /* for W_EXITCODE() */
#include <kapp.h>
#include "browsercache.h"
#include <qregexp.h>
#include <qasyncimageio.h>
#include "device.h"
#include "dropselector.h"
#include "qfileinfo.h"
#include <errno.h>
#include "printer.h"
#include "printjob.h"

extern QList<QStrList> gHistory;
extern int gnCanForward;
extern int gnGoCount;
extern QString gsExtraCaption;

BOOL CanDoFileCopy(); // in mainfrm.cpp

QWidget *GetMenuBar();
void OnDocumentLoaded(int status, void *pParam); //forward declaration
void OnImageLoaded(int status, void *pParam); //forward declaration
void OnImageToSaveLoaded(int status, void *pParam); //forward declaration

extern BOOL gbShowTree;
extern BOOL gbUseBigIcons;
extern BOOL gbShowAddressBar;

extern int gnDesiredAppWidth;
extern int gnDesiredAppHeight;
extern int gnDesiredAppX;
extern int gnDesiredAppY;
extern int gnWebOperationsCount;
extern int gbUseBigIcons;

pid_t gMainPID = getpid();

//static BOOL bIntranet = FALSE;
static BOOL bLastRedirectionFailed = FALSE;
static bool bInside = false;

/* The following flags are used as return values for CopyAgent. */

#define F_INTRANET	W_EXITCODE(1, 0)

#define F_SUCCESS		W_EXITCODE(0, 0)
#define F_FAILURE		W_EXITCODE(2, 0)
#define F_INFO			W_EXITCODE(4, 0)

////////////////////////////////////////////////////////////////////////////

CRightPanel::CRightPanel(QWidget *parent, const char *name)
	: CListView(parent, name),
		QDropSite(viewport()),
		KXMLGUIClient((KXMLGUIClient*)(parent)),
		m_Timer((CListView*)this),
		m_RedirectionTimer((CListView*)this)
#ifndef QT_20
		,m_DropZone(viewport(), DndURL)
#endif
{

	setXMLFile("RightPanelui.rc");
	addColumn("");
	addColumn("");
	addColumn("");
	addColumn("");
	//ActivateActions();

#ifndef QT_20  // commented because don't have this method in new CListView
	showSortArrow(true);	// turn on sort indication arrows
#endif
	setIconView(gbUseBigIcons);

#ifndef QT_20
	connect(&m_DropZone, SIGNAL(dropEnter(KDNDDropZone*)), this, SLOT(OnKDNDDropEnter(KDNDDropZone*)));
	connect(&m_DropZone, SIGNAL(dropAction(KDNDDropZone*)), this, SLOT(OnKDNDDropAction(KDNDDropZone*)));
	connect(&m_DropZone, SIGNAL(dropLeave(KDNDDropZone*)), this, SLOT(OnKDNDDropLeave(KDNDDropZone*)));
#endif
	m_bHasFocus = FALSE;
	m_bTextSelected = FALSE;
	m_pTemporarySelectedDragDestination = NULL;
	m_bMayBeEditLabel = FALSE;
  m_bCatchReleaseMode = FALSE;
	m_bBackgroundEnabled = TRUE;
	m_EnabledBaseColor = colorGroup().base();
	m_pExternalItem = NULL;
	//m_bInDrag = FALSE;

	setAcceptDrops(true);
	connect(&m_Timer, SIGNAL(timeout()), this, SLOT(ActivateEvent()));
	connect(&m_RedirectionTimer, SIGNAL(timeout()), this, SLOT(DoRedirection()));
	connect(this, SIGNAL(doubleClicked(CListViewItem*)), this, SLOT(OnDoubleClicked(CListViewItem*)));
	//connect(this, SIGNAL(rightButtonClicked(CListViewItem*, const QPoint&, int)), this, SLOT(OnRightClicked(CListViewItem*, const QPoint&, int)));

	connect(&gTreeExpansionNotifier, SIGNAL(ExpansionDone(CNetworkTreeItem *)), this, SLOT(OnExpansionDone(CNetworkTreeItem *)));
	connect(&gTreeExpansionNotifier, SIGNAL(ExpansionCanceled(CNetworkTreeItem *)), this, SLOT(OnExpansionCanceled(CNetworkTreeItem *)));
  connect(&gTreeExpansionNotifier, SIGNAL(RefreshRequested(const char *)), this, SLOT(OnRefreshRequested(const char *)));
	connect(this, SIGNAL(selectionChanged()), this, SLOT(OnSelectionChanged()));
	setMultiSelection(TRUE);

	/* Init movie */

	extern unsigned char search_movie_data[];
	extern int gSearchMovieLen;
	QByteArray x;

	x.duplicate((LPCSTR)&search_movie_data[0], gSearchMovieLen);

	m_Movie = QMovie(x);
	m_Movie.setBackgroundColor(colorGroup().base());

	m_pSearchMovieLabel = new QLabel(this);

	m_pSearchMovieLabel->resize(knSEARCH_MOVIE_WIDTH, knSEARCH_MOVIE_HEIGHT);
	m_pSearchMovieLabel->setMovie(m_Movie);
	m_pSearchMovieLabel->setMargin(0);
	m_pSearchMovieLabel->hide();
	m_pSearchMovieLabel->movie()->pause();
	m_pSearchMovieLabel->setBackgroundMode(NoBackground);

	/* Navigation canceled label */

	m_pNavigationCanceledLabel = new QLabel(LoadString(knNAVIGATION_CANCELED), this);
	m_pNavigationCanceledLabel->setBackgroundColor(colorGroup().base());
	m_pNavigationCanceledLabel->setAutoResize(TRUE);

	m_pNavigationCanceledLabel->hide();

	/* Browser widget */

  m_pHTMLView = new KHTMLPart(this, "Browser");
	connect(m_pHTMLView, SIGNAL(onURL(const QString&)), this, SLOT(OnURL(const QString&)));

	connect(m_pHTMLView->browserExtension(),
					SIGNAL(openURLRequest(const KURL &, const KParts::URLArgs &)),
					this,
					SLOT(OnOpenURL(const KURL &, const KParts::URLArgs &)));

	connect(m_pHTMLView,
					SIGNAL(setWindowCaption(const QString &)),
					this,
					SLOT(OnSetTitle(const QString &)));

	m_pHTMLView->enableJScript(true);
	m_pHTMLView->hide();
  setFocusProxy(NULL);
	ActivateActions();
}

////////////////////////////////////////////////////////////////////////////

CRightPanel::~CRightPanel()
{
	disconnect(this, SIGNAL(doubleClicked(CListViewItem*)), this, SLOT(OnDoubleClicked(CListViewItem*)));
	//disconnect(this, SIGNAL(rightButtonClicked(CListViewItem*, const QPoint&, int)), this, SLOT(OnRightClicked(CListViewItem*, const QPoint&, int)));
	disconnect(&gTreeExpansionNotifier, SIGNAL(ExpansionDone(CNetworkTreeItem *)), this, SLOT(OnExpansionDone(CNetworkTreeItem *)));
	disconnect(&gTreeExpansionNotifier, SIGNAL(ExpansionCanceled(CNetworkTreeItem *)), this, SLOT(OnExpansionCanceled(CNetworkTreeItem *)));
	delete m_pSearchMovieLabel;
	CHistory::Instance()->Commit();
	CBrowserCache::Instance()->EnforceMaxSize();
}

////////////////////////////////////////////////////////////////////////////

void CRightPanel::OnStop()
{
  if (IsWebBrowser())
	{
#ifndef QT_20
		m_pHTMLView->setFocus();
#endif
		m_RedirectionTimer.stop();
		bLastRedirectionFailed = FALSE;
	}

	if (gnWebOperationsCount > 0
#ifndef QT_20
   || m_pHTMLView->isVisible()
#endif
   )
	{
#ifndef QT_20
		m_pHTMLView->cancelAllRequests();
#endif

		gTasks.KillEntireKind(OnDocumentLoaded);
		gTasks.KillEntireKind(OnImageLoaded);

		while (gnWebOperationsCount > 0)
		{
			gnWebOperationsCount--;
			gTreeExpansionNotifier.DoEndWorking();
		}

		//m_pHTMLView->hide();
		//setFocusProxy(NULL);
	}
	else
	{
		if (m_pSearchMovieLabel->isVisible())
		{
			m_pSearchMovieLabel->hide();
			m_pSearchMovieLabel->movie()->pause();

			m_pNavigationCanceledLabel->setText(LoadString(knNAVIGATION_CANCELED));
			m_pNavigationCanceledLabel->move((width()-m_pNavigationCanceledLabel->width())/2, (height() - m_pNavigationCanceledLabel->height())/2);
		//m_pNavigationCanceledLabel->recreate(m_pHTMLView, 0, QPoint((width()-m_pNavigationCanceledLabel->width())/2, (height() - m_pNavigationCanceledLabel->height())/2));
			m_pNavigationCanceledLabel->show();
      time(&m_LastRefreshTime);
		}
	}
}

////////////////////////////////////////////////////////////////////////////

void CRightPanel::refresh()
{
}

////////////////////////////////////////////////////////////////////////////

void CRightPanel::OnExpansionDone(CNetworkTreeItem *pItem)
{
	if (pItem->ExpansionStatus() == keNotExpanded)
    return;

  if (m_pExternalItem == (CListViewItem*)pItem && !bInside)
	{
    m_LastRefreshTime = 0; // force refresh
		ActivateEvent();
	}
}

////////////////////////////////////////////////////////////////////////////

void CRightPanel::OnExpansionCanceled(CNetworkTreeItem *pItem)
{
	if (m_pExternalItem == (CListViewItem*)pItem)
		OnStop();
}

////////////////////////////////////////////////////////////////////////////

bool CRightPanel::event(QEvent *e)
{
	// This is a hack workaround for a Qt problem.
	// Default Qt menu implementation will move focus from the current widget to the menu item whenever a
	// mouse is over the menu. That causes major inconvenience for a Corel Explorer user, because the
	// window which has current focus is displaying its selected items (left tree and right panel).
	// As focus goes away to the menu, current selection disappears from the view.
	// To deal with that problem, we set a focus proxy for a menu to be the current view's viewport widget.
	// This prevents focus from switching to the menu. Unfortunately, that workaround has a side effect:
	// menu also sets its focus policy to NONE, and because the view is the proxy, that sets the current focus
	// policy for the view to NONE. Qt implementation of the menubar is not customizable, all the
	// functions that could possibly be overloaded are private and/or non-virtual. So we have to sit on the
	// event handler and try to fix our focus policy whenever a new event comes by.
	// Ugly, eh? I know, but it works :-)

	if (focusPolicy() != QWidget::StrongFocus)
		setFocusPolicy(QWidget::StrongFocus);

	bool retcode = CListView::event(e);

	if (e->type() == 0x999)
		ActivateEvent();

	return retcode;
}

////////////////////////////////////////////////////////////////////////////

/* This function will be called when network credentials are to be
   confirmed with the user.
	 Returns:
			0: quit or escape
			1: credentials updated, please retry
*/

BOOL CRightPanel::PromptForPassword(CRemoteFileContainer *pItem)
{
	CPasswordDlg dlg(pItem->FullName(FALSE), NULL);

	if (1 == dlg.exec())
	{
		CCredentials cred(dlg.m_UserName,dlg.m_Password,dlg.m_Workgroup);

		pItem->m_nCredentialsIndex = gCredentials.Find(cred);

		if (pItem->m_nCredentialsIndex == -1)
			pItem->m_nCredentialsIndex = gCredentials.Add(cred);
		else
			if (gCredentials[pItem->m_nCredentialsIndex].m_Password != cred.m_Password)
				gCredentials[pItem->m_nCredentialsIndex].m_Password = cred.m_Password;

		return TRUE;
	}

	pItem->SetExpansionStatus(keNotExpanded);
	gTreeExpansionNotifier.Cancel(pItem);

	return FALSE;
}


void CRightPanel::SetHeaderType(int nType)
{
	switch (nType)
	{
		case 0:  // Name, Comment
		{
			setNumCols(2);

			setColumnText(1, LoadString(knCOMMENT));
			setColumnAlignment(1, AlignLeft);

			setColumnWidth(0, width() / 2);
			setColumnWidth(1, width() / 2);
		}
		break;

		case 1: // Name, Size, Attributes, Modified
		{
			setNumCols(4);

			setColumnText(1, LoadString(knSIZE));
			setColumnAlignment(1, AlignRight);

			setColumnWidth(1, 70);

			QString s = LoadString(knATTRIBUTES);
			setColumnText(2, s);

			QPainter p;
			p.begin(this);
			int width2 = p.boundingRect(0,0,200,50, AlignLeft, s).width() + 10;
			setColumnWidth(2, width2);
			p.end();

			setColumnText(3, LoadString(knMODIFIED));
			setColumnWidth(3, 120);

			int width0 = viewport()->width() - 120 - 70 - width2 - 2;

			if (width0 < 100)
				width0 = 100;

			if (width0 > 300)
				width0 = 300;

			setColumnWidth(0, width0);
		}
		break;

		case 2:
		{
			setNumCols(4);

			setColumnText(0, LoadString(knMOUNTED_ON));
			setColumnWidth(0, 100);

			setColumnText(1, LoadString(knFILESYSTEM));
			setColumnAlignment(1, AlignLeft);
			setColumnWidth(1, 100);

			setColumnText(2, LoadString(knTOTAL_SIZE));
			setColumnWidth(2, 70);
			setColumnText(3, LoadString(knFREE_SPACE));
			setColumnWidth(3, 70);
		}
		break;

    case 3: // Trash
    {
      setNumCols(5);
			setColumnWidth(0, 70);
      setColumnText(1, LoadString(knORIGINAL_LOCATION));
			setColumnAlignment(1, AlignLeft);
			setColumnWidth(1, 150);
      setColumnText(2, LoadString(knDATE_DELETED));
			setColumnWidth(2, 120);

      QString s = LoadString(knATTRIBUTES);
      setColumnText(3, s);
			QPainter p;
			p.begin(this);
			int width2 = p.boundingRect(0,0,200,50, AlignLeft, s).width() + 10;
			setColumnWidth(3, width2);
			p.end();

      setColumnText(4, LoadString(knSIZE));
      setColumnWidth(4, 70);
    }
    break;

    case 4: // Name, Type, Comment
    {
      setNumCols(3);

			QString s = LoadString(knTYPE);
			setColumnText(1, s);
			setColumnAlignment(1, AlignLeft);

			QPainter p;
			p.begin(this);
			int width2 = p.boundingRect(0,0,200,50, AlignLeft, s).width() + 10;
			setColumnWidth(1, width2);
			p.end();

			setColumnText(2, LoadString(knCOMMENT));
			setColumnAlignment(2, AlignLeft);

			width2 = (width() - width2) / 2;
      setColumnWidth(0, width2);
			setColumnWidth(2, width2);
    }
    break;

    case 5: // Name, Connection, Location, Manufacturer, Model
    {
      setColumnText(0, LoadString(knNAME));
      setNumCols(5);

      setColumnText(1, LoadString(knCONNECTION));
			setColumnAlignment(1, AlignLeft);
      setColumnText(2, LoadString(knLOCATION));
      setColumnText(3, LoadString(knMANUFACTURER));
      setColumnAlignment(3, AlignRight);
      setColumnText(4, LoadString(knMODEL));
			setColumnWidth(0, width() / 5);
			setColumnWidth(1, width() / 5);
			setColumnWidth(2, width() / 5);
			setColumnWidth(3, width() / 5);
			setColumnWidth(4, width() / 5);
    }
    break;

    case 6:
    {
      setNumCols(6);
      setColumnText(0, LoadString(knDOCUMENT));
			setColumnAlignment(0, AlignLeft);
      setColumnText(1, LoadString(knUSER));
			setColumnAlignment(1, AlignLeft);
      setColumnText(2, LoadString(knSTATUS));
			setColumnAlignment(2, AlignLeft);
      setColumnText(3, LoadString(knSIZE));
			setColumnAlignment(3, AlignLeft);
      setColumnText(4, LoadString(knJOB_FORMAT));
			setColumnAlignment(4, AlignLeft);
      setColumnText(5, LoadString(knTIME_SUBMITTED));
			setColumnAlignment(5, AlignLeft);
			setColumnWidth(0, width() / 6);
			setColumnWidth(1, width() / 6);
			setColumnWidth(2, width() / 6);
			setColumnWidth(3, width() / 6);
			setColumnWidth(4, width() / 6);
			setColumnWidth(5, width() / 6);
    }
    break;
	}
}

////////////////////////////////////////////////////////////////////////////

void CRightPanel::SetBackgroundEnabled(BOOL bIsEnabled, int nMessageID)
{
  if (m_bBackgroundEnabled != bIsEnabled)
	{
		m_bBackgroundEnabled = bIsEnabled;

		QPalette pal = palette();
		QColorGroup x = pal.normal();
		QColorGroup x2(x.foreground(), x.background(), x.light(), x.dark(), x.mid(), x.text(),	bIsEnabled ? m_EnabledBaseColor : colorGroup().background());
		m_pNavigationCanceledLabel->setBackgroundColor(x2.base());
		m_Movie.setBackgroundColor(x2.base());

		QPalette pal2(x2,x2,x2);
		setPalette(pal2);
	}

	if (bIsEnabled)
	{
		if (!iconView())
			header()->show();

		m_pNavigationCanceledLabel->hide();
	}
	else
	{
		if (!iconView())
			header()->hide();

		m_pNavigationCanceledLabel->setText(LoadString(nMessageID));

		m_pNavigationCanceledLabel->move((width()-m_pNavigationCanceledLabel->width())/2, (height() - m_pNavigationCanceledLabel->height())/2);

		m_pNavigationCanceledLabel->show();
	}
}

void CRightPanel::RefreshListview(CListViewItem *qitem, BOOL bIsOurTree, BOOL &bEnableBackground, int &nMessageID)
{
	int i;

  clear();
	m_CurrentSelection.clear();
	m_pStoredCurrentItem = NULL;

	if (bIsOurTree)
	{
		CNetworkTreeItem* item = (CNetworkTreeItem*)qitem;

		setSorting(0);
		setColumnText(0, LoadString(knNAME));

		BOOL bIsFileContainer =
			(item->Kind() == keShareItem ||
			 item->Kind() == keFileItem ||
			 item->Kind() == kePrinterItem ||
			 item->Kind() == keFileSystemItem ||
			 item->Kind() == keLocalFileItem ||
			 item->Kind() == keFTPFileItem ||
			 item->Kind() == keFTPSiteItem ||
       item->Kind() == keDeviceItem);

		//if (!bIsFileContainer)
		//{
    switch (item->ExpansionStatus())
			{
				default:
				break;

				case keNotExpanded:
					erase();
					emit UpdateCompleted(0, -1., 0);
					m_pSearchMovieLabel->move((width()-m_pSearchMovieLabel->width())/2, (height() - m_pSearchMovieLabel->height())/2);
					m_pSearchMovieLabel->movie()->unpause();
					m_pNavigationCanceledLabel->hide();
					m_pSearchMovieLabel->show();
          item->Fill();

					if (!item->childCount() && item->ExpansionStatus() == keExpansionComplete)
						item->setExpandable(FALSE);

        if (item->ExpansionStatus() == keExpansionComplete)
				  break;
				else
          return; // We will be notified when expansion is complete

				case keExpanding:
					erase();
					emit UpdateCompleted(0, -1., 0);
					m_pSearchMovieLabel->move((width()-m_pSearchMovieLabel->width())/2, (height() - m_pSearchMovieLabel->height())/2);
					m_pSearchMovieLabel->movie()->unpause();
					m_pNavigationCanceledLabel->hide();
					m_pSearchMovieLabel->show();
				return;
			}
		//}

		m_pSearchMovieLabel->hide();
		m_pSearchMovieLabel->movie()->pause();

		if (bIsFileContainer)
    {
      if (item->Kind() == keLocalFileItem &&
          IsTrashFolder(item->FullName(FALSE)))
        SetHeaderType(3);
      else
        SetHeaderType(1);
    }

		switch (item->Kind())
		{
			default:
			break;

			/* List of filesystems */

			case keMyComputerItem:
			{
				/* First prepare the header */
				SetHeaderType(2);

				CListViewItem *c = item->firstChild();

				for (i=item->childCount(); i > 0; i--, c = c->nextSibling())
					new CFileSystemItem(this, (CFileSystemInfo*)(CFileSystemItem *)c, m_pExternalItem);

				m_TotalSize = -1.;
			}
			break;


      case keNFSShareItem:
				bEnableBackground = FALSE;
				nMessageID = knEMPTY_MESSAGE;
			break;

			/* Local file */
			case keFileSystemItem:
			case keLocalFileItem:
      case keDeviceItem:
			{
				QString Path = item->FullName(FALSE);

        if (Path.isEmpty()) // Device not yet mounted
        {
          bEnableBackground = FALSE;
          nMessageID = knDEVICE_NOT_MOUNTED;
          break;
        }

        if (access(Path, 0))
				{
					emit GoParentRequest();
					//gTreeExpansionNotifier.DoItemDestroyed(item);
					//delete item;
					return;
				}

        int nFolderCount = 0;
			  clear();
			  m_TotalSize = 0;

        if (IsTrashFolder(Path))
        {
          CTrashEntryArray list;

          if (GetTrashEntryList(Path, list))
          {
            int i;

            for (i=0; i < list.count(); i++)
            {
              CTrashEntryItem *pItem = new CTrashEntryItem(this, m_pExternalItem, &list[i]);

              if (S_ISDIR(pItem->m_OriginalFileMode))
                nFolderCount++;

              m_TotalSize += pItem->m_FileSize;
            }
          }
        }
        else
        {
          CWaitLogo w;
					CFileArray list;
				  GetLocalFileList(Path, &list, TRUE);

					if (EACCES == errno)
						bEnableBackground = FALSE;

				  if (item->ExpansionStatus() == keNotExpanded)
					  ((CLocalFileContainer*)item)->Fill(list);

				  for (i=0; i < list.count(); i++)
				  {
  					CLocalFileItem *pItem = new CLocalFileItem(this, m_pExternalItem, &list[i]);

					  if (pItem->IsFolder())
  						nFolderCount++;

					  m_TotalSize += pItem->GetFileSize();
				  }

				  if (list.count() == 0 && access(Path, X_OK))
					  bEnableBackground = FALSE;

          if (nFolderCount != item->childCount())
          {
            RescanItem(item);
          }
        }
			};
			break;

			/* FTP */
			case keFTPFileItem:
			case keFTPSiteItem:
			{
				CFileArray list;
				CSMBErrorCode retcode;
				CRemoteFileContainer *nitem = (CRemoteFileContainer *)item;

				do
				{
					retcode = ((CFTPFileContainer*)qitem)->GetFTPFileList(item->FullName(FALSE), &list, nitem->CredentialsIndex(), TRUE);

					if (m_pExternalItem != qitem) // safely return if the context has already changed
						return;
				}
				while (retcode == keErrorAccessDenied && PromptForPassword(nitem));

				if (item->ExpansionStatus() == keNotExpanded)
					((CFTPFileContainer*)item)->Fill(list);

				clear();
				m_TotalSize = 0;

				for (i=0; i < list.count(); i++)
						m_TotalSize += (new CFTPFileItem(this, m_pExternalItem, &list[i]))->GetFileSize();
			};
			break;

			/* Printers folder */
      case kePrinterRootItem:
      {
        SetHeaderType(5);

        CListViewItem *c = item->firstChild();

				for (i=item->childCount(); i > 0; i--, c = c->nextSibling())
				{
					CPrinterItem *pPrinter = new CPrinterItem(this, *(CPrinterInfo*)(CPrinterItem*)c);
					pPrinter->setExpandable(false);
				}

				m_TotalSize = -1.;
      }
      break;

      /* Local Printer */
      case keLocalPrinterItem:
      {
        SetHeaderType(6);
				m_TotalSize = -1.;

        CPrintJobList list;

        if (keSuccess == GetPrintQueue(item->text(0), list))
        {
          QListIterator<CPrintJobInfo> it(list);

          for (; it.current() != NULL; ++it)
            new CPrintJobItem(this, m_pExternalItem, *it.current());
        }
      }
      break;

      /* NFS network */

			case keNFSRootItem:
			{
				SetHeaderType(0);
				CListViewItem *c = item->firstChild();

				for (i=item->childCount(); i > 0; i--, c = c->nextSibling())
				{
					CNFSServerItem *pServer = new CNFSServerItem(this, (CNFSServerInfo*)(CNFSServerItem *)c);
					pServer->setExpandable(false);
				}

				m_TotalSize = -1.;
			};
			break;

			case keNFSHostItem:
			{
				SetHeaderType(0);
				CListViewItem *c = item->firstChild();

				for (i=item->childCount(); i > 0; i--, c = c->nextSibling())
					new CNFSShareItem(this, m_pExternalItem, (CNFSShareInfo*)(CNFSShareItem *)c);

				m_TotalSize = -1.;
			};
			break;

			/* Windows network */
			case keMSRootItem:
			{
				SetHeaderType(0);
				CListViewItem *c = item->firstChild();

				for (i=item->childCount(); i > 0; i--, c = c->nextSibling())
					new CWorkgroupItem(this, (CSMBWorkgroupInfo*)(CWorkgroupItem *)c);

				m_TotalSize = -1.;
			};
			break;

			case keWorkgroupItem:
			{
				SetHeaderType(0);
				CListViewItem *c = item->firstChild();

				for (i=item->childCount(); i > 0; i--, c = c->nextSibling())
				{
					CServerItem *pServer = new CServerItem(this, m_pExternalItem, (CSMBServerInfo*)(CServerItem *)c);
					pServer->setExpandable(false);
				}

				m_TotalSize = -1.;
			};
			break;

			case keServerItem:
			{
				SetHeaderType(4);
				CListViewItem *c = item->firstChild();

				for (i=item->childCount(); i > 0; i--, c = c->nextSibling())
					new CShareItem(this, m_pExternalItem, (CSMBShareInfo*)(CShareItem *)c);

				m_TotalSize = -1.;
			};
			break;

			case keShareItem:
			case keFileItem:
			case kePrinterItem:
			{
				CFileArray list;
				CRemoteFileContainer *nitem = (CRemoteFileContainer *)item;
				CSMBErrorCode retcode;

				do
				{
					retcode = GetFileList(nitem->FullName(TRUE), &list, nitem->CredentialsIndex(), TRUE);

					if (m_pExternalItem != qitem) // safely return if the context has already changed
						return;
				}
				while (retcode == keErrorAccessDenied && PromptForPassword(nitem));

				if (item->ExpansionStatus() == keNotExpanded)
					((CNetworkFileContainer*)item)->Fill(list);

				clear();
				m_TotalSize = 0;

				for (i=0; i < list.count(); i++)
					m_TotalSize += (new CFileItem(this, m_pExternalItem, &list[i]))->GetFileSize();
			}
			break;
		}

		int nHiddenPrefix;
    OnSetTitle(DetachHiddenPrefix(item->FullName(FALSE), nHiddenPrefix));
	}
	else /* must be desktop */
	{
		if (m_pSearchMovieLabel->isVisible())
    {
      m_pSearchMovieLabel->hide();
		  m_pSearchMovieLabel->movie()->pause();
    }

    setColumnText(0, LoadString(knNAME));

		setSorting(5);

		setNumCols(1);

		m_nItemCount = qitem->childCount();

		if (m_nItemCount)
		{
			CListViewItem *c = qitem->firstChild();

			for (i=0; i < m_nItemCount; i++, c = c->nextSibling())
			{
				CWindowsTreeItem *pNewItem = new CWindowsTreeItem(this, c->text(0), "", "","", "",c->key(0, 0));

				const QPixmap *pPixmap = c->pixmap(0);
				const QPixmap *pPixmapBig = c->iconViewPixmap();

				if (pPixmap != NULL)
				{
					pNewItem->setPixmap(0, *pPixmap);
				}

				if (NULL != pPixmapBig)
				{
					pNewItem->setIconViewPixmap(*pPixmapBig);
				}
			}
		}

		m_TotalSize = -1.;

		OnSetTitle(m_pExternalItem->text(0));
	}
}

void CRightPanel::ActivateEvent()
{
	if (bInside)
		return;

	bInside = true;

	QString ParentPath;
	CListViewItem *qitem = m_pExternalItem;
	BOOL bIsOurTree;
	CNetworkTreeItem* item = NULL;

	if (NULL != m_pExternalItem)
	{
		bIsOurTree = CNetworkTreeItem::IsNetworkTreeItem(qitem) ||	CNetworkTreeItem::IsMyComputerItem(qitem);

    if (bIsOurTree)
    {
      if (NULL != m_pExternalItem)
        ((CNetworkTreeItem*)m_pExternalItem)->InitPixmap();

      item = (CNetworkTreeItem*)qitem;
    }
	}
	else
	{
    clear();
		bInside = false;
		return;
	}

	BOOL bEnableBackground = TRUE;
  int nMessageID = knACCESS_DENIED;

  // *** This is the right panel auto-refresh code. ***
	// The idea here is that we use m_Timer member variable to
	// call this function periodically.
	// First time m_LastRefreshTime will be 0 and we will setup the frequency at which
	// m_Timer will fire according to the type of the tree node.
	// On subsequent calls we check if tree node's ContentsChanged() function
	// returns FALSE. If that's the case, we return immediately.
	// Otherwise, we force tree node rescan and also fall to the code further down,
	// to update right panel...

	if (bIsOurTree)
	{
    time_t timeold = m_LastRefreshTime;
		time(&m_LastRefreshTime);

		if (timeold == 0)
		{
      SaveRefreshSelection();
			m_Timer.stop();
			m_Timer.start(item->DesiredRefreshDelay());
		}
		else
		{
      if (item->ExpansionStatus() == keExpanding)
      {
        bInside = false;
        return;
      }

			QTime t;
			t.start(); // start clock

			BOOL bChanged;

      if (IsExcludedFromRescan(item->FullName(FALSE)))
      {
        bChanged = FALSE;
      }
      else
      {
        if (item->Kind() == keDeviceItem)
          ((CDeviceItem *)item)->AccessDevice();

        bChanged = item->ContentsChanged(timeold, childCount(), firstChild());
      }

			unsigned long nInterval;

      if (item->Kind() == keDeviceItem)
        nInterval = 2000;
      else
        nInterval = t.elapsed() * 50;

			if (nInterval < 2000)
				nInterval = 2000;
			else
				if (nInterval > 30000 && nInterval < 300000)
					nInterval = 30000; // 30 seconds for slower connections
				else
					if (nInterval > 300000)
						nInterval = 60000; // 1 minute for very slow ones...

			//printf("Path = [%s], Interval = %d\n", (LPCSTR)item->FullName(FALSE), nInterval);

      m_Timer.stop();
			m_Timer.start(nInterval);

			if (bChanged)
			{
				SaveRefreshSelection();
				RescanItem(item);
			}
			else
			{
				bInside = false;
				return;
			}
		}
	}
	else
		m_Timer.stop();

	if (this == CLabelEditor::m_pParentView && NULL != CLabelEditor::m_pLE)
	{
		delete CLabelEditor::m_pLE;
	}

	// *** End of right panel auto-refresh code ***

	if (bIsOurTree && ((CNetworkTreeItem*)qitem)->Kind() == keWebPageItem)
	{
		clear();
		ShowBrowser((LPCSTR)((CNetworkTreeItem*)qitem)->FullName(FALSE)
#ifdef QT_20
  .latin1()
#endif
    );
		bInside = false;
		return;
	}
	else
	{
		setFocusProxy(NULL);
#ifndef QT_20
    m_pHTMLView->hide();
#endif
  }

	if (m_bBackgroundEnabled)
		m_pNavigationCanceledLabel->hide();

  RefreshListview(qitem, bIsOurTree, bEnableBackground, nMessageID);

  if (bIsOurTree && ((CNetworkTreeItem *)qitem)->ExpansionStatus() == keNotExpanded)
  {
    bInside = false;
    return;
  }

	SetBackgroundEnabled(bEnableBackground, nMessageID);

	RestoreRefreshSelection();

	emit UpdateCompleted(childCount(), m_TotalSize, 0);

	bInside = false;
}

void CRightPanel::DoRedirection()
{
	// Redirection and refresh are the same thing at the user level.
	if (m_CurrentBrowserURL == m_RedirectionURL)
		OnRefreshWebBrowser();
	else
		emit ChdirRequest(m_pExternalItem, m_RedirectionURL);
}

////////////////////////////////////////////////////////////////////////////

void CRightPanel::OnDoubleClicked(CListViewItem *pItem)
{
  if (IsWebBrowser())
	{
#ifndef QT_20
		m_pHTMLView->setFocus();
#endif
    m_RedirectionTimer.stop();
	}

	if (IS_NETWORKTREEITEM(pItem))
	{
		CNetworkTreeItem* pI = (CNetworkTreeItem*)pItem;

		if (keFTPFileItem == pI->Kind() && !((CFTPFileItem*)pI)->IsFolder())
		{
			QString URL;
      URL = MakeItemURL((CNetworkTreeItem*)pItem);

#if (QT_VERSION < 200)
      KMimeType::InitStatic();
			KMimeType::init();

			KMimeType *t = KMimeType::findByName("text/html");
			RunMimeBinding(FALSE, URL, NULL == t ? NULL : (LPCSTR)t->getDefaultBinding());
#else
    printf("TODO: implement RunMimeBinding\n");
#endif
      return;
		}

    if (keLocalFileItem == pI->Kind() ||
				keFileItem == pI->Kind())
		{
			CSMBFileInfo *pFile = (keLocalFileItem == pI->Kind()) ?
            ((CSMBFileInfo *)(CLocalFileItem *)pI) :
            ((CSMBFileInfo *)(CFileItem *)pI);

			if (!pFile->IsFolder())
			{
				cout<<"pItem->text(0) = "<<pItem->text(0)<<endl;
				cout<<"m_pExternalItem->text(0) = "<<m_pExternalItem->text(0)<<endl;
				emit PartRequest(pItem);
				return;
			}
		}

    if (pI->Kind() == keNFSShareItem)
			return;
	}

	cout<<"pItem->text(0) = "<<pItem->text(0)<<endl;
	cout<<"m_pExternalItem->text(0) = "<<m_pExternalItem->text(0)<<endl;
	emit ChdirRequest(m_pExternalItem, pItem->text(0));
}

////////////////////////////////////////////////////////////////////////////

void CRightPanel::BuildItemList(CListViewItem *pItem, QList<CListViewItem>& ItemList)
{
	if (pItem != NULL && !isSelected(pItem))
	{
		/* Unselect all items then... */

		CListViewItem *pI;

		for (pI = firstChild(); pI != NULL; pI = pI->nextSibling())
		{
			if (isSelected(pI))
			{
				pI->setSelected(FALSE);
				repaintItem(pI);
			}
		}

		/* Select this item */
		setSelected(pItem, TRUE);
    ItemList.append(pItem);
	}
	else
	{
		CListViewItem *pI;

		for (pI = firstChild(); pI != NULL; pI = pI->nextSibling())
		{
			if (isSelected(pI))
      {
        ItemList.append(pI);
      }
		}
	}
}

////////////////////////////////////////////////////////////////////////////

void CRightPanel::OnRightClicked(CListViewItem *pItem, const QPoint& p, int)
{
	viewport()->setFocus();

	pItem = GetItemAt(p);

	if (NULL != pItem)
	{
		QList<CListViewItem> ItemList;

		BuildItemList(pItem, ItemList);

		setCurrentItem(pItem);

		viewport()->setFocus();
	}
	else
		if (NULL != currentItem())
			setSelected(currentItem(), FALSE);
}

////////////////////////////////////////////////////////////////////////////

void CRightPanel::resizeEvent(QResizeEvent *e)
{
	CListView::resizeEvent(e);
  m_pHTMLView->view()->setGeometry(0,0,width(),height());
}

////////////////////////////////////////////////////////////////////////////

void CRightPanel::OnSelectionChanged()
{
	CListViewItem *pItem;

	int nObjectsSelected = 0;
	double TotalSizeSelected = 0.;

	for (pItem = firstChild(); pItem != NULL; pItem = pItem->nextSibling())
	{
		if (isSelected(pItem))
		{
			nObjectsSelected++;

			if (IS_NETWORKTREEITEM(pItem))
			{
				CNetworkTreeItem* pI = (CNetworkTreeItem*)pItem;

				QString Size;

				switch (pI->Kind())
				{
					default:
					break;

					case keLocalFileItem:
						Size = ((CSMBFileInfo*)(CLocalFileItem*)pI)->m_FileSize;
					break;

					case keFileItem:
						Size = ((CSMBFileInfo*)(CFileItem*)pI)->m_FileSize;
					break;

					case keFTPFileItem:
						Size = ((CSMBFileInfo*)(CFTPFileItem*)pI)->m_FileSize;
					break;
				}

				if (!Size.isEmpty())
					TotalSizeSelected += atoi((LPCSTR)Size
#ifdef QT_20
  .latin1()
#endif
          );
			}
		}
	}

	if (nObjectsSelected == 0)
		emit UpdateCompleted(childCount(), m_TotalSize, 0);
	else
		emit UpdateCompleted(0, TotalSizeSelected, nObjectsSelected);
}

////////////////////////////////////////////////////////////////////////////

void CRightPanel::RefreshIcons()
{
	if (NULL != m_pExternalItem)
	{
		CListViewItem *ec;

		for (ec = m_pExternalItem->firstChild(); ec != NULL; ec = ec->nextSibling())
		{
			CListViewItem *c;

			for (c = firstChild(); c != NULL; c = c->nextSibling())
			{
				if (!strcmp((LPCSTR)c->text(0)
#ifdef QT_20
  .latin1()
#endif
        , (LPCSTR)ec->text(0)
#ifdef QT_20
  .latin1()
#endif
        ))
				{
					c->setPixmap(0, *ec->pixmap(0));
				}
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////

BOOL IsCursorCode(int KeyCode)
{
	return
		KeyCode == Qt::Key_Home ||
		KeyCode == Qt::Key_End ||
		KeyCode == Qt::Key_Left ||
		KeyCode == Qt::Key_Right ||
		KeyCode == Qt::Key_Up ||
		KeyCode == Qt::Key_Down ||
		KeyCode == Qt::Key_Prior ||
		KeyCode == Qt::Key_Next;
}

////////////////////////////////////////////////////////////////////////////

void CRightPanel::keyPressEvent(QKeyEvent *e)
{
	switch (e->key())
	{
		case Qt::Key_Home:
		{
			ensureItemVisible(firstChild());

			if (ControlButton != (e->state() & ControlButton))
			{
				setCurrentItem(firstChild());
				SelectCurrentOnly();
			}
		}
		return;

		case Qt::Key_End:
		{
			CListViewItem *pItem = firstChild();
			CListViewItem *below;

			if (iconView())
			{
				while ((below = pItem->itemBelow()) != NULL)
					pItem = below;
				while ((below = pItem->itemRight()) != NULL)
					pItem = below;
			}
			else
			{
				while ((below = pItem->itemBelow()) != NULL)
					pItem = below;
			}

			ensureItemVisible(pItem);

			if (ControlButton != (e->state() & ControlButton))
			{
				setCurrentItem(pItem);
				SelectCurrentOnly();
			}
		}
		return;

		case Qt::Key_A:
			if (ControlButton == (e->state() & ControlButton)) // Ctrl+A invokes "Select All"
			{
				SelectAll();
				return;
			}
		break;

		case Qt::Key_X:
			if (ControlButton == (e->state() & ControlButton)) // Ctrl+X invokes "Cut"
			{
				emit CutRequest();
				return;
			}
		break;

		case Qt::Key_C:
			if (ControlButton == (e->state() & ControlButton)) // Ctrl+C invokes "Copy"
			{
				emit CopyRequest();
				return;
			}
		break;

    case Qt::Key_T:
      if (ControlButton == (e->state() & ControlButton)) // Ctrl+C invokes "Copy"
      {
        emit ConsoleRequest();
        return;
      }
    break;

		case Qt::Key_V:
			if (ControlButton == (e->state() & ControlButton)) // Ctrl+V invokes "Paste"
			{
				emit PasteRequest();
				return;
			}
		break;

		case Qt::Key_Z:
			if (ControlButton == (e->state() & ControlButton)) // Ctrl+Z invokes "Undo"
			{
				emit UndoRequest();
				return;
			}
		break;

		case Qt::Key_Delete:
			if (AltButton != (e->state() & AltButton) && ControlButton != (e->state() & ControlButton))
			{
				if (ShiftButton == (e->state() & ShiftButton) ||
            (IS_NETWORKTREEITEM(m_pExternalItem) &&
						IsTrashFolder(((CNetworkTreeItem*)m_pExternalItem)->FullName(FALSE))))
				{
					emit NukeRequest();
				}
				else
				{
					emit DeleteRequest();
				}

				//m_LastRefreshTime = 0; // force refresh...

				return;
			}
		break;

		case Qt::Key_Enter:
		case Qt::Key_Return:
			if (currentItem() != NULL)
			{
				if (AltButton == (e->state() & AltButton)) // Alt+Enter invokes "Properties"
					emit PropertiesRequest();
				else
					emit ChdirRequest(m_pExternalItem, currentItem()->text(0));
			}
		break;

		case Qt::Key_Backspace:
			emit GoParentRequest();
		break;

		case Qt::Key_F2:
		{
			if (ControlButton != (e->state() & ControlButton) &&
					ShiftButton != (e->state() & ShiftButton) &&
					AltButton != (e->state() & AltButton) &&
					NULL != currentItem() &&
					((CWindowsTreeItem*)currentItem())->CanEditLabel())
			{
				SelectCurrentOnly();
				StartLabelEdit();
			}
		}
		break;

		case Qt::Key_F6:
			if (AltButton != (e->state() & AltButton))
				emit TabRequest(ShiftButton == (e->state() & ShiftButton));
		break;
	}

	if (currentItem() != NULL &&
		!isSelected(currentItem()) &&
		IsCursorCode(e->key()) &&
		ShiftButton == (e->state() & ShiftButton))
		setSelected(currentItem(), TRUE);

	CListView::keyPressEvent(e);

	if ((isalnum(e->ascii()) || IsCursorCode(e->key())) &&
		currentItem() != NULL &&
		ControlButton != (e->state() & ControlButton) &&
		ShiftButton != (e->state() & ShiftButton))
	{
		/* Unselect all items but current then... */
		SelectCurrentOnly();
	}
}

////////////////////////////////////////////////////////////////////////////

void CRightPanel::contentsDragMoveEvent(QDragMoveEvent *e)
{
	OnDNDEnter(viewport()->mapToGlobal(e->pos()), e);
}

////////////////////////////////////////////////////////////////////////////

void CRightPanel::contentsDragEnterEvent(QDragEnterEvent *e)
{
	if (DecodeURLList(e, m_DraggedURLList))
		e->accept();
}

////////////////////////////////////////////////////////////////////////////

void CRightPanel::contentsDragLeaveEvent(QDragLeaveEvent *)
{
	m_ScrollMode = 0;
	m_DraggedURLList.clear();

	if (NULL != m_pTemporarySelectedDragDestination)
		setSelected(m_pTemporarySelectedDragDestination, FALSE);

	m_pTemporarySelectedDragDestination = NULL;
}

////////////////////////////////////////////////////////////////////////////

void CRightPanel::contentsDropEvent(QDropEvent *e)
{
	m_ScrollMode = 0;
	QStrList list;

	if (QUrlDrag::decode(e, list))
	{
		QString URL;

		CNetworkTreeItem *pItem = (CNetworkTreeItem *)
		((m_pTemporarySelectedDragDestination == NULL ||
			!isSelected(m_pTemporarySelectedDragDestination)) ? m_pExternalItem : m_pTemporarySelectedDragDestination);

    CDropSelector Selector;

  	if (IsTrashFolder(FullName(pItem)))
    {
      Selector.Go(this, QCursor::pos(), TRUE);

      if (Selector.type() == keDropMove)
        StartDelete(list, TRUE);
    }
    else
    {
      BOOL bMove = FALSE;

      if (m_ButtonState & RightButton)
      {
				BOOL bCanMove = ItemAcceptsThisDrop(pItem, list, TRUE);

				Selector.Go(this, QCursor::pos(), FALSE, !bCanMove);

        if (Selector.type() == keDropMove)
          bMove = TRUE;
        else
          if (Selector.type() != keDropCopy)
            return; // Aborted
      }

      URL = MakeItemURL(pItem);
  		StartCopyMove(list, (LPCSTR)URL
#ifdef QT_20
  .latin1()
#endif
      , bMove, FALSE);
    }
	}
}

////////////////////////////////////////////////////////////////////////////

QString CRightPanel::FullName(CListViewItem *pItem)
{
	QString s;

	if (CNetworkTreeItem::IsNetworkTreeItem(m_pExternalItem) ||	CNetworkTreeItem::IsMyComputerItem(m_pExternalItem))
	{
		s = ((CNetworkTreeItem *)m_pExternalItem)->FullName(FALSE);

		if (NULL != pItem && pItem != m_pExternalItem && s.right(1) != s.left(1))
			s += s.left(1);	// forward slash or backslash
	}

	if (NULL != pItem && pItem != m_pExternalItem)
		s += pItem->text(0);

	return s;
}

////////////////////////////////////////////////////////////////////////////

#ifndef QT_20
void CRightPanel::mousePressEvent(QMouseEvent *e)
#else
void CRightPanel::viewportMousePressEvent(QMouseEvent *e)
#endif
{
	if (e->button() == LeftButton || e->button() == RightButton)
	{
		CListViewItem *pNewCurrentItem = itemAt(e->pos());

		if (NULL == GetItemAt(viewport()->mapToGlobal(e->pos())))
		{
			SelectCurrentOnly();
			setSelected(currentItem(), FALSE);
			return;
		}

		if (e->button() == LeftButton)
		{
			CListViewItem *pOldCurrentItem = currentItem();

			// See if we have a chance to switch into the label editing (rename) mode.
			// Can only go there if we are clicking again on already selected column 0 of the item.
			// Also we ask the item if it allows rename operation.

			if (m_bMayBeEditLabel == 2)
				m_bMayBeEditLabel = FALSE;
			else
			{
				if (pNewCurrentItem == pOldCurrentItem && isSelected(pNewCurrentItem))
				{
					if (((CWindowsTreeItem*)pNewCurrentItem)->IsOKToEditLabel(e->x()))
						m_bMayBeEditLabel = TRUE;
				}
			}

			setCurrentItem(pNewCurrentItem);

			if (ShiftButton == (e->state() & ShiftButton))
			{
				// First determine whether we should go up or down (from new to old)...
				CListViewItem *pI;

				BOOL bGoDown = FALSE;

				for (pI = firstChild(); pI != NULL; pI = pI->nextSibling())
				{
					if (pI == pNewCurrentItem)
					{
						bGoDown = TRUE;
						break;
					}

					if (pI == pOldCurrentItem)
						break;
				}

				// Now build a list of items from the new to old in the correct order
				// ...Too bad Qt doesn't have prevSibling() :-(
				QList<CListViewItem> list;

				for (pI = bGoDown ? pNewCurrentItem : pOldCurrentItem; pI != NULL; pI = pI->nextSibling())
				{
					if (bGoDown)
						list.append(pI);
					else
						list.insert(0, pI);

					if (pI == (bGoDown ? pOldCurrentItem : pNewCurrentItem))
					 break;
				}

				// Now traverse the list we just built...

				QListIterator<CListViewItem> it(list);
				BOOL bCurrentFlag = TRUE;

				for (; it.current() != NULL; ++it)
				{
					pI = it.current();

					if (ControlButton == (e->state() & ControlButton))
					{
						if (!isSelected(pI))
						{
							pI->setSelected(TRUE);
							repaintItem(pI);
						}
					}
					else
					{
						BOOL bWasSelected = isSelected(pI);

						if (bWasSelected != bCurrentFlag)
						{
							pI->setSelected(bCurrentFlag);
							repaintItem(pI);
						}

						if (bCurrentFlag && bWasSelected)
							bCurrentFlag = FALSE;
					}
				}

				emit selectionChanged();
			}
			else
			if (ControlButton != (e->state() & ControlButton))
			{
        if (!isSelected(pNewCurrentItem))
          SelectCurrentOnly();
        else
        {
          m_bCatchReleaseMode = TRUE;
          setCurrentItem(pNewCurrentItem);
        }
			}
			else
				setSelected(pNewCurrentItem, !isSelected(pNewCurrentItem));
		}
    else /* right button */
    {
   		OnRightClicked(NULL, viewport()->mapToGlobal(e->pos()), 0);
      m_bMayBeEditLabel = FALSE;
    }
	}
	else
	{
#ifndef QT_20
		CListView::mousePressEvent(e);
#else
		CListView::viewportMousePressEvent(e);
#endif
	}
	m_bCanBeginDrag = TRUE;
  m_StartDragPoint = e->pos();
}

////////////////////////////////////////////////////////////////////////////

#ifndef QT_20
void CRightPanel::mouseDoubleClickEvent(QMouseEvent *e)
#else
void CRightPanel::viewportMouseDoubleClickEvent(QMouseEvent *e)
#endif
{
	m_bMayBeEditLabel = FALSE;
#ifndef QT_20
	CListView::mouseDoubleClickEvent(e);
#else
	CListView::viewportMouseDoubleClickEvent(e);
#endif
}

////////////////////////////////////////////////////////////////////////////

#ifndef QT_20
void CRightPanel::mouseReleaseEvent(QMouseEvent *e)
#else
void CRightPanel::viewportMouseReleaseEvent(QMouseEvent *e)
#endif
{
#ifndef QT_20
	CListView::mouseReleaseEvent(e);
#else
	CListView::viewportMouseReleaseEvent(e);
#endif
	m_bCanBeginDrag = FALSE;

	if (e->button() == RightButton)
	{
    emit PopupMenuRequest(viewport()->mapToGlobal(e->pos()));
	  m_bMayBeEditLabel = FALSE;
	}
  else
  {
    if (m_bCatchReleaseMode)
    {
      if (SelectCurrentOnly())
        m_bMayBeEditLabel = FALSE; // if there was a multiple selection we just removed, there will be no label editing...

      m_bCatchReleaseMode = FALSE;
    }
  }

	if (m_bMayBeEditLabel)
	{
		StartLabelEdit();
	}

	m_bMayBeEditLabel = FALSE;
}

////////////////////////////////////////////////////////////////////////////

#ifndef QT_20
void CRightPanel::mouseMoveEvent(QMouseEvent *e)
#else
void CRightPanel::viewportMouseMoveEvent(QMouseEvent *e)
#endif
{
	//if (!m_bInDrag)
	//	CListView::mouseMoveEvent(e);
	m_bMayBeEditLabel = FALSE;
  m_bCatchReleaseMode = FALSE;

	if (m_bCanBeginDrag &&
		(CNetworkTreeItem::IsNetworkTreeItem(m_pExternalItem) || CNetworkTreeItem::IsMyComputerItem(m_pExternalItem)))
	{
		CNetworkTreeItem *pDragParent = (CNetworkTreeItem *)m_pExternalItem;

		QPoint dist = e->pos() - m_StartDragPoint;

    if (dist.x() * dist.x() + dist.y() * dist.y() < 16 ||
        pDragParent->Kind() == keNFSHostItem ||
				pDragParent->Kind() == keNFSRootItem ||
				pDragParent->Kind() == keMSRootItem ||
				pDragParent->Kind() == keWorkgroupItem)
			return;

		if (!CanDoFileCopy())
			return;


		QList<CListViewItem> ItemList;

    CListViewItem *pItem = GetItemAt(viewport()->mapToGlobal(e->pos()));
		BuildItemList(pItem, ItemList);

		if (ItemList.count() > 0)
		{
			m_bCanBeginDrag = FALSE;

			QListIterator<CListViewItem> it(ItemList);
			QStrList list;

			BOOL bFound = FALSE;

			for (; it.current() != NULL; ++it)
			{
				CListViewItem *pItem = it.current();

        if (IS_NETWORKTREEITEM(pItem))
        {
          QString URL;
          CNetworkTreeItem *pI = (CNetworkTreeItem *)pItem;

          URL = MakeItemURL(pI);

          if (URL.right(1) != "/")
          {
            if ((pI->Kind() == keFileItem && ((CFileItem*)pI)->IsFolder()) ||
                (pI->Kind() == keLocalFileItem && ((CLocalFileItem*)pI)->IsFolder()) ||
                (pI->Kind() == keFTPFileItem && ((CFTPFileItem*)pI)->IsFolder()) ||
                (pI->Kind() == keTrashEntryItem && ((CTrashEntryItem*)pI)->IsFolder()) ||
                (pI->Kind() == keShareItem))
            {
              URL += "/";
            }
          }

          list.append((LPCSTR)URL
#ifdef QT_20
  .latin1()
#endif
          );
				}

				QRect r = itemRect(pItem);

				if (!bFound && r.contains(e->pos()))
					bFound = TRUE;
			}

			if (bFound)
			{
				QUrlDrag *d = new QUrlDrag(list, viewport());

				if (ItemList.count() == 1)
				{
					QRect r = itemRect(pItem);

					d->setPixmap(CreateDragPixmap(pItem), QPoint(e->pos().x() - r.left(), e->pos().y() - r.top()));
				}
				else
          d->setPixmap(*LoadPixmap(keManyFilesIconBig));

				d->dragCopy();
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////

void CRightPanel::ResetRefreshTimer(int n)
{
	m_Timer.stop();
	m_Timer.start(1000);

	if (n == 1 || n == 4)
		m_LastRefreshTime = 0;
}

////////////////////////////////////////////////////////////////////////////

void CRightPanel::focusInEvent(QFocusEvent * /*e*/)
{
  if (m_bHasFocus) // return from menubar or its popup menus
    return;

  m_bHasFocus = TRUE;

	m_bMayBeEditLabel = 2;

	if (m_CurrentSelection.count() > 0)
	{
		QListIterator<CListViewItem> it(m_CurrentSelection);

		for (; it.current() != NULL; ++it)
		{
			it.current()->setSelected(TRUE);
			repaintItem(it.current());
		}

		m_CurrentSelection.clear();
		emit selectionChanged();
	}

	if (NULL == m_pStoredCurrentItem)
	{
		CListViewItem *pItem = firstChild();

		if (NULL != pItem && !isSelected(pItem))
		{
			setCurrentItem(pItem);
		}
	}
	else
	{
		setCurrentItem(m_pStoredCurrentItem);
		m_pStoredCurrentItem = NULL;
	}
}

////////////////////////////////////////////////////////////////////////////

void CRightPanel::focusOutEvent(QFocusEvent * /*e*/)
{
	setFocusPolicy(QWidget::StrongFocus);
  QTimer::singleShot(0, this, SLOT(OnFocusLost()));
}

////////////////////////////////////////////////////////////////////////////

void CRightPanel::SelectAll()
{
	CListViewItem *pI;

	for (pI = firstChild(); pI != NULL; pI = pI->nextSibling())
	{
		if (!isSelected(pI))
		{
			pI->setSelected(TRUE);
			repaintItem(pI);
		}
	}

	emit selectionChanged();
}


void CRightPanel::ActivateActions()
{
	KActionCollection *coll = actionCollection();
	KAction * action; //= new KAction;

  for (int i=0; i < gRightPanelMenuSize; i++)
	{ ;
  
		action = new KAction(LoadString(gRightPanelMenu[i].m_nItemLabelID), 0, this->parent()->parent(), gRightPanelMenu[i].m_pSlot, coll, gRightPanelActions[i]);
		if (i<8)
			action->setEnabled(false);
		m_ActionList.append(action);
  
	}
	return;
}

////////////////////////////////////////////////////////////////////////////

void CRightPanel::InvertSelection()
{
	CListViewItem *pI;

	for (pI = firstChild(); pI != NULL; pI = pI->nextSibling())
	{
    pI->setSelected(!isSelected(pI));
		repaintItem(pI);
	}

	emit selectionChanged();
}

////////////////////////////////////////////////////////////////////////////

void CRightPanel::SaveRefreshSelection()
{
	if (IsWebBrowser())
	  return;

  if (NULL == m_pExternalItem)
	{
		m_SavedPath = "";
		return;
	}

	if (IS_NETWORKTREEITEM(m_pExternalItem))
	{
		CNetworkTreeItem* pI = (CNetworkTreeItem*)m_pExternalItem;
    m_SavedPath = pI->FullName(FALSE);
	}
	else
		m_SavedPath = m_pExternalItem->text(0);

	m_ContentsX = contentsX(); // save scroll offset
	m_ContentsY = contentsY();

	m_RefreshSelection.clear();
	m_RefreshSelection.setAutoDelete(TRUE);

	if (m_CurrentSelection.count() > 0)
	{
		QListIterator<CListViewItem> it(m_CurrentSelection);

		for (; it.current() != NULL; ++it)
			m_RefreshSelection.append(new QString(it.current()->text(0)));

    m_RefreshStoredCurrent = m_pStoredCurrentItem->text(0);
	}
	else
	{
		CListViewItem *pI;

		if (currentItem() == NULL)
			m_RefreshStoredCurrent = "";
		else
			m_RefreshStoredCurrent = currentItem()->text(0);

    for (pI = firstChild(); pI != NULL; pI = pI->nextSibling())
		{
			if (isSelected(pI))
				m_RefreshSelection.append(new QString(pI->text(0)));
		}
	}
}

////////////////////////////////////////////////////////////////////////////

void CRightPanel::RestoreRefreshSelection()
{
	if (m_SavedPath.isEmpty())
		return;

	if (IS_NETWORKTREEITEM(m_pExternalItem))
	{
		CNetworkTreeItem* pI = (CNetworkTreeItem*)m_pExternalItem;

		if (m_SavedPath != pI->FullName(FALSE))
			return;
	}
	else
		if (m_SavedPath != m_pExternalItem->text(0))
			return;

	CListViewItem *pI;

  BOOL bHaveFocus = m_bHasFocus;

	for (pI = firstChild(); pI != NULL; pI = pI->nextSibling())
	{
		if (pI->text(0) == m_RefreshStoredCurrent)
		{
			if (bHaveFocus)
				setCurrentItem(pI);
			else
			{
				m_pStoredCurrentItem = pI;
			}

			break;
		}
	}

	m_RefreshStoredCurrent = "";

	QListIterator<QString> it(m_RefreshSelection);

	QString *pS;

	for (; it.current() != NULL; ++it)
	{
		pS = it.current();

		for (pI = firstChild(); pI != NULL; pI = pI->nextSibling())
		{
			if (pI->text(0) == *pS)
			{
				if (bHaveFocus)
					setSelected(pI, TRUE);
				else
					m_CurrentSelection.append(pI);

				break;
			}
		}
	}

	m_RefreshSelection.clear();

	qApp->processEvents();
	setContentsPos(m_ContentsX, m_ContentsY);
}

////////////////////////////////////////////////////////////////////////////
// SelectCurrentOnly() will remove selection from all selected items
// but current.
// It returns the number of items that have been unselected.

int CRightPanel::SelectCurrentOnly()
{
	CListViewItem *pI;
	int nCount = 0;

	for (pI = firstChild(); pI != NULL; pI = pI->nextSibling())
	{
		if (isSelected(pI) && pI != currentItem())
    {
      nCount++;
			pI->setSelected(FALSE);
			repaintItem(pI);
    }
	}

	emit selectionChanged();

	if (!isSelected(currentItem()))
		setSelected(currentItem(), TRUE);

  return nCount;
}

////////////////////////////////////////////////////////////////////////////

void CRightPanel::SelectByName(LPCSTR sName)
{
	CListViewItem *pI;

	for (pI = firstChild(); pI != NULL; pI = pI->nextSibling())
	{
		if (!strcmp((LPCSTR)pI->text(0)
#ifdef QT_20
  .latin1()
#endif
    , sName))
		{
			setCurrentItem(pI);
			setSelected(pI, TRUE);
			ensureItemVisible(pI);
		}
		else
			if (isSelected(pI))
				setSelected(pI, FALSE);
	}
}

////////////////////////////////////////////////////////////////////////////

void CRightPanel::StartLabelEdit()
{
	CListViewItem *pItem = currentItem();

	if (NULL != pItem && isSelected(pItem))
	{
		if (IS_NETWORKTREEITEM(pItem))
			((CWindowsTreeItem *)pItem)->StartLabelEdit();
	}
}

#if (QT_VERSION < 200)
////////////////////////////////////////////////////////////////////////////
// Returns TRUE if u and v are the same URL except for the anchor part, FALSE
// otherwise. For example, http://www.troll.no/qt/qwidget.html and
// http://www.troll.no/qt/qwidget#details are the same URL.

static BOOL SameURLExceptAnchors(LPCSTR u, LPCSTR v)
{
	if (u == NULL || v == NULL)
		return (u == v);

	while (*u != '\0' && *u != '#')
	{
		if (*u++ != *v++)
			return FALSE;
	}
	return (*v == '\0' || *v == '#');
}
#endif
////////////////////////////////////////////////////////////////////////////

CVector<QString,QString&> gVisited;

void CRightPanel::ShowBrowser(LPCSTR _url)
{
  m_pHTMLView->view()->setGeometry(0,0,width(),height());
	KURL url(_url);
	m_pHTMLView->openURL(url);
	m_pHTMLView->show();
}

////////////////////////////////////////////////////////////////////////////

void OnDocumentLoaded(int /*status*/, void * /*pParam*/)
{
}

////////////////////////////////////////////////////////////////////////////

void OnImageLoaded(int /*status*/, void */*pParam*/)
{
}

////////////////////////////////////////////////////////////////////////////

void OnImageToSaveLoaded(int /*status*/, void * /*pParam*/)
{
#if (QT_VERSION < 200)
	gTreeExpansionNotifier.DoEndWorking();
	gnWebOperationsCount--;
	CDocumentRequest *pRequest = (CDocumentRequest*)pParam;

	//printf("OnImageToSaveLoaded: status = %d, URL=%s, Local=%s\n", status, (LPCSTR)pRequest->m_URL, (LPCSTR)pRequest->m_LocalName);

	struct stat st;

	if (!stat(pRequest->m_LocalName, &st))
	{
		if (st.st_size <= 0)
		{
			//printf("Image %s saved, but the size is 0!!!!\n", (LPCSTR)pRequest->m_URL);
			if (pRequest->m_bTemp)
				unlink(pRequest->m_LocalName);
		}
	}
	else
		printf("Unable to save %s to %s\n", (LPCSTR)pRequest->m_URL, (LPCSTR)pRequest->m_LocalName);

	delete pRequest;
#endif
}

////////////////////////////////////////////////////////////////////////////

void CRightPanel::OnSetTitle(const QString &_title)
{
	m_CurrentTitle = LoadString(knAPP_TITLE) + gsExtraCaption + QString(": ") + _title;
	qApp->mainWidget()->setCaption(m_CurrentTitle);
}

////////////////////////////////////////////////////////////////////////////

void CRightPanel::OnRedirect(int delaySecs, const char *url)
{
	m_RedirectionTimer.stop();
	m_RedirectionURL = url;

	if (delaySecs < 0)
		delaySecs = 0;

	// If the redirection URL is invalid, we must not fall in a loop!
	if (!bLastRedirectionFailed)
		m_RedirectionTimer.start(delaySecs * 1000, TRUE);		// single shot
}

////////////////////////////////////////////////////////////////////////////

void CRightPanel::SetExternalItem(CListViewItem *pItem)
{
  if (NULL != m_pExternalItem && IS_NETWORKTREEITEM(m_pExternalItem))
  {
    CNetworkTreeItem *pI = (CNetworkTreeItem *)m_pExternalItem;

		pI->InitPixmap();

    if (pI->Kind() == keDeviceItem)
    {
      CDeviceItem *pDev = (CDeviceItem*)pI;

      if (pDev->m_MountedOn.isEmpty())
      {
        pDev->SetExpansionStatus(keNotExpanded);
      }
    }
  }

	m_pExternalItem = pItem;
}

////////////////////////////////////////////////////////////////////////////

// pos is in Global coordinates!

void CRightPanel::OnDNDEnter(const QPoint& position, QDragMoveEvent *e)
{
	m_ButtonState = GetMouseState(this);

  CListViewItem *pItem = GetItemAt(position);
	QPoint pos = viewport()->mapFromGlobal(position);

	QRect r = itemRect(pItem);

 	if (pos.y() < r.height()*2)
	{
		if (m_ScrollMode != 1)
		{
			QTimer::singleShot(300, this, SLOT(ScrollDown()));
			m_ScrollMode = 1;
		}
	}
	else
		if (pos.y() > viewport()->height()-r.height()*2)
		{
			if (m_ScrollMode != 2)
			{
				QTimer::singleShot(300, this, SLOT(ScrollUp()));
				m_ScrollMode = 2;
			}
		}
		else
			m_ScrollMode = 0;

	if (pItem != NULL)
	{
		if (pItem == m_pTemporarySelectedDragDestination)
			return;

		if (IS_NETWORKTREEITEM(pItem) &&
				((CNetworkTreeItem*)pItem)->ItemAcceptsDrops())
		{
			if (NULL != m_pTemporarySelectedDragDestination)
				setSelected(m_pTemporarySelectedDragDestination, FALSE);

			if (isSelected(pItem))
				m_pTemporarySelectedDragDestination = NULL;
			else
			{
				m_pTemporarySelectedDragDestination = pItem;
				setSelected(pItem, TRUE);
			}

			if (NULL != e)
      {
        if (ItemAcceptsThisDrop((CNetworkTreeItem*)pItem, m_DraggedURLList, FALSE))
          e->accept();
        else
          e->ignore();
      }
		}
		else
		{
			if (NULL != e)
				e->ignore();

			if (NULL != m_pTemporarySelectedDragDestination)
			{
				setSelected(m_pTemporarySelectedDragDestination, FALSE);
				m_pTemporarySelectedDragDestination = NULL;
			}
		}
	}
	else
	{
		if (NULL != m_pTemporarySelectedDragDestination)
			setSelected(m_pTemporarySelectedDragDestination, FALSE);

		if (NULL != e)
		{
			if (IS_NETWORKTREEITEM(m_pExternalItem) &&
					((CNetworkTreeItem*)m_pExternalItem)->ItemAcceptsDrops() &&
					ItemAcceptsThisDrop((CNetworkTreeItem*)m_pExternalItem, m_DraggedURLList, FALSE))
				e->accept();
			else
				e->ignore();
		}
	}
}

////////////////////////////////////////////////////////////////////////////

#ifndef QT_20
void CRightPanel::OnKDNDDropAction(KDNDDropZone *pZone)
{
	QStrList &list = pZone->getURLList();
	QString URL;

	CNetworkTreeItem *pItem = (CNetworkTreeItem *)
	((m_pTemporarySelectedDragDestination == NULL || !isSelected(m_pTemporarySelectedDragDestination)) ? m_pExternalItem : m_pTemporarySelectedDragDestination);

	QString Path(pItem->FullName(FALSE));
	CDropSelector Selector;

	if (IsTrashFolder(Path))
	{
		Selector.Go(this, QCursor::pos(), TRUE);

		if (Selector.type() == keDropMove)
			StartDelete(list, TRUE);
	}
	else
	{
		BOOL bMove = FALSE;

		if (m_ButtonState & RightButton)
		{
			BOOL bCanMove = ItemAcceptsThisDrop(pItem, list, TRUE);

			Selector.Go(this, QCursor::pos(), FALSE, !bCanMove);

			if (Selector.type() == keDropMove)
				bMove = TRUE;
			else
				if (Selector.type() != keDropCopy)
					return; // Aborted
		}

		QString URL = MakeItemURL((CNetworkTreeItem *)pItem);
		StartCopyMove(list, (LPCSTR)URL
#ifdef QT_20
  .latin1()
#endif
    , bMove, FALSE);
	}
}

////////////////////////////////////////////////////////////////////////////

void CRightPanel::OnKDNDDropEnter(KDNDDropZone *pZone)
{
	OnDNDEnter(QPoint(pZone->getMouseX(), pZone->getMouseY()), NULL);
}
////////////////////////////////////////////////////////////////////////////

void CRightPanel::OnKDNDDropLeave(KDNDDropZone *pZone)
{
	dragLeaveEvent(NULL);
}
#endif

////////////////////////////////////////////////////////////////////////////

void CRightPanel::ScrollDown()
{
	if (1 == m_ScrollMode)
	{
		scrollBy(0, iconView() ? -66 : -18);

		if (m_DraggedURLList.count() > 0)
		{
			QDragMoveEvent x(viewport()->mapFromGlobal(QCursor::pos()));
      dragMoveEvent(&x);
		}
		else
			OnDNDEnter(QCursor::pos(), NULL);

		QTimer::singleShot(300, this, SLOT(ScrollDown()));
	}
}

////////////////////////////////////////////////////////////////////////////

void CRightPanel::ScrollUp()
{
	if (2 == m_ScrollMode)
	{
		scrollBy(0, iconView() ? 66 : 18);

		if (m_DraggedURLList.count() > 0)
		{
			QDragMoveEvent x(viewport()->mapFromGlobal(QCursor::pos()));
      dragMoveEvent(&x);
		}
		else
			OnDNDEnter(viewport()->mapFromGlobal(QCursor::pos()), NULL);

		QTimer::singleShot(300, this, SLOT(ScrollUp()));
	}
}

////////////////////////////////////////////////////////////////////////////

void CRightPanel::OnRefreshWebBrowser()
{
        gVisited.clear();
        CBrowserCache::Instance()->SetLookupEnabled(FALSE);
        m_pHTMLView->begin(m_CurrentBrowserURL);
        m_pHTMLView->end();

        m_pHTMLView->openURL(m_CurrentBrowserURL);

}


////////////////////////////////////////////////////////////////////////////

void CRightPanel::OnCopyBrowserSelection()
{
#ifndef QT_20
  CCorelClipboard *cb = GetCorelClipboard();

	if (NULL != cb)
	{
		QString text = "";

		m_pHTMLView->getSelectedText(text);
		cb->setText(text.simplifyWhiteSpace());
	}
#endif
}

////////////////////////////////////////////////////////////////////////////

BOOL CRightPanel::IsWebBrowser()
{
#ifndef QT_20
  return m_pHTMLView != NULL  && m_pHTMLView->isVisible();
#else
  return false;
#endif
}

////////////////////////////////////////////////////////////////////////////

BOOL CRightPanel::IsWebBrowserWithFocus()
{
#ifndef QT_20
	return m_pHTMLView != NULL && m_pHTMLView->hasFocus();
#else
	return false;
#endif
}

////////////////////////////////////////////////////////////////////////////

CListViewItem *CRightPanel::GetItemAt(const QPoint& p)
{
	QPoint pos = viewport()->mapFromGlobal(p);
	CListViewItem *pItem = itemAt(pos);

	if (NULL == pItem || iconView())
	{
		return pItem;
	}

	int x = pos.x();

	if (x < header()->cellPos(header()->mapToActual(0)) ||
			x > header()->cellPos(header()->mapToActual(0)) + columnWidth(0))
		return NULL;

	QPainter painter;
	painter.begin(this);
	int w = painter.boundingRect(0, 0, columnWidth(0), 500, AlignLeft, pItem->text(0)).width();
	painter.end();

	if (x > header()->cellPos(header()->mapToActual(0)) + w + itemMargin()*2 + pItem->pixmap(0)->width())
		return NULL;

	return pItem;
}

////////////////////////////////////////////////////////////////////////////

void CRightPanel::OnRefreshRequested(const char *Path)
{
	if (NULL != m_pExternalItem &&
      IS_NETWORKTREEITEM(m_pExternalItem))
  {
    if (((CNetworkTreeItem*)m_pExternalItem)->FullName(FALSE) == Path)
    {
      RescanItem((CNetworkTreeItem*)m_pExternalItem);

      m_LastRefreshTime = 0; // force refresh
      ActivateEvent();
    }
  }
}

////////////////////////////////////////////////////////////////////////////

bool CRightPanel::eventFilter(QObject *pObject, QEvent *pEvent)
{
  if (m_pCurrentFocusTrack == pObject && pEvent->type() == Event_FocusOut)
  {
    m_pCurrentFocusTrack->removeEventFilter(this);
    QTimer::singleShot(0, this, SLOT(OnFocusLost()));
  }

  return CListView::eventFilter(pObject, pEvent);
}

////////////////////////////////////////////////////////////////////////////

void CRightPanel::OnFocusLost()
{
  QWidget *pFocus = qApp->focusWidget();

  if (NULL != pFocus &&
      (pFocus->inherits("QPopupMenu") || pFocus->inherits("QMenuBar")))
  {
    m_pCurrentFocusTrack = pFocus;
    m_pCurrentFocusTrack->installEventFilter(this);
  }
  else
  {
    if (this != pFocus)
    {
      m_pStoredCurrentItem = currentItem(); // this is for label editor

      m_bHasFocus = FALSE;
      m_CurrentSelection.clear();

      CListViewItem *pI;

      for (pI = firstChild(); pI != NULL; pI = pI->nextSibling())
      {
        if (isSelected(pI))
        {
          pI->setSelected(FALSE);
          repaintItem(pI);
          m_CurrentSelection.append(pI);
        }
      }

      emit selectionChanged();

      m_pStoredCurrentItem = currentItem();
      setCurrentItem(NULL);
    }
  }
}

////////////////////////////////////////////////////////////////////////////

void CRightPanel::OnURL(const QString &_url)
{
	emit StatusMessage(_url);
}

////////////////////////////////////////////////////////////////////////////

void CRightPanel::OnOpenURL(const KURL& _url, const KParts::URLArgs &_args)
{
	m_pHTMLView->browserExtension()->setURLArgs(_args);
	m_pHTMLView->openURL(_url);
	emit ChdirRequest(m_pExternalItem, _url.url(-1));
}

////////////////////////////////////////////////////////////////////////////

