/* Name: tips.cpp

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
#include "common.h"
#include "localfile.h"
#include "ftpfile.h"
#include "tips.h"

void CTreeToolTip::maybeTip(const QPoint& pos)
{
	CListViewItem *item = m_pParent->itemAt(pos);

	if (item != NULL)
	{
		QRect r(m_pParent->itemRect(item));

		QString s;

		LPCSTR p;

		if (CNetworkTreeItem::IsMyComputerItem(item) &&
			((CNetworkTreeItem*)item)->Kind() == keLocalFileItem)
		{
			s = ((CLocalFileItem *)item)->GetTip();
			p = s;
		}
		else
			if (CNetworkTreeItem::IsNetworkTreeItem(item))
			{
				CNetworkTreeItem *pI = (CNetworkTreeItem*)item;

				if (pI->Kind() == keFTPFileItem)
				{
					s = ((CFTPFileItem *)item)->GetTip();
					p = s;
				}
				else
					if (pI->Kind() == keFileItem || pI->Kind() == keShareItem)
					{
						s = ((CNetworkFileContainer *)item)->FullName(FALSE);
						p = s;
					}
					else
					{
				    s = item->text(1);
            p = s;
          }
			}
			else
      {
				s = item->text(1);
        p = s;
      }
		if (NULL != p && *p != '\0')
		{
			tip(r,p);
		}
	}
}
