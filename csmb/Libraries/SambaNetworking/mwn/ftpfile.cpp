/* Name: ftpfile.cpp

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
#include <sys/types.h>
#include <sys/stat.h>
#include "ftpfile.h"
#include "smbutil.h"
#include "PasswordDlg.h"
#include "ftpsession.h"
#include <qmessagebox.h>
#include "qapplication.h"
#include <time.h> // for mktime and strftime
#include <errno.h>

void CFTPFileItem::Init()
{
	m_nCredentialsIndex = -1; /* inherit from parent */
	
	InitPixmap();
		
	if (m_FileAttributes[0] == 'b' || m_FileAttributes[0] == 'c')
		setText(1, m_FileSize + "  "); // major, minor are here
	else
		if (m_FileAttributes[0] == '-')
			setText(1, SizeInKilobytes((unsigned long)atol(m_FileSize)) + "  ");

	struct tm Tm = *localtime(&m_FileDate);
  
	char AMPM[100];
	strftime(AMPM, sizeof(AMPM), "%I:%M %P" , &Tm);
	QString AMPMString(AMPM[0] == '0' ? &AMPM[1] : AMPM);

	char TimeBuf[100];

	sprintf(
		TimeBuf, 
		"%d/%d/%d %s", 
		Tm.tm_mon+1,
		Tm.tm_mday,
		Tm.tm_year > 99 ? Tm.tm_year + 1900 : Tm.tm_year,
		(LPCSTR)AMPMString.upper());
	
	/*char buf[100];strftime(buf, sizeof(buf), "%c" , &Tm);*/
	
	setText(3, TimeBuf);
}

////////////////////////////////////////////////////////////////////////////

CSMBErrorCode CFTPFileContainer::GetFTPFileList(LPCSTR Url, CFileArray *pFileList, int nCredentialsIndex, BOOL bWantRegularFiles)
{
	CFtpSession *pSession = gFtpSessions.GetSession(this, Url);
	
	if (pSession == NULL)
    return keUnknownHost;
	
  pSession->GetFTPFileList(Url, pFileList, nCredentialsIndex, bWantRegularFiles);

	return keSuccess;
}

////////////////////////////////////////////////////////////////////////////

void CFTPFileContainer::Fill()
{
	/* OK, let's expand */

	SetExpansionStatus(keExpanding);

	CFileArray list;

DoAgain:;
		
	switch (GetFTPFileList((LPCSTR)FullName(FALSE), &list, CredentialsIndex(), FALSE))
	{
    case keUnknownHost:
      SetExpansionStatus(keDeleteRequested);
    break;

    case keErrorAccessDenied:
		{
      CPasswordDlg dlg((LPCSTR)FullName(FALSE), NULL, NULL, FALSE);
			
			switch (dlg.exec())
			{
				case 1: 
				{
					CCredentials cred(dlg.m_UserName,dlg.m_Password,dlg.m_Workgroup);
					
					m_nCredentialsIndex = gCredentials.Find(cred);
					
					if (m_nCredentialsIndex == -1)
					  m_nCredentialsIndex = gCredentials.Add(cred);
					else
						if (gCredentials[m_nCredentialsIndex].m_Password != cred.m_Password)
							gCredentials[m_nCredentialsIndex].m_Password = cred.m_Password;
				}
				goto DoAgain;
			
				default: // Quit or Escape
					SetExpansionStatus(keNotExpanded);
					gTreeExpansionNotifier.Cancel(this);
					return;
			}
		}
		break;

		case keSuccess:
		{
      int i;

			for (i=0; i < list.count(); i++)
				new CFTPFileItem(this, this, &list[i]);

			SetExpansionStatus(keExpansionComplete);
		}
		break;

    default:
			SetExpansionStatus(keNotExpanded);
	}
	
	gTreeExpansionNotifier.Fire(this);
}

////////////////////////////////////////////////////////////////////////////

