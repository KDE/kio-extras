/* Name: almosthtmlview.cpp

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

#include "common.h"
#include "almosthtmlview.h"
#include "history.h"
//#include <khtml_part.h>

CAlmostHTMLView::CAlmostHTMLView(QWidget *_parent, 
																 LPCSTR _name, 
																 int 
#if (QT_VERSION < 200)
																 _flags
#endif
																 , 
																 KHTMLView *
#if (QT_VERSION < 200)
																 _parent_view
#endif
																 )
//: KHTMLView(new KHTMLPart(), _parent, _name)

#if (QT_VERSION < 200)
: KHTMLView(_parent, _name, _flags, _parent_view)
#else
: KHTMLPart(_parent, _name)
#endif

{
}


void CAlmostHTMLView::begin(LPCSTR _url, int _dx, int _dy)
{
	KHTMLView::begin(_url, _dx, _dy);
}


KHTMLView *CAlmostHTMLView::newView(QWidget *_parent, LPCSTR _name, int _flags)
{
	return new CAlmostHTMLView(_parent, _name, _flags, this);
}

bool CAlmostHTMLView::URLVisited(LPCSTR _url)
{
	return CHistory::Instance()->Visited(_url);
}
