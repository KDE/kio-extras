/* Name: ipedit.h

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


#ifndef IPEdit_included
#define IPEdit_included

#include <qframe.h>
#include <qlined.h>
#include <qlabel.h>
//#include "netidpaneldata.h"
#include <qlined.h>

// this class is here to allow for highlighting an edit box
// when focus is changed to it
class NetCLineEdit : public QLineEdit
{
	Q_OBJECT
public:
    NetCLineEdit (QWidget *parent, const char *name)
      : QLineEdit(parent, name) {}

protected:
	void focusInEvent (QFocusEvent*);
	void mousePressEvent (QMouseEvent*);
};


#define CURSOR_FORWARD	0x01
#define CURSOR_BACKWARD	0x02

class OctetEdit : public NetCLineEdit
{
    Q_OBJECT

public:

		OctetEdit
		(
		 QWidget* parent = NULL,
		 const char* name = NULL
		);

    virtual ~OctetEdit() {};

		void setFlags(char flags) { m_flags = flags; };

		bool isEmpty()						{ return (text().length()==0); }

public slots:


protected slots:

signals:
	void home();
	void end();
	void next( bool bSetSelect );
	void prev( bool bDelete );

protected:
	void keyPressEvent ( QKeyEvent * e );
	
	char m_flags;
};




class IPEdit : public QFrame
{
    Q_OBJECT

public:

	IPEdit
	(
		QWidget* parent = NULL,
		const char* name = NULL
	);

	virtual ~IPEdit();

	void setFont( const QFont& font );
	void setFocusPolicy ( FocusPolicy policy );

	void setText(const QString& text);
	QString getText();
	bool isEmpty() { return (getText().length() == 0); };
protected:
	void setupLabel(QLabel *label);
	void fixNormalColorGroup(QWidget *pWidget);
	void fixDisabledColorGroup(QWidget *pWidget);
	void paintEvent(QPaintEvent *event);
	
public slots:
	void moveHome( );
	void moveEnd( );
	void moveForward( bool bSetSelect );
	void moveBackward( bool bDelete );
	void setEnabled ( bool enable );
	
protected slots:

	void handleTextChange(const QString& text);        

signals:

	void textChanged(const QString& text);
	
protected:
	OctetEdit* m_Edit1;
	OctetEdit* m_Edit2;
	OctetEdit* m_Edit3;
	OctetEdit* m_Edit4;

	QLabel* m_Label1;
	QLabel* m_Label2;
	QLabel* m_Label3;

	QString m_strBuf;
	bool m_bChanged;
};

#endif // IPEdit_included
