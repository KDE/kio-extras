/* Name: ipedit.cpp

   Description: This file is a part of the ccui library.

   Authors:	Brian Jones
            Carlo Robazza

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

#include <stdio.h>
#include <qkeycode.h>
#include <qpalette.h>
#include <qcolor.h>
#include <qlayout.h>
#include "ipedit.h"
#include "ipedit.moc"

#define Inherited QFrame

OctetEdit::OctetEdit(QWidget* parent, const char* name) : 
	NetCLineEdit( parent, name)
{
	setMinimumSize( 0, 0 );
	setMaximumSize( 32767, 32767 );
	setFocusPolicy( QWidget::StrongFocus );
	setBackgroundMode( QWidget::PaletteBase );
	setFontPropagation( QWidget::NoChildren );
	setPalettePropagation( QWidget::NoChildren );
	setMaxLength( 3 );
	setEchoMode( OctetEdit::Normal );
	setText( "" );
	setFrame( FALSE );
	m_flags = 0;
}


void OctetEdit::keyPressEvent ( QKeyEvent * e )
{
	if ((e->key() == Key_Enter) || (e->key() == Key_Return))
	{
		e->ignore();
		return;
	}

	/* special keys */
	switch (e->key())
	{
	/* cursor-positions let handle normal */
	case Key_Home:
		emit home();
		break;

	case Key_End:
		emit end();
		break;

	case Key_Left:
		if (cursorPosition() == 0 && (m_flags & CURSOR_BACKWARD))
			emit prev(false);
		else
			NetCLineEdit::keyPressEvent(e);
		break;

	case Key_Right:
		if ((cursorPosition() == (int)text().length()) && (m_flags & CURSOR_FORWARD))
			emit next(false);
		else
			NetCLineEdit::keyPressEvent(e);
		break;

	/* Backspace needs a new definiton */
   	case Key_Backspace:
		if (cursorPosition() == 0 && (m_flags & CURSOR_BACKWARD))
		{
			emit prev(true);
		}			
		else
			NetCLineEdit::keyPressEvent(e);
		break;

	case Key_Delete:
		NetCLineEdit::keyPressEvent(e);
		break;

	case Key_Period:
		{			
			if (text().length() == 0 || hasMarkedText())
				break;
			
			int iLen = text().length();
			if ((m_flags & CURSOR_FORWARD) && iLen > 0 && cursorPosition() == iLen)
			{
				emit next(true);
			}
		}
		break;

	default:
		if ((e->ascii() >= Key_0) && (e->ascii() <= Key_9))
		{
			NetCLineEdit::keyPressEvent(e);
			
			if (text().length() != 0)
			{
				int iLen = text().length();
				if (iLen == 3 && cursorPosition() == 3)
				{
					emit next(true);
				}
			}
		}
		break;

//TODO-remove
/*	
	case Key_Period:
		{
			const char *szText = text();
			if (szText == 0 || hasMarkedText())
				break;

			
			int iLen = strlen(szText);
			if ((m_flags & CURSOR_FORWARD) && iLen > 0 && cursorPosition() == iLen)
			{
				emit next(true);
			}
		}
		break;

	default:
		if ((e->ascii() >= Key_0) && (e->ascii() <= Key_9))
		{
			NetCLineEdit::keyPressEvent(e);
			
			const char *szText = text();
			if (szText != 0)
			{
				int iLen = strlen(szText);
				if (iLen == 3 && cursorPosition() == 3)
				{
					emit next(true);
				}
			}
		}
		break;
*/
	}
}


