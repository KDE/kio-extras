/* Name: rightpanel.h

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

#ifndef __INC_RIGHTPANEL_H__
#define __INC_RIGHTPANEL_H__

#include "common.h"
#include "treeitem.h"
#include <qheader.h>
#include <qlabel.h>
#include <qlist.h>
#include <qtimer.h>
#include "qdropsite.h"
#include <qdragobject.h>
#include "almosthtmlview.h"
#include "explres.h"
#ifndef QT_20
#include "drag.h"
#endif

#include "khtml_part.h"
#include <kaction.h>



class KHTMLPart;
class KHTMLView;
class CRightPanel;

class CRightPanel : public CListView, public QDropSite, public KXMLGUIClient
{
	Q_OBJECT
public:
	CRightPanel(QWidget *parent = 0, const char *name = 0);
	~CRightPanel();
	void RefreshIcons();
	BOOL PromptForPassword(CRemoteFileContainer *pItem);
	void SetHeaderType(int nType);
	void SelectAll();
	void InvertSelection();
	void SelectByName(LPCSTR sName);
	void SetExternalItem(CListViewItem *pItem);
	void ActivateActions();

signals:

	void PartRequest(CListViewItem*);
	void ChdirRequest(CListViewItem* Base, QString Destination);
	void GoParentRequest();

	void UndoRequest();
	void CutRequest();
	void CopyRequest();
	void PasteRequest();
	void DeleteRequest();
	void NukeRequest();
	void BackRequest();
	void ForwardRequest();
  void ConsoleRequest();
	void BookmarkURLRequest(const char *Title, const char *Url);

	void UpdateCompleted(int nObjectsCount, double TotalSize, int nSelectedCount);
	void PopupMenuRequest(const QPoint& p);
	void PropertiesRequest();
	void TabRequest(BOOL bIsBacktab);
  void StatusMessage(const char *);
	void DocumentDone(const char *InitialURL, const char *NewUrl, BOOL bIntranet);
	void BrowserTextSelected(BOOL bSelected);

public slots:
	void OnStop();
	void refresh();
	void ActivateEvent();
	void DoRedirection();
	void OnDoubleClicked(CListViewItem *item);
	void OnRightClicked(CListViewItem *pItem, const QPoint& p, int);
	void OnExpansionDone(CNetworkTreeItem *);
	void OnExpansionCanceled(CNetworkTreeItem *);
	void OnSelectionChanged();
	void OnSetTitle(const QString &_title);
	void OnRedirect(int delaySecs, const char *url);
	void OnURL(const QString &_url);
	void OnOpenURL(const KURL& _url, const KParts::URLArgs &_args);
#ifndef QT_20
//	void OnKDNDDropEnter(KDNDDropZone *pZone);
//	void OnKDNDDropAction(KDNDDropZone *pZone);
//	void OnKDNDDropLeave(KDNDDropZone*);
#endif
	void ScrollDown();
	void ScrollUp();
	void OnRefreshWebBrowser();
	void OnCopyBrowserSelection();
  void OnRefreshRequested(const char *Path);
  void OnFocusLost();

protected:
	void contentsDragEnterEvent(QDragEnterEvent *);
	void contentsDragMoveEvent(QDragMoveEvent *);
	void contentsDragLeaveEvent(QDragLeaveEvent *);
	void contentsDropEvent(QDropEvent *);

	void resizeEvent(QResizeEvent *e);
	bool event(QEvent *e);
	void keyPressEvent(QKeyEvent *e);
#ifndef QT_20
	void mouseMoveEvent(QMouseEvent *e);
	void mousePressEvent(QMouseEvent *e);
	void mouseReleaseEvent(QMouseEvent *e);
	void mouseDoubleClickEvent(QMouseEvent *e);
#else
	void viewportMouseMoveEvent(QMouseEvent *e);
	void viewportMousePressEvent(QMouseEvent *e);
	void viewportMouseReleaseEvent(QMouseEvent *e);
	void viewportMouseDoubleClickEvent(QMouseEvent *e);
#endif
	void focusInEvent(QFocusEvent *e);
	void focusOutEvent(QFocusEvent *e);
	void SetBackgroundEnabled(BOOL bIsEnabled, int nMessageID);
	void OnDNDEnter(const QPoint& pos, QDragMoveEvent *e);
  bool eventFilter(QObject *pObject, QEvent *pEvent);

public:

	CListViewItem *m_pExternalItem;
	void ResetRefreshTimer(int n);
	QString FullName(CListViewItem *pItem);
	void ShowBrowser(LPCSTR Url);

	BOOL IsWebBrowser();
	BOOL IsWebBrowserWithFocus();
	KHTMLPart *m_pHTMLView;

	CListViewItem *GetItemAt(const QPoint& p);

private:
	QLabel *m_pSearchMovieLabel;
	QLabel *m_pNavigationCanceledLabel;
	QFrame *m_pBrowserFrame;
	QString m_LastLoadedBrowserURL;
	QString m_CurrentBrowserURL;
	QString m_RedirectionURL;
	QString m_CurrentTitle;
	QString m_CurrentFrameTarget;

	QMovie m_Movie;
	double m_TotalSize;
	int m_nItemCount;
	BOOL m_bCanBeginDrag;
  QPoint m_StartDragPoint;
	void BuildItemList(CListViewItem *pItem, QList<CListViewItem>& ItemList);
	BOOL m_bRefreshMode;
	QTimer m_Timer;
	QTimer m_RedirectionTimer;
	time_t m_LastRefreshTime;
	QList<CListViewItem> m_CurrentSelection;
	QList<KAction> m_ActionList;
	CListViewItem *m_pStoredCurrentItem;
	//BOOL m_bInDrag;
	CListViewItem *m_pTemporarySelectedDragDestination;
	QStrList m_DraggedURLList;
	BOOL m_bMayBeEditLabel;
	QList<QString> m_RefreshSelection;
	QString m_RefreshStoredCurrent;
	BOOL m_bBackgroundEnabled;
	QColor m_EnabledBaseColor;
private:
#ifndef QT_20
	KDNDDropZone m_DropZone;
#endif

	int m_ScrollMode;
  BOOL m_bCatchReleaseMode;
  int m_ButtonState;

	int m_ContentsX;
	int m_ContentsY;
	QString m_SavedPath;
  QObject *m_pCurrentFocusTrack;

	void SaveRefreshSelection();
	void RestoreRefreshSelection();
	int SelectCurrentOnly();
	void StartLabelEdit();
	void RefreshListview(CListViewItem *qitem, BOOL bIsOurTree, BOOL &bEnableBackground, int &nMessageID);
public:
	BOOL m_bHasFocus;
	BOOL m_bTextSelected;
};

#endif /* __INC_RIGHTPANEL_H__ */
