/* Name: mainwindow.cpp

   Description: This file is a part of the Corel File Manager application.

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

//****************************************************************************
//
//	Module:				mainwindow.cpp
//
//	Author:				Brian Rolfe
//
//  Description:	Implements class methods for CMainWindow
//
//
//****************************************************************************

#include "mainwindow.h"

#include <qapplication.h>
#include <qframe.h>


//****************************************************************************
//
//	Method:		CMainWindow(QWidget *pParent,
//												const char *szName,
//												WFlags flags,
//												bool bFrame)
//
//	Purpose:	This is the only constructor.  This method creates the
//						main frame and sets its style.
//
//****************************************************************************

CMainWindow::CMainWindow(QWidget *pParent,
												 const char *szName,
												 WFlags flags,
												 bool bFrame)
						:	KParts::MainWindow(szName, flags)

{
	m_pCentralWidget = 0;
	m_bFrame = bFrame;
	if (m_bFrame)
	{
		m_pMainFrame = new QFrame(this);
		m_pMainFrame->setFrameStyle(QFrame::Panel | QFrame::Sunken);
		m_pMainFrame->setLineWidth(2);
		m_pMainFrame->hide();
	}
}
// CMainWindow()


//****************************************************************************
//
//	Method:		setCentralWidget(QWidget *pCentralWidget)
//
//	Purpose:	Overridden virtual method so I can pass the main frame
//						as the central widget and control the widget they sent me
//						and place it in the frame.
//
//****************************************************************************
void CMainWindow::setCentralWidget(QWidget *pCentralWidget)
{
	if (m_bFrame)
	{
		m_pCentralWidget = pCentralWidget;
		if (pCentralWidget)
		{
			m_pMainFrame->show();
			QMainWindow::setCentralWidget(m_pMainFrame);
			fixCentralWidget();
		}
		else
		{
			m_pMainFrame->hide();
		}
	}
	else
	{
		if (pCentralWidget)
		{
			QMainWindow::setCentralWidget(pCentralWidget);
		}
	}
}
// setCentralWidget()


//****************************************************************************
//
//	Method:		show()
//
//	Purpose:	Overridden virtual method to show the CMainWindow.
//						The class show the main window and then resizes the users
//						central widget to fit in the frame.
//
//****************************************************************************
void CMainWindow::show()
{
	QMainWindow::show();
	if (m_bFrame)
	{
		qApp->processEvents();
		fixCentralWidget();
	}
}
// show()


//****************************************************************************
//
//	Method:		eventFilter(QObject *pObject, QEvent *pEvent)
//
//	Purpose:	This method catches all events and causes the users central
//						widget to be moved and resized if the event is a resize event.
//
//****************************************************************************
bool CMainWindow::eventFilter(QObject *pObject, QEvent *pEvent)
{
#ifdef QT_20
	if (m_pCentralWidget && m_bFrame && pEvent->type() == QEvent::Resize)
#else
	if (m_pCentralWidget && m_bFrame && pEvent->type() == Event_Resize)
#endif
	{
		fixCentralWidget();
	}

	return QMainWindow::eventFilter(pObject, pEvent);
}
// eventFilter()


//****************************************************************************
//
//	Method:		fixCentralWidget()
//
//	Purpose:	This method centers and sizes the users central widget into the
//						frame created.
//
//****************************************************************************
void CMainWindow::fixCentralWidget()
{
	if (m_pCentralWidget && m_bFrame)
	{
		int nLineWidth = m_pMainFrame->lineWidth();
		m_pCentralWidget->setGeometry(m_pMainFrame->x() + nLineWidth,
														 			m_pMainFrame->y() + nLineWidth,
														 			m_pMainFrame->width() - nLineWidth * 2,
														 			m_pMainFrame->height() - nLineWidth * 2);
	}
}
// fixCentralWidget()


void CMainWindow::addToolBar(QToolBar *pToolBar,
														 const char *szLabel,
														 ToolBarDock edge,
														 bool bNewLine)
{
	QMainWindow::addToolBar(pToolBar, szLabel, edge, bNewLine);

	if (m_pCentralWidget && m_bFrame)
	{
		m_pMainFrame->hide();
		m_pMainFrame->raise();
		m_pCentralWidget->raise();
		m_pMainFrame->show();
	}
}


void CMainWindow::removeToolBar(QToolBar *pToolBar)
{
	QMainWindow::removeToolBar(pToolBar);
	if (m_pCentralWidget && m_bFrame)
	{
		m_pMainFrame->hide();
		m_pMainFrame->raise();
		m_pCentralWidget->raise();
		m_pMainFrame->show();
	}
}


QWidget *CMainWindow::centralWidget()
{
	if (m_bFrame)
	{
		return m_pCentralWidget;
	}
	else
	{
		return QMainWindow::centralWidget();
	}
}


QWidget *CMainWindow::centralFrame()
{
	if (m_bFrame)
	{
		return m_pMainFrame;
	}
	else
	{
		return NULL;
	}
}
