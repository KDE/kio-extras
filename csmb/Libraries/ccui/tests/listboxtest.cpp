#include <qapplication.h>
#include <qlistbox.h>
#include <qlabel.h>
#include <qstring.h>

#include <corellistboxitem.h>

#include "listboxtest.h"
#include "listboxtest.moc"

MainWin::MainWin( QWidget * parent, const char * name ) :
    QSplitter( Vertical, parent, name )
{
    m_pListBox = new QListBox( this );
	// Make QListBox::item() public in Qt 1
    m_pLabel = new QLabel( this );

// In Qt 1, the signal is emitted only on a double-click
#ifdef QT_2
    connect( m_pListBox, SIGNAL(selectionChanged(QListBoxItem *)),
        this, SLOT(slotSelectionChanged(QListBoxItem *)) );
#else
    connect( m_pListBox, SIGNAL(selected(int)),
    	this, SLOT(slotSelected(int)) );
#endif

    m_pListBox->insertItem( new CListBoxItem( "Item 1", QPixmap("default.xpm"),
        new QString("sample string 1") ) );
    m_pListBox->insertItem( new CListBoxItem( "Item 2", QPixmap(),
        new QString("sample string 2") ) );
    m_pListBox->insertItem( new CListBoxItem( "Item 3", QPixmap("default.xpm"),
        0 ) );
}

// only required for Qt 1
void MainWin::slotSelected( int n )
{
    slotSelectionChanged( m_pListBox->item( n ) );
}

void MainWin::slotSelectionChanged( QListBoxItem *item )
{
    QString s( "Item \"" );
    s += item->text();
    s += "\", has ";

    QString * pUserData = (QString *) ((CListBoxItem *) item)->GetUserData();
    if (pUserData)
    {
        s += "user data \"";
        s += *pUserData;
        s += "\"";
    }
    else
        s += "no user data";

    m_pLabel->setText( s );
}

int main( int argc, char **argv )
{
    QApplication a( argc, argv );

    // Create the wizard
    MainWin w;

    // run the app
    a.setMainWidget( &w );
    w.show();
    return a.exec();
}

