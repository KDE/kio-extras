/* Name: labeledit.h

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

#ifndef __INC_LABELEDIT_H__
#define __INC_LABELEDIT_H__

#include "common.h"
#include "qframe.h"
#include "qlineedit.h"

class CWindowsTreeItem;
class CListView;
class QLabel;

class CLabelEditorInternal : public QLineEdit
{
public:
	CLabelEditorInternal(QWidget *parent) : QLineEdit(parent) {}
protected:
	void focusOutEvent(QFocusEvent *e);
	void keyPressEvent(QKeyEvent *e);
	void mousePressEvent(QMouseEvent *e)
	{
		QFontMetrics fm = fontMetrics();
		QString s(text());

		if (e->pos().x() >= fm.width(s, s.length()))
		{
			if (markedText() == s)
			{
				setSelection(s.length(), 0);
				setCursorPosition(s.length());
				repaint(FALSE);
				return;
			}
		}

		QLineEdit::mousePressEvent(e);
	}

	void mouseDoubleClickEvent(QMouseEvent *e)
	{
		QLineEdit::mouseDoubleClickEvent(e);
#ifdef QT_20
		setCursorPosition(text().length());
#else
		setCursorPosition(strlen(text()));
#endif
	}
private:
	friend class CLabelEditor;
};

class CLabelEditor : public QFrame
{
	Q_OBJECT
public:
	CLabelEditor(QWidget *parent, CWindowsTreeItem *pItem);
	~CLabelEditor();

	void setText(LPCSTR s);
	void setSelection(int n1, int n2);
	void setGeometry(int x, int y, int w, int h);
	void OnEditEnd();

signals:
	void RemoveRequest(CLabelEditor *pLE);
protected:
	BOOL DoRename();
	CLabelEditorInternal m_Edit;
  bool event(QEvent *e);
protected slots:
  void OnEditTextChanged(const char *);

private:
	friend class CLabelEditorInternal;
	CWindowsTreeItem *m_pItem;
	BOOL m_bNoRemoveMode;
  QLabel *m_pFillLabel;
  int m_nInitialWidth;
public:
  void setFocus();
	static CListView *m_pParentView;
	static CLabelEditor *m_pLE; // the only instance we can have :-)
};

#endif /* __INC_LABELEDIT_H__ */
