/* Name: flattoolbar.h

   Description: This file is a part of the libmwn library.

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


#if (QT_VERSION >= 200)

#ifndef __INC_FLATTOOLBAR_H__
#define __INC_FLATTOOLBAR_H__

#include "qtoolbar.h"

class CFlatToolBar : public QToolBar
{
public:
	CFlatToolBar(const char *szLabel,
							 QMainWindow *pParent,
							 QMainWindow::ToolBarDock dock = QMainWindow::Top,
							 bool bNewLine = false,
							 const char *szName = 0) :
    QToolBar(szLabel, 
             pParent, 
             dock, 
             bNewLine, 
             szName)
  {
  }

  CFlatToolBar(const char *szLabel,
							 QMainWindow *pMainWindow,
							 QWidget *parent,
							 bool bNewLine = false,
							 const char *szName = 0,
							 WFlags flags = 0) :
    QToolBar(szLabel, 
             pMainWindow, 
             parent, 
             bNewLine, 
             szName, 
             flags)
  {
  }
	
  CFlatToolBar(QMainWindow *pParent = 0, 
               const char *szName = 0) :
    QToolBar(pParent, szName)
  {
  }
	
protected:
	void paintEvent(QPaintEvent *)
  {
  }
};

#endif // __INC_FLATTOOLBAR_H__
#endif // QT_VERSION