void CFTPFileContainer::Fill(CFileArray& list)
{
	SetExpansionStatus(keExpanding);

	int i;

	for (i=0; i < list.count(); i++)
	{
		if (list[i].IsFolder())
			new CFTPFileItem(this, this, &list[i]);
	}

	setExpandable(childCount() > 0);
	SetExpansionStatus(keExpansionComplete);
	
	listView()->repaintItem(this);
}

////////////////////////////////////////////////////////////////////////////

QString CFTPFileContainer::FullName(BOOL bDoubleSlashes)
{
	QString s;

	CNetworkTreeItem *pParent = (CNetworkTreeItem*)m_pLogicalParent;

	if (pParent != NULL)
	{
		s = pParent->FullName(bDoubleSlashes);
		s.append("/");
		s.append(text(0));
	}
	else
		s = "";

	return s;
}

////////////////////////////////////////////////////////////////////////////

void CFTPFileItem::setup()
{
	setExpandable(IsFolder());
	CListViewItem::setup();
}

////////////////////////////////////////////////////////////////////////////

QTTEXTTYPE CFTPFileItem::text(int column) const
{
	switch (column)
	{
		case -1:
			return "Network";

		case 0: // Name
		default:
			return m_FileName;
		
		case 1: // Size
			return CListViewItem::text(1);
		
		case 2: // Type
			return m_FileAttributes;
		
		case 3: // Modified
			return CListViewItem::text(3);
		
		case 4:
			return (LPCSTR)m_Owner;
		
		case 5:
			return (LPCSTR)m_Group;
	}
}

////////////////////////////////////////////////////////////////////////////

QTKEYTYPE CFTPFileItem::key(int nColumn, bool ascending) const
{
	static QString s;

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

	return (QTKEYTYPE)s;
}

////////////////////////////////////////////////////////////////////////////

CSMBErrorCode CFTPFileItem::Rename(LPCSTR sNewLabel)
{
	QString Path = FullName(FALSE);

	CFtpSession *pSession = gFtpSessions.GetSession(this, Path);

TryAgain:;

	if (NULL != pSession)
	{
		CSMBErrorCode retcode = pSession->RenameFile(Path, sNewLabel);
    
		switch (retcode)
		{
			case keSuccess:
			{
				if (parent() != m_pLogicalParent)
				{
					CListViewItem *child;

					for (child = m_pLogicalParent->firstChild(); 
               child != NULL; 
               child = child->nextSibling())
					{
						if (m_FileName == child->text(0))
						{
							((CFTPFileItem *)child)->m_FileName = sNewLabel;
							child->listView()->repaintItem(child);
							break;
						}
					}
				}
				
				m_FileName = sNewLabel;
			}
			return keSuccess;

			case keErrorAccessDenied:
			default: // ### todo: what other situations are?
			{
        if (!ReportCommonFileError(Path, EACCES,  FALSE, knSTR_RENAME, knUNABLE_TO_RENAME))
					goto TryAgain;

				return keStoppedByUser;
			}

			case keFileNotFound:
			{
				if (!ReportCommonFileError(Path, ENOENT,  FALSE, knSTR_RENAME, knUNABLE_TO_RENAME))
					goto TryAgain;

				((CFTPFileContainer*)m_pLogicalParent)->m_bRefreshForced = TRUE;

				return keStoppedByUser;
			}
		}
	}
	
  return keWrongParameters; // should never get there...
}

////////////////////////////////////////////////////////////////////////////

