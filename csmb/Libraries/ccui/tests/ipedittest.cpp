//ipedittest.cpp

#include "ipedittest.h"
#include "ipedit.h"

#include <stdio.h>
#include <qapplication.h>
#include <qpushbutton.h>
#include <qwindowsstyle.h>
#include <qlineedit.h>

QMyWidgetView::QMyWidgetView( QWidget* parent, const char* name )
	: QWidget(parent,name)
{
	int x1 = 170;
	int y1 = 100;

	resize(x1,y1);

	setMinimumSize(x1,y1);
	setMaximumSize(x1,y1);

	m_pAddr = new IPEdit(this,"IP Addr");
	m_pAddr->setGeometry(10,10,145,25);

	connect(m_pAddr,SIGNAL(textChanged(const QString&)),this,SLOT(IPAddrChanged(const QString&)));

	m_pEdit = new QLineEdit(this,"IP edit");
	m_pEdit->setGeometry(10,40,145,25);

	m_pButton = new QPushButton("Set IP Address",this,"IP button");
	m_pButton->setGeometry(10,70,145,25);

	connect(m_pButton,SIGNAL(clicked()),this,SLOT(clickedButton()));
}

void QMyWidgetView::IPAddrChanged(const QString& text)
{
	fprintf(stderr,"text changed to %s\n",text.latin1());
	m_pEdit->setText(text);
}

void QMyWidgetView::clickedButton()
{
	QString s = m_pEdit->text();
	fprintf(stderr,"setting text to %s\n",s.latin1());
	m_pAddr->setText(s);
}

#include "ipedittest.moc"

int main( int argc, char **argv )
{
	QApplication a(argc,argv);
	a.setStyle(new QWindowsStyle);

	QMyWidgetView w;

	a.setMainWidget( &w );

	w.show();
	return a.exec();
}
