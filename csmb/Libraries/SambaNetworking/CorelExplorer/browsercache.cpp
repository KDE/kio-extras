/* Name: browsercache.cpp

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

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <qstring.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "common.h"
#include "browsercache.h"

#define URL_MAX_LENGTH 1023
#define PRIME 149

static unsigned HashPjw(LPCSTR Key)
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

static BOOL HasExpired(LPCSTR ExpiresTime)
{
	if (NULL == ExpiresTime || '\0' == ExpiresTime[0])
		return FALSE;

	char CurrentTime[64];
	time_t t;

	time(&t);
	strftime(CurrentTime, 63, "%Y/%m/%d %H:%M:%S", localtime(&t));
	return (strcmp(ExpiresTime, CurrentTime) < 0);
}

CBrowserCache *CBrowserCache::m_Instance = NULL;

CBrowserCache *CBrowserCache::Instance()
{
	if (NULL == m_Instance)
	{
		QString m_Path = getenv("HOME");
		m_Path += "/.CorExpCache";

		if (0 != access((LPCSTR)m_Path
#ifdef QT_20
  .latin1()
#endif
    , F_OK))
		{
			if (0 != mkdir((LPCSTR)m_Path
#ifdef QT_20
  .latin1()
#endif
      , 0755))
				m_Path = "";
		}

		m_Instance = new CBrowserCache((LPCSTR)m_Path
#ifdef QT_20
  .latin1()
#endif
    );
	}
	return m_Instance;
}

void CBrowserCache::SetMaxSize(int nSize)
{
	m_nMaxSize = nSize;
}

// It's better to use the number of blocks than the actual file sizes, because
// that reflects move closely the space taken on the hard disk. On the other
// hand, st_blocks is not POSIX while st_size is.
#if 1
#define SIZE(st) ((st).st_blocks << 9)
#else
#define SIZE(st) ((st).st_size)
#endif

struct SEntry {
	int nHash;
	time_t TimeOfLastAccess;
	int nSize;
	BOOL bKeepMe;
};

// Compares two entries according to their time, but considers 0 as infinity.
static int CompareEntriesByTime(const void *x, const void *y)
{
	time_t tx = ((SEntry *) x)->TimeOfLastAccess;
	time_t ty = ((SEntry *) y)->TimeOfLastAccess;

	if (tx == ty)
		return 0;
	else if (0 == tx)
		return 1;
	else if (0 == ty || tx < ty)
		return -1;
	else
		return 1;
}

// Compares two entries according to their nHash value
static int CompareEntriesByHash(const void *x, const void *y)
{
	int nHashX = ((SEntry *) x)->nHash;
	int nHashY = ((SEntry *) y)->nHash;

	return (nHashX - nHashY);
}

void CBrowserCache::EnforceMaxSize()
{
	SEntry Table[PRIME];

	DIR *dp;
	struct dirent *dirp;

	QString Pathname;
	struct stat st;
	int nTotalSize = 0;

	for (int i = 0; i < PRIME; i++)
	{
		Table[i].nHash = i;
		Table[i].TimeOfLastAccess = 0;
		Table[i].nSize = 0;
		Table[i].bKeepMe = TRUE;
	}

	// First pass: We fill the table with the time of last access for a particular
	// node and with the size of the whole node.

	if (NULL == (dp = opendir((LPCSTR)m_Path
#ifdef QT_20
  .latin1()
#endif

  )))
	{
		// cannot open cache directory
		return;
	}

	while (NULL != (dirp = readdir(dp)))
	{
		int nHash, no;
		int nScan;

		nScan = sscanf(dirp->d_name, "%x.%d", &nHash, &no);
		if (1 == nScan) 			// index file
		{
			Pathname = m_Path + "/" + dirp->d_name;
			if (!stat((LPCSTR)Pathname
#ifdef QT_20
  .latin1()
#endif
      , &st) && S_ISREG(st.st_mode))
			{
				Table[nHash].TimeOfLastAccess = st.st_atime;
				Table[nHash].nSize += SIZE(st);
				nTotalSize += SIZE(st);
			}
		}
		else if (2 == nScan)	// cached file
		{
			Pathname = m_Path + "/" + dirp->d_name;
			if (!stat((LPCSTR)Pathname
#ifdef QT_20
  .latin1()
#endif
      , &st) && S_ISREG(st.st_mode))
			{
				Table[nHash].nSize += SIZE(st);
				nTotalSize += SIZE(st);
			}
		}
	}
	closedir(dp);

	if (nTotalSize <= m_nMaxSize)
	{
		// all right, the cache is already small enough
		return;
	}

	// Now put the oldest entries are at the beginning of the table. Those are the
	// ones we want to delete first. We will set the bKeepMe flag to FALSE for
	// each of those we want to get rid of.

	qsort(Table, PRIME, sizeof(SEntry), CompareEntriesByTime);

	for (int i = 0; i < PRIME && nTotalSize > m_nMaxSize; i++)
	{
		Table[i].bKeepMe = FALSE;
		nTotalSize -= Table[i].nSize;
	}

	// Put the entries back in their original order.

	qsort(Table, PRIME, sizeof(SEntry), CompareEntriesByHash);

	// Second pass: We remove the files that we don't want to keep.

	if (NULL == (dp = opendir((LPCSTR)m_Path
#ifdef QT_20
  .latin1()
#endif

  )))
	{
		// cannot open cache directory
		return;
	}

	while (NULL != (dirp = readdir(dp)))
	{
		int nHash, no;
		int nScan;

		nScan = sscanf(dirp->d_name, "%x.%d", &nHash, &no);
		if (nScan > 0) 			// index or cached file, we don't care
		{
			if (!Table[nHash].bKeepMe)
			{
				Pathname = m_Path + "/" + dirp->d_name;
				unlink((LPCSTR)Pathname
#ifdef QT_20
  .latin1()
#endif
        );
			}
		}
	}
	closedir(dp);
}

void CBrowserCache::Clear()
{
	int nMaxSize0 = m_nMaxSize;

	m_nMaxSize = 0;
	EnforceMaxSize();
	m_nMaxSize = nMaxSize0;
}

// arbitrary limit below which we won't even try to cache
#define NOCACHE 8192

void CBrowserCache::Insert(LPCSTR Url, LPCSTR LocalName, BOOL bIntranet,
		LPCSTR Expires)
{
	if (m_Path.isNull() || m_nMaxSize < NOCACHE || strlen(Url) > URL_MAX_LENGTH)
	{
		if (HasExpired(Expires))
		{
			// Remove any prior version of the document from cache.
			Remove(Url);
		}

		// We don't move it into cache. We need to delete it though.
		unlink(LocalName);
		return;
	}

	QString CachedName = Lookup(Url);
	if (!CachedName.isNull())
	{
		/* There is already a file corresponding to that URL. Simply replace it. */
		if (0 != rename(LocalName, (LPCSTR)CachedName
#ifdef QT_20
  .latin1()
#endif
    ))
			unlink(LocalName);
		return;
	}

	Remove(Url);	// we should care for it ourselves

	QString IndexName;
	IndexName.sprintf("%s/%.8x", (LPCSTR)m_Path
#ifdef QT_20
  .latin1()
#endif

  , HashPjw(Url));

	FILE *IndexFile = fopen((LPCSTR)IndexName
#ifdef QT_20
  .latin1()
#endif
  , "a");
	if (NULL == IndexFile)
	{
		/* We're unable to create an index for this file. Better to give up. */
		unlink(LocalName);
		return;
	}

	CachedName.sprintf("%s.%ld", (LPCSTR)IndexName
#ifdef QT_20
  .latin1()
#endif
  , ftell(IndexFile));

	if (0 == rename(LocalName, (LPCSTR)CachedName
#ifdef QT_20
  .latin1()
#endif
  ))
	{
		/* The file is now in cache. We can add the URL to the index file. */
		fprintf(IndexFile, "%c %s {", (bIntranet ? 'L' : 'E'), Url);
		if (NULL != Expires)
			fprintf(IndexFile, "%s", Expires);
		fprintf(IndexFile, "}\n");
	}
	else
	{
		// For some reason we were unable to move the file into cache. We have to
		// give up.
		unlink(LocalName);
	}
	fclose(IndexFile);
}

