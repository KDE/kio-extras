/* Name: home.cpp

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
#include "home.h"
#include "explres.h"
#include "filesystem.h"
#include <sys/time.h>
#include <unistd.h>
#include "exports.h"

CHomeItem::CHomeItem(CListViewItem *parent, CListViewItem *pLogicalParent, CSMBFileInfo *pInfo) :
	CLocalFileItem(parent, pLogicalParent, pInfo)
{
  time(&m_LastUpdateTime);

  // Update global filesystem list...

	time(&gFileSystemListTimestamp);
	GetFileSystemList(&gFileSystemList);

	m_FileName = getenv("HOME");
	setPixmap(0, *LoadPixmap(keHomeIcon));

	SetPixmapID(0xffff,0,0); // not set

	QObject::connect(&m_Timer, SIGNAL(timeout()), this, SLOT(CheckRefresh()));
	m_Timer.start(1000); // once a second
}

void CHomeItem::Fill()
{
  time(&m_LastUpdateTime);
  CLocalFileItem::Fill();
}

QString CHomeItem::FullName(BOOL /*bDoubleSlashes*/)
{
	return m_FileName;
}


QTTEXTTYPE CHomeItem::text(int column) const
{
	switch (column)
	{
		case -1:
			return "MyComputer";

		case 0: // Name
		default:
			return LoadString(knSTR_MY_HOME); //(QTTEXTTYPE)m_FileName;
		case 1: // Size
			return CListViewItem::text(1);
		case 2: // Type
			return (QTTEXTTYPE)m_FileAttributes;
		case 3: // Modified
			return CListViewItem::text(3);
	}
}

void CHomeItem::CheckRefresh()
{
  if (IsScreenSaverRunning())
		return;

	gTreeExpansionNotifier.DoCheckRefresh();
	struct stat st;

  if (ExpansionStatus() == keExpanding)
    return;

	if (!stat((LPCSTR)gSmbConfLocation
#ifdef QT_20
  .latin1()
#endif
  , &st))
	{
		if (st.st_mtime > m_LastUpdateTime)
		{
			time(&m_LastUpdateTime);
			ReReadSambaConfiguration();
			gTreeExpansionNotifier.DoSambaConfigurationChanged();
		}
	}

	if (!stat("/etc/exports", &st))
	{
		if (st.st_mtime > m_LastUpdateTime)
		{
			time(&m_LastUpdateTime);
			ReadNFSShareList();
			gTreeExpansionNotifier.DoSambaConfigurationChanged();
		}
	}

  if (stat("/etc/mtab", &st))
		return; // cannot stat /etc/mtab, give up...

	if (st.st_mtime > gFileSystemListTimestamp)
	{
    time(&gFileSystemListTimestamp);
		GetFileSystemList(&gFileSystemList);
    gTreeExpansionNotifier.DoMountListChanged();
	}

	if (!IsExcludedFromRescan((LPCSTR)m_FileName
#ifdef QT_20
  .latin1()
#endif
  ))
  {
		struct timeval tv1, tv2;
		struct timezone tz;

		gettimeofday(&tv1, &tz);

		if (ContentsChanged(0, -1))
    {
  		RescanItem(this);
    }
  	else
    {
  		CheckChangedFromItem(this);
    }

		gettimeofday(&tv2, &tz);
		double TimeSpent = (double)tv2.tv_sec * 1000000. + tv2.tv_usec - (double)tv1.tv_sec * 1000000. - tv1.tv_usec;

		m_Timer.stop();
		long Delay = (long)TimeSpent / 70;

		if (Delay < 1000)
			Delay = 1000;

		m_Timer.start(Delay);

		//printf("Setting to %ld millisec\n", Delay);
	}
}

