/* Name: autotopcombo.cpp

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

#include <iostream.h>
#include <ctype.h>
#include <qlistbox.h>
#include <qlineedit.h>
#include "autotopcombo.h"
#include "history.h"

#if QT_VERSION >= 200
//#error Check if m_bAlternateHack is necessary with Qt 2.0 or above.
#endif

CAutoTopCombo::CAutoTopCombo(QWidget *parent, const char *name)
	: CTopCombo(parent, name)
{
	m_pMatchListBox = NULL;
}

CAutoTopCombo::CAutoTopCombo(bool rw, QWidget *parent, const char *name)
	: CTopCombo(rw, parent, name)
{
	if (rw)
	{
		/*connect(&m_Kompletor, SIGNAL(setText(const char *)), this,
				SLOT(setURLCompletedText(const char *)));
			*/

		m_pMatchListBox = new QListBox(0, 0,
				WStyle_Customize | WStyle_NoBorder | WStyle_Tool);
		m_pMatchListBox->setAutoScrollBar(TRUE);
		m_pMatchListBox->setBottomScrollBar(FALSE);
		m_pMatchListBox->setAutoBottomScrollBar(TRUE);
		m_pMatchListBox->setFrameStyle(QFrame::Box | QFrame::Plain);
		m_pMatchListBox->setLineWidth(1);
		m_pMatchListBox->setFocusProxy(getLineEdit());
		m_pMatchListBox->resize(getLineEdit()->width() - 22, m_pMatchListBox->height());

		connect(m_pMatchListBox, SIGNAL(selected(int)), getLineEdit(), SIGNAL(returnPressed()));

		m_bRightIsEnd = FALSE;
		m_bAlternateHack = FALSE;
	}
	else
	{
		m_pMatchListBox = NULL;
	}
}

CAutoTopCombo::~CAutoTopCombo()
{
	delete m_pMatchListBox;
}

void CAutoTopCombo::setURLCompletedText(const char *text)
{
	QString strText = currentText();

	if (text && strText != text)
	{
		QString newText = text;
		int nE = newText.length();

		// It seems to be wise to keep the trailing slash, because: (1) Joseph M.,
		// a typical user, is complaining about the missing slash; (2) keeping it
		// saves saves one key stroke; and (3) it gives instant feedback to the user
		// that the file is a directory. But if you want the trailing slash to
		// vanish, simply restore the following code.
#if 0
		// remove trailing slash
		if (nE >= 3 && newText[0] == '/' && newText[nE - 1] == '/')
		{
			newText.truncate(nE - 1);
			nE--;
		}
#endif

		setEditText((LPCSTR)newText
#ifdef QT_20
  .latin1()
#endif
      );

		int n = strText.length();
		if (n > 0)
		{
			int i;

			// found match between strings
			for(i = 0; i < n && i < nE
					&& ((const char *)strText
#ifdef QT_20
  .latin1()
#endif
          )[i] == newText[i]; i++)
			{
					;
			}
			if (i > 0)
				getLineEdit()->validateAndSet(newText, i, i, nE);
		}
	}
}

void CAutoTopCombo::focusChanged()
{
	m_bRightIsEnd = FALSE;
	if (m_pMatchListBox)
	{
		m_pMatchListBox->hide();
		m_pMatchListBox->clear();
		m_OldCount = 0;
		m_OldFirst = (const char *) 0;
		m_OldLast = (const char *) 0;
	}
}

#define MAX_LINES_IN_MATCH_COMBO 30

bool CAutoTopCombo::doAutoCompletion()
{
  if (!m_pMatchListBox)
		return FALSE;

	QString ct = getLineEdit()->text();
	QString it;
	QStrList newStrList;
	CHistory *His = CHistory::Instance();
	QString found;
	bool bPrepended = FALSE;
	int foundAt = -1;
	unsigned foundLength = 100000;
	int nLines = 0;

	if (ct[0] == '/' && ct[1] != '/')
	{
		focusChanged();
		m_bRightIsEnd = TRUE;
#ifdef QT_20
    m_Kompletor.makeCompletion((LPCSTR)ct.latin1());
#else
		m_Kompletor.edited(ct);
		m_Kompletor.make_completion();
#endif
		return TRUE;
	}

	m_bRightIsEnd = TRUE;

#if (QT_VERSION < 200)
  if (ct.find(':') < 0 && isalpha(ct[0]))
#else
    if (ct.find(':') < 0 && ct[0].isLetter())
#endif
	{
		ct.prepend("http://");
		bPrepended = TRUE;
	}

	if (strncmp("http://www.", (LPCSTR)ct
#ifdef QT_20
  .latin1()
#endif
                                        , ct.length()) != 0 &&
			strncmp("ftp://ftp.", (LPCSTR)ct
#ifdef QT_20
  .latin1()
#endif
                                        , ct.length()) != 0)
	{
		CHistory::Iterator k = His->FindVisited((LPCSTR)ct
#ifdef QT_20
  .latin1()
#endif
    );

		while (*k != *His->EndVisited())
		{
			it = (*k).left(ct.length());
			if (ct != it)
				break;

			if ((*k).length() < foundLength)
			{
				found = (*k);
				foundLength = (*k).length();
				foundAt = nLines;
			}
			newStrList.append((LPCSTR)(*k)
#ifdef QT_20
  .latin1()
#endif
      );
			nLines++;
			k++;
		}
	}

	if (foundAt >= 0)
	{
		if (bPrepended)
		{
			ct = ct.mid(7, 100000);
#if (QT_VERSION < 200)
      found.detach();
#endif
			found = found.mid(7, 100000);
		}
		getLineEdit()->setCursorPosition(ct.length());
		getLineEdit()->validateAndSet(found, ct.length(), ct.length(),
				found.length());
	}

	if (nLines > 0 && nLines <= MAX_LINES_IN_MATCH_COMBO)
	{
		if (m_OldCount == 0
				|| newStrList.count() != m_OldCount
				|| newStrList.first() != m_OldFirst
				|| newStrList.last() != m_OldLast)
		{
			m_pMatchListBox->setUpdatesEnabled(FALSE);
			m_pMatchListBox->move(getLineEdit()->mapToGlobal(QPoint(0,
					getLineEdit()->height() + 2)));
			m_pMatchListBox->setFixedVisibleLines(nLines > 7 ? 7 : nLines);
			m_pMatchListBox->clear();
			m_pMatchListBox->insertStrList(&newStrList);
			m_pMatchListBox->setUpdatesEnabled(TRUE);
			m_OldCount = newStrList.count();
			m_OldFirst = *newStrList.first();
			m_OldLast = *newStrList.last();
			m_pMatchListBox->setCurrentItem(-1 /*foundAt*/);
			m_pMatchListBox->setTopItem(foundAt);
			m_pMatchListBox->repaint();
		}
		m_pMatchListBox->show();
	}
	else
		m_pMatchListBox->hide();

	return TRUE;
}

