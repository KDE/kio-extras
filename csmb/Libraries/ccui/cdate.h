/* Name: cdate.h

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


#ifndef _CDATE_H_
#define _CDATE_H_

#include <qdatetime.h>
#include <qcombobox.h>
#include <qtableview.h>

#define DATE_COMBO_X	150
#define DATE_COMBO_Y	25

class CDateWidget;

class CDateComboBox : public QComboBox
{
	Q_OBJECT
public:
	CDateComboBox( QWidget* parent=0, const char* name=0 );
	CDateComboBox( QDate d, QWidget* parent=0, const char* name=0 );

	CDateWidget* getDateWidget() { return m_dateWidget;};
protected:
	void mousePressEvent( QMouseEvent* e );
	void keyPressEvent( QKeyEvent* e );
protected slots:
	void dateChange(QDate d);
private:
	void initWidget(QDate d);

	void popup();

	bool m_bPopup;

	CDateWidget* m_dateWidget;
};

#define DELTA_X	5
#define DELTA_Y	5

#define CELL_WIDTH	30
#define CELL_HEIGHT	30

#define X_MAXCOLS	7
#define Y_MAXROWS	7

#define SIZE_X	CELL_WIDTH*X_MAXCOLS
#define SIZE_Y	CELL_HEIGHT*Y_MAXROWS

class CDateWidget : public QTableView
{
	Q_OBJECT
public:
	CDateWidget( QDate d, QWidget* parent=0, const char* name=0 , WFlags f = 0 );
	CDateWidget( QWidget* parent=0, const char* name=0, WFlags f = 0 );

	~CDateWidget();

	void setDoubleClickHide(bool bHide) {m_bHide = bHide;};
	bool getDoubleClickHide() {return m_bHide;};

	void setDate(QDate d);
	QDate getDate();

	static QString dateString(QDate d);
public slots:
	void nextMonth();
	void prevMonth();

	void nextYear();
	void prevYear();

	void nextDay();
	void prevDay();
protected slots:
	void retKeyClicked();
	void escKeyClicked();
signals:
	void dateChanged(QDate d);
protected:
	friend class CDateComboBox;

	void setUpdate(bool bUpdate) {m_bUpdate = bUpdate;};
	bool getUpdate() {return m_bUpdate;};

	void updateDate( int row, int col );
	void updateCell( int row, int col, bool erase=TRUE );

	void paintCell( QPainter* p, int row, int col );

	void mouseDoubleClickEvent( QMouseEvent* e );
	void mousePressEvent( QMouseEvent* e );
	void keyPressEvent( QKeyEvent* e );

	void focusInEvent( QFocusEvent* e );
	void focusOutEvent( QFocusEvent* e );    
private:
	void initWidget(QDate d);

	QDate nextMonthDate(QDate d);
	QDate prevMonthDate(QDate d);

	QDate nextYearDate(QDate d);
	QDate prevYearDate(QDate d);

	QDate nextDayDate(QDate d);
	QDate prevDayDate(QDate d);

    	QString cellContent( int row, int col ) const;
	void setCellContent( int row, int col, QString s, bool update=FALSE );

	int indexOf( int row, int col ) const;

	QString* m_strings;

	int m_rowSelect;
	int m_colSelect;

	QDate m_date;

	bool m_bUpdate;
	bool m_bHide;
};

#endif // _CDATE_H_
