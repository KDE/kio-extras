/* Name: labeledit.cpp

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

#include "wintree.h"
#include "qapplication.h"
#include <qmessagebox.h>
#include "qtimer.h"
#include "qevent.h"
#include "qfontmetrics.h"
#include "qlabel.h"

////////////////////////////////////////////////////////////////////////////

CListView *CLabelEditor::m_pParentView = NULL;
CLabelEditor *CLabelEditor::m_pLE = NULL;

////////////////////////////////////////////////////////////////////////////

CLabelEditor::CLabelEditor(QWidget *parent, CWindowsTreeItem *pItem) :
	QFrame(parent, "LabelEdit"), m_Edit(this)
{
  m_nInitialWidth = -1;
  m_pItem = pItem;
	setFrameStyle(Box | Plain);
	m_Edit.setFrame(0); // we have our own frame :-)
	m_Edit.show();
  connect(&m_Edit, SIGNAL(textChanged(const char *)), this, SLOT(OnEditTextChanged(const char *)));
	
  m_bNoRemoveMode = FALSE;
		
	m_pParentView = pItem->listView();
	m_pLE = this;
  m_pFillLabel = new QLabel(parent, "");
  
  m_pFillLabel->setBackgroundColor(pItem->listView()->colorGroup().base());
}

////////////////////////////////////////////////////////////////////////////

CLabelEditor::~CLabelEditor()
{
	m_pParentView = NULL;
	m_pLE = NULL;
  delete m_pFillLabel;
}

////////////////////////////////////////////////////////////////////////////

void CLabelEditor::OnEditTextChanged(const char *pNewText)
{
  int nWidth = fontMetrics().width(pNewText) + 15;

  //if (nWidth < 80)
  //  nWidth = 80;
  
  if (nWidth > ((QWidget*)parent())->width() - x()) // don't allow to be bigger than visible area
  {
    nWidth = ((QWidget*)parent())->width() - x();

    if (nWidth < 20)
      nWidth = 20; // just basic safety :)
  }
  
  if (width() != nWidth)
  {
    if (-1 == m_nInitialWidth)
      m_nInitialWidth = width();

    resize(nWidth, height());
    m_Edit.resize(nWidth - 2, height() - 2);
    
    int nFillWidth = m_nInitialWidth - width();
    
    if (nFillWidth > 0)
    {
      m_pFillLabel->setGeometry(x()+width(), y(), nFillWidth, height());
      m_pFillLabel->show();
    }
    else
      m_pFillLabel->hide();
  }
}

////////////////////////////////////////////////////////////////////////////

void CLabelEditor::setText(LPCSTR s)
{
	m_Edit.setText(s);
}

////////////////////////////////////////////////////////////////////////////

void CLabelEditor::setSelection(int n1, int n2)
{
	m_Edit.setSelection(n1, n2);
}

////////////////////////////////////////////////////////////////////////////

void CLabelEditor::setFocus()
{
  m_Edit.setFocus();
}

////////////////////////////////////////////////////////////////////////////

void CLabelEditorInternal::focusOutEvent(QFocusEvent *e)
{
  if (NULL != qApp->focusWidget())
    qApp->postEvent(parent(),
      new QEvent(
#ifdef QT_20
        (QEvent::Type)
#endif
                      0x900));
}

////////////////////////////////////////////////////////////////////////////

void CLabelEditor::OnEditEnd()
{
  if (!m_bNoRemoveMode)
		DoRename();
}

////////////////////////////////////////////////////////////////////////////

bool CLabelEditor::event(QEvent *e)
{
  if (e->type() == 0x900)
  {
    OnEditEnd();
    return false;
  }
  
  return QFrame::event(e);
}

////////////////////////////////////////////////////////////////////////////

BOOL CLabelEditor::DoRename()
{
  CSMBErrorCode retcode;
  CWindowsTreeItem *pItem;

  qApp->processEvents();
	
	QString sNewName(m_Edit.text());

	if (sNewName == m_pItem->text(0))
	{
		emit RemoveRequest(this);
		return FALSE; // nothing changed...
	}
	
  m_Edit.setFocus();
	
  if (!IsValidFileName(sNewName))
	{
		QMessageBox::critical(qApp->mainWidget(), LoadString(knSTR_RENAME), LoadString(knFILENAME_CANNOT_CONTAIN));

		m_bNoRemoveMode = FALSE;
		goto ReturnFocus;
	}

	if (sNewName == "." || sNewName == "..")
	{
		QMessageBox::critical(qApp->mainWidget(), LoadString(knSTR_RENAME), LoadString(knSTR_YOU_MUST_TYPE_A_FILENAME));
		m_bNoRemoveMode = FALSE;
		goto ReturnFocus;
	}

	pItem = m_pItem;
  retcode = pItem->Rename(sNewName);
	m_bNoRemoveMode = FALSE;

	if (keSuccess == retcode)
	{
		emit RemoveRequest(this);
		gTreeExpansionNotifier.DoItemRenamed(pItem);

		return TRUE; // do not call QLineEdit::keyPressEvent(e); because now our object is gone!
	}
  else
    if (keStoppedByUser == retcode)
    {
      emit RemoveRequest(this);
      return TRUE;
    }
	
ReturnFocus:;  
  m_Edit.setFocus();
	return FALSE;
}

////////////////////////////////////////////////////////////////////////////

void CLabelEditor::setGeometry(int x, int y, int w, int h)
{
	QFrame::setGeometry(x, y, w, h+1);
	m_Edit.setGeometry(1, 1, w-2, h-2);
}

////////////////////////////////////////////////////////////////////////////

void CLabelEditorInternal::keyPressEvent(QKeyEvent *e) 
{
	switch (e->key())
	{
		case Qt::Key_Escape:
			((CLabelEditor*)parent())->m_bNoRemoveMode = TRUE;
      ((QWidget*)(parent()->parent()))->setFocus();
			((CLabelEditor*)parent())->emit RemoveRequest((CLabelEditor*)parent());
		return; // do not call QLineEdit::keyPressEvent(e); because now our object is gone!

		case Qt::Key_Enter:
		case Qt::Key_Return:
		{
			((CLabelEditor*)parent())->m_bNoRemoveMode = TRUE;
			
      if (((CLabelEditor*)parent())->DoRename())
				return; // do not call QLineEdit::keyPressEvent(e); because now our object is gone!
		}
		break;
	}

	QLineEdit::keyPressEvent(e);
}

////////////////////////////////////////////////////////////////////////////

