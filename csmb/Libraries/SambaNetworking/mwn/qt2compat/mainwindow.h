/* Name: mainwindow.h

   Description: This file is a part of the libmwn library.

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


#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "common.h"
#include <ktmainwindow.h>
class QPaintEvent;
class QResizeEvent;
class QFrame;
class QEvent;
class QToolBar;

class CMainWindow : public KTMainWindow
{
	Q_OBJECT
public:
	CMainWindow(QWidget *pParent = 0,
							const char *szName = 0,
							WFlags = 0,
							bool bFrame = false);
	void setCentralWidget(QWidget *);
	QWidget *centralWidget();
	QWidget *centralFrame();
	void show();
	bool eventFilter(QObject *, QEvent *);
	void addToolBar(QToolBar *,
									const char *szLabel,
									ToolBarDock edge = Top,
									bool bNewLine = false);
	void removeToolBar(QToolBar *);

protected:
	void paintEvent(QPaintEvent *) {;}
	void fixCentralWidget();

private:
	QFrame *m_pMainFrame;
	QWidget *m_pCentralWidget;
	bool m_bFrame;

	// copy/assignment operator so people can't use them
	CMainWindow(CMainWindow &);
	CMainWindow &operator=(CMainWindow &);
};

#endif // MAINWINDOW_H
