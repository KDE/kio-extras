#ifndef _MAILADDRESS_H
#define _MAILADDRESS_H "$Id$"
/**********************************************************************
 *
 *   mailaddress.h - mail adress handler
 *   Copyright (C) 2000
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *   Send comments and bug fixes to
 *
 *********************************************************************/

#include <qstring.h>
#include <qcstring.h>
#include "rfcdecoder.h"

class mailAddress {
public: 
	mailAddress();
	~mailAddress();
	mailAddress(char *aCStr);
	mailAddress(const mailAddress &);
    mailAddress & operator = (const mailAddress &);
	
	void setUser(const QCString &aUser) { user = aUser;};
	const QCString &getUser() { return user;};
	void setHost(const QCString &aHost) { host = aHost;};
	const QCString &getHost() { return host;};

	void setFullName(const QString &aFull) { fullName = aFull;};
	void setFullNameRaw(const QCString &aFull);
	const QString &getFullName() { return fullName;};

	void setComment(const QString &aComment) { comment = aComment;};
	void setCommentRaw(const QCString &);
	const QString &getComment() { return comment;};

	int parseAddress(char *);
	const QCString getStr();
	bool isEmpty();

private:
	QCString user;
	QCString host;
	QString fullName;
	QString comment;
};

#endif
