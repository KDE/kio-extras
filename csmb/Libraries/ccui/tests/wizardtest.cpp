#include <iostream>

#include <kapp.h>
#include <klocale.h>

#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qstring.h>

#include <wizard.h>

#include "wizardtest.h"
#include "wizardtest.moc"

#include "ccui_common.h"
#ifdef QT_2
#include <kcmdlineargs.h>
#endif

const int NUM_PAGES = 5;

MyWizard::MyWizard( int nButton1, int nButton2, int nButton3, int nButton4,
        int nButton5, bool bSpacing, QWidget *parent, const char *name,
        bool bModal ) :
    CCorelWizard( nButton1, nButton2, nButton3, nButton4, nButton5, bSpacing,
        parent, name, bModal )
{
}

bool MyWizard::checkLast( int r )
{
    if (r == Accepted)
        cout << "User clicked Done" << endl;
    else
        cout << "User clicked Cancel" << endl;

    return true;
}

MyWizardPage::MyWizardPage( int nPage, bool bFinalPage, int nNextPage,
        int nPrevPage, const char* pszImage, const char* pszTitle,
        bool bCheckPage, QWidget *parent, const char *name ) :
    CWizardPage( nPage, bFinalPage, nNextPage, nPrevPage, pszImage,
        pszTitle, bCheckPage, parent, name )
{
    QVBoxLayout * pLayout = new QVBoxLayout( this );

    pLayout->addWidget( new QLabel( "Enter the tag for this page:", this ) );

    m_pLineEdit = new QLineEdit( this );
    pLayout->addWidget( m_pLineEdit );

    if ( nPage == 0 )
        m_pPrevLabel = new QLabel( "There is no previous page.", this );
    else
        m_pPrevLabel = new QLabel( "Previous page is unset.", this );
    pLayout->addWidget( m_pPrevLabel );

    if ( nPage == NUM_PAGES-1 )
        m_pNextLabel = new QLabel( "There is no next page.", this );
    else
        m_pNextLabel = new QLabel( "Next page is unset.", this );
    pLayout->addWidget( m_pNextLabel );
}

bool MyWizardPage::checkPage( int /*nButton*/ )
{
    // we could do validation here or simply use a signal to tell others
    // that this page has changed
    emit signalUpdated( m_pLineEdit->text() );
    return true;
}

void MyWizardPage::slotNextUpdated( QString s )
{
    if (s.isEmpty())
        m_pNextLabel->setText( "Next page is unset." );
    else
        m_pNextLabel->setText( QString("Next page is \"") + s + QString("\"") );
}

void MyWizardPage::slotPrevUpdated( QString s )
{
    if (s.isEmpty())
        m_pPrevLabel->setText( "Previous page is unset." );
    else
        m_pPrevLabel->setText( QString("Previous page is \"") + s + QString("\"") );
}

int main( int argc, char **argv )
{
#ifdef QT_2
    KCmdLineArgs::init( argc, argv, "wizardtest", "CCorelWizard Test", "0.1" );
    KApplication::addCmdLineOptions();
    KApplication a;
#else
    KApplication a( argc, argv );
#endif

    // Create the wizard
    MyWizard *pWizard = new MyWizard(
        eButtonPrev, eButtonNext, eButtonCancel, eButtonDone, eButtonHelp,
        /* spacing? */  true,   // if true, buttons not shown leave gaps
                                // if false, leave no spaces
        /* parent */    0,
        /* name */      "test wizard",
        /* modal? */    false   // make this true to run with wizard->exec()
                                // or false to display with wizard->show()
    );
    pWizard->setDoneReplacesNextOnFinalPage( false );
    pWizard->setDefaultButton( eButtonNext );

    // Create the pages
    MyWizardPage * pPages[NUM_PAGES];
    int i;
    for ( i = 0; i < NUM_PAGES; i++ )
    {
        QString strTitle( "Page #" + (new QString)->setNum(i) );

        pPages[i] = new MyWizardPage(
            /* page # */        i,
            /* final page? */   (i == NUM_PAGES-1),
            /* next page */     -1,
            /* prev page */     -1,
            /* image */         "default.xpm",
            /* title */         strTitle,
            /* check page? */   true,   // clicking next is allowed by default
                                        // override checkPage() to alter
            /* parent */        pWizard
        );

        // show all buttons but Help on every page
        int nButtonShowMask = eButtonNext | eButtonPrev | eButtonCancel |
            eButtonDone;

        // on first page, enable Next and Cancel
        // on last page, enable Prev, Cancel and Done
        // on all others, enable Next, Prev and Cancel
        int nButtonEnableMask = eButtonCancel;
        if (i > 0) nButtonEnableMask |= eButtonPrev;
        if (i < NUM_PAGES-1)
            nButtonEnableMask |= eButtonNext;
        else
            nButtonEnableMask |= eButtonDone;

        pWizard->addPage(
            /* pointer to page */   pPages[i],
            /* enabled buttons */   nButtonEnableMask,
            /* buttons to show */   nButtonShowMask,
            /* first page shown? */ (i == 0)
        );
    }

    // have each page display the data from the pages on either side
    for ( i = 0; i < NUM_PAGES; i++ )
    {
        if (i > 0)
            QObject::connect( pPages[i-1], SIGNAL(signalUpdated(QString)),
                pPages[i], SLOT(slotPrevUpdated(QString)) );
        if (i < NUM_PAGES-1)
            QObject::connect( pPages[i+1], SIGNAL(signalUpdated(QString)),
                pPages[i], SLOT(slotNextUpdated(QString)) );
    }

    // run the app
    a.setMainWidget( pWizard );
    pWizard->show();
    return a.exec();
}

