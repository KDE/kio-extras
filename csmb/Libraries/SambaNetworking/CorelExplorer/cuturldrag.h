/* Name: cuturldrag.h

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

#ifndef __INC_CUTURLDRAG_H__
#define __INC_CUTURLDRAG_H__

#include "qglobal.h"

#if (QT_VERSION >= 200)
#include "qdragobject.h"
#define CCorelStoredDrag QStoredDrag
#define CCorelMimeSource QMimeSource
#define CCorelUrlDrag QUrlDrag
#define CCorelTextDrag QTextDrag
#else
#include "coreldragobject.h"
#endif

class  CCorelCutUrlDrag: public CCorelStoredDrag
{
	Q_OBJECT

public:
	CCorelCutUrlDrag(QStrList urls, QWidget * dragSource = 0, const char * name = 0);
	CCorelCutUrlDrag(QWidget * dragSource = 0, const char * name = 0);
	~CCorelCutUrlDrag();

	void setUrls(QStrList urls);

	static bool canDecode(CCorelMimeSource* e);
	static bool decode(CCorelMimeSource* e, QStrList& i);
};


#endif /* __INC_CUTURLDRAG_H__ */
