//listboxtest.h
#ifndef _LISTBOX_TEST_
#define _LISTBOX_TEST_

#include <qsplitter.h>

class QListBox;
class QListBoxItem;
class QLabel;

class MainWin : public QSplitter
{
    Q_OBJECT
public:
    MainWin( QWidget * parent = 0, const char * name = 0 );

private slots:
    void slotSelected( int );
    void slotSelectionChanged( QListBoxItem * );
 
private:
    QListBox	* m_pListBox;
    QLabel    	* m_pLabel;
};

#endif//_LISTBOX_TEST_