void CAutoTopCombo::justBeforeReturnPressed()
{
	if (m_pMatchListBox && m_pMatchListBox->isVisible() && -1 != m_pMatchListBox->currentItem())
	{
		getLineEdit()->setText(m_pMatchListBox->text(
				m_pMatchListBox->currentItem()));
	}


	QString strippedText = getLineEdit()->text();
  cout<<"strippedText= "<<strippedText<<endl;
	strippedText = strippedText.stripWhiteSpace();

	getLineEdit()->setText(strippedText);
}

void CAutoTopCombo::resizeEvent(QResizeEvent *e)
{
	CTopCombo::resizeEvent(e);
	if (m_pMatchListBox)
	{
		m_pMatchListBox->resize(getLineEdit()->width() - 22,
				m_pMatchListBox->height());
	}
}

void CAutoTopCombo::keyPressEvent(QKeyEvent *e)
{
	if (m_bRightIsEnd && e->key() == Qt::Key_Right)
#if (QT_VERSION >= 200)
    *e = QKeyEvent(QEvent::KeyPress, Qt::Key_End, e->ascii(), e->state());
#else
    *e = QKeyEvent(Event_KeyPress, Qt::Key_End, e->ascii(), e->state());
#endif

	m_bRightIsEnd = FALSE;

	if (m_pMatchListBox && m_pMatchListBox->isVisible())
	{
    if (e->key() == Qt::Key_Enter ||
        e->key() == Qt::Key_Return)
    {
      justBeforeReturnPressed();
      focusChanged();
    }

    if (e->key() == Qt::Key_Escape)
		{
			e->accept();
			focusChanged();
		}
		else if (e->key() == Qt::Key_Up)
		{
			e->accept();
			if (!m_bAlternateHack)
			{
				if (m_pMatchListBox->currentItem() > 0)
				{
					m_pMatchListBox->setCurrentItem(m_pMatchListBox->currentItem() - 1);
					if (m_pMatchListBox->topItem() > m_pMatchListBox->currentItem())
						m_pMatchListBox->setTopItem(m_pMatchListBox->currentItem());
				}
			}
			m_bAlternateHack = !m_bAlternateHack;
			return;
		}
		else if (e->key() == Qt::Key_Down)
		{
			e->accept();
			if (!m_bAlternateHack)
			{
				if (m_pMatchListBox->currentItem() + 1
						< (int) m_pMatchListBox->count())
				{
					m_pMatchListBox->setCurrentItem(m_pMatchListBox->currentItem() + 1);
					if (m_pMatchListBox->topItem() + m_pMatchListBox->numItemsVisible()
							<= m_pMatchListBox->currentItem())
					{
						m_pMatchListBox->setBottomItem(m_pMatchListBox->currentItem());
					}
				}
			}
			m_bAlternateHack = !m_bAlternateHack;
			return;
		} else if (e->key() == Qt::Key_Prior) {
			e->accept();
			if (!m_bAlternateHack)
			{
				if (m_pMatchListBox->currentItem() > 0)
				{
					if (m_pMatchListBox->currentItem() == m_pMatchListBox->topItem())
					{
						int prior = m_pMatchListBox->currentItem()
								- m_pMatchListBox->numItemsVisible() + 1;
						if (prior < 0)
							prior = 0;
						m_pMatchListBox->setCurrentItem(prior);
						m_pMatchListBox->setTopItem(prior);
					}
					else
						m_pMatchListBox->setCurrentItem(m_pMatchListBox->topItem());
				}
			}
			m_bAlternateHack = !m_bAlternateHack;
			return;
		}
		else if (e->key() == Qt::Key_Next)
		{
			e->accept();
			if (!m_bAlternateHack)
			{
				int bottomItem = m_pMatchListBox->topItem()
						+ m_pMatchListBox->numItemsVisible() - 1;
				if (m_pMatchListBox->currentItem() + 1
						< (int) m_pMatchListBox->count())
				{
					if (m_pMatchListBox->currentItem() == bottomItem)
					{
						int next = m_pMatchListBox->currentItem()
								+ m_pMatchListBox->numItemsVisible() - 1;
						if (next >= (int) m_pMatchListBox->count())
							next = (int) m_pMatchListBox->count() - 1;
						m_pMatchListBox->setCurrentItem(next);
						m_pMatchListBox->setBottomItem(next);
					}
					else
						m_pMatchListBox->setCurrentItem(bottomItem);
				}
			}
			m_bAlternateHack = !m_bAlternateHack;
			return;
		}
	}
	CTopCombo::keyPressEvent(e);
}
