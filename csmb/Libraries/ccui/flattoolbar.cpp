/* Name: flattoolbar.cpp

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



#include "flattoolbar.h"


//****************************************************************************
//
//	Method:		CFlatToolBar(const char *szLabel,
//												 QMainWindow *pParent,
//												 QMainWindow::ToolBarDock dock,
//												 bool bNewLine,
//												 const char *szName)
//
//	Purpose:	A constructor.
//
//****************************************************************************
CFlatToolBar::CFlatToolBar(const char *szLabel,
													 QMainWindow *pParent,
													 QMainWindow::ToolBarDock dock,
													 bool bNewLine,
													 const char *szName)
						:	QToolBar(szLabel, pParent, dock, bNewLine, szName)
{
}
// CFlatToolBar()


//****************************************************************************
//
//	Method:		CFlatToolBar(const char *szLabel,
//												 QMainWindow *pMainWindow,
//												 QWidget *pParent,
//												 bool bNewLine,
//												 const char *szName,
//												 WFlags flags)
//
//	Purpose:	A constructor.
//
//****************************************************************************
CFlatToolBar::CFlatToolBar(const char *szLabel,
													 QMainWindow *pMainWindow,
													 QWidget *pParent,
													 bool bNewLine,
													 const char *szName,
													 WFlags flags)
						:	QToolBar(szLabel, pMainWindow, pParent, bNewLine, szName, flags)
{
}
// CFlatToolBar()


//****************************************************************************
//
//	Method:		CFlatToolBar(QMainWindow *pParent, const char *szName)
//
//	Purpose:	A constructor.
//
//****************************************************************************
CFlatToolBar::CFlatToolBar(QMainWindow *pParent, const char *szName)
						:	QToolBar(pParent, szName)
{
}
// CFlatToolBar()
