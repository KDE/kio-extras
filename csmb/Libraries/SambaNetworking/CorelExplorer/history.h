/* Name: history.h

   Description: This file is a part of the Corel File Manager application.

   Author:	Jasmin Blanchette

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

#ifndef __INC_HISTORY_H__
#define __INC_HISTORY_H__

#include <qstring.h>
#include "common.h"
#include "skiplist.h"
#include <kparts/part.h>
#include <ktrader.h>


class CHistoryItem
{
public:
	CHistoryItem();

	void setStrList(QStrList strList){m_StrList = strList;}
	void setFile(BOOL file){m_bFile = file;}
	void setPart(KParts::ReadOnlyPart* part){m_pPart = part;}
	void setListViewItem(CListViewItem* listViewItem){m_pListViewItem = listViewItem;}
	void setPartServices(KTrader::OfferList services){ m_PartServices = services; }
	QStrList getStrList(){ return m_StrList; }
	BOOL getFile(){ return m_bFile; }
	KParts::ReadOnlyPart* getPart(){ return m_pPart; }
	CListViewItem* getListViewItem(){return m_pListViewItem; }
	KTrader::OfferList getPartServices(){ return m_PartServices; }

protected:
	QStrList m_StrList;
	BOOL m_bFile;
	KParts::ReadOnlyPart* m_pPart;
	KTrader::OfferList m_PartServices;
	CListViewItem* m_pListViewItem;
};

// Use SetExpirationDelay() to set the number of days before a link is marked
// unvisited again. Call ExpirationDelay() to obtain the current value. If you
// want to clear history, call Clear() and (possibly) Commit().

class CHistory
{
public:
	typedef CSkipList<QString>::Iterator Iterator;

	static CHistory *Instance();

	~CHistory();

	void SetExpirationDelay(int nDays);
	void SetVisited(LPCSTR Url);
	void Clear();
	void Reload();
	void Commit();
	BOOL Visited(LPCSTR Url) const;
	Iterator FindVisited(LPCSTR Url) const;
	Iterator BeginVisited() const;
	Iterator EndVisited() const;
	int ExpirationDelay() const { return m_nExpirationDelay; }

protected:
	CHistory(LPCSTR HistoryName);

private:
	// disable copy and assignment
	CHistory(const CHistory&);
	CHistory& operator=(const CHistory&);

	struct SEntry
	{
		BOOL bDeja;
		double Time;
		QString Url;
		SEntry *pNext;

		SEntry(BOOL bDeja0, double Time0, LPCSTR Url0, SEntry *pNext0)
			: bDeja(bDeja0),
			  Time(Time0),
				Url(Url0),
				pNext(pNext0) { }
	};

	static int HashPjw(LPCSTR Key);

	void SetVisited(LPCSTR Url, time_t Time);

	static CHistory *m_Instance;

	QString m_HistoryName;

	// The same information is kept in two places: First in m_Table, a hash table,
	// for telling fast whether a URL has been visited before. Most of the time
	// it hasn't been visited, so it's good to exit fast then. (It is used heavily
	// during parsing of HTML files.) Then in m_SkipList, an alphabetical list of
	// the visited URLs, useful for auto-completion.

	enum { PRIME = 2011 };
	SEntry *m_Table[PRIME];
	CSkipList<QString> m_SkipList;
	int m_nExpirationDelay;
};

#endif // __INC_HISTORY_H__
