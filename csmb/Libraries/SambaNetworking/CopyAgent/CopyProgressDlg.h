/* Name: CopyProgressDlg.h

   Description: This file is a part of the CopyAgent.

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

/**********************************************************************

	--- Qt Architect generated file ---

	File: CopyProgressDlg.h
	Last generated: Wed Mar 3 11:54:46 1999

 *********************************************************************/

#ifndef CCopyProgressDlg_included
#define CCopyProgressDlg_included

#include "CopyProgressDlgData.h"
#include "common.h"
#include <qmovie.h>
#include <qevent.h>

typedef enum _CopyState
{
	keStarting,
	keBuildingFileList,
	keCopying,
	keDone
} CCopyState;

class CCopyProgressDlg : public CCopyProgressDlgData
{
	Q_OBJECT

public:

	CCopyProgressDlg
	(
		QWidget* parent = NULL,
		const char* name = NULL
	);

	virtual ~CCopyProgressDlg();
	
	QMovie m_Movie;
public slots:
	void ShowProgress();
	void DoStuff();
	BOOL CountFolder(LPCSTR Path);
	void keyPressEvent(QKeyEvent *e);
	void OnFileJobWarning(int);

protected:
	void show();
	void done(int r);

private:
	void PrepareExit();
	CCopyState m_State;
	CFileJob m_FileJob;
	time_t m_StartTime;
	BOOL m_bIdling;
	QStringArray m_UNCPath;
	int m_nOperationStringID;
	int m_nPreparingStringID;
};

#endif // CCopyProgressDlg_included
