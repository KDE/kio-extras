/* Name: autotopcombo.h

   Description: This file is a part of the Corel File Manager application.

   Author:	Jasmin Blanchette

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

#ifndef __INC_AUTOTOPCOMBO_H__
#define __INC_AUTOTOPCOMBO_H__

#include "kurlcompletion.h"
#include "topcombo.h"

class CAutoTopCombo : public CTopCombo
{
	Q_OBJECT
public:
	CAutoTopCombo(QWidget *parent = NULL, const char *name = NULL);
	CAutoTopCombo(bool rw, QWidget *parent = NULL, const char *name = NULL);
	~CAutoTopCombo();

public slots:
	void setURLCompletedText(const char *text);

protected:
	virtual void resizeEvent(QResizeEvent *e);
	virtual void keyPressEvent(QKeyEvent *e);

	virtual void focusChanged();
	virtual bool doAutoCompletion();
	virtual void justBeforeReturnPressed();

private:
	KURLCompletion m_Kompletor;
	QListBox *m_pMatchListBox;
	unsigned m_OldCount;
	QString m_OldFirst;
	QString m_OldLast;
	bool m_bRightIsEnd;
	bool m_bAlternateHack;
};

#endif // __INC_AUTOTOPCOMBO_H__
