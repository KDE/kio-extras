/* Name: PropDialog.h

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

#include "common.h"
#include "PropDialog.h"

#include "FSPropGeneral.h"
#include "FilePropGeneral.h"
#include "FTPFilePropGeneral.h"
#include "ServPropGeneral.h"
#include "NetFilePropGeneral.h"
#include "SharePropGeneral.h"
#include "DevicePropGeneral.h"
#include "TrashEntryPropGeneral.h"
#include "filesystem.h"
#include "SharingPage.h"
#include "SharingPageNFS.h"
#include "FilePermissions.h"
#include <qtabbar.h>
#include <qlist.h>
#include "propres.h"
#include "device.h"
#define Inherited CPropDialogData
#include "expcommon.h"
#include "smbserver.h"
#include "smbshare.h"
#include "smbutil.h"
#include "PasswordDlg.h"
#include "qmessagebox.h"

// printutil includes

#include "commonfunc.h"
#include "generaldlg.h"
#include "advanceddlg.h"
#include "outputdlg.h"

////////////////////////////////////////////////////////////////////////////

QObject *GetOKButton(QObject *pTabDialog)
{
	return FindChildByName((QWidget*)pTabDialog, "ok");
}

////////////////////////////////////////////////////////////////////////////

void CPropDialog::HandleLocalPrinter(LPCSTR PrinterName, BOOL& bCanDoSambaSharing)
{
  BOOL bIsReadOnly = !IsSuperUser();

  m_pPrinterObject = new KPrinterObject;
  setPrinterObject(PrinterName, m_pPrinterObject);

  switch (m_pPrinterObject->getPrinterType())
  {
    default:  // anything unexpected? Umm.. I dunno...
    case PRINTER_TYPE_LOCAL:
      m_pPrinterGeneral = new KPrinterGeneralLocalDlg(bIsReadOnly, this, "page1", m_pPrinterObject);
      bCanDoSambaSharing = TRUE;
    break;

    case PRINTER_TYPE_UNIX:
      m_pPrinterGeneral = new KPrinterGeneralUnixDlg(bIsReadOnly, this, "page1", m_pPrinterObject);
    break;

    case PRINTER_TYPE_SMB:
      m_pPrinterGeneral = new KPrinterGeneralSambaDlg(bIsReadOnly, this, "page1", m_pPrinterObject);
    break;
  }

  addTab(m_pPrinterGeneral, LoadString(knGENERAL));

  m_pPrinterAdvanced = new KPrinterAdvancedDlg(bIsReadOnly, this, "page2", m_pPrinterObject);
  addTab(m_pPrinterAdvanced, LoadString(kn_ADVANCED));

  m_pPrinterOutput = new KPrinterOutputDlg(bIsReadOnly, this, "page3", m_pPrinterObject);
  addTab(m_pPrinterOutput, LoadString(kn_OUTPUT));
}

CPropDialog::CPropDialog
(
	QList<CNetworkTreeItem>& ItemList,
	BOOL bStartFromSharing,
	QWidget* parent,
	const char* name
)
	:
	Inherited( parent, name )
{
	m_pPrinterObject = NULL;
  m_pPrinterGeneral = NULL;
  m_pPrinterAdvanced = NULL;
  m_pPrinterOutput = NULL;
  m_pWindowsSharingPage = NULL;

  connect(this, SIGNAL(aboutToShow()), SLOT(OnInitDialog()));

	m_bStartFromSharing = bStartFromSharing;

	BOOL bCanDoSambaSharing = FALSE;
	BOOL bCanDoNFSSharing = FALSE;
	QString Caption;
	QString SharingPath;
	m_pFilePermissionsPage = NULL;
	m_pNFSSharingPage = NULL;

	if (ItemList.count() > 0)
	{
		CNetworkTreeItem *pItem = ItemList.getFirst();

		switch (pItem->Kind())
		{
			default:
			break;

			case keLocalFileItem:
			{
				CFilePropGeneral *Page = new CFilePropGeneral(ItemList, this, "page1");
				addTab(Page, LoadString(knGENERAL));

				CLocalFileItem *pI = (CLocalFileItem*)pItem;

				if (ItemList.count() == 1 && pI->CanEditPermissions())
				{
          m_pFilePermissionsPage = new CFilePermissions((LPCSTR)pItem->FullName(FALSE)
#ifdef QT_20
  .latin1()
#endif
          ,
                                         (LPCSTR)pItem->text(0)
#ifdef QT_20
  .latin1()
#endif
                                         ,
                                         pItem->BigPixmap(),
                                         this,
                                         (LPCSTR)"PermissionsPage");
          addTab(m_pFilePermissionsPage, LoadString(kn_PERMISSIONS));
				}

				bCanDoSambaSharing = ((CLocalFileItem*)pItem)->IsFolder();
        bCanDoNFSSharing = bCanDoSambaSharing;

				if (bCanDoSambaSharing)
					SharingPath = pItem->FullName(FALSE);
			}
			break;

			case keFTPFileItem:
			{
				CFTPFilePropGeneral *Page = new CFTPFilePropGeneral(ItemList, this, "page1");
				addTab(Page, LoadString(knGENERAL));
			}
			break;

			case keFileSystemItem:
			{
				QListIterator<CNetworkTreeItem> it(ItemList);

				for (; it.current() != NULL; ++it)
				{
					pItem = it.current();
					CFileSystemItem* pFSI = (CFileSystemItem*)pItem;

					CFSPropGeneral *Page = new CFSPropGeneral(pFSI, this, "page1");
					addTab(Page, ItemList.count() == 1 ? LoadString(knGENERAL) : TOQTTEXTTYPE(pItem->text(0))
//#ifdef QT_20
//  .latin1()
//#endif
          );
					Caption.sprintf(LoadString(knPROPERTIES_OF_X), (LPCSTR)pFSI->m_MountedOn
#ifdef QT_20
  .latin1()
#endif
          );

          if (ItemList.count() == 1 && pFSI->CanEditPermissions())
          {
            m_pFilePermissionsPage = new CFilePermissions((LPCSTR)pItem->FullName(FALSE)
#ifdef QT_20
  .latin1()
#endif
            ,
                                           (LPCSTR)pItem->text(0)
#ifdef QT_20
  .latin1()
#endif
                                           ,
                                           pItem->BigPixmap(),
                                           this,
                                           (LPCSTR)"PermissionsPage");
            addTab(m_pFilePermissionsPage, LoadString(kn_PERMISSIONS));
          }

          bCanDoSambaSharing = TRUE;
          bCanDoNFSSharing = TRUE;
          SharingPath = pFSI->m_MountedOn;
				}
			}
			break;

			case keServerItem:
			{
				CServPropGeneral *Page = new CServPropGeneral(
          (LPCSTR)pItem->text(0)
#ifdef QT_20
  .latin1()
#endif
          , // Server name
          (LPCSTR)((CServerItem*)pItem)->m_Comment
#ifdef QT_20
  .latin1()
#endif
          , // Server comment
          (LPCSTR)pItem->m_pLogicalParent->text(0)
#ifdef QT_20
  .latin1()
#endif
          ,
          this,
          (LPCSTR)"page1");

        addTab(Page, LoadString(knGENERAL));
			}
			break;

			case keFileItem:
			{
				CFileItem *pFileItem = (CFileItem *)pItem;

        CNetFilePropGeneral *Page = new CNetFilePropGeneral(
          (LPCSTR)pFileItem->text(0)
#ifdef QT_20
  .latin1()
#endif
          ,
          (LPCSTR)pFileItem->FullName(FALSE)
#ifdef QT_20
  .latin1()
#endif
          ,
          (LPCSTR)pFileItem->m_FileSize
#ifdef QT_20
  .latin1()
#endif
          ,
          (LPCSTR)pFileItem->text(3)
#ifdef QT_20
  .latin1()
#endif
          , // Modify date
          (LPCSTR)pFileItem->m_FileAttributes
#ifdef QT_20
  .latin1()
#endif
          ,
          pFileItem->Pixmap(TRUE),
          this,
          (LPCSTR)"page1");

        addTab(Page, LoadString(knGENERAL));
			}
			break;

			case keShareItem:
			{
				CSharePropGeneral *Page = new CSharePropGeneral(
          (LPCSTR)pItem->text(0)
#ifdef QT_20
  .latin1()
#endif
          , // Share name
          (LPCSTR)pItem->FullName(FALSE)
#ifdef QT_20
  .latin1()
#endif
          ,
          (LPCSTR)pItem->text(1)
#ifdef QT_20
  .latin1()
#endif
          , // Share type
          (LPCSTR)pItem->text(2)
#ifdef QT_20
  .latin1()
#endif
          , // Share comment
          this,
          (LPCSTR)"page1");

        addTab(Page, LoadString(knGENERAL));
			}
			break;

      case keTrashEntryItem:
      {
        CTrashEntryItem *pTrashEntry = (CTrashEntryItem *)pItem;

				CTrashEntryPropGeneral *Page = new CTrashEntryPropGeneral(pTrashEntry, this, "page1");
				addTab(Page, LoadString(knGENERAL));
      }
      break;

      case keLocalPrinterItem:
      {
        HandleLocalPrinter((LPCSTR)pItem->text(0)
#ifdef QT_20
  .latin1()
#endif
        , bCanDoSambaSharing);

        if (bCanDoSambaSharing)
          SharingPath = pItem->FullName(FALSE);
      }
      break;

      case keDeviceItem:
      {
				CDeviceItem *pDev = (CDeviceItem *)pItem;

        CDevicePropGeneral *Page = new CDevicePropGeneral(
          (LPCSTR)pItem->text(0)
#ifdef QT_20
  .latin1()
#endif

          , // Device name
          (LPCSTR)pDev->m_Device
#ifdef QT_20
  .latin1()
#endif
          , // Device file name
          (LPCSTR)pDev->m_Model
#ifdef QT_20
  .latin1()
#endif
          , // Device model
          (LPCSTR)pDev->m_Driver
#ifdef QT_20
  .latin1()
#endif
          , // Device driver
          (LPCSTR)pDev->m_MountedOn
#ifdef QT_20
  .latin1()
#endif
          , // Mounted On
          pDev->Pixmap(TRUE),
          this,
          (LPCSTR)"page1");

        addTab(Page, LoadString(knGENERAL));
        bCanDoSambaSharing = !pDev->m_MountedOn.isEmpty();
        bCanDoNFSSharing = bCanDoSambaSharing;

        if (bCanDoSambaSharing)
        {
          SharingPath = pDev->m_MountedOn;

          CFSPropGeneral *Page = new CFSPropGeneral((CFileSystemInfo*)pDev, this, "page3");
          addTab(Page, "File System");
        }
      }
      break;
		}

		if (Caption.isEmpty())
			Caption.sprintf(LoadString(knXY_PROPERTIES), (LPCSTR)pItem->text(0)
#ifdef QT_20
  .latin1()
#endif
                                                                          , ItemList.count() == 1 ? "" : ", ...");
	}

	setCaption(Caption);

	if (ItemList.count() == 1 &&
		gbNetworkAvailable &&
		bCanDoSambaSharing)
	{
		m_pWindowsSharingPage = new CSharingPage((LPCSTR)SharingPath
#ifdef QT_20
  .latin1()
#endif
                                                        ,ItemList.getFirst()->BigPixmap(), this, (LPCSTR)"page2");
		QTab *z = new QTab();
		z->label = LoadString(knSHARING_2);
		z->id = 1;
		addTab(m_pWindowsSharingPage, z);
	}
	else
		m_pWindowsSharingPage = NULL;

	if (bCanDoNFSSharing && HasNFSSharing())
	{
		m_pNFSSharingPage = new CSharingPageNFS((LPCSTR)SharingPath
#ifdef QT_20
  .latin1()
#endif
                                                                , ItemList.getFirst()->BigPixmap(), this, (LPCSTR)"page4");
		QTab *z = new QTab();
		z->label = LoadString(knNFS_SHARING);
		z->id = 1;
		addTab(m_pNFSSharingPage, z);
	}

	setCancelButton();

	QObject *pOK = GetOKButton(this);

	disconnect(pOK, SIGNAL(clicked()), this, SLOT(accept()));
	connect(pOK, SIGNAL(clicked()), this, SLOT(OnOK()));

  QTimer::singleShot(50, this, SLOT(FixSize()));
  //connect(&gTreeExpansionNotifier, SIGNAL(ItemDestroyed(CListViewItem*)), SLOT(OnItemDestroyed(CListViewItem*)));
}

////////////////////////////////////////////////////////////////////////////

CPropDialog::CPropDialog
(
	LPCSTR URL,
	BOOL bStartFromSharing,
	QWidget* parent,
	const char* name
)
	:
	Inherited( parent, name )
{
	m_pPrinterObject = NULL;
  m_pPrinterGeneral = NULL;
  m_pPrinterAdvanced = NULL;
  m_pPrinterOutput = NULL;
  m_pWindowsSharingPage = NULL;
	m_pFilePermissionsPage = NULL;

  connect(this, SIGNAL(aboutToShow()), SLOT(OnInitDialog()));

	m_bStartFromSharing = bStartFromSharing;

	BOOL bCanDoSambaSharing = FALSE;
  BOOL bCanDoNFSSharing = FALSE;

	QString Caption;
	QString SharingPath;
	m_pNFSSharingPage = NULL;

  QPixmap *pPixmap = NULL;

	if (IsUNCPath(URL))
  {
    QString Server, Share, Path;

    ParseUNCPath(URL, Server, Share, Path);
    Path = MakeSlashesForward((LPCSTR)Path
#ifdef QT_20
  .latin1()
#endif
    );

    if (!Path.isEmpty())
    {
      CFileArray list;
      int nCredentialsIndex = 0;
      QString ParentURL = MakeSlashesForward((LPCSTR)GetParentURL(URL)
#ifdef QT_20
  .latin1()
#endif

      );
      QString FileName = URL + ParentURL.length() + 1;

FileDoAgain:;

      switch (GetFileList((LPCSTR)ParentURL
#ifdef QT_20
  .latin1()
#endif
      , &list, nCredentialsIndex, TRUE, (LPCSTR)FileName
#ifdef QT_20
  .latin1()
#endif

      ))
      {
        case keErrorAccessDenied:
        {
          CPasswordDlg dlg((LPCSTR)ParentURL
#ifdef QT_20
  .latin1()
#endif
          , NULL);

          switch (dlg.exec())
          {
            case 1:
            {
              CCredentials cred(dlg.m_UserName,dlg.m_Password,dlg.m_Workgroup);

              nCredentialsIndex = gCredentials.Find(cred);

              if (nCredentialsIndex == -1)
                nCredentialsIndex = gCredentials.Add(cred);
              else
                if (gCredentials[nCredentialsIndex].m_Password != cred.m_Password)
                  gCredentials[nCredentialsIndex].m_Password = cred.m_Password;
            }

            goto FileDoAgain;

            default: // Quit or Escape
              exit(-1); // cancelled by user
          }
        }
        break;

        case keSuccess:
        {
          if (list.count() > 0)
          {
            CSMBFileInfo *pInfo = &list[0];

            pPixmap = pInfo->IsFolder() ? LoadPixmap(keClosedFolderIconBig) :
                        GetFilePixmap(pInfo->m_FileName, FALSE, -1, TRUE);

            CNetFilePropGeneral *Page = new CNetFilePropGeneral(
              (LPCSTR)pInfo->m_FileName
#ifdef QT_20
  .latin1()
#endif

              ,
              URL,
              (LPCSTR)pInfo->m_FileSize
#ifdef QT_20
  .latin1()
#endif

              ,
              FormatDateTime(pInfo->m_FileDate),
              (LPCSTR)pInfo->m_FileAttributes
#ifdef QT_20
  .latin1()
#endif
              ,
              pPixmap,
              this,
              (LPCSTR)"page1");

            addTab(Page, LoadString(knGENERAL));
          }
        }
        break;

        default:
          exit(-1); // Oops
      }
    }
    else
    {
      if (!Share.isEmpty())
      {
        CShareArray list;
        int nCredentialsIndex = 0;

ShareDoAgain:;

	      switch (GetShareList((LPCSTR)Server
#ifdef QT_20
  .latin1()
#endif

        , &list, nCredentialsIndex))
	      {
		      case keErrorAccessDenied:
		      case keNetworkError:
		      {
			      QString a;
			      a.sprintf("\\\\%s", (LPCSTR)Server
#ifdef QT_20
  .latin1()
#endif

            );

			      CPasswordDlg dlg((LPCSTR)a
#ifdef QT_20
  .latin1()
#endif

            , NULL);

			      switch (dlg.exec())
			      {
				      case 1:
				      {
					      CCredentials cred(dlg.m_UserName,dlg.m_Password,dlg.m_Workgroup);

					      nCredentialsIndex = gCredentials.Find(cred);

					      if (nCredentialsIndex == -1)
					        nCredentialsIndex = gCredentials.Add(cred);
					      else
						      if (gCredentials[nCredentialsIndex].m_Password != cred.m_Password)
							      gCredentials[nCredentialsIndex].m_Password = cred.m_Password;
				      }
				      goto ShareDoAgain;

              default: // Quit or Escape
                exit(-1); // Cancelled by user
			      }
		      }
		      break;

		      case keSuccess:
		      {
			      int i;
            CSMBShareInfo *pInfo = NULL;

			      for (i=0; i < list.count(); i++)
            {
              if (!stricmp((LPCSTR)list[i].m_ShareName
#ifdef QT_20
  .latin1()
#endif

              , (LPCSTR)Share
#ifdef QT_20
  .latin1()
#endif

              ))
              {
                pInfo = &list[i];
                break;
              }
            }

            if (NULL != pInfo)
            {
              CSharePropGeneral *Page = new CSharePropGeneral(
                (LPCSTR)Share
#ifdef QT_20
  .latin1()
#endif
                , // Share name
                URL, // Full name
                (LPCSTR)pInfo->m_ShareType
#ifdef QT_20
  .latin1()
#endif

                , // Share type
                (LPCSTR)pInfo->m_Comment
#ifdef QT_20
  .latin1()
#endif

                , // Share comment
                this,
                (LPCSTR)"page1");

              pPixmap = Page->Pixmap();

              addTab(Page, LoadString(knGENERAL));
            }
      		}
		      break;

		      default:
			      exit(-1);
	      }
      }
      else
      {
        QString Workgroup;
        QString Comment;

        if (keSuccess == GetWorkgroupAndCommentByServer((LPCSTR)Server
#ifdef QT_20
  .latin1()
#endif

                                                                      , Workgroup

                                                                                          , Comment

                                                                                                                ))
        {
          CServPropGeneral *Page = new CServPropGeneral(
            (LPCSTR)Server
#ifdef QT_20
  .latin1()
#endif
            , // Server name
            (LPCSTR)Comment
#ifdef QT_20
  .latin1()
#endif
            , // Server comment
            (LPCSTR)Workgroup
#ifdef QT_20
  .latin1()
#endif
            ,
            this,
            (LPCSTR)"page1");

          addTab(Page, LoadString(knGENERAL));

          pPixmap = LoadPixmap(keServerIconBig);
        }
      }
    }
  }
  else
  {
    if (strlen(URL) > 10 && !strnicmp(URL, "printer://", 10))
    {
      HandleLocalPrinter(URL+10, bCanDoSambaSharing);

      if (bCanDoSambaSharing)
        SharingPath = URL;

      pPixmap = LoadPixmap(kePrinterIconBig);
    }
    else  // local file then
    {
      struct stat st;

      if (!lstat(URL, &st))
      {
        int i;

        for (i=0; i < gFileSystemList.count(); i++)
        {
          QString s = gFileSystemList[i].m_MountedOn;

          //if (s.right(1) != "/")
          //  s += "/";

          if (!strcmp((LPCSTR)s
#ifdef QT_20
  .latin1()
#endif
          , URL))
          {
  					CFSPropGeneral *Page = new CFSPropGeneral(&gFileSystemList[i], this, "page1");
  					addTab(Page, LoadString(knGENERAL));
            pPixmap = gFileSystemList[i].Pixmap(TRUE);

            break;
          }
        }

        if (i == gFileSystemList.count())
        {
          CFilePropGeneral *Page = new CFilePropGeneral(URL, st, this, "page1");
          addTab(Page, LoadString(knGENERAL));
          pPixmap = Page->Pixmap();
        }

        m_pFilePermissionsPage = new CFilePermissions(URL, URL, pPixmap, this, "PermissionsPage");
        addTab(m_pFilePermissionsPage, LoadString(kn_PERMISSIONS));

        bCanDoSambaSharing = S_ISDIR(st.st_mode);
        bCanDoNFSSharing = bCanDoSambaSharing;
        SharingPath = URL;
      }
    }
  }

	if (Caption.isEmpty())
		Caption.sprintf(LoadString(knXY_PROPERTIES), URL, "");

	setCaption(Caption);

  if (gbNetworkAvailable && bCanDoSambaSharing)
	{
		m_pWindowsSharingPage = new CSharingPage((LPCSTR)SharingPath
#ifdef QT_20
  .latin1()
#endif
    , pPixmap, this, (LPCSTR)"page2");

    QTab *z = new QTab();
		z->label = LoadString(knSHARING_2);
		z->id = 1;
		addTab(m_pWindowsSharingPage, z);
	}
	else
		m_pWindowsSharingPage = NULL;

	if (bCanDoNFSSharing && HasNFSSharing())
	{
		m_pNFSSharingPage = new CSharingPageNFS((LPCSTR)SharingPath
#ifdef QT_20
  .latin1()
#endif
    , pPixmap, this, (LPCSTR)"page4");
		QTab *z = new QTab();
		z->label = LoadString(knNFS_SHARING);
		z->id = 1;
		addTab(m_pNFSSharingPage, z);
	}

	setCancelButton();

	QObject *pOK = GetOKButton(this);

	disconnect(pOK, SIGNAL(clicked()), this, SLOT(accept()));
	connect(pOK, SIGNAL(clicked()), this, SLOT(OnOK()));

  QTimer::singleShot(50, this, SLOT(FixSize()));
}

////////////////////////////////////////////////////////////////////////////

CPropDialog::~CPropDialog()
{
	if (NULL != m_pPrinterObject)
    m_pPrinterObject = NULL;
}

////////////////////////////////////////////////////////////////////////////

void CPropDialog::OnOK()
{
	QWidget *pOK = (QWidget *)GetOKButton(this);
  pOK->setEnabled(FALSE);

  if (m_pWindowsSharingPage != NULL && !m_pWindowsSharingPage->Apply())
  {
    pOK->setEnabled(TRUE); // Sharing page does its own validation and returns
    return;
  }

  if (NULL != m_pPrinterGeneral)
  {
    pOK->setEnabled(TRUE);

    if (IsSuperUser())
    {
      // Validate printer property pages. Currently, only "General" page
      // requires validation. Some additional validate() calls may be
      // required in the future.

      if (!m_pPrinterGeneral->validate())
      {
        showPage(m_pPrinterGeneral);
        return; // validation failed
      }

      if(!m_pPrinterGeneral->setPrinterGeneralProperty(m_pPrinterObject) ||
         !m_pPrinterOutput->setPrinterOutputProperty(m_pPrinterObject) ||
         !m_pPrinterAdvanced->setPrinterAdvancedProperty(m_pPrinterObject))
      {
        // We have same error message for all failed pages. Is this enough?

        QMessageBox::warning(this,
                             LoadString(knERROR),
                             LoadString(knUNABLE_TO_SAVE_PRINTER_PROPERTIES),
                             LoadString(knOK));

        return; // Save failed
      }
    }

    accept();
    return;
  }

  if ((m_pFilePermissionsPage != NULL && !m_pFilePermissionsPage->Apply()) ||
      (m_pNFSSharingPage != NULL && !m_pNFSSharingPage->Apply()))
  {
    pOK->setEnabled(TRUE);
  }
	else
	  accept();
}

////////////////////////////////////////////////////////////////////////////

void CPropDialog::OnInitDialog()
{
	if (m_bStartFromSharing && m_pWindowsSharingPage != NULL)
	{
		tabBar()->show();
		showPage(m_pWindowsSharingPage);
	}
}

////////////////////////////////////////////////////////////////////////////

void CPropDialog::FixSize()
{
  setMinimumSize(510,445);
  //QWidget::setMaximumSize(500,400); -- this kills X-Server :-(
}

////////////////////////////////////////////////////////////////////////////


