/* Name: notifier.h

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

#ifndef __INC_NOTIFIER_H__
#define __INC_NOTIFIER_H__

#include <qobject.h>
#include "qtimer.h"
#include "qlist.h"
#include "mapper.h"

class CNetworkTreeItem;
class CListViewItem;

class CTreeExpansionNotifier : public QObject
{
	Q_OBJECT
public:
	~CTreeExpansionNotifier()
	{
		UnmapAllDelayed();
	}

	CTreeExpansionNotifier()
	{
		m_nWorkingLevel=0;
	}

	void Fire(CNetworkTreeItem *x)
	{
		emit ExpansionDone(x);
	}

	void Cancel(CNetworkTreeItem *x)
	{
		emit ExpansionCanceled(x);
	}

	void DoStartWorking();
	void DoEndWorking();

	void DoItemRenamed(CListViewItem *pItem)
	{
		emit ItemRenamed(pItem);
	}

	void DoItemDestroyed(CListViewItem *pItem)
	{
		emit ItemDestroyed(pItem);
	}

	void DoStartRescanItem(CListViewItem *pItem)
	{
		emit StartRescanItem(pItem);
	}

	void DoEndRescanItem(CListViewItem *pItem)
	{
		emit EndRescanItem(pItem);
	}

	void ScheduleUnmap(const char *UNC)
	{
		m_DelayedUnmap.append(new QString(UNC));

		QTimer::singleShot(5000, this, SLOT(DoDelayedUnmap()));
	}

	void DoSambaConfigurationChanged()
	{
		emit SambaConfigurationChanged();
	}

	void DoErrorAccessingURL(const char *Url)
	{
		emit ErrorAccessingURL(Url);
	}

	void DoFileJobWarning(int bOn)
	{
		emit FileJobWarning(bOn);
	}

	void DoRefreshRequested(const char *Path)
	{
		emit RefreshRequested(Path);
	}

  void DoMountListChanged()
  {
    emit MountListChanged();
  }

	void DoCheckRefresh()
	{
		emit CheckRefresh();
	}

public slots:
	void DoDelayedUnmap()
	{
		if (m_DelayedUnmap.count() > 0)
		{
			//printf("[Delayed] unmap of %s\n", (const char *)(*m_DelayedUnmap.at(0)));

			QString s;
#ifdef QT_20
			netunmap(s, m_DelayedUnmap.at(0)->latin1());
#else
			netunmap(s, *m_DelayedUnmap.at(0));
#endif
			m_DelayedUnmap.remove((unsigned int)0);
		}
	}

	void UnmapAllDelayed()
	{
		//printf("Unmap all delayed %d items...\n", m_DelayedUnmap.count());
		while (m_DelayedUnmap.count() > 0)
			DoDelayedUnmap();
	}

signals:
	void ItemRenamed(CListViewItem *);
	void ItemDestroyed(CListViewItem *);
	void ExpansionDone(CNetworkTreeItem *);
	void ExpansionCanceled(CNetworkTreeItem *);
	void StartRescanItem(CListViewItem *);
	void EndRescanItem(CListViewItem *);
	void StartWorking();
	void EndWorking();
	void SambaConfigurationChanged();
	void ErrorAccessingURL(const char *Url);
	void FileJobWarning(int);
	void RefreshRequested(const char *Path);
  void MountListChanged();
	void CheckRefresh();
private:
	int m_nWorkingLevel;
	QList<QString> m_DelayedUnmap;
};

extern CTreeExpansionNotifier gTreeExpansionNotifier;

class CWaitLogo
{
public:
	CWaitLogo()
	{
		gTreeExpansionNotifier.DoStartWorking();
	}

	~CWaitLogo()
	{
		gTreeExpansionNotifier.DoEndWorking();
	}
};

#endif /* __INC_NOTIFIER_H__ */