void CBrowserCache::Remove(LPCSTR Url)
{
	if (m_Path.isNull() || strlen(Url) > URL_MAX_LENGTH)
		return;

	char cStatus;
	char UrlInFile[URL_MAX_LENGTH + 1];
	char Expires[256];
	int nValidCount = 0;
	int nOffset = 0;

	QString IndexName;
	IndexName.sprintf("%s/%.8x", (LPCSTR)m_Path
#ifdef QT_20
  .latin1()
#endif
  , HashPjw(Url));

	FILE *IndexFile = fopen((LPCSTR)IndexName
#ifdef QT_20
  .latin1()
#endif
  , "rw");
	if (NULL == IndexFile)
		return;

	while (3 ==
			fscanf(IndexFile, "%c %s {%[^}]}\n", &cStatus, UrlInFile, Expires))
	{
		int nNewOffset = ftell(IndexFile);

		if ('X' != cStatus) 	// 'X' means deleted
		{
			nValidCount++;

			if (!strcmp(Url, UrlInFile))
			{
				fseek(IndexFile, (long) nOffset, SEEK_SET);
				fputc('X', IndexFile);
				fseek(IndexFile, (long) nNewOffset, SEEK_SET);
				nValidCount--;

				QString CachedName;
				CachedName.sprintf("%s.%d", (LPCSTR)IndexName
#ifdef QT_20
  .latin1()
#endif
        , nOffset);
				unlink((LPCSTR)CachedName
#ifdef QT_20
  .latin1()
#endif
        );
			}
		}
		nOffset = nNewOffset;
	}
	fclose(IndexFile);

	if (!nValidCount)
		unlink((LPCSTR)IndexName
#ifdef QT_20
  .latin1()
#endif
    );
}

