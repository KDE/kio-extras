/* Name: almosthtmlview.h

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

#ifndef __INC_ALMOSTHTMLVIEW_H__
#define __INC_ALMOSTHTMLVIEW_H__

#include "common.h"
//#include <khtml.h>
//#include <khtmlview.h>


#if (QT_VERSION < 200)
#include <khtmlview.h>
#else
#include <khtml_part.h>
#define KHTMLView KHTMLPart
#endif


class CAlmostHTMLView : public KHTMLView
{
	Q_OBJECT
public:
	CAlmostHTMLView(QWidget *_parent = NULL, LPCSTR _name = NULL, int _flags = 0, KHTMLView *_parent_view = NULL);

	virtual void begin(LPCSTR _url = 0, int _dx = 0, int _dy = 0);
	virtual KHTMLView *newView(QWidget *_parent = NULL, LPCSTR _name = NULL, int _flags = 0);

	virtual bool URLVisited(LPCSTR _url);
};

#endif // __INC_ALMOSTHTMLVIEW_H__
