/* Name: smbworkgroup.cpp

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
#include "smbworkgroup.h"
#include "smbserver.h"
#include "smbutil.h"
#include "PasswordDlg.h"
#include "qmessagebox.h"
#include "qapplication.h"

int CWorkgroupItem::CredentialsIndex()
{
	return m_nCredentialsIndex;
}

void CWorkgroupItem::Fill()
{
	SetExpansionStatus(keExpanding);

	CServerArray list;

DoAgain:;		
	
	switch (GetServerList(m_WorkgroupName, m_MasterName, &list, CredentialsIndex()))
	{
		case keNetworkError:
		{
			QString a;
			
			a.sprintf("%s is not accessible.\n\nThe list of servers for this workgroup is not currently available", (LPCSTR)m_WorkgroupName);

			QMessageBox::critical(qApp->mainWidget(), qApp->mainWidget()->caption(), (LPCSTR)a);

			SetExpansionStatus(keExpansionComplete);
			//SetExpansionStatus(keNotExpanded);
		}
		break;
			
		case keErrorAccessDenied:
		{
			QString a;
			a.sprintf("\\\\%s", (LPCSTR)m_MasterName);

			CPasswordDlg dlg((LPCSTR)a, NULL);
			
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
            {
              gCredentials[m_nCredentialsIndex].m_Password = cred.m_Password;

              if (!m_nCredentialsIndex)
              {
                QString s;
                s.sprintf("setenv DEFAULTPASSWORD %s", (LPCSTR)cred.m_Password);
                ServerExecute(s);
              }
            }
				}
				goto DoAgain;
			
				default: // Quit or Escape
					SetExpansionStatus(keNotExpanded);
			}
		}
		break;

		case keSuccess:
		{
			int i;

			for (i=0; i < list.count(); i++)
				new CServerItem(this, this, &list[i]);

			SetExpansionStatus(keExpansionComplete);
		}
		break;

		default:
			SetExpansionStatus(keNotExpanded);
	}

	gTreeExpansionNotifier.Fire(this);
}

void CWorkgroupArray::Print()
{
	int i;

	printf("Workgroup list:\n");

	for (i=0; i < count(); i++)
	{
		printf("%s %s", i == 0 ? "" : ",", (LPCSTR)(*this)[i].m_WorkgroupName);
		
		if ((i % 9) == 0)
			printf("\n");
	}

	printf("\n");
}
