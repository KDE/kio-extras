//datetest.h
#ifndef _DATE_TEST_
#define _DATE_TEST_

#include "qwidget.h"

class CDateComoBox;
class QPushButton;
class CDateWidget;

class QMyWidgetView : public QWidget
{
	Q_OBJECT
public:
	QMyWidgetView( QWidget* parent = 0, const char* name = 0 );
private slots:
	void onButton1();
	void onButton2();
	void myDateChange1(QDate d);
	void myDateChange2(QDate d);
private:
	CDateComboBox* m_pdc;
	QPushButton* m_pb1;
	QPushButton* m_pb2;
	CDateWidget* m_pdw;
};

#endif//_DATE_TEST_