IPEdit::IPEdit
(
	QWidget* parent,
	const char* name
)
	:
	Inherited( parent, name )
{
	QFont font( "Courier", 12, 50, 0 );
	font.setStyleHint( QFont::Courier );
	font.setCharSet( (QFont::CharSet)0 );
	
	m_Edit1 = new OctetEdit( this, "LineEdit_1" );
	m_Edit1->setFont( font );
	m_Edit1->setFlags( CURSOR_FORWARD );
	m_Edit1->setFocus();
	fixDisabledColorGroup(m_Edit1);
	connect(m_Edit1, SIGNAL(home()), this, SLOT(moveHome()));
	connect(m_Edit1, SIGNAL(end()), this, SLOT(moveEnd()));
	connect(m_Edit1, SIGNAL(next(bool)), this, SLOT(moveForward(bool)));

	connect(m_Edit1, SIGNAL(textChanged(const QString&)), this, SLOT(handleTextChange(const QString&)));

	m_Edit2 = new OctetEdit( this, "LineEdit_2" );
	m_Edit2->setFont( font );
	m_Edit2->setFlags( CURSOR_FORWARD | CURSOR_BACKWARD );
	fixDisabledColorGroup(m_Edit2);
	connect(m_Edit2, SIGNAL(home()), this, SLOT(moveHome()));
	connect(m_Edit2, SIGNAL(end()), this, SLOT(moveEnd()));
	connect(m_Edit2, SIGNAL(next(bool)), this, SLOT(moveForward(bool)));
	connect(m_Edit2, SIGNAL(prev(bool)), this, SLOT(moveBackward(bool)));

	connect(m_Edit2, SIGNAL(textChanged(const QString&)), this, SLOT(handleTextChange(const QString&)));

	m_Edit3 = new OctetEdit( this, "LineEdit_3" );
	m_Edit3->setFont( font );
	m_Edit3->setFlags( CURSOR_FORWARD | CURSOR_BACKWARD );
	fixDisabledColorGroup(m_Edit3);
	connect(m_Edit3, SIGNAL(home()), this, SLOT(moveHome()));
	connect(m_Edit3, SIGNAL(end()), this, SLOT(moveEnd()));
	connect(m_Edit3, SIGNAL(next(bool)), this, SLOT(moveForward(bool)));
	connect(m_Edit3, SIGNAL(prev(bool)), this, SLOT(moveBackward(bool)));

	connect(m_Edit3, SIGNAL(textChanged(const QString&)), this, SLOT(handleTextChange(const QString&)));

	m_Edit4 = new OctetEdit( this, "LineEdit_4" );
	m_Edit4->setFont( font );
	m_Edit4->setFlags( CURSOR_BACKWARD );
	connect(m_Edit4, SIGNAL(home()), this, SLOT(moveHome()));
	connect(m_Edit4, SIGNAL(end()), this, SLOT(moveEnd()));
	connect(m_Edit4, SIGNAL(prev(bool)), this, SLOT(moveBackward(bool)));

	connect(m_Edit4, SIGNAL(textChanged(const QString&)), this, SLOT(handleTextChange(const QString&)));

	fixDisabledColorGroup(m_Edit4);

	m_Label1 = new QLabel( this, "Label_1" );
	m_Label1->setFont( font );
	setupLabel(m_Label1);

	m_Label2 = new QLabel( this, "Label_2" );
	m_Label2->setFont( font );
	setupLabel(m_Label2);

	m_Label3 = new QLabel( this, "Label_3" );
	m_Label3->setFont( font );
	setupLabel(m_Label3);

	QBoxLayout* qtarch_layout_1 = new QBoxLayout( this, QBoxLayout::LeftToRight, 2, 0, NULL );
	qtarch_layout_1->addStrut( 0 );
	qtarch_layout_1->addWidget( m_Edit1, 1, 36 );
	QBoxLayout* qtarch_layout_1_2 = new QBoxLayout( QBoxLayout::TopToBottom, 0, NULL );
	qtarch_layout_1->addLayout( qtarch_layout_1_2, 0 );
	qtarch_layout_1_2->addStrut( 6 );
	qtarch_layout_1_2->addWidget( m_Label1, 1, 36 );
	qtarch_layout_1->addWidget( m_Edit2, 1, 36 );
	QBoxLayout* qtarch_layout_1_4 = new QBoxLayout( QBoxLayout::TopToBottom, 0, NULL );
	qtarch_layout_1->addLayout( qtarch_layout_1_4, 0 );
	qtarch_layout_1_4->addStrut( 6 );
	qtarch_layout_1_4->addWidget( m_Label2, 1, 36 );
	qtarch_layout_1->addWidget( m_Edit3, 1, 36 );
	QBoxLayout* qtarch_layout_1_6 = new QBoxLayout( QBoxLayout::TopToBottom, 0, NULL );
	qtarch_layout_1->addLayout( qtarch_layout_1_6, 0 );
	qtarch_layout_1_6->addStrut( 6 );
	qtarch_layout_1_6->addWidget( m_Label3, 1, 36 );
	qtarch_layout_1->addWidget( m_Edit4, 1, 36 );

//TODO-changed background colors directly
	setBackgroundColor(white);

	setMinimumSize( 0, 0 );
	setMaximumSize( 32767, 32767 );
	setFrameStyle(Panel | Sunken);
	setLineWidth(2);
	m_bChanged = false;
}


IPEdit::~IPEdit()
{
}

