//ipedittest.cpp

#include "clistviewedit.h"
#include "editlist.h"
#include <stdio.h>
#include <qapplication.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qwindowsstyle.h>
#include <qvalidator.h>

QMyWidgetView::QMyWidgetView( QWidget* parent, const char* name )
	: QWidget(parent,name)
{
	int x1 = 400;
	int y1 = 300;

	resize(x1,y1);

	setMinimumSize(x1,y1);
	setMaximumSize(x1,y1);

	m_pList = new CListViewEdit(this, "edit list");

      apcEntryType[0] = new class CListViewEdit::Entry<QLineEdit>(& QLineEdit::text, & QLineEdit::setText, m_pList, "apcEntryType[0]");
      dynamic_cast<QLineEdit *>(apcEntryType[0])->setValidator(new QDoubleValidator(apcEntryType[0], "no name"));
      
      m_pList->pcOwner = this;
      m_pList->pEntryType = & QMyWidgetView::entryType;
      m_pList->pValidator = & QMyWidgetView::validator;
      m_pList->pPredicate = & QMyWidgetView::predicate;

      m_pList->addColumn("Col1");
      m_pList->addColumn("Col2");
      m_pList->addColumn("Col3");
      m_pList->addColumn("Col4");
      m_pList->addColumn("Col5");

	m_pList->setColumnWidth(0,140);
	m_pList->setColumnWidth(1,100);
	m_pList->setColumnWidth(2,80);
	m_pList->setColumnWidth(3,60);
	m_pList->setColumnWidth(4,40);

      apcListViewItem[0] = new QListViewItem(m_pList, "Root 1");
      apcListViewItem[0]->setText(1,"a");
      apcListViewItem[0]->setText(2,"b");
      apcListViewItem[0]->setText(3,"c");
      apcListViewItem[0]->setText(4,"d");

      apcListViewItem[1] = new QListViewItem(apcListViewItem[0], "Child 1");
      apcListViewItem[1]->setText(1,"i");
      apcListViewItem[1]->setText(2,"j");
      apcListViewItem[1]->setText(3,"k");
      apcListViewItem[1]->setText(4,"l");

      apcListViewItem[2] = new QListViewItem(apcListViewItem[0], "Child 2");
      apcListViewItem[2]->setText(1,"g");
      apcListViewItem[2]->setText(2,"h");
      apcListViewItem[2]->setText(3,"g");
      apcListViewItem[2]->setText(4,"h");

      apcListViewItem[3] = new QListViewItem(m_pList, "Root 2");
      apcListViewItem[3]->setText(1,"e");
      apcListViewItem[3]->setText(2,"f");
      apcListViewItem[3]->setText(3,"g");
      apcListViewItem[3]->setText(4,"h");

      apcListViewItem[4] = new QListViewItem(apcListViewItem[3], "Child 3");
      apcListViewItem[4]->setText(1,"m");
      apcListViewItem[4]->setText(2,"n");
      apcListViewItem[4]->setText(3,"o");
      apcListViewItem[4]->setText(4,"p");

      apcListViewItem[5] = new QListViewItem(apcListViewItem[3], "Child 4");
      apcListViewItem[5]->setText(1,"q");
      apcListViewItem[5]->setText(2,"r");
      apcListViewItem[5]->setText(3,"s");
      apcListViewItem[5]->setText(4,"t");

	m_pList->setGeometry(10,10,380,200);

	m_pLabel = new QLabel(this,"label");
	m_pLabel->setText("Editable list view - push button to enter edit mode:");
	m_pLabel->setGeometry(10,220,380,30);

	m_pButton = new QPushButton(this,"button");
	m_pButton->setText("Enter edit mode");
	m_pButton->setGeometry(10,260,380,30);
	connect(m_pButton,SIGNAL(clicked()),this,SLOT(clickedButton()));
}

void QMyWidgetView::clickedButton()
{
	printf("clicked button\n");
/*
	QListViewItem* p = m_pList->selectedItem();
	if (p)
	{
		bool bFound = false;
		int k = -1;
		for (int j = 0; j < m_pList->columns(); j++)
		{
			bFound = predicate(this,p,j)
			if (bFound)
			{
				k = j;
				break;
			}
		}

		if (bFound)
			m_pList->cellSelected(p,QPoint(0,0),k);
	}
*/
}

    bool QMyWidgetView::validator(QObject * a_cOwner, QListViewItem * a_pcListViewItem, int a_iColumn)
    {
      QMyWidgetView* pWidget = dynamic_cast<QMyWidgetView *>(a_cOwner);
      QLineEdit * pLineEdit = dynamic_cast<QLineEdit *>(pWidget->apcEntryType[0]);
      
      QString cText = pLineEdit->text();
      int iCursorPosition = pLineEdit->cursorPosition();
      return pLineEdit->validator()->validate(cText, iCursorPosition) == QValidator::Acceptable;
    }
    
    bool QMyWidgetView::predicate(QObject * a_cOwner, QListViewItem * a_pcListViewItem, int a_iColumn)
    {
      QMyWidgetView* pWidget = (QMyWidgetView*) (a_cOwner);
	fprintf(stdout,"predicate\n");
      if (a_pcListViewItem == pWidget->apcListViewItem[1] || 
        a_pcListViewItem == pWidget->apcListViewItem[2])
      {
        switch (a_iColumn)
        {
        case 1:
        case 3:
        case 4:
	   return true;
        }
      }
      else if (a_pcListViewItem == pWidget->apcListViewItem[0] || 
        a_pcListViewItem == pWidget->apcListViewItem[3])
      {
        switch (a_iColumn)
        {
        case 0:
        case 2:
	   return true;
        }
      }
      else if (a_pcListViewItem == pWidget->apcListViewItem[4] || 
        a_pcListViewItem == pWidget->apcListViewItem[5])
      {
        switch (a_iColumn)
        {
        case 0:
        case 1:
        case 2:
        case 4:
	   return true;
        }
      }
      return false;
    }

    QWidget * QMyWidgetView::entryType(QObject * a_cOwner, QListViewItem * a_pcListViewItem, int a_iColumn)
    {
      QMyWidgetView* pWidget = (QMyWidgetView*) (a_cOwner);

	fprintf(stdout,"entry type\n");
      return pWidget->apcEntryType[0];
    }
#include "editlist.moc"

int main( int argc, char **argv )
{
	QApplication a(argc,argv);
	a.setStyle(new QWindowsStyle);

	QMyWidgetView w;

	a.setMainWidget( &w );

	w.show();
	return a.exec();
}

