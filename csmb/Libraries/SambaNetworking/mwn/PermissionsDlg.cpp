/* Name: PermissionsDlg.cpp

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

#include "PermissionsDlg.h"
#include "qlistbox.h"
#include "inifile.h"
#include "corellistboxitem.h"
#include "PermissionsAddDlg.h"
#include <qcombobox.h>

#define Inherited CPermissionsDlgData

#ifdef QT_20
typedef QListBox CWorkaroundListBox;
#endif 

////////////////////////////////////////////////////////////////////////////

CPermissionsDlg::CPermissionsDlg
(
	CSection *pSection,
	QWidget* parent,
	const char* name
)
	:
	Inherited( parent, name )
{
	m_bChanged = FALSE;
	m_pSection = pSection;
	
	m_pAccessThroughShareLabel->setText(LoadString(knACCESS_THROUGH_SHARE));
	m_pNameLabel->setText(LoadString(knNAME_COLON));
	m_pAccessTypeLabel->setText(LoadString(knACCESS_TYPE_COLON));
	m_AddButton->setText(LoadString(kn_ADD_DOTDOTDOT));
	m_RemoveButton->setText(LoadString(kn_REMOVE));
	m_OKButton->setText(LoadString(knOK));
	m_CancelButton->setText(LoadString(knCANCEL));

	m_ShareName->setText(m_pSection->m_SectionName);
	m_ShareName->setFixedWidth(500-(30+m_pAccessThroughShareLabel->width()));
	m_ShareName->move(30+m_pAccessThroughShareLabel->width(), 10);

	setCaption(LoadString(knSTR_ACCESS_THROUGH_SHARE));

	m_bIsPrinter = m_pSection->IsPrintable();
	
  if (m_bIsPrinter)
    m_AccessType->insertItem(LoadString(knSTR_PRINT));
  else
  {
    m_AccessType->insertItem(LoadString(knSTR_FULL_CONTROL));
	  m_AccessType->insertItem(LoadString(knSTR_READ));
  }
	
  m_AccessType->insertItem(LoadString(knSTR_NO_ACCESS));

  if (m_pSection->IsPublic())
	{
		QString s((LPCSTR)LoadString(knSTR_EVERYONE)); 

		s += "\t\t\t";

		if (m_pSection->IsWritable())
			s += LoadString(knSTR_FULL_CONTROL);
		else
			s += LoadString(knSTR_READ);


		m_List->insertItem(new CListBoxItem(s, *LoadPixmap(keWorldIcon), NULL));
	}

	//----------- Full control ---------------

  LPCSTR p = m_pSection->Value("write list");
	
	while (NULL != p && *p != '\0')
	{
		QString name((LPCSTR)ExtractWord(p, ", \t"));
		
		if (!name.isEmpty())
		{
			if (name[0] == '@') // see if it is a group
			{
				QString s((LPCSTR)name+1);
			   
				s += "\t\t\t";
				s += LoadString(m_bIsPrinter ? knSTR_PRINT : knSTR_FULL_CONTROL);

				m_List->insertItem(new CListBoxItem(s, *LoadPixmap(keUserGroupIcon), (void*)1));
			}
			else	// just a user then...
			{
				QString s((LPCSTR)name);
			   
				s += "\t\t\t";
				s += LoadString(m_bIsPrinter ? knSTR_PRINT : knSTR_FULL_CONTROL);
				
				m_List->insertItem(new CListBoxItem(s, *LoadPixmap(keUserFaceIcon), NULL));
			}
		}
		else
			break;
	}

	// ------------------------------- Read list
	// Read list is a list of users who are given read-only access to the share.
  // In case of printer that is equivalent to "no access"

	p = m_pSection->Value("read list");
	
	while (NULL != p && *p != '\0')
	{
		QString name((LPCSTR)ExtractWord(p, ", \t"));
	   
		if (!name.isEmpty())
		{
			if (name[0] == '@') // see if it is a group
			{
				QString s((LPCSTR)name+1);

				s += "\t\t\t";
				s += LoadString(m_bIsPrinter ? knSTR_NO_ACCESS : knSTR_READ);

        m_List->insertItem(new CListBoxItem(s, *LoadPixmap(keUserGroupIcon), (void*)1));
			}
			else	// just a user then...
			{
				QString s((LPCSTR)name);
			   	
				s += "\t\t\t";
				s += LoadString(m_bIsPrinter ? knSTR_NO_ACCESS : knSTR_READ);
				
				m_List->insertItem(new CListBoxItem(s, *LoadPixmap(keUserFaceIcon), NULL));
			}
		}
		else
			break;
	}

  // --------- No access -------------------

	p = m_pSection->Value("invalid users");
	
	while (NULL != p && *p != '\0')
	{
		QString name((LPCSTR)ExtractWord(p, ", \t"));
	   
		if (!name.isEmpty())
		{
			if (name[0] == '@') // see if it is a group
			{
				QString s((LPCSTR)name+1);

				s += "\t\t\t";
				s += LoadString(knSTR_NO_ACCESS);

        m_List->insertItem(new CListBoxItem(s, *LoadPixmap(keUserGroupIcon), (void*)1));
			}
			else	// just a user then...
			{
				QString s((LPCSTR)name);
			   	
				s += "\t\t\t";
				s += LoadString(knSTR_NO_ACCESS);
				
				m_List->insertItem(new CListBoxItem(s, *LoadPixmap(keUserFaceIcon), NULL));
			}
		}
		else
			break;
	}
	
  // ------------------------------------------------ 

  if (m_List->count() > 0)
		m_List->setCurrentItem(0);
}

////////////////////////////////////////////////////////////////////////////

CPermissionsDlg::~CPermissionsDlg()
{
}

////////////////////////////////////////////////////////////////////////////

void CPermissionsDlg::done(int r)
{
	if (1 == r && m_bChanged)
	{
		QString ReadList, WriteList, NoAccessList;
		BOOL bIsPublic = FALSE;
		BOOL bIsReadOnly = FALSE;

		CWorkaroundListBox *pList = (CWorkaroundListBox*)m_List;

		int nCount = m_List->count();
		int i;

		for (i=0; i < nCount; i++)
		{
			QString s(m_List->text(i));
			LPCSTR p = (LPCSTR)s;

			QString Name((LPCSTR)ExtractWord(p, "\t"));
			QString AccessType((LPCSTR)ExtractWord(p,"\t"));
			
			BOOL bIsGroup = (((CListBoxItem*)pList->item(i))->GetUserData() == (void*)1);
			BOOL bIsEveryone = !strcmp(Name, LoadString(knSTR_EVERYONE));

			if (bIsGroup)
				Name = "@" + Name;

			if (!strcmp(AccessType, LoadString(knSTR_READ)))
			{
				if (bIsEveryone)
				{
					bIsPublic = TRUE;
					bIsReadOnly = TRUE;
				}
				else
				{
					if (!ReadList.isEmpty())
						ReadList += ",";

					ReadList += Name;
				}
			}
			else
			{
				if (!strcmp(AccessType, LoadString(knSTR_FULL_CONTROL)) ||
            !strcmp(AccessType, LoadString(knSTR_PRINT)))
				{
					if (bIsEveryone)
						bIsPublic = TRUE;
					else
					{
						if (!WriteList.isEmpty())
							WriteList += ",";

						WriteList += Name;
					}
				}
				else
				{
					if (!bIsEveryone)
					{
						if (!NoAccessList.isEmpty())
							NoAccessList += ",";

						NoAccessList += Name;
					}
				}
			}
		}

		m_pSection->SetValue(ReadList, "read list");
		m_pSection->SetValue(WriteList, "write list");
		m_pSection->SetValue(NoAccessList, "invalid users");
		m_pSection->SetPublic(bIsPublic);
		m_pSection->SetWritable(!bIsReadOnly);
	}

	QDialog::done(r);
}

////////////////////////////////////////////////////////////////////////////

void CPermissionsDlg::OnAccessTypeComboChanged(int nIndex)
{
	int nListIndex = m_List->currentItem();

	if (nListIndex != -1)
	{
		m_bChanged = TRUE;
		
		CListBoxItem *pItem = (CListBoxItem*)((CWorkaroundListBox*)m_List)->item(nListIndex);
		LPCSTR pText = pItem->text();

		QString Name((LPCSTR)ExtractWord(pText, "\t"));

		Name += "\t\t\t";
		Name += m_AccessType->text(nIndex);

		m_List->changeItem(new CListBoxItem(Name, *pItem->pixmap(), pItem->GetUserData()), nListIndex);
	}
}

////////////////////////////////////////////////////////////////////////////

void CPermissionsDlg::OnAdd()
{
	CPermissionsAddDlg dlg(m_bIsPrinter);
	
	if (QDialog::Accepted == dlg.exec())
	{
		m_bChanged = TRUE;

		CWorkaroundListBox* pList = (CWorkaroundListBox*)m_List;
	   
		int nCount = pList->count();
		int i;

		QString s((LPCSTR)dlg.m_Name);

		QPixmap *pPixmap;

		void *UserData = NULL;

		if (!strcmp(dlg.m_Name, LoadString(knSTR_EVERYONE)))
			pPixmap = LoadPixmap(keWorldIcon);
		else
			if (dlg.m_bIsGroup)
			{
				pPixmap = LoadPixmap(keUserGroupIcon);
				UserData = (void*)1;
			}
			else
				pPixmap = LoadPixmap(keUserFaceIcon);

		CListBoxItem *pNewItem = new CListBoxItem(dlg.m_Name + "\t\t\t" + dlg.m_Access, *pPixmap, UserData);
		
		s += "\t\t\t*";

		for (i=0; i < nCount; i++)
		{
			CListBoxItem *pItem = (CListBoxItem *)pList->item(i);

			QString Name(pItem->text());
			
			BOOL bWasGroup = (pItem->GetUserData() == (void*)1);

			if (bWasGroup == dlg.m_bIsGroup &&
				Match((LPCSTR)Name, (LPCSTR)s))
			{
				m_List->changeItem(pNewItem, i);
				
				if (i == m_List->currentItem())
					OnListSelChanged(i);
				
				break;
			}
		}

		if (i == nCount)
			m_List->insertItem(pNewItem);
	}
}

////////////////////////////////////////////////////////////////////////////

void CPermissionsDlg::OnRemove()
{
	int nListIndex = m_List->currentItem();
	
	if (nListIndex != -1)
	{
		m_List->removeItem(nListIndex);
		m_bChanged = TRUE;
	}
}

////////////////////////////////////////////////////////////////////////////

void CPermissionsDlg::OnListSelChanged(int nListIndex)
{
	if (nListIndex != -1)
	{
		CListBoxItem *pItem = (CListBoxItem*)((CWorkaroundListBox*)m_List)->item(nListIndex);
		LPCSTR pText = pItem->text();

		ExtractWord(pText, "\t");

		QString Type((LPCSTR)ExtractWord(pText, "\t"));

		QListBox *pList = m_AccessType->listBox();
		
		int nCount = pList->count();
		int i;

		for (i=0; i < nCount; i++)
		{
			if (!strcmp(pList->text(i), Type))
			{
				m_AccessType->setCurrentItem(i);
				break;
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////
