/* Name: browsercache.h

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

#ifndef __INC_BROWSERCACHE_H__
#define __INC_BROWSERCACHE_H__

#include <qstring.h>

// Call SetMaxSize() to set the maximum size of the cache (in bytes), and
// MaxSize() to get it. It defaults to 512 KB and has to be set each time the
// program is run, at the beginning. EnforceMaxSize() trims the cache from any
// excess weight. It should be called before exiting. Clear() removes all files
// in the disk cache.

class CBrowserCache
{
public:
	static CBrowserCache *Instance();

	void SetMaxSize(int nSize);
	void EnforceMaxSize();
	void Clear();
	void Insert(LPCSTR Url, LPCSTR LocalName, BOOL bIntranet = FALSE,
			LPCSTR Expires = NULL);
	void Remove(LPCSTR Url);
	void SetLookupEnabled(BOOL bEnable);
	QString Lookup(LPCSTR Url, BOOL *bIntranet = NULL);

	int MaxSize() const { return m_nMaxSize; }

protected:
	CBrowserCache(LPCSTR Path);

private:
	static CBrowserCache *m_Instance;

	QString m_Path;
	BOOL m_bLookupEnabled;
	int m_nMaxSize;
};

#endif // __INC_BROWSERCACHE_H__
