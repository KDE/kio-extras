/* Name: mapdialog.cpp

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

/**********************************************************************

	--- Qt Architect generated file ---

	File: mapdialog.cpp
	Last generated: Wed Nov 4 11:07:25 1998

 *********************************************************************/

#include "qlistview.h"

#if (QT_VERSION < 200)
#define QListView CListView
#endif

#include "listview.h"

#include "mapdialog.h"
#include "qfiledialog.h"
#include "qheader.h"
#include "msroot.h"
#include "nfsroot.h"
#include "credentials.h"
#include <qmessagebox.h>

#include <stdlib.h> // for getenv()...

#define Inherited CMapDialogData

////////////////////////////////////////////////////////////////////////////

CMapDialog::CMapDialog
(
	LPCSTR MountPoint,
	LPCSTR UNCPath,
	BOOL bReconnectAtLogon,
	int nCredentialsIndex,
	CMSWindowsNetworkItem *pOtherTree,
	QWidget* parent,
	const char* name
)
	:
	Inherited(parent, name)
{
  setCaption(LoadString(knMOUNT_NETWORK_SHARE));
	/*ConvertDlgUnits(this);*/
	
	m_pShareToMountLabel->setText(LoadString(knSHARE_TO_MOUNT_COLON));
	m_pShareToMountLabel->setBuddy(m_pUNCPath);

  m_pMountPointLabel->setText(LoadString(knMOUNT_POINT_COLON));
  m_pMountPointLabel->setBuddy(m_pMountPoint);

	m_pBrowseButton->setText(LoadString(kn_BROWSE_DOTDOTDOT));
	
  m_pConnectAsLabel->setText(LoadString(knCONNECT_AS_COLON));
  m_pConnectAsLabel->setBuddy(m_pConnectAs);

	m_pReconnectAtLogon->setText(LoadString(kn_RECONNECT_AT_LOGON));
	
	m_pSharedDirectoriesLabel->setText(LoadString(knSHARED_DIRECTORIES_COLON));
	m_pSharedDirectoriesLabel->setBuddy(m_Tree);

  m_OKButton->setText(LoadString(knOK));
	m_pCancelButton->setText(LoadString(knCANCEL));
	
	int LargestLabel;
	if (m_pConnectAsLabel->width()>m_pMountPointLabel->width())
		if (m_pConnectAsLabel->width()>m_pShareToMountLabel->width())
			LargestLabel=m_pConnectAsLabel->width();
		    else
			LargestLabel=m_pShareToMountLabel->width();
	    else
		if (m_pMountPointLabel->width()>m_pShareToMountLabel->width())
			LargestLabel=m_pMountPointLabel->width();
		    else
			LargestLabel=m_pShareToMountLabel->width();

	m_pUNCPath->setGeometry( 20+LargestLabel, 15, 398-(20+LargestLabel), 22 );
	m_pMountPoint->setGeometry( 20+LargestLabel, 45, 398-(20+LargestLabel), 22 );
	m_pConnectAs->setGeometry( 20+LargestLabel, 107, 398-(20+LargestLabel), 22 );
	
	int nReconnectWidth = m_pReconnectAtLogon->sizeHint().width();

	m_pReconnectAtLogon->setGeometry(width() - 8 - nReconnectWidth, m_pReconnectAtLogon->y(), nReconnectWidth, m_pReconnectAtLogon->height());

	m_OKButton->setDefault(TRUE);

  QString sMountPoint(MountPoint);

  if (sMountPoint.isEmpty())
  {
    sMountPoint = getenv("HOME");
  }
  
  if (sMountPoint.right(1) != "/")
    sMountPoint += "/";

  m_pMountPoint->setText(sMountPoint);
	
  connect(m_pUNCPath, SIGNAL(textChanged(const char *)), this, SLOT(OnUNCEditChanged(const char *)));

  m_pUNCPath->setText(UNCPath);
  m_pUNCPath->setFocus();
	
	m_pReconnectAtLogon->setChecked(bReconnectAtLogon);

	m_nCredentialsIndex = nCredentialsIndex;

	/* Tree */

	((QWidget*)m_Tree->header())->hide();
	m_Tree->installEventFilter(this);
	
	CMSWindowsNetworkItem *pRoot = NULL;

	if (gbNetworkAvailable)
		pRoot = new CMSWindowsNetworkItem(m_Tree, keDetail_Shares);
	
  /*CNFSNetworkItem *pNFSRoot = */
	new CNFSNetworkItem(m_Tree);
	
	connect(m_Tree, SIGNAL(doubleClicked(CListViewItem*)), this, SLOT(OnDoubleClicked(CListViewItem*)));
	
	// Filling the tree

	m_bAbort = FALSE;

	if (NULL != pRoot)
  {
    if (NULL != pOtherTree && pOtherTree->ExpansionStatus() == keExpansionComplete)
	  {
		  //if (pOtherTree->ExpansionStatus() == keNotExpanded)
			//  pOtherTree->Fill();

		  if (pOtherTree->childCount())
			  pRoot->Fill(pOtherTree);
		  else
			  m_bAbort = TRUE;
	  }

	  if (!m_bAbort)
	  {
  		//pRoot->setOpen(TRUE);
  	
		  //if (!pRoot->childCount())
//  			m_bAbort = TRUE;
  	}
  }
	
	// We update "Connect As..." control because filling the tree
	// could potentially update gCredentials[0]

	UpdateConnectAs();
}

