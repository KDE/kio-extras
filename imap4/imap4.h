#ifndef _IMAP4_H
#define _IMAP4_H "$Id$"
/**********************************************************************
 *
 *   imap4.h  - IMAP4rev1 KIOSlave
 *   Copyright (C) 1999  John Corey
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
 *   Send comments and bug fixes to jcorey@fruity.ath.cx
 *
 *********************************************************************/

#include <sys/types.h>
#include <sys/time.h>

#include <stdio.h>

#include <qstringlist.h>
#include <qstring.h>
#include <qlist.h>

#include <kio/slavebase.h>

enum IMAP_COMMAND {
// Any State
    ICMD_CAPABILITY, ICMD_NOOP, ICMD_LOGOUT,
// Non-Authenticated State
    ICMD_AUTHENTICATE, ICMD_LOGIN,
    ICMD_SEND_AUTH,
//  Authenticated State
    ICMD_SELECT, ICMD_EXAMINE, ICMD_CREATE, ICMD_DELETE, ICMD_RENAME,
    ICMD_SUBSCRIBE, ICMD_UNSUB, ICMD_LIST, ICMD_LSUB, ICMD_STATUS, ICMD_APPEND,
// Selected State
    ICMD_CHECK, ICMD_CLOSE, ICMD_EXPUNGE, ICMD_SEARCH, ICMD_FETCH, ICMD_STORE,
    ICMD_COPY, ICMD_UID
};

class CMD_Struct {
 public:
  QString identifier, args;
  enum IMAP_COMMAND type;
  bool sent;
};

class IMAP4Protocol : public KIO::SlaveBase
{
public:
  IMAP4Protocol (const QCString &pool, const QCString &app);

  virtual void setHost( const QString &_host, int _port, const QString &_user, const QString &_pass );

  virtual void get( const KURL &_url );
  virtual void stat( const KURL &_url );
  virtual void del( const KURL &_url, bool isFile );
  virtual void listDir( const KURL &_url );

protected:

  ssize_t getSize( const KURL &_url );

  virtual void slotTestDir( const char *_url );

  void startLoop();
  
  
protected:
  QList<CMD_Struct> pending;
  unsigned int command (enum IMAP_COMMAND cmd, const QString &args);
  void sendNextCommand();
  void imap4_close ();
  bool imap4_open ();
  void imap4_login(); // handle loggin in
  void imap4_exec();  // executes the IMAP action
  void processList(QString str);  // processes LIST/LSUB responses

  int m_iSock, m_cmd;
  unsigned int m_uLastCmd;
  struct timeval m_tTimeout;
  FILE *fp;
  QString m_sCurrentMBX;

  QString authType;
  QStringList capabilities, serverResponses;
  int authState;
  QString authKey, urlPath, folderDelimiter;
  QString action;

  QString m_sServer, m_sPass, m_sUser;
  int m_iPort;
};

#endif
