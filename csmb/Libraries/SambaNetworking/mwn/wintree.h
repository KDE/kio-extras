/* Name: wintree.h

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


/*
 * NOTE: This source file was created using tab size = 2.
 * Please respect that setting in case of modifications.
 */

#ifndef __INC_WINTREE_H__
#define __INC_WINTREE_H__

#include "common.h"
#include "labeledit.h"
#include "listview.h"

class CWindowsTreeItemBuddy : public QObject
{
	Q_OBJECT
public slots:
	void OnRemoveLabelEditor(CLabelEditor *pLE);
};

class CWindowsTreeItem  : public CListViewItem
{
public:
	CWindowsTreeItem(CListView *parent) : CListViewItem(parent) {}
	CWindowsTreeItem(CListViewItem *parent) : CListViewItem(parent) {}
	
	CWindowsTreeItem(CListView *parent, 
										LPCSTR p0, 
										LPCSTR p1=NULL, 
										LPCSTR p2=NULL, 
										LPCSTR p3=NULL, 
										LPCSTR p4=NULL, 
										LPCSTR p5=NULL, 
										LPCSTR p6=NULL, 
										LPCSTR p7=NULL) :
		CListViewItem(parent, p0,p1,p2,p3,p4,p5,p6,p7) {}

	CWindowsTreeItem(CListViewItem *parent, 
										LPCSTR p0, 
										LPCSTR p1=NULL, 
										LPCSTR p2=NULL, 
										LPCSTR p3=NULL, 
										LPCSTR p4=NULL, 
										LPCSTR p5=NULL, 
										LPCSTR p6=NULL, 
										LPCSTR p7=NULL) :
		CListViewItem(parent, p0,p1,p2,p3,p4,p5,p6,p7) {}

public:
	void paintCell(QPainter * p, const QColorGroup & cg, int column, int width, int align);
	void paintFocus(QPainter *p, const QColorGroup &cg,	const QRect & r);
	void StartLabelEdit();
	
	virtual BOOL CanEditLabel()
	{
		return FALSE;
	}

	virtual CSMBErrorCode Rename(LPCSTR /*sNewLabel*/)
	{
		return keWrongParameters;
	}

	virtual BOOL CanCreateSubfolder()
	{
		return FALSE;
	}

	virtual BOOL CreateSubfolder(QString& /*sFolderName*/)
	{
		return FALSE;
	}

	BOOL IsOKToEditLabel(int xoff);
};

#endif /* __INC_WINTREE_H__ */
