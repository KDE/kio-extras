/* Name: cdate.cpp

   Description: This file is a part of the ccui library.

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


#include <qpainter.h>
#include <qkeycode.h>
#include <qaccel.h>
#include <stdio.h>
#include <stdlib.h>
#include <qmessagebox.h>
#include "cdate.h"
#include "common.h"
#include "resource.h"

#include "langinfo.h"

static char* days[7] = {0};

static char* months[12] = {0};

CDateComboBox::CDateComboBox(QWidget* parent, const char* name)
	: QComboBox(false,parent,name)
{
	initWidget(QDate::currentDate());
}

CDateComboBox::CDateComboBox(QDate d, QWidget* parent, const char* name)
	: QComboBox(false,parent,name)
{
	initWidget(d);
}

void CDateComboBox::initWidget(QDate d)
{
	//get days of week from locale
	for (int nDays = 0; nDays < 7; nDays++)
	{
		if (nDays == 7-1)
		{
			//put sunday specially
			days[nDays] = nl_langinfo(ABDAY_1);
		}
		else
		{
			//start with monday
			days[nDays] = nl_langinfo(ABDAY_2+nDays); 
		}
		//make sure first char is uppercase
//		*days[nDays] &= ~('a'-'A');
//		fprintf(stderr,"day[%d] %s\n",nDays,days[nDays]);
	}

	//get months of year from locale
	for (int nMonths = 0; nMonths < 12; nMonths++)
	{
		months[nMonths] = nl_langinfo(ABMON_1+nMonths);
		//make sure first char is uppercase
//		*months[nMonths] &= ~('a'-'A');
//		fprintf(stderr,"month[%d] %s\n",nMonths,months[nMonths]);
	}

	int x = DATE_COMBO_X;
	int y = DATE_COMBO_Y;

	resize(x,y);

	setMinimumSize(x,y);
	setMaximumSize(x,y);

	m_dateWidget = new CDateWidget(d,0,0,WType_Popup);//no caption
	ASSERT(m_dateWidget);

	connect(m_dateWidget,SIGNAL(dateChanged(QDate)),this,SLOT(dateChange(QDate)));
	//set date first time in our combo because we dont get signal first time
	ASSERT(d == m_dateWidget->getDate());

//	QString s = CDateWidget::dateString(d);

//	ASSERT(count() == 0);
	//we insert only one item
//	insertItem(s);
//	ASSERT(count() == 1);
	//make sure only item is selected
//	setCurrentItem(0);

	dateChange(d);

	m_bPopup = false;
}

void CDateComboBox::mousePressEvent(QMouseEvent* e)
{
	fprintf(stderr,"mouse press event %d,%d\n",e->x(),e->y());
	popup();
}

void CDateComboBox::keyPressEvent(QKeyEvent* e)
{
	fprintf(stderr,"key press event %d status %d\n",e->key(),e->state());
	//combo is popped up by F4 and ALT-KeyUp/Down keys
	//we will use same code to pop down list in table view
	if ( e->key() == Key_F4 ||
		( ( e->key() == Key_Down || e->key() == Key_Up) 
			&& ((e->state() & (AltButton | ControlButton)) == AltButton) ) )
	{
		e->accept();
		popup();
		return;
	}

	QComboBox::keyPressEvent(e);
}

void CDateComboBox::popup()
{
	//update it from our widget with one exception
	if (!m_dateWidget->getUpdate())
	{
		m_bPopup = m_dateWidget->isVisible();
	}
	else
	{
		m_dateWidget->setUpdate(false);
	}

	m_bPopup = !m_bPopup;

	fprintf(stderr,"popup %d\n",m_bPopup);
	if (!m_bPopup)
	{
		m_dateWidget->hide();
		return;
	}

	QPoint pos = mapToGlobal( QPoint(0,height()) );

	int x = pos.x();
	int y = pos.y();
	int w = m_dateWidget->width();
	int h = m_dateWidget->height();

	m_dateWidget->move( x,y );
	m_dateWidget->show();
}

void CDateComboBox::dateChange(QDate d)
{
	QString s = CDateWidget::dateString(d);
	//display in combo box
//	ASSERT(count() == 1);
	removeItem(0);
	ASSERT(count() == 0);
	insertItem(s,0);
	ASSERT(count() == 1);
	setCurrentItem(0);
}

QString CDateWidget::dateString(QDate d)
{
	QString s;
	char szBuffer[256] = {0};
	sprintf(szBuffer,IDS_FORMAT,days[d.dayOfWeek()-1],months[d.month()-1],d.day(),d.year());
	s = szBuffer;
	return s;
}

CDateWidget::CDateWidget( QWidget *parent, const char *name, WFlags f )
    : QTableView(parent,name,f)
{
	initWidget( QDate::currentDate() );
}

CDateWidget::CDateWidget( QDate d, QWidget *parent, const char *name, WFlags f )
    : QTableView(parent,name,f)
{
	initWidget( d );
}

void CDateWidget::initWidget(QDate d)
{
	m_bUpdate = false;
	m_bHide = true;

	m_rowSelect = -1;
	m_colSelect = -1;

	setFocusPolicy( StrongFocus );
	setBackgroundColor(white);

	int ncols = X_MAXCOLS;
	int nrows = Y_MAXROWS;

	setNumCols( ncols );
	setNumRows( nrows );

	int cellx = CELL_WIDTH;
	int celly = CELL_HEIGHT;

	setCellWidth( cellx );
	setCellHeight( celly );

	setTableFlags( Tbl_autoScrollBars |
		Tbl_clipCellPainting |	
		Tbl_smoothScrolling );	

	int dx = SIZE_X;
	int dy = SIZE_Y;

	resize( dx, dy );

	setMinimumSize( dx, dy );
	setMaximumSize( dx, dy );

	m_strings = new QString[numRows() * numCols()];
	ASSERT(m_strings);

	setDate(d);
}

void CDateWidget::setDate(QDate d)
{
	if ((d.year() == m_date.year()) && d.month() == m_date.month() &&
		(d.day() == m_date.day()))
	{
		fprintf(stderr,"setting same date\n");
		return;
	}

	QString s = dateString(d);
	setCaption(s);

	bool bRepaint = true;
	//if same year and month - dont repaint
	if ((d.year() == m_date.year()) && d.month() == m_date.month())
	{
		bRepaint = false;
	}

	m_date = d;

	emit dateChanged(m_date);

	if (!bRepaint)
	{
		int oldRow = m_rowSelect;
		int oldCol = m_colSelect;

		//find out new selection
		QDate d1;
		d1.setYMD(d.year(),d.month(),1);

		//first row reserved for days of week
		int j = 1;
		//starting row is day of week - 1 based
		int i = d1.dayOfWeek()-1;
		//how many we have to display
		int k = d.daysInMonth();
		//current day of month - 1 based
		int l = 1;

		bool bFound = false;

		while (k > 0)
		{
			//set current day selection
			if (l == m_date.day())
			{
				m_rowSelect = j;
				m_colSelect = i;
				bFound = true;
			}
			i++;
			if (i == numRows())
			{
				j++;
				i = 0;
			}
			l++;
			k--;
		}
		ASSERT(bFound);
		//now update cells

		// erase previous marking
		updateCell( oldRow, oldCol );
		
		// show new current cell
		updateCell( m_rowSelect, m_colSelect );	

		return;
	}

	int j = 0;
	int i = 0;

	//paint date calendar
	for (i = 0; i < numCols(); i++)
	{
		s.sprintf("@%s",days[i]);
		setCellContent(j,i,s);
	}
	//helper date-first day of current month
	QDate d1;
	d1.setYMD(d.year(),d.month(),1);

	//go to next row
	j++;
	//starting row is day of week - 1 based
	i = d1.dayOfWeek()-1;
	//how many we have to display
	int k = d.daysInMonth();
	//current day of month - 1 based
	int l = 1;
	//define previous/next month
	d1 = prevMonthDate(getDate());

	int n = d1.daysInMonth();
	int m = 0;
	
	//complete previous month days
	for (m = i-1 ; m >= 0; m--)
	{
		s.sprintf("!%d",n);
		setCellContent(j,m,s);
		n--;
	}
	//note:current month elements are preceeded by "$" char
	//prev/next monght elements are preceeded by "!" char
	//in this way paint method knows if to display normal
	//or gray chars for the text in current cell
	//day of week elements - first row elements are preceeded by
	//"@" char - they are bold type chars

	bool bFound = false;

	while (k > 0)
	{
		s.sprintf("$%d",l);
		setCellContent(j,i,s);
		//set current day selection
		if (l == m_date.day())
		{
			m_rowSelect = j;
			m_colSelect = i;
			bFound = true;
		}
		i++;
		if (i == numRows())
		{
			j++;
			i = 0;
		}
		l++;
		k--;
	}
	ASSERT(bFound);
	//complete next month days
	//define previous/next month
	d1 = nextMonthDate(getDate());
	//start with first day in next month
	n = 1;
	m = 0;
	
	//complete previous month days
	for (m = i ; m < numCols(); m++)
	{
		s.sprintf("!%d",n);
		setCellContent(j,m,s);
		n++;
	}

	//now if last row not completed complete it with next month also
	if (j < numCols()-1)
	{
		j++;

		for (m = 0 ; m < numCols(); m++)
		{
			s.sprintf("!%d",n);
			setCellContent(j,m,s);
			n++;
		}
	}
	ASSERT(j == numCols()-1);
	repaint();
}

QDate CDateWidget::getDate()
{
	return m_date;
}

CDateWidget::~CDateWidget()
{
	delete[] m_strings;
}

QString CDateWidget::cellContent( int row, int col ) const
{
	return m_strings[indexOf( row, col )];
}

void CDateWidget::setCellContent( int row, int col, QString s, bool update )
{
	m_strings[indexOf( row, col )] = s;

	if (update)
	{
		updateCell( row, col );
	}
}

void CDateWidget::paintCell( QPainter* p, int row, int col )
{
//	fprintf(stderr,"paint cell %d,%d\n",col,row);

	int w = cellWidth( col );
	int h = cellHeight( row );

	int x1 = 0;
	int y1 = 0;

	int x2 = w - 1;
	int y2 = h - 1;

	int dx = DELTA_X;
	int dy = DELTA_Y;

	//draw rectangle around cell
//	p->drawLine( x2, y1, x2, y2 );
//	p->drawLine( x1, y2, x2, y2 );

	//check if current cell
	if ( (row == m_rowSelect) && (col == m_colSelect) ) 
	{
		//draw special dashed rectangle if without focus
		if ( hasFocus() ) 
		{
			p->drawRect( x1+dx, y1+dy, x2-dx-dx, y2-dy-dy );	
		}
		else 
		{
			p->setPen( DotLine );
			p->drawRect( x1+dx, y1+dy, x2-dx-dx, y2-dy-dy );
			p->setPen( SolidLine );
		}
	}

	//draw it
	QString s = m_strings[indexOf(row,col)];
	//if title (first row) special bold font
	if (s[0] == '@')
	{
		p->setFont(QFont("Arial",12,QFont::Bold,TRUE));
		//draw line underneath day name
		p->drawLine( x1, y2, x2, y2 );
	}
	else
	{
		p->setFont(QFont("Arial",12));
	}
	//if previous/next month paint with gray
	if (s[0] == '!')
	{
		p->setPen(gray);
	}
	else
	{
		p->setPen(black);
	}

	p->drawText( 0, 0, w, h, AlignCenter, s.data()+1 );
}

void CDateWidget::mouseDoubleClickEvent( QMouseEvent* e )
{
	//duplicated from mousePressEvent
	//retKeyClicked is called if current celll is allowed to be clicked
	fprintf(stderr,"internal mouse double click event %d,%d\n",e->x(),e->y());

	//save previous cell
	int oldRow = m_rowSelect;
	int oldCol = m_colSelect;

	//get mouse pointer position
	QPoint clickedPos = e->pos();

	//map to row and column
	m_rowSelect = findRow( clickedPos.y() );
	m_colSelect = findCol( clickedPos.x() );

	if (m_rowSelect == -1 || m_colSelect == -1)
	{
		m_rowSelect = oldRow;
		m_colSelect = oldCol;
		//check coordinates of mouse
		//if inside combo box a mouse click will come to combo also
		//you must let it close the date table itself
		int x = e->x();
		int y = e->y();
		int x1 = DATE_COMBO_X;
		int y1 = DATE_COMBO_Y;
		if ((x >=0) && (x < x1) && (y < 0) && (y >= -y1))
		{
			fprintf(stderr,"inside combo region\n");
			m_bUpdate = true;
		}
		escKeyClicked();
		return;
	}
	//get contents
	QString s = m_strings[indexOf(m_rowSelect,m_colSelect)];

	//set current cell if allowed
	if (s[0] == '$')
	{
		//only if current cell has moved
//bug fix:mouse clicked comes also so even if same row and column allow it
//		if ( (m_rowSelect != oldRow) || (m_colSelect != oldCol) ) 
		{
			// erase previous marking
			updateCell( oldRow, oldCol );
		
			// show new current cell
			updateCell( m_rowSelect, m_colSelect );	
			//update m_date member
			updateDate( m_rowSelect, m_colSelect );

			//special for double click
			retKeyClicked();
		}
	}
	else
	{
		//change not allowed restore previous selection
		m_rowSelect = oldRow;
		m_colSelect = oldCol;
	}
}

void CDateWidget::mousePressEvent( QMouseEvent* e )
{
	fprintf(stderr,"internal mouse press event %d,%d\n",e->x(),e->y());
	//save previous cell
	int oldRow = m_rowSelect;
	int oldCol = m_colSelect;

	//get mouse pointer position
	QPoint clickedPos = e->pos();

	//map to row and column
	m_rowSelect = findRow( clickedPos.y() );
	m_colSelect = findCol( clickedPos.x() );

	if (m_rowSelect == -1 || m_colSelect == -1)
	{
		m_rowSelect = oldRow;
		m_colSelect = oldCol;
		//check coordinates of mouse
		//if inside combo box a mouse click will come to combo also
		//you must let it close the date table itself
		int x = e->x();
		int y = e->y();
		int x1 = DATE_COMBO_X;
		int y1 = DATE_COMBO_Y;
		if ((x >=0) && (x < x1) && (y < 0) && (y >= -y1))
		{
			fprintf(stderr,"inside combo region\n");
			m_bUpdate = true;
		}
		escKeyClicked();
		return;
	}
	//get contents
	QString s = m_strings[indexOf(m_rowSelect,m_colSelect)];

	//set current cell if allowed
	if (s[0] == '$')
	{
		//only if current cell has moved
		if ( (m_rowSelect != oldRow) || (m_colSelect != oldCol) ) 
		{
			// erase previous marking
			updateCell( oldRow, oldCol );
		
			// show new current cell
			updateCell( m_rowSelect, m_colSelect );	
			//update m_date member
			updateDate( m_rowSelect, m_colSelect );
		}
	}
	else
	{
		//change not allowed restore previous selection
		m_rowSelect = oldRow;
		m_colSelect = oldCol;
	}
}

void CDateWidget::keyPressEvent( QKeyEvent* e )
{
	fprintf(stderr,"internal key press event %d state %d\n",e->key(),e->state());

	//popup close widget if F4 key pressed
	if ( e->key() == Key_F4 ||
		( ( e->key() == Key_Down || e->key() == Key_Up) 
			&& ((e->state() & (AltButton | ControlButton)) == AltButton) ) )
	{
		escKeyClicked();
		return;
	}

	//same for tab - focus moves to next
	//do not eat key to be passed to control that takes tab
	if ( e->key() == Key_Tab)
	{
		escKeyClicked();
		e->ignore();
		return;
	}


	if ((e->state() & (AltButton | ControlButton)) == (AltButton | ControlButton))
	{
		switch (e->key())
		{
			case Key_Down:
				prevMonth();
				return;
			case Key_Up:
				nextMonth();
				return;
			case Key_PageUp:
				nextYear();
				return;
			case Key_PageDown:
				prevYear();
				return;
			case Key_Left:
				prevDay();
				return;
			case Key_Right:
				nextDay();
				return;
		}
	}
//	add return and esc keys here
//	if ((e->state() & (AltButton | ControlButton)) == (AltButton | ControlButton))
	{
		switch (e->key())
		{
			case Key_Return:
				retKeyClicked();
				return;
			case Key_Escape:
				escKeyClicked();
				return;
		}
	}

	//save previous cell
	int oldRow = m_rowSelect;
	int oldCol = m_colSelect;

	//last cell in view
	int edge = 0;

	switch( e->key() ) 
	{
		case Key_Left:

			if( m_colSelect > 0 ) 
			{
				m_colSelect--;

				edge = leftCell();

				if ( m_colSelect < edge )
				{
					setLeftCell( edge - 1 );
				}
			}
			break;

		case Key_Right:

			if( m_colSelect < numCols()-1 ) 
			{
				m_colSelect++;

				edge = lastColVisible();

				if ( m_colSelect >= edge )
				{
					setLeftCell( leftCell() + 1 );
				}
			}
			break;

		case Key_Up:

			if( m_rowSelect > 0 ) 
			{
				m_rowSelect--;

				edge = topCell();

				if ( m_rowSelect < edge )
				{
					setTopCell( edge - 1 );
				}
			}
			break;

		case Key_Down:

			if( m_rowSelect < numRows()-1 ) 
			{
				m_rowSelect++;

				edge = lastRowVisible();

				if ( m_rowSelect >= edge )
				{
					setTopCell( topCell() + 1 );
				}
			}
			break;
		default:
			e->ignore();

			return;	
	}

	//get contents
	QString s = m_strings[indexOf(m_rowSelect,m_colSelect)];

	//set current cell if allowed
	if (s[0] == '$')
	{
		//only if current cell has moved
		if ( (m_rowSelect != oldRow) || (m_colSelect != oldCol) ) 
		{
			// erase previous marking
			updateCell( oldRow, oldCol );
		
			// show new current cell
			updateCell( m_rowSelect, m_colSelect );	
			//update m_date member
			updateDate( m_rowSelect, m_colSelect );
		}
	}
	else
	{
		//change not allowed restore previous selection
		m_rowSelect = oldRow;
		m_colSelect = oldCol;
	}
}

void CDateWidget::nextMonth()
{
	QDate d1 = nextMonthDate(getDate());

	setDate(d1);
}

void CDateWidget::prevMonth()
{
	QDate d1 = prevMonthDate(getDate());

	setDate(d1);
}

QDate CDateWidget::prevMonthDate(QDate d)
{
	QDate d1 = d;
	//if first month goto previous year 12th month
	if (d1.month() == 1)
	{
		d1.setYMD(d1.year()-1,12,d1.day());
	}
	else
	{
		d1.setYMD(d1.year(),d1.month()-1,d1.day());
	}

	return d1;
}

QDate CDateWidget::nextMonthDate(QDate d)
{
	QDate d1 = d;
	//if last month goto next year first month
	if (d1.month() == 12)
	{
		d1.setYMD(d1.year()+1,1,d1.day());
	}
	else
	{
		d1.setYMD(d1.year(),d1.month()+1,d1.day());
	}

	return d1;
}

void CDateWidget::nextYear()
{
	QDate d1 = nextYearDate(getDate());

	setDate(d1);
}

void CDateWidget::prevYear()
{
	QDate d1 = prevYearDate(getDate());

	setDate(d1);
}

QDate CDateWidget::nextYearDate(QDate d)
{
	QDate d1 = d;

	d1.setYMD(d1.year()+1,d1.month(),d1.day());

	return d1;
}

QDate CDateWidget::prevYearDate(QDate d)
{
	QDate d1 = d;

	d1.setYMD(d1.year()-1,d1.month(),d1.day());

	return d1;
}

void CDateWidget::nextDay()
{
	QDate d1 = nextDayDate(getDate());

	setDate(d1);
}

void CDateWidget::prevDay()
{
	QDate d1 = prevDayDate(getDate());

	setDate(d1);
}

QDate CDateWidget::nextDayDate(QDate d)
{
	QDate d1 = d;

	if (d1.day() == d1.daysInMonth())
	{
		//now check if we are in last month on year on last day
		if (d1.month() == 12)
		{
			d1.setYMD(d1.year()+1,1,1);
		}
		else
		{
			d1.setYMD(d1.year(),d1.month()+1,1);
		}
	}
	else
	{
		d1.setYMD(d1.year(),d1.month(),d1.day()+1);
	}

	return d1;
}

QDate CDateWidget::prevDayDate(QDate d)
{
	QDate d1 = d;

	if (d1.day() == 1)
	{
		//now check if we are in first day of first month
		if (d1.month() == 1)
		{
			d1.setYMD(d1.year()-1,12,1);
			//set right day
			d1.setYMD(d1.year(),d1.month(),d1.daysInMonth());
		}
		else
		{
			d1.setYMD(d1.year(),d1.month()-1,1);
			//set right day
			d1.setYMD(d1.year(),d1.month(),d1.daysInMonth());
		}
	}
	else
	{
		d1.setYMD(d1.year(),d1.month(),d1.day()-1);
	}

	return d1;
}

void CDateWidget::focusInEvent( QFocusEvent* e )
{
	//generates 2 paintCell events - why?
	updateCell( m_rowSelect, m_colSelect );
}    

void CDateWidget::focusOutEvent( QFocusEvent* e )
{
	//generates 1 paintCell event - ok
	updateCell( m_rowSelect, m_colSelect );		
}    

int CDateWidget::indexOf( int row, int col ) const
{
	return (row * numCols()) + col;
}

void CDateWidget::retKeyClicked()
{
	//save system date also here
	QString s;
	QTime t = QTime::currentTime();
	s.sprintf(IDS_COMMAND_FORMAT,IDS_DATE_DIR,IDS_DATE_COMMAND,
		m_date.month(),m_date.day(),
		t.hour(),t.minute(),
		m_date.year(),t.second());

	int nRet = system(s.data());

	if ( nRet != 0 )
	{
		QMessageBox::warning(0,tr(IDS_DATE_ERROR1),tr(IDS_DATE_ERROR));
	}
	else
	{
		fprintf(stderr,"ret key clicked\n");
		if (m_bHide)
		{
			hide();
		}
	}
}

void CDateWidget::escKeyClicked()
{
	fprintf(stderr,"esc key clicked\n");
	if (m_bHide)
	{
		hide();
	}
}

void CDateWidget::updateCell( int row, int col, bool erase )
{
	fprintf(stderr,"update cell %d,%d\n",col,row);

	QTableView::updateCell( row, col, erase );
}

void CDateWidget::updateDate( int row, int col)
{
	//set m_date member based on row and column
	QDate d1;
	d1.setYMD(m_date.year(),m_date.month(),1);

	//first row reserved for days of week
	int j = 1;
	//starting row is day of week - 1 based
	int i = d1.dayOfWeek()-1;
	//how many we have to display
	int k = m_date.daysInMonth();
	//current day of month - 1 based
	int l = 1;

	bool bFound = false;

	while (k > 0)
	{
		//set current day selection
		if ((row == j) && (col == i))
		{
			bFound = true;
			break;
		}
		i++;
		if (i == numRows())
		{
			j++;
			i = 0;
		}
		l++;
		k--;
	}
	ASSERT(bFound);

	//l variable is new day in this month
	m_date.setYMD(m_date.year(),m_date.month(),l);

	emit dateChanged(m_date);
}
