/* Name: pixmapcombo.h

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


#ifndef __INC_PIXMAPCOMBO_H__
#define __INC_PIXMAPCOMBO_H__

#include "qcombobox.h"

class CPixmapCombo :  public QComboBox
{
public:
	CPixmapCombo(QWidget *parent=0, const char *name=0) :
		QComboBox(parent, name)
	{
	}
  
	CPixmapCombo(bool rw, QWidget *parent=0, const char *name=0) :
		QComboBox(rw, parent, name)
	{
	}

	void paintEvent(QPaintEvent *event);
};
#endif /* __INC_PIXMAPCOMBO_H__ */

