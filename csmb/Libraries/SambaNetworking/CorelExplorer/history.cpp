/* Name: history.cpp

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "common.h"
#include "history.h"

#define DEFAULT_EXPIRATION_DELAY 30 /* days */
#define URL_MAX_LENGTH 1023
#define NOW 0.0

CHistoryItem::CHistoryItem()
{
	m_StrList = NULL;
	m_bFile = false;
	m_pPart = 0L;
	m_pListViewItem = NULL;
}

inline bool operator<(const QString& s, const QString& t)
{
	return (strcmp(s.data(), t.data()) < 0);
}

CHistory *CHistory::m_Instance = NULL;

CHistory::~CHistory()
{
	Commit();
	Clear();
}

CHistory *CHistory::Instance()
{
	if (NULL == m_Instance)
	{
		QString HistoryName = getenv("HOME");
		HistoryName += "/.CorExpHistory";
		m_Instance = new CHistory((LPCSTR)HistoryName
#ifdef QT_20
  .latin1()
#endif
    );
	}
	return m_Instance;
}

void CHistory::SetExpirationDelay(int nDays)
{
	m_nExpirationDelay = nDays;
}

void CHistory::SetVisited(LPCSTR Url)
{
	SetVisited(Url, NOW);
}

void CHistory::Clear()
{
	for (int i = 0; i < PRIME; i++)
	{
		SEntry *e = m_Table[i];
		SEntry *f;

		while (e != NULL)
		{
			f = e->pNext;
			delete e;
			e = f;
		}
	}
	m_SkipList.Clear();
}

void CHistory::Reload()
{
	long Then;
	time_t TThen, Now;
	char Buf[URL_MAX_LENGTH + 2];

	FILE *HistoryFile = fopen((LPCSTR)m_HistoryName
#ifdef QT_20
  .latin1()
#endif
  , "r");
	if (NULL == HistoryFile)
		return;

	Now = time(NULL);
	while (fscanf(HistoryFile, "%ld %[^\n]", &Then, Buf) == 2)
	{
		TThen = (time_t) Then;
		if (difftime(Now, TThen) < m_nExpirationDelay * 60 * 60 * 24)
			SetVisited(Buf, Then);
	}
	fclose(HistoryFile);
}

void CHistory::Commit()
{
	Reload();

	FILE *HistoryFile = fopen((LPCSTR)m_HistoryName
#ifdef QT_20
  .latin1()
#endif
, "w");
	if (NULL == HistoryFile)
		return;

	for (int i = 0; i < PRIME; i++)
	{
		SEntry *e = m_Table[i];
		SEntry *f;

		while (e != NULL)
		{
			f = e->pNext;
			fprintf(HistoryFile, "%ld %s\n", (long) e->Time, (LPCSTR) e->Url
#ifdef QT_20
  .latin1()
#endif
      );
			e->bDeja = TRUE;
			e = f;
		}
	}
	fclose(HistoryFile);
}

BOOL CHistory::Visited(LPCSTR Url) const
{
	SEntry *e = m_Table[HashPjw(Url)];

	while (NULL != e)
	{
		if (!strcmp((LPCSTR)e->Url
#ifdef QT_20
  .latin1()
#endif
    , Url))
			return TRUE;

		e = e->pNext;
	}
	return FALSE;
}

CHistory::Iterator CHistory::FindVisited(LPCSTR Url) const
{
	return m_SkipList.Find(Url);
}

CHistory::Iterator CHistory::BeginVisited() const
{
	return m_SkipList.Begin();
}

CHistory::Iterator CHistory::EndVisited() const
{
	return m_SkipList.End();
}

CHistory::CHistory(LPCSTR HistoryName)
	: m_HistoryName(HistoryName)
{
	for (int i = 0; i < PRIME; i++)
	{
		m_Table[i] = NULL;
	}
	m_nExpirationDelay = DEFAULT_EXPIRATION_DELAY;
	Reload();
}

int CHistory::HashPjw(LPCSTR Key)
{
	register unsigned g;
	register unsigned h = 0;
	register unsigned char c;

	while ('\0' != (c = *Key++))
	{
		h = (h << 4) + c;
		if (0 != (g = h & 0xf0000000))
		{
			h ^= g >> 24;
			h ^= g;
		}
	}
	return (h % PRIME);
}

void CHistory::SetVisited(LPCSTR Url, time_t Time)
{
	int k = HashPjw(Url);
	BOOL bDeja = (Time != NOW);
	SEntry *e = m_Table[k];
	SEntry *f;

	if (Time == NOW)
		Time = time(NULL);

	if (NULL == e)
	{
		e = new SEntry(bDeja, Time, Url, NULL);
		m_Table[k] = e;
	}
	else
	{
		do
		{
			if (!strcmp((LPCSTR)e->Url
#ifdef QT_20
  .latin1()
#endif
      , Url))
			{
				e->bDeja |= bDeja;
				e->Time = Time;
				return;
			}

			f = e;
			e = e->pNext;
		}
		while (e != NULL);

		e = new SEntry(FALSE, Time, Url, NULL);
		f->pNext = e;
	}
	m_SkipList.Insert(e->Url);
}
