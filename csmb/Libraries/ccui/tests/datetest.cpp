//main.cpp

#include <stdio.h>
#include <qapplication.h>
#include <qpushbutton.h>
#include "common.h"
#include "cdate.h"
#include "datetest.h"

QMyWidgetView::QMyWidgetView( QWidget* parent , const char* name )
	: QWidget(parent,name)
{
	int x1 = 640;
	int y1 = 480;

	resize(x1,y1);

	setMinimumSize(x1,y1);
	setMaximumSize(x1,y1);

	m_pb1 = new QPushButton(this,"button 1");
	m_pb1->setGeometry(10,10,50,20);
	m_pb1->setText("++");
	connect(m_pb1,SIGNAL(clicked()),this,SLOT(onButton1()));

	m_pb2 = new QPushButton(this,"button 2");
	m_pb2->setGeometry(70,10,50,20);
	m_pb2->setText("--");
	connect(m_pb2,SIGNAL(clicked()),this,SLOT(onButton2()));

	m_pdc = new CDateComboBox(QDate(2001,2,2),this,"date combo");
//	m_pdc->resize(400,300);
	m_pdc->move(10,40);
	CDateWidget* pdw = m_pdc->getDateWidget();
	connect(pdw,SIGNAL(dateChanged(QDate)),this,SLOT(myDateChange1(QDate)));

	m_pdw = new CDateWidget(this,"date widget");
//	m_pdw->resize(400,300);
	m_pdw->move(250,40);
	m_pdw->setDoubleClickHide(false);
	connect(m_pdw,SIGNAL(dateChanged(QDate)),this,SLOT(myDateChange2(QDate)));
}

void QMyWidgetView::onButton1()
{
	fprintf(stderr,"button1\n");
	CDateWidget* pdw = m_pdc->getDateWidget();
	QDate d = pdw->getDate();
	pdw->setDate(d.addDays(+5));
}

void QMyWidgetView::onButton2()
{
	fprintf(stderr,"button2\n");
	CDateWidget* pdw = m_pdc->getDateWidget();
	QDate d = pdw->getDate();
	pdw->setDate(d.addDays(-5));
}

void QMyWidgetView::myDateChange1(QDate d)
{
	fprintf(stderr,"date(1) changed to %d/%d/%d\n",d.day(),d.month(),d.year());
}

void QMyWidgetView::myDateChange2(QDate d)
{
	fprintf(stderr,"date(2) changed to %d/%d/%d\n",d.day(),d.month(),d.year());
}

#include "datetest.moc"

int main( int argc, char **argv )
{
	QApplication a(argc,argv);
#ifdef QT_20	
	a.setStyle(new QWindowsStyle);
#else
	a.setStyle(WindowsStyle);
#endif	
	QMyWidgetView w;

	a.setMainWidget( &w );

	w.show();
	return a.exec();
}
