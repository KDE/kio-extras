//ipedittest.h
#ifndef _IPEDIT_TEST_
#define _IPEDIT_TEST_

#include "qwidget.h"

class IPEdit;
class QLineEdit;
class QPushButton;

class QMyWidgetView : public QWidget
{
	Q_OBJECT
public:
	QMyWidgetView( QWidget* parent = 0, const char* name = 0 );
private slots:
	void IPAddrChanged(const QString& text);

	void clickedButton();
private:
	IPEdit* m_pAddr;
	QLineEdit* m_pEdit;
	QPushButton* m_pButton;
};

#endif//_IPEDIT_TEST_
