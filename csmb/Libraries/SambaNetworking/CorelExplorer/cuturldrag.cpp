/* Name: cuturldrag.cpp

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


#include "cuturldrag.h"
#include <stdio.h>

CCorelCutUrlDrag::CCorelCutUrlDrag(QStrList urls,
	    QWidget * dragSource, const char * name) :
    CCorelStoredDrag("application/CorelExplorer", dragSource, name)
{
	setUrls(urls);
}

/*
  Creates a object to drag.  You will need to call
  setUrls() before you start the drag().
*/
CCorelCutUrlDrag::CCorelCutUrlDrag(QWidget * dragSource, const char * name) :
    CCorelStoredDrag("application/CorelExplorer", dragSource, name)
{
}

/*
  Destroys the object.
*/

CCorelCutUrlDrag::~CCorelCutUrlDrag()
{
}

/*!
  Changes the list of \a urls to be dragged.
*/

void CCorelCutUrlDrag::setUrls(QStrList urls)
{
	QByteArray a;
	int c=0;
	
	for (const char* s = urls.first(); s; s = urls.next())
	{
		int l = strlen(s)+1;
		a.resize(c+l);
		memcpy(a.data()+c,s,l);
		c+=l;
	}
	
	a.resize(c-1); // chop off last nul
	setEncodedData(a);
}


/*
  Returns TRUE if decode() would be able to decode e.
*/

bool CCorelCutUrlDrag::canDecode(CCorelMimeSource* e)
{
	return e->provides("application/CorelExplorer");
}

/*
  Decodes URLs from e, placing the result in l (which is first cleared).

  Returns TRUE if the event contained a valid list of URLs.
*/

bool CCorelCutUrlDrag::decode(CCorelMimeSource* e, QStrList& l)
{
	QByteArray payload = e->encodedData("application/CorelExplorer");
	
	if (payload.size())
	{
		//e->accept();
		l.clear();
		l.setAutoDelete(TRUE);
		uint c=0;
		
		char* d = payload.data();
		
		while (c < payload.size())
		{
			uint f = c;
			
			while (c < payload.size() && d[c])
				c++;
			
			if (c < payload.size())
			{
				l.append(d+f);
				c++;
			}
			else
			{
#if (QT_VERSION < 200)
        QString s(d+f,c-f+1);
#else
        QCString s(d+f,c-f+1);
#endif				
        l.append(s);
			}
		}
		
		return TRUE;
	}
	
	return FALSE;
}