void IPEdit::paintEvent(QPaintEvent *event)
{
	fixDisabledColorGroup(m_Edit1);
	fixDisabledColorGroup(m_Edit2);
	fixDisabledColorGroup(m_Edit3);
	fixDisabledColorGroup(m_Edit4);

	fixNormalColorGroup(m_Label1);
	fixNormalColorGroup(m_Label2);
	fixNormalColorGroup(m_Label3);

	fixDisabledColorGroup(m_Label1);
	fixDisabledColorGroup(m_Label2);
	fixDisabledColorGroup(m_Label3);
	
	QFrame::paintEvent(event);
}

void IPEdit::fixDisabledColorGroup(QWidget *pWidget)
{
	QPalette p = pWidget->palette();
	QColorGroup cg = p.disabled();
	QColorGroup cgNew(cg.foreground(), cg.background(), cg.light(), cg.dark(), cg.mid(), cg.background().dark(300), cg.base());
	p.setDisabled(cgNew);
	pWidget->setPalette(p);
}

void IPEdit::fixNormalColorGroup(QWidget *pWidget)
{
	QPalette p = m_Edit1->palette();
	QColorGroup cg = p.normal();
//	QColorGroup cgNew(cg.foreground(), cg.base(), cg.light(), cg.dark(), cg.mid(), cg.text(), cg.base());
	QColorGroup cgNew(cg.foreground(), cg.base(), cg.light(), cg.dark(), cg.mid(), cg.text(), 
		cg.text(),cg.base(),cg.background());
	p.setNormal(cgNew);
//TODO-changed background color directly
	pWidget->setBackgroundColor(white);
	pWidget->setPalette(p);
}

void IPEdit::setupLabel(QLabel *label)
{
	label->setFocusPolicy( QWidget::NoFocus );
	label->setBackgroundMode( QWidget::PaletteBackground );
	label->setFontPropagation( QWidget::NoChildren );
	label->setPalettePropagation( QWidget::NoChildren );
	label->setText( "." );
	label->setAlignment( AlignCenter );
	label->setMargin( -1 );
	fixNormalColorGroup(label);
	fixDisabledColorGroup(label);
}

QString IPEdit::getText()
{
	m_strBuf = "";

	OctetEdit* aEdit[4] = {m_Edit1,m_Edit2,m_Edit3,m_Edit4};

	QString Octet;

	for (int i = 0; i < 4; i++)
	{
 		Octet = aEdit[i]->text();

//		fprintf(stderr,"[%d]%s\n",i,Octet.latin1());
		if (Octet.length() == 0 || Octet.toInt() > 255)
			return 0;
		else
		{
			m_strBuf += Octet;
			if (i < 3)
				m_strBuf += ".";
		}
	}

//	fprintf(stderr,"ext:%s\n",m_strBuf.latin1());
	return m_strBuf;
}

void IPEdit::setText(const QString& text)
{
	m_Edit1->clear();
	m_Edit2->clear();
	m_Edit3->clear();
	m_Edit4->clear();

	QString Text = text;
	
	if (Text.length() == 0)
		return;

	OctetEdit* aEdit[4] = {m_Edit1,m_Edit2,m_Edit3,m_Edit4};

	int j = -1;

	for (int i = 0; i < 4; i++)
	{
		j = Text.find(".");

		if (j != -1)
		{
			aEdit[i]->setText(Text.left(j));
			Text = Text.mid(j+1);
		}
		else
		{
			//set last element and exit
			aEdit[i]->setText(Text);
			break;
		}
	}
			
	m_bChanged = false;
}
//TODO-remove
/*
const char* IPEdit::getText()
{
	m_strBuf = "";
	
	const char *szOctet = m_Edit1->text();
	if (szOctet == 0 || strlen(szOctet) == 0 || QString(szOctet).toInt() > 255)
		return 0;
	else
	{
		m_strBuf += szOctet;
		m_strBuf += ".";
	}
	
	szOctet = m_Edit2->text();
	if (szOctet == 0 || strlen(szOctet) == 0 || QString(szOctet).toInt() > 255)
		return 0;
	else
	{		
		m_strBuf += szOctet;
		m_strBuf += ".";
	}

	szOctet = m_Edit3->text();
	if (szOctet == 0 || strlen(szOctet) == 0 || QString(szOctet).toInt() > 255)
		return 0;
	else
	{		
		m_strBuf += szOctet;
		m_strBuf += ".";
	}

	szOctet = m_Edit4->text();
	if (szOctet == 0 || strlen(szOctet) == 0 || QString(szOctet).toInt() > 255)
		return 0;
	else
	{		
		m_strBuf += szOctet;
	}
	
	return (const char *)m_strBuf;
}

void IPEdit::setText(const char *szText)
{
	m_Edit1->clear();
	m_Edit2->clear();
	m_Edit3->clear();
	m_Edit4->clear();
	
	if (szText == 0 || szText[0] == 0)
		return;
			
	char *szBuf = new char[strlen(szText)+1];
	strcpy(szBuf, szText);
	
	char *szOctet = strtok(szBuf, ".");
	if (szOctet != 0)
		m_Edit1->setText(szOctet);

	szOctet = strtok(0, ".");
	if (szOctet != 0)
		m_Edit2->setText(szOctet);
	
	szOctet = strtok(0, ".");
	if (szOctet != 0)
		m_Edit3->setText(szOctet);
	
	szOctet = strtok(0, ".");
	if (szOctet != 0)
		m_Edit4->setText(szOctet);

	m_bChanged = false;
}
*/
void IPEdit::moveHome()
{
	m_Edit1->setCursorPosition(0);
	m_Edit1->setFocus();
	m_Edit1->repaint(true);	
}

