/* Name: rename.cpp
            
    Description: This file is a part of the printutil shared library.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.


*/

/*
 * NOTE: This source file was created using tab size = 2.
 * Please respect that setting in case of modifications.
 */

#include <kapp.h>
#include <klocale.h>
#include <stdlib.h>
#include <stdio.h>
#include <kbuttonbox.h>
//#include <kmsgbox.h>
#include "rename.h"
#include "commonfunc.h"
#include <qlabel.h>

//
// Crenamedlg class
//
//****************************************************************************
//
// Method: Crenamedlg( QWidget* parent, const char* name, const QString &)
//
// Purpose: constructor
//
//****************************************************************************
Crenamedlg::Crenamedlg(QWidget* parent, const char* name,
			const QString oldName)
   :QDialog(parent, name, TRUE)
{
fprintf(stderr, "\n\nRename a printer, old name = %s", (const char*)oldName
#ifdef QT_20
  .latin1()
#endif
                                                                              );
  setCaption(i18n("Printer Rename"));

  layout = new QVBoxLayout(this, 10);
  QGridLayout *grid = new QGridLayout(2, 2);
  layout->addLayout(grid);

  QLabel* lb1 = new QLabel(this, "lb1");
  lb1->setText(i18n("Old Name: "));
  lb1->setMinimumSize(lb1->sizeHint());
  lb1->setAlignment(AlignRight|AlignVCenter);
  grid->addWidget(lb1, 0, 0, AlignRight);

  leusername1 = new QLineEdit( this, "LineEdit_1" );

  // ensure it fits at least 12 characters
  leusername1->setText( "XXXXXXXXXXXX" );
  leusername1->setMinimumSize( leusername1->sizeHint() );

  // clear text
  leusername1->setText((const char*)oldName
#ifdef QT_20
  .latin1()
#endif
                                            );

  leusername1->setEchoMode( QLineEdit::Normal );
  grid->addWidget(leusername1, 0, 1);

  QLabel* lb2 = new QLabel(this, "lb2");
  lb2->setText(i18n("New Name: "));
  lb2->setMinimumSize(lb2->sizeHint());
  lb2->setAlignment(AlignRight|AlignVCenter);
  grid->addWidget(lb2, 1, 0, AlignRight);

  leusername2 = new QLineEdit( this, "LineEdit_2" );

  // ensure it fits at least 12 characters
  leusername2->setText( "XXXXXXXXXXXX" );
  leusername2->setMinimumSize( leusername2->sizeHint() );

  // clear text
  leusername2->setText( "" );
  leusername2->setFocus();
  leusername2->setEchoMode( QLineEdit::Normal );
  grid->addWidget(leusername2, 1, 1);

  // add a button box
  KButtonBox *bbox = new KButtonBox(this);

  // make buttons right aligned
  bbox->addStretch(1);

  // the default buttons
  pbOk = bbox->addButton(i18n("Ok"));
  pbCancel = bbox->addButton(i18n("Cancel"));
  pbOk->setDefault(true);

  // establish callbacks
  QObject::connect(pbOk, SIGNAL(clicked()), this, SLOT(ok()));
  QObject::connect(pbCancel, SIGNAL(clicked()), this, SLOT(cancel()));
  bbox->setMinimumSize(bbox->sizeHint());

  layout->addWidget(bbox);
  layout->freeze();

}

//****************************************************************************
//
// Method: ~Crenamedlg()
//
// Purpose: distructor
//
//****************************************************************************
Crenamedlg::~Crenamedlg()
{
  delete leusername1;
  delete leusername2;
  delete pbOk;
  delete pbCancel;
  delete layout;
}

//****************************************************************************
//
// Method:  ok()
//
// Purpose: when ok button clicked, the new name will be checked and will
//          replace the old  name
//
//****************************************************************************
void Crenamedlg::ok()
{
  QString tmp = leusername2->text();
	QString oldName = leusername1->text();
  if(varifyNickName(tmp))
  {
		fprintf(stderr, "\n new name ok");
		rename(oldName, tmp);
		accept();
	}
	else
	{
	 	return ;
	}
}

//****************************************************************************
//
// Method: cancel()
//
// Purpose: when cancel button being clicked, the dialog will be close and the
//				  printer name will not be changed
//
//****************************************************************************
void Crenamedlg::cancel()
{
  reject();
}

//****************************************************************************
//
// Method: rename(const QString & oldName, const QString & newName)
//
// Purpose: when ok button being clicked, the dialog will be close and the
//				  printer name will be changed
//
//****************************************************************************
void Crenamedlg::rename(const QString & oldName, const QString & newName)
{
fprintf(stderr, "\noldName = %s, newName = %s",
									(const char*)oldName
#ifdef QT_20
  .latin1()
#endif
                                        , (const char*)newName
#ifdef QT_20
  .latin1()
#endif
                                                                    );
	Aps_PrinterHandle printerHandle;
	Aps_Result result;
	if ((result = Aps_OpenPrinter((const char*)oldName
#ifdef QT_20
  .latin1()
#endif
                                                    , &printerHandle))
															== APS_SUCCESS)
	{
		 if ((result = Aps_PrinterRename(printerHandle, (const char*)newName
#ifdef QT_20
  .latin1()
#endif
     ))
															!= APS_SUCCESS)
			{
       	fprintf(stderr, "\n Err, Aps_Rename() = %d", result);
			}
	}
	else
	{
   	fprintf(stderr, "\n Err, Aps_OpenPrinter() = %d", result);
	}
}
