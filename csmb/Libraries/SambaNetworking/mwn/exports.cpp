/* Name: exports.cpp

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

#include "common.h"
#include "exports.h"

////////////////////////////////////////////////////////////////////////////

static QStrList gNFSShareList;

////////////////////////////////////////////////////////////////////////////

void ReadNFSShareList()
{
	gNFSShareList.clear();
	
	if (!HasNFSSharing())
		return;

	FILE *f = fopen("/etc/exports", "r");
	
	if (NULL != f)
	{
		fseek(f,0L,2);
		long nSize = ftell(f);
		fseek(f,0L,0);
		char *pBuf = new char[nSize+1];
		
		while (!feof(f))
		{
			pBuf[0] = '\0';

			fgets(pBuf, nSize+1, f);
			
			if (feof(f))
				break;
			
			LPCSTR p = pBuf;
			
			if (*pBuf == '#' || *pBuf == '\n' || *pBuf == '\0')
				continue;
	
			int nLen = strlen(pBuf);

			if (nLen > 0 && pBuf[--nLen] == '\n')
				pBuf[nLen] = '\0';
				 
      while (!feof(f) && pBuf[strlen(pBuf)-1] == '\\')
			{
				fgets(pBuf+strlen(pBuf)-1, nSize+1, f);
				
				if (pBuf[strlen(pBuf)-1] == '\n')
					pBuf[strlen(pBuf)-1] = '\0';
			}
			
			// Strip any leading whitespace characters
			
			while (*p != '\0' && (*p == (char)9 || *p == (char)10 || *p == (char)11 || *p == (char)12 || *p == (char)13 || *p == (char)32))
				p++;
	   
			if (*p == '\0')
				continue;
			
			QString ShareName(ExtractWord(p));
			gNFSShareList.append(ShareName);
		}
		
		delete []pBuf;
		fclose(f);
	}
}

////////////////////////////////////////////////////////////////////////////

bool IsNFSShared(LPCSTR pPath)
{
	QStrListIterator it(gNFSShareList);

	for (it.toFirst(); NULL != it.current(); ++it)
	{
		if (IsSamePath(it.current(), pPath))
			return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////

