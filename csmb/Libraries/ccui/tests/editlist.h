//editlist.h
#ifndef _EDITLIST_TEST_
#define _EDITLIST_TEST_

#include "qwidget.h"

class CListViewEdit;
class QLabel;
class QListViwItem;
class QPushButton;

class QMyWidgetView : public QWidget
{
	Q_OBJECT
public:
	QMyWidgetView( QWidget* parent = 0, const char* name = 0 );

	QListViewItem * apcListViewItem[14];
	QWidget* apcEntryType[2];
	
	static bool predicate(QObject * a_cOwner, QListViewItem * a_pcListViewItem, int a_iColumn);
	static bool validator(QObject * a_cOwner, QListViewItem * a_pcListViewItem, int a_iColumn);
	static QWidget * entryType(QObject *, QListViewItem *, int);
public slots:
	void clickedButton();
private:
	CListViewEdit* m_pList;
	QLabel* m_pLabel;
	QPushButton* m_pButton;
};

#endif//_EDITLIST_TEST_