QPixmap *CFTPFileItem::Pixmap(BOOL bIsBig)
{ 
	if (IsFolder())
	{
		BOOL bIsLink = IsLink();
		
		if (!bIsBig)
			SetPixmapID(keClosedFolderIcon, FALSE, bIsLink);
	
	  return LoadPixmap(bIsBig ? keClosedFolderIconBig : keClosedFolderIcon, bIsLink, FALSE);
	}
	
	switch (m_FileAttributes[0]
#ifdef QT_20
    .latin1()
#endif
          )
	{
		case 's':
			return LoadPixmap(keMSRootIcon);

		case 'c':
		case 'b':
			return LoadPixmap(bIsBig ? keDeviceIconBig : keDeviceIcon);

		case 'p':
			return LoadPixmap(keFIFOIcon);

		case 'l':
			if (S_ISDIR(m_TargetMode))
			{
				SetPixmapID(keClosedFolderIcon, FALSE, TRUE);

				return LoadPixmap(bIsBig ? keClosedFolderIconBig : keClosedFolderIcon, TRUE, FALSE);
			}
			else
				if (S_IFCHR == (m_TargetMode & S_IFMT) || S_IFBLK == (m_TargetMode & S_IFMT))
					return LoadPixmap(bIsBig ? keDeviceIconBig : keDeviceIcon, 1);
				else
					if (S_ISFIFO(m_TargetMode))
						return LoadPixmap(keFIFOIcon, 1);
					else
						return GetFilePixmap(m_TargetName, TRUE, (m_TargetMode & S_IXUSR) == S_IXUSR ||	(m_TargetMode & S_IXGRP) == S_IXGRP || (m_TargetMode & S_IXOTH) == S_IXOTH, bIsBig);
	}
	
	return GetFilePixmap(m_FileName, FALSE, m_FileAttributes.contains('x'), bIsBig);
}

////////////////////////////////////////////////////////////////////////////

QString CFTPFileItem::GetTip()
{
	return (m_FileAttributes[0] == 'l') ? QString("-> ") + m_TargetName : FullName(FALSE);
}

////////////////////////////////////////////////////////////////////////////

CSMBErrorCode GetFTPFileList(LPCSTR Url, CFileArray *pFileList, int nCredentialsIndex, BOOL bWantRegularFiles)
{
	CFtpSession *pSession = gFtpSessions.GetSession(NULL, Url, nCredentialsIndex);
	
	if (pSession != NULL)
		pSession->GetFTPFileList(Url, pFileList, nCredentialsIndex, bWantRegularFiles);

	return keSuccess;
}

BOOL CFTPFileContainer::CanCreateSubfolder()
{
  return TRUE;
}

BOOL CFTPFileContainer::CreateSubfolder(QString& NewFolderName)
{
  QString sFolderName = NewFolderName;
  QString Path = FullName(FALSE);

  if (Path.right(1) != "/")
    Path += "/";
  
  int nSuffixIndex = 2;

TryAgain:;

  if (FTPMakeDir(Path+NewFolderName))
  {
    if (EPERM == errno) // Cancelled by user
    {
      return FALSE;
    }
    
    if (EEXIST == errno)
    {
      NewFolderName.sprintf("%s (%d)", (LPCSTR)sFolderName, nSuffixIndex++);
      goto TryAgain;
    }
    
    if (ECONNREFUSED == errno)
    {
      CNetworkTreeItem *pI = (CNetworkTreeItem *)m_pLogicalParent;
      
      while (NULL != pI && pI->Kind() != keFTPSiteItem)
        pI = (CNetworkTreeItem*)pI->m_pLogicalParent;

      ReportCommonFileError(pI->FullName(FALSE), errno, FALSE, knCREATE_NEW_FOLDER);
      return FALSE;
    }

    QString msg = LoadString(knUNABLE_TO_CREATE_FOLDER);
    
    if (EACCES == errno)
    {
      QString a;
      a.sprintf(LoadString(knEACCES), (LPCSTR)NewFolderName);
      msg += "\n";
      msg += a;
    }
    
    QMessageBox::critical(qApp->mainWidget(), 
                          LoadString(knCREATE_NEW_FOLDER),
                          (LPCSTR)msg,
                          LoadString(knOK));
    return FALSE;
  }
  else
    return TRUE;
}

