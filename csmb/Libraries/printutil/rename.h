/* Name: rename.h
            
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

#ifndef _RENAME_H
#define _RENAME_H

#include <qevent.h>
#include <qpushbt.h>
#include <qpixmap.h>
#include <qmessagebox.h>
#include <qdialog.h>
#include <qpushbt.h>
#include <qlined.h>
#include <qlayout.h>


class Crenamedlg : public QDialog
{
  Q_OBJECT
  class mainDlg;
public:
  Crenamedlg(QWidget* parent = NULL, const char* name = NULL,
			const QString oldName = NULL);
   ~Crenamedlg();
	void rename(const QString &oldName, const QString & newName);
public slots:
  void ok();
  void cancel();
	
private:
  QVBoxLayout *layout;
  QPushButton *pbOk;
  QPushButton *pbCancel;
  QLineEdit   *leusername1;
  QLineEdit   *leusername2;
  QMessageBox message;
};
#endif
