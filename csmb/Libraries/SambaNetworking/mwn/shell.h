/* Name: shell.h

   Description: This file is a part of the libmwn library.

   Author:	Oleg Noskov (olegn@corel.com)

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

#ifndef __INC_SHELL_H__
#define __INC_SHELL_H__

#include "common.h"
#include <termios.h>
#include "qstrlist.h"

class CPseudoTerminal: public QObject
{
	Q_OBJECT

public:
	CPseudoTerminal(LPCSTR OperationDescription, char *pResultString = NULL, int nResultSize = 0);
	~CPseudoTerminal();
	
	int run(QStrList & args, const char* term);
	void send_byte(char s);
	void send_string(const char* s);

public slots:
	void send_bytes(const char* s, int len);
	void setSize(int lines, int columns);

signals:
	void done(int status);
	void block_in(const char* s, int len);
	void written();

private slots:

	BOOL DataReceived(FILE *f);
	void DataWritten(int);

private:

	void makeShell(const char* dev, QStrList & args, const char* term);
	int m_fd;
	struct termios   tp;
	int	login_shell;
	char *m_pResultString;
	int m_nResultSize;
	QString m_OperationDescription;
};

#endif /* __INC_SHELL_H__ */