////////////////////////////////////////////////////////////////////////////

CMapDialog::~CMapDialog()
{
}

////////////////////////////////////////////////////////////////////////////

void CMapDialog::OnBrowse()
{
	QString Target = QFileDialog::getExistingDirectory(m_pMountPoint->text(), this, LoadString(knBROWSE_FOR_FOLDER));
	
	if ((LPCSTR)Target != NULL)
		m_pMountPoint->setText(Target);
}

////////////////////////////////////////////////////////////////////////////

void CMapDialog::done(int r)
{
	if (r == Accepted)
	{
		/* Get and validate mount point and share name */

		m_MountPoint = m_pMountPoint->text();
		m_UNCPath = m_pUNCPath->text();
    
    /* Removed May 31, 2000: we now allow spaces in share names
    
    if (-1 != m_UNCPath.find(' '))
    {
			QMessageBox::warning(this, LoadString(knUNC_PATH), LoadString(knSHARE_NAME_SPACES_NOT_ALLOWED));
			m_pUNCPath->setFocus();
			return;
    }
		*/

		if (m_MountPoint.isEmpty())
		{
			QMessageBox::warning(this, LoadString(knMOUNT_POINT), LoadString(knENTER_MOUNT_POINT));
			m_pMountPoint->setFocus();
			return;
		}

    /* Removed May 31, 2000: we now allow spaces in mount points
    if (-1 != m_MountPoint.find(' '))
    {
			QMessageBox::warning(this, LoadString(knMOUNT_POINT), LoadString(knMOUNT_POINT_SPACES_NOT_ALLOWED));
			m_pMountPoint->setFocus();
			return;
    }
    */
		
    if (!EnsureDirectoryExists(m_MountPoint))
		{
			m_pMountPoint->setFocus();
			return;
		}

		if (!CanMountAt(m_MountPoint))
		{
			QString s;
			s.sprintf(LoadString(knUNABLE_TO_MOUNT), (LPCSTR)m_MountPoint);

			QMessageBox::warning(this, LoadString(knMOUNT_POINT), (LPCSTR)s);
			m_pMountPoint->setFocus();
			return;
		}

		/* Get and validate UNC path */

		if (m_UNCPath.isEmpty())
		{
			QMessageBox::warning(this, LoadString(knUNC_PATH), LoadString(knENTER_UNC_PATH));
			m_pUNCPath->setFocus();
			return;
		}

		if (m_UNCPath.left(6) != "nfs://")
		{
      QString Server, Share, Path;
  
      if (!ParseUNCPath(m_UNCPath, Server, Share, Path) || Share.isEmpty())
			{
				QMessageBox::warning(this, LoadString(knUNC_PATH), LoadString(knBAD_UNC_PATH));
				m_pUNCPath->setFocus();
				return;
			}
		}

		/* Get "reconnect at logon" flag */

		m_bReconnectAtLogon = m_pReconnectAtLogon->isChecked();

		/* Get "connect as" string */

		QString s(m_pConnectAs->text());

		if (!s.isEmpty())
		{
			CCredentials cred;

			LPCSTR p = (LPCSTR)s;
			cred.m_Workgroup = ExtractWord(p, "/\\");
			cred.m_UserName = ExtractWord(p, "/\\");
			
			cred.m_Password = gCredentials[0].m_Password;

			if (cred.m_UserName.isEmpty())
			{
				cred.m_UserName = cred.m_Workgroup;
				cred.m_Workgroup = gCredentials[m_nCredentialsIndex].m_Workgroup;
			}
		
			//printf("Workgroup = <%s>, Username = <%s>\n",
			//				(LPCSTR)cred.m_Workgroup,
			//				(LPCSTR)cred.m_UserName);

			m_nCredentialsIndex = gCredentials.Find(cred);
		
			if (m_nCredentialsIndex == -1)
				m_nCredentialsIndex = gCredentials.Add(cred);
		}
	}

	QDialog::done(r);
}

