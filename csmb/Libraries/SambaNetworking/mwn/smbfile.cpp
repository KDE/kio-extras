/* Name: smbfile.cpp

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

#include <stdio.h>
#include "smbfile.h"
#include "smbutil.h"
#include "PasswordDlg.h"
#include <time.h> // for mktime and strftime
#include "qmessagebox.h"
#include "qapplication.h"

BOOL gbIncludeRegularFilesInTree = FALSE;

void CFileItem::Init()
{
	m_nCredentialsIndex = -1; /* inherit from parent */
	InitPixmap();

	if (NULL == (LPCSTR)m_FileSize)
		m_FileSize = "0";
		
	if (!IsFolder())
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

QTTEXTTYPE CFileItem::text(int column) const
{
	switch (column)
	{
		case -1:
			return "Network";

		case 0: // Name
		default:
			return (QTTEXTTYPE)m_FileName;
		case 1: // Size
			return CListViewItem::text(1);
		case 2: // Type
			return (QTTEXTTYPE)m_FileAttributes;
		case 3: // Modified
			return CListViewItem::text(3);
	}
}

void CNetworkFileContainer::Fill()
{
	/* OK, let's expand */

	SetExpansionStatus(keExpanding);

	CFileArray list;

DoAgain:;
	
  //printf("Cred index = %d\n", CredentialsIndex());	
	
  switch (GetFileList((LPCSTR)FullName(TRUE), &list, CredentialsIndex(), gbIncludeRegularFilesInTree))
	{
		case keErrorAccessDenied:
		{
			CPasswordDlg dlg((LPCSTR)FullName(FALSE), NULL);
			
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
				new CFileItem(this, this, &list[i]);

			SetExpansionStatus(keExpansionComplete);
		}
		break;

		default:
			SetExpansionStatus(keNotExpanded);
	}
	
	gTreeExpansionNotifier.Fire(this);
}

void CNetworkFileContainer::Fill(CFileArray& list)
{
	SetExpansionStatus(keExpanding);

	int i;

	for (i=0; i < list.count(); i++)
	{
		if (list[i].IsFolder())
			new CFileItem(this, this, &list[i]);
	}

	setExpandable(childCount() > 0);
	SetExpansionStatus(keExpansionComplete);
}

QString CNetworkFileContainer::FullName(BOOL bDoubleSlashes)
{
	QString s;

	CNetworkTreeItem *pParent = (CNetworkTreeItem*)m_pLogicalParent;

	if (pParent != NULL)
	{
		s = pParent->FullName(bDoubleSlashes);
		s.append(bDoubleSlashes ? "\\\\" : "\\");
		s.append(text(0));
	}
	else
		s = "";

	if (s.contains("ftp://"))
		s = MakeSlashesForward(s);

	return s;
}

void CFileItem::setup()
{
	setExpandable(IsFolder());
	CListViewItem::setup(); 
}

void CFileArray::Print()
{
	int i;

	printf("File list:\n");

	for (i=0; i < count(); i++)
	{
		printf("%s %s", i == 0 ? "" : ",", (LPCSTR)(*this)[i].m_FileName);
		
		if ((i % 9) == 0)
			printf("\n");
	}

	printf("\n");
}

QPixmap *CFileItem::Pixmap(BOOL bIsBig)
{ 
	if (IsFolder())
	{
		if (!bIsBig)
			SetPixmapID(keClosedFolderIcon, FALSE, FALSE);

		return LoadPixmap(bIsBig ? keClosedFolderIconBig : keClosedFolderIcon);
	}
	
	return GetFilePixmap(m_FileName, FALSE, -1, bIsBig);
}

BOOL CNetworkFileContainer::CanCreateSubfolder()
{
  return TRUE;
}

BOOL CNetworkFileContainer::CreateSubfolder(QString& NewFolderName)
{
  QString sFolderName = NewFolderName;
  QString Path = FullName(FALSE);

  if (Path.right(1) != "\\")
    Path += "\\";
  
  int nSuffixIndex = 2;
  CSMBErrorCode ret;

TryAgain:;
  
  switch (SMBFolderExists(Path+NewFolderName, CredentialsIndex()))
  {
    case -3: // exists but is not a folder
    case 0:  // exists and writable
    case 1:  // exists and is not writable

      if (nSuffixIndex > 200)
        return FALSE; // give up
      
      NewFolderName.sprintf("%s (%d)", (LPCSTR)sFolderName, nSuffixIndex++);
      goto TryAgain;
    
    case -2:
      return FALSE; // Cancelled by user
  }

  ret = SMBMkdir(Path+NewFolderName, CredentialsIndex());

  if (keSuccess != ret)
  {
    QString msg = LoadString(knUNABLE_TO_CREATE_FOLDER);
    
    if (keErrorAccessDenied == ret)
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

CSMBErrorCode CFileItem::Rename(LPCSTR sNewLabel)
{
	CSMBErrorCode retcode = SMBRename(FullName(FALSE), sNewLabel);
  
  if (keSuccess == retcode)
  {
    if (parent() != m_pLogicalParent)
    {
      CListViewItem *child;

      for (child = m_pLogicalParent->firstChild(); child != NULL; child = child->nextSibling())
      {
        if (m_FileName == child->text(0))
        {
          ((CFileItem *)child)->m_FileName = sNewLabel;
          child->listView()->repaintItem(child);
          break;
        }
      }
    }

    m_FileName = sNewLabel;
  }
	else
	{
		QString a;
		a.sprintf(LoadString(knUNABLE_TO_RENAME_FILE_X), (LPCSTR)FullName(FALSE));
	
		QMessageBox::critical(qApp->mainWidget(), LoadString(knERROR_RENAMING_FILE), (LPCSTR)a);
    return keStoppedByUser;
	}
  
  return retcode;
}

