/* Name: flattoolbar.h

   Description: This file is a part of the ccui library.

   Author:	Brian Rolfe

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


#ifndef FLATTOOLBAR_H
#define FLATTOOLBAR_H

#include "ccui_common.h"

#include <qtoolbar.h>

class CFlatToolBar : public QToolBar
{
	Q_OBJECT
public:
	CFlatToolBar(const char *szLabel,
							 QMainWindow *,
							 QMainWindow::ToolBarDock = QMainWindow::Top,
							 bool bNewLine = false,
							 const char *szName = 0);
	CFlatToolBar(const char *szLabel,
							 QMainWindow *,
							 QWidget *,
							 bool bNewLine = false,
							 const char *szName = 0,
							 WFlags flags = 0);
	CFlatToolBar(QMainWindow *pParent = 0, const char *szName = 0);
	
protected:
	void paintEvent(QPaintEvent *) {;}
};

#endif // FLATTOOLBAR_H
