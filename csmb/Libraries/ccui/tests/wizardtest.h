//wizardtest.h
#ifndef _WIZARD_TEST_
#define _WIZARD_TEST_

#include <wizardpage.h>

class QLineEdit;
class QLabel;
class QString;

class MyWizard : public CCorelWizard
{
    Q_OBJECT

public:
  MyWizard(int nButton1=eButtonPrev, int nButton2=eButtonNext,
	       int nButton3=eButtonCancel, int nButton4=eButtonDone,
	       int nButton5=eButtonHelp, bool bSpacing=true,
	       QWidget *parent=0, const char *name=0, bool bModal=false);

protected:
    // override checkList() to handle user pressing Done or Cancel
    virtual bool checkLast( int r );
};

class MyWizardPage : public CWizardPage
{
    Q_OBJECT

public:
    MyWizardPage( int nPage, bool bFinalPage = false, int nNextPage = -1,
        int nPrevPage = -1, const char* pszImage = 0,
        const char* pszTitle = 0, bool bCheckPage = true,
		QWidget *parent=0, const char *name=0 );

public slots:
    void slotPrevUpdated( QString );
    void slotNextUpdated( QString );

signals:
    void signalUpdated( QString );

protected:
    // override checkPage() to handle user pressing Prev or Next
    virtual bool checkPage( int /*nButton*/ );

private:
    QLineEdit * m_pLineEdit;
    QLabel *    m_pNextLabel;
    QLabel *    m_pPrevLabel;
};

#endif//_WIZARD_TEST_