////////////////////////////////////////////////////////////////////////////

void CMapDialog::OnDoubleClicked(CListViewItem *pItem)
{
	if (NULL == pItem)
		return;

	if (!pItem->isExpandable())
	{
		CNetworkTreeItem *pTreeItem = (CNetworkTreeItem*)pItem;

		m_pUNCPath->setText(pTreeItem->FullName(FALSE));

		if (!stricmp(m_pConnectAs->text(), m_DefaultConnectAs))
		{
			m_nCredentialsIndex = pTreeItem->CredentialsIndex();
			UpdateConnectAs();
		}
	}
}

////////////////////////////////////////////////////////////////////////////

void CMapDialog::UpdateConnectAs()
{
  if (gCredentials[m_nCredentialsIndex].m_Workgroup	!= "%notset%")
  {
    m_DefaultConnectAs.sprintf("%s\\%s", (LPCSTR)gCredentials[m_nCredentialsIndex].m_Workgroup, (LPCSTR)gCredentials[m_nCredentialsIndex].m_UserName);
	  m_pConnectAs->setText(m_DefaultConnectAs);
  }
}

////////////////////////////////////////////////////////////////////////////

void CMapDialog::OnUNCEditChanged(const char *s)
{
  QString MountPoint = m_pMountPoint->text();
  int nIndex = MountPoint.findRev('/');
  
  if (-1 != nIndex)
  {
    if (strlen(s) > 5 && !strncmp(s, "nfs://", 6))
    {
      QString Path(s+6);

      int nIndex2 = Path.findRev('/');
      
      if (-1 != nIndex2)
        m_pMountPoint->setText(MountPoint.left(nIndex+1) + Path.mid(nIndex2+1, Path.length()));

      m_pConnectAs->setEnabled(FALSE);
      m_pConnectAsLabel->setEnabled(FALSE);
    }
    else
    {
      QString Server, Share, Path;
  
      if (ParseUNCPath(s, Server, Share, Path) && !Share.isEmpty())
      {
        m_pMountPoint->setText(MountPoint.left(nIndex+1) + Share);
      }
      
      m_pConnectAs->setEnabled(TRUE);
      m_pConnectAsLabel->setEnabled(TRUE);
    }
  }
}

////////////////////////////////////////////////////////////////////////////

bool CMapDialog::eventFilter(QObject *object, QEvent *event)
{
#ifdef QT_20
  if (event->type() == QEvent::KeyPress &&
#else
  if (event->type() == Event_KeyPress &&
#endif      
      ((QKeyEvent *)event)->key() == Qt::Key_Space)
	{
		OnDoubleClicked(m_Tree->currentItem());
		return TRUE;
	}
	
	return FALSE;
}

////////////////////////////////////////////////////////////////////////////