void IPEdit::moveEnd()
{
	m_Edit4->setCursorPosition(m_Edit4->text().length());

	m_Edit4->setFocus();
	m_Edit4->repaint(true);	
}

void IPEdit::moveForward( bool bSetSelect )
{
	OctetEdit *pEdit = (OctetEdit *)focusWidget();
	if (pEdit == m_Edit1)
		pEdit = m_Edit2;
	else if (pEdit == m_Edit2)
		pEdit = m_Edit3;
	else if (pEdit == m_Edit3)
		pEdit = m_Edit4;

	int iLen = pEdit->text().length();
	if (bSetSelect && iLen > 0)
	{
		pEdit->setSelection(0, iLen);
		pEdit->setCursorPosition(iLen);
	}
	else
		pEdit->setCursorPosition(0);

	focusNextPrevChild(true);
	pEdit->repaint(true);	
}

void IPEdit::moveBackward( bool bDelete )
{
	OctetEdit *pEdit = (OctetEdit *)focusWidget();
	if (pEdit == m_Edit2)
		pEdit = m_Edit1;
	else if (pEdit == m_Edit3)
		pEdit = m_Edit2;
	else if (pEdit == m_Edit4)
		pEdit = m_Edit3;

	int iLen = pEdit->text().length();
	if (bDelete && iLen > 0)
	{
		pEdit->setText(pEdit->text().left(iLen-1));
	}

	pEdit->setCursorPosition(pEdit->text().length());

//TODO-remove
/*		
	if (bDelete && strlen(pEdit->text()) > 0)
	{
		QString strText(pEdit->text());
		pEdit->setText(strText.left(strlen(pEdit->text())-1));
	}

	pEdit->setCursorPosition(strlen(pEdit->text()));
*/	
	focusNextPrevChild(false);
	pEdit->repaint(true);	
}

void IPEdit::handleTextChange(const QString& text)
{
//TODO changed signal to come on every char change not dependent on m_bChanged
//	if (!m_bChanged)
	{
		m_bChanged = true;
		emit textChanged(getText());
	}
}

void IPEdit::setFont(const QFont& font)
{
  QFont NewFont(font);
  NewFont.setStyleHint(QFont::Courier);
  NewFont.setFixedPitch(true);

  m_Edit1->setFont(NewFont);
  m_Edit2->setFont(NewFont);
  m_Edit3->setFont(NewFont);
  m_Edit4->setFont(NewFont);

  m_Label1->setFont(NewFont);
  m_Label2->setFont(NewFont);
  m_Label3->setFont(NewFont);
}

void IPEdit::setEnabled ( bool enable )
{
  m_Edit1->setEnabled(enable);
  m_Edit2->setEnabled(enable);
  m_Edit3->setEnabled(enable);
  m_Edit4->setEnabled(enable);

  m_Label1->setEnabled(enable);
  m_Label2->setEnabled(enable);
  m_Label3->setEnabled(enable);
}

void IPEdit::setFocusPolicy ( FocusPolicy policy )
{
	m_Edit1->setFocusPolicy(policy);
	m_Edit2->setFocusPolicy(policy);
	m_Edit3->setFocusPolicy(policy);
	m_Edit4->setFocusPolicy(policy);
	QFrame::setFocusPolicy(NoFocus);
}

#include <qpixmap.h>
#include <qlayout.h>
//#include "netidpaneldata.h"
//#include "netidpaneldata.moc"

#include <qlabel.h>

void NetCLineEdit::focusInEvent(QFocusEvent* pEvent)
{
	// when we get the focus, highlight the text
	QLineEdit::focusInEvent(pEvent);
	selectAll();
}

void NetCLineEdit::mousePressEvent(QMouseEvent* pEvent)
{
	// when the mouse is clicked anywhere in the field, un-highlight the text
	QLineEdit::mousePressEvent(pEvent);
	deselect();
}


