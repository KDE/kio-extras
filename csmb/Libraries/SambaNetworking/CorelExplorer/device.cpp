/* Name: device.cpp

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
#include "device.h"
#include "filesystem.h"
#include <time.h>
#include "inifile.h"
#include "notifier.h"

void CDeviceItem::Init(CDeviceInfo *pInfo)
{
  *((CDeviceInfo*)this) = *pInfo;
  InitPixmap();
  QObject::connect(&gTreeExpansionNotifier, SIGNAL(MountListChanged()), this, SLOT(OnMountListChanged()));
}

QTTEXTTYPE CDeviceItem::text(int column) const
{
  switch (column)
  {
    case -1:
      return "MyComputer";
    case 0:
      return m_DisplayName;
    case 1:
      return m_Device;
    default:
      return "";
  }
}

BOOL CDeviceItem::ContentsChanged(time_t SinceWhen, int nOldChildCount, CListViewItem *pOldFirst)
{
  BOOL bWas = m_MountedOn.isEmpty();

  CheckRefresh();

  if (bWas != m_MountedOn.isEmpty()) // just mounted or unmounted
    return TRUE;

  if (m_MountedOn.isEmpty())
    return FALSE;
  else
  {
    BOOL b = IsExcludedFromRescan(m_MountedOn) ? FALSE : CLocalFileContainer::ContentsChanged(SinceWhen, nOldChildCount, pOldFirst);
    return b;
  }
}

void CDeviceItem::AccessDevice()
{
  if (m_MountedOn.isEmpty() &&
      !m_AutoMountLocation.isEmpty())
    access(m_AutoMountLocation, 0); // this will force automount daemon's mount attempt...
}

void CDeviceItem::Fill()
{
  AccessDevice();
  CheckRefresh();

  if (!m_MountedOn.isEmpty())
    CLocalFileContainer::Fill();
  else
  {
    SetExpansionStatus(keExpansionComplete);
    gTreeExpansionNotifier.Fire(this);
  }
}

void CDeviceItem::CheckRefresh()
{
  struct stat st;

	if (stat("/etc/mtab", &st))
		return; // cannot stat /etc/mtab, give up...

	if (st.st_mtime > m_LastUpdateTime)
	{
    time(&m_LastUpdateTime);

    CFileSystemArray list;

    GetFileSystemList(&list);

    int i;
    m_MountedOn = "";

    for (i=0; i < list.count(); i++)
    {
      CFileSystemInfo *pI = &list[i];

      if (pI->m_Name == m_Device)
      {
        pI->m_DriveType = m_DriveType; // preserve drive type because pI doesn't have it yet
        *((CFileSystemInfo*)this) = *pI; //m_MountedOn = pI->m_MountedOn;

      }
    }
  }
}

QString CDeviceItem::FullName(BOOL /*bDoubleSlashes*/)
{
  return m_MountedOn;
}

void CDeviceItem::OnMountListChanged()
{
  int i;
  BOOL bFound = FALSE;

  for (i=0; i < gFileSystemList.count(); i++)
  {
    CFileSystemInfo *pI = &gFileSystemList[i];

    if (pI->m_Name == m_Device)
    {
      bFound = TRUE;
      break;
    }
  }

  if (!bFound)
  {
    while (NULL != firstChild())
      delete firstChild();

    m_MountedOn = "";

    setOpen(FALSE);
    SetExpansionStatus(keNotExpanded);
    setExpandable(TRUE);
  }
}


