/* Name: msroot.cpp

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
#include "common.h"
#include "array.h"
#include "msroot.h"
#include "smbworkgroup.h"
#include "smbserver.h"
#include "smbshare.h"
#include "smbutil.h"
#include "PasswordDlg.h"

////////////////////////////////////////////////////////////////////////////

void CMSWindowsNetworkItem::Fill()
{
  if (ExpansionStatus() != keNotExpanded)
		return;
	
	SetExpansionStatus(keExpanding);

  GetMasterBrowser();
  
	if (gWorkgroupList.count() == 0)
	{
DoAgain:;		
		switch (GetWorkgroupList())
		{
			case keErrorAccessDenied:
			case keNetworkError:
			default:
			{
				if (gCredentials[0].m_Workgroup != "%notset%")
				{
					CPasswordDlg dlg(text(0), NULL);
			
					switch (dlg.exec())
					{
						case 1: 
						{
							gCredentials[0].m_UserName = dlg.m_UserName;
							gCredentials[0].m_Workgroup = dlg.m_Workgroup;
							gCredentials[0].m_Password = dlg.m_Password;
						}
						goto DoAgain;
				
						default: // Quit or Escape
						break;
					}
				}
				
				SetExpansionStatus(keNotExpanded);
				gTreeExpansionNotifier.Cancel(this);
			}
			return;

			case keSuccess:
			break;
		}
	}
	
	int i;

	for (i=0; i < gWorkgroupList.count(); i++)
		new CWorkgroupItem(this, &gWorkgroupList[i]);

	SetExpansionStatus(keExpansionComplete);
	
	gTreeExpansionNotifier.Fire(this);
}

////////////////////////////////////////////////////////////////////////////

CListViewItem* CMSWindowsNetworkItem::FindAndExpand(LPCSTR UNCPath)
{
	QString Workgroup;
	QString Server;
	QString Share;
	QString Path;
	CListViewItem *child;
	CListViewItem *pTargetItem;

	if (!isOpen())
		setOpen(TRUE);

	// Special case: URL in form "workgroup://workgroupname"

  if (strlen(UNCPath) > strlen(GetHiddenPrefix(knWORKGROUP_HIDDEN_PREFIX))
      && !strncmp(UNCPath, GetHiddenPrefix(knWORKGROUP_HIDDEN_PREFIX), strlen(GetHiddenPrefix(knWORKGROUP_HIDDEN_PREFIX))))
  {
    for (child = firstChild(); child != NULL; child = child->nextSibling())
    {
      if (!stricmp(UNCPath + strlen(GetHiddenPrefix(knWORKGROUP_HIDDEN_PREFIX)), child->text(0)))
      {
        if (!child->isOpen())
          child->setOpen(TRUE);

        return child;
      }
    }

    return NULL; // couldn't find workgroup :(
  }
  
  // OK, is should be normal UNC path then....

  if (!ParseUNCPath(UNCPath, Server, Share, Path))
	{
		return NULL;
	}
	
	//printf("%s: Server=[%s], Share=[%s], Path=[%s]\n",
//		UNCPath, (LPCSTR)Server, (LPCSTR)Share, (LPCSTR)Path);

	// Step 1: try to find that server in our tree first.
	// That's gonna be significantly faster than go searching the network
Again:;

	for (child = firstChild(); child != NULL; child = child->nextSibling())
	{
		CListViewItem *subchild;

		for (subchild = child->firstChild(); subchild != NULL; subchild = subchild->nextSibling())
		{
			if (!stricmp(subchild->text(0), Server))
			{
				child->setOpen(TRUE);
				pTargetItem = subchild;
        child = subchild;
				goto ServerFound;
			}
		}
	}

	// Go search the network then...

	if (keSuccess != GetWorkgroupByServer(Server, Workgroup))
	{
		// Oops... OK, let's expand network neighbourhood (current workgroup) node and try to find it there

		for (child = firstChild(); child != NULL; child = child->nextSibling())
		{
			if (!stricmp(child->text(0), gCredentials[0].m_Workgroup))
				break;
		}
		
		if (NULL == child)
			return NULL; // oops, even the current workgroup isn't there. Give up...

		if (((CNetworkTreeItem*)child)->ExpansionStatus() != keExpansionComplete)
		{
		  ((CNetworkTreeItem*)child)->Fill();
			goto Again;
		}
		
		return NULL; // nope, it doesn't seem to work out... I Give up...
	}

	//printf("Workgroup for %s is %s\n", (LPCSTR)Server, (LPCSTR)Workgroup);

	for (child = firstChild(); child != NULL; child = child->nextSibling())
	{
		if (!stricmp(child->text(0), Workgroup))
			break;
	}

	if (child == NULL) // Workgroup not found, let's add it to the list
	{
		CSMBWorkgroupInfo wgi;
		wgi.m_WorkgroupName = Workgroup;
		wgi.m_MasterName = "";
		int nIndex = gWorkgroupList.Add(wgi);
		child = new CWorkgroupItem(this, &gWorkgroupList[nIndex]);
	}

	// Get the list of all servers in that workgroup if necessary...
	{
	    CWorkgroupItem *pWorkgroupItem = (CWorkgroupItem*)child;

	pWorkgroupItem->setOpen(TRUE);
	
	// Now search for the server in that workgroup

	for (child = pWorkgroupItem->firstChild(); child != NULL; child = child->nextSibling())
	{
		if (!stricmp(child->text(0), Server))
			break;
	}
	
	if (child == NULL) // Server not found, something 
					   // must be wrong, but who cares...
	{
		//printf("Server %s thinks it is in the workgroup %s, but domain controller\ndoesn't know about it...\n", (LPCSTR)Server, (LPCSTR)Workgroup);
		
		CSMBServerInfo svi;
		svi.m_ServerName = Server.upper();
		svi.m_Comment = "";
		
		child = new CServerItem(pWorkgroupItem, pWorkgroupItem, &svi);
	}
	}
ServerFound:;
	CServerItem *pServerItem = (CServerItem*)child;
	
	pTargetItem = (CListViewItem*)pServerItem;


	pTargetItem->setOpen(TRUE);
			
	if (!Share.isEmpty())
	{
		for (child = pTargetItem->firstChild(); child != NULL; child = child->nextSibling())
		{
			if (!stricmp(child->text(0), Share))
				break;
		}

		if (child != NULL)
		{
			pTargetItem = child;
			pTargetItem->setOpen(TRUE);

			while (!Path.isEmpty())	// This will go deep to last folder
			{
				LPCSTR p = (LPCSTR)Path;

        QString Folder = ExtractWord(p,"\\/");
				
        if (Folder.isEmpty())
          break;
        
        Path = p;
				
				printf("Looking up %s\n", (LPCSTR)Folder);

				for (child = pTargetItem->firstChild(); child != NULL; child = child->nextSibling())
				{
					if (!stricmp(child->text(0), Folder))
					{
						printf("...found!\n");
						break;
					}
				}

				if (child != NULL)
				{
					pTargetItem = child;
					
					if (((CNetworkTreeItem*)pTargetItem)->Kind() == keFileItem &&
							!((CFileItem*)pTargetItem)->IsFolder())
					{
					}
					else
						pTargetItem->setOpen(TRUE);
				}
				else
					printf("Not found!\n");
			}
		}
	}

	return pTargetItem;
}

////////////////////////////////////////////////////////////////////////////

void CMSWindowsNetworkItem::Fill(CMSWindowsNetworkItem *pOther)
{
	if (pOther->ExpansionStatus() == keNotExpanded)
		Fill();
	else
	{
		SetExpansionStatus(keExpanding);
		
		CWorkgroupItem *pItemW;

		for (pItemW = (CWorkgroupItem*)(pOther->firstChild()); pItemW != NULL; pItemW = (CWorkgroupItem *)(pItemW->nextSibling()))
		{
			CWorkgroupItem *pWorkgroup = new CWorkgroupItem(this, pItemW);

			if (pItemW->ExpansionStatus() == keExpansionComplete)
			{
				CServerItem *pItemS;
				
				for (pItemS = (CServerItem*)(pItemW->firstChild()); pItemS != NULL; pItemS = (CServerItem*)(pItemS->nextSibling()))
				{
					CServerItem *pServer = new CServerItem(pWorkgroup, pWorkgroup, pItemS);
					
					pServer->m_nCredentialsIndex = pItemS->CredentialsIndex();

					if (pItemS->ExpansionStatus() == keExpansionComplete)
					{
						CShareItem *pItemSH;

						for (pItemSH = (CShareItem*)(pItemS->firstChild()); pItemSH != NULL; pItemSH = (CShareItem*)(pItemSH->nextSibling()))
						{
							CShareItem *pShare = new CShareItem(pServer, pServer, pItemSH);
							pShare->m_nCredentialsIndex = pItemSH->CredentialsIndex();
						}
						
						pServer->SetExpansionStatus(keExpansionComplete);
					}
				}

				pWorkgroup->SetExpansionStatus(keExpansionComplete);
			}
		}

		SetExpansionStatus(keExpansionComplete);
		gTreeExpansionNotifier.Fire(this);
	}
}

////////////////////////////////////////////////////////////////////////////