void CBrowserCache::SetLookupEnabled(BOOL bEnable)
{
	m_bLookupEnabled = bEnable;
}

QString CBrowserCache::Lookup(LPCSTR Url, BOOL *bIntranet)
{
	if (m_Path.isNull() || strlen(Url) > URL_MAX_LENGTH || !m_bLookupEnabled)
		return NULL;

	char cStatus;
	char UrlInFile[URL_MAX_LENGTH + 1];
	char Expires[256];
	int nOffset = 0;

	QString IndexName;
	IndexName.sprintf("%s/%.8x", (LPCSTR)m_Path
#ifdef QT_20
  .latin1()
#endif
  , HashPjw(Url));

	FILE *IndexFile = fopen((LPCSTR)IndexName
#ifdef QT_20
  .latin1()
#endif
  , "rw");
	if (NULL == IndexFile)
		return NULL;

	Expires[0] = '\0';

	while (fscanf(IndexFile, "%c %s {%[^}]}\n", &cStatus, UrlInFile, Expires) >=
			2)
	{
		if ('X' != cStatus) 	// 'X' means deleted
		{
			if (!strcmp(Url, UrlInFile))
			{
				fclose(IndexFile);

				if (HasExpired(Expires))
				{
					Remove(Url);
					return NULL;
				}
				else
				{
					if (NULL != bIntranet)
						*bIntranet = ('L' == cStatus);

					QString CachedFile;
					CachedFile.sprintf("%s.%d", (LPCSTR)IndexName
#ifdef QT_20
  .latin1()
#endif
          , nOffset);
					return CachedFile;
				}
			}
		}
		nOffset = ftell(IndexFile);
		Expires[0] = '\0';
	}
	fclose(IndexFile);
	return NULL;
}

CBrowserCache::CBrowserCache(LPCSTR Path)
	: m_Path(Path)
{
	m_bLookupEnabled = TRUE;
	m_nMaxSize = 524288;
	EnforceMaxSize();
}
