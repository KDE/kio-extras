/* Name: DisconnectDlg.cpp

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


/**********************************************************************

	--- Qt Architect generated file ---

	File: DisconnectDlg.cpp
	Last generated: Sun Jan 17 18:14:46 1999

 *********************************************************************/

#include "common.h"
#include "filesystem.h"
#include "smbutil.h"
#include "DisconnectDlg.h"
#include "corellistboxitem.h"
#include "explres.h"

#define Inherited CDisconnectDlgData

CDisconnectDlg::CDisconnectDlg
(
	QWidget* parent,
	const char* name
)
	:
	Inherited( parent, name )
{
	setCaption(LoadString(knDISCONNECT_NETWORK_SHARE));
	m_pOKButton->setText(LoadString(knOK));
	m_pCancelButton->setText(LoadString(knCANCEL));
	m_pOKButton->setDefault(TRUE);
	m_pOKButton->setAutoDefault(TRUE);

	m_pInfo = NULL;

	int i;

	for (i=0; i < gFileSystemList.count(); i++)
	{
		if (gFileSystemList[i].m_Type == "smbfs" ||
				gFileSystemList[i].m_Type == "nfs")
		{
			QString s;

			s.sprintf(LoadString(knX_ON_Y), (LPCSTR)gFileSystemList[i].m_Name
#ifdef QT_20
  .latin1()
#endif
      , (LPCSTR)gFileSystemList[i].m_MountedOn
#ifdef QT_20
  .latin1()
#endif
      );

			m_FilesystemList.Add(gFileSystemList[i]);
      m_List->insertItem(new CListBoxItem((LPCSTR)s
#ifdef QT_20
  .latin1()
#endif
      , *LoadPixmap(keNetworkFileSystemIcon), (void*)(&m_FilesystemList[m_FilesystemList.count()-1])));
		}
	}
}

CDisconnectDlg::~CDisconnectDlg()
{
}

void CDisconnectDlg::done(int r)
{
	if (r == 1)
	{
		int nListIndex = m_List->currentItem();

		if (nListIndex != -1)
		{
			CListBoxItem *pItem = (CListBoxItem*)((QListBox*)m_List)->item(nListIndex);

			if (pItem != NULL)
				m_pInfo = (CFileSystemInfo*)pItem->GetUserData();
		}
	}

	QDialog::done(r);
}

