/* Name: commonfunc.h
            
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

#ifndef __COMMONFUNC_H
#define __COMMONFUNC_H
#include <qstring.h>
#include <aps.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <qstrlist.h>
#include <ctype.h>
#include <stdlib.h>
#include <qstring.h>
#include <qlineedit.h>
#include "printerobject.h"
#include "mywizard.h"
#define LIST_TOOL "/bin/ls"

#define CMD_WIDTH 80
#define LINE_WIDTH 1000

//fill a printer object info from a printerName
void setPrinterObject(const QString printerName,
										KPrinterObject *printerobject);
// used for set readonly or write and read
bool administrator();

// initial a printer wizard
bool init( CPrinterWizard *);

//  load wizard and add a printer
void addAPrinter();

// delete the printer with name printerName
void deleteAPrinter(const char *printerName);

// used for read device file
int GetCommandOutput(char *buffer[], int buffer_size, bool newLineTermination = true,
											char* tool = NULL, char* path = NULL, char *options = NULL);

// set printer info from the wizard																	
void wiz_setPrinterInfo(CPrinterWizard *wiz_dlg, KPrinterObject * aPrinter);

// get all printers name which currently added
QStrList allPrintersName();

// check the printer name when adding a new printer
bool uniquePrinterName(const char*);

// check the new printer name when renaming a printer's name
bool uniqueNewName(const char*, const char*);

//valid the input string
bool validFileName( const QString& strNameToValidate );

//remaind the old settings
void keepOldInfo(KPrinterObject *aPrinter);

//set as default printer
void setAsDefaultPrinter(const char* printerName);

//display the user's uid and euid
void displayID();

// check the printer name is valid or not
bool varifyNickName(const QString& strName);

//rename a printer
void rename_printer(const QString & printerName);
#endif
