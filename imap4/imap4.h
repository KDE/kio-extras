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

#include <qstring.h>
#include <qlist.h>

#include <kio_interface.h>
#include <kio_base.h>
#include <kio_filter.h>
#include <kurl.h>

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

class IMAP4Protocol : public KIOProtocol
{
public:
  IMAP4Protocol( KIOConnection *_conn );
  
  virtual void slotGetSize( const char *_url );
  virtual void slotGet( const char *_url );
  virtual void slotPut( const char *_url, int _mode, bool _overwrite,
			bool _resume, unsigned int );
  virtual void slotCopy( const char *_source, const char *_dest );
  virtual void slotData( void *_p, int _len );
  virtual void slotDataEnd();
  virtual void slotDel( QStringList& _source );

  virtual void slotListDir( const char *_url );
  virtual void slotTestDir( const char *_url );

  void jobData(void *_p, int _len);
  void jobDataEnd();
  void jobError( int _errid, const char *_txt );

  void startLoop();
  
  KIOConnection* connection() { return KIOConnectionSignals::m_pConnection; }
  
 protected:
  QList<CMD_Struct> pending;
  unsigned int command (enum IMAP_COMMAND cmd, const char *args);
  void sendNextCommand();
  void imap4_close ();
  bool imap4_open (KURL &_url);
  void imap4_login(); // handle loggin in
  void imap4_exec();  // executes the IMAP action
  void processList(QString str);  // processes LIST/LSUB responses

  int m_iSock, m_cmd;
  unsigned int m_uLastCmd;
  struct timeval m_tTimeout;
  FILE *fp;
  KIOJobBase* m_pJob;
  QString m_sCurrentMBX;

  QString authType, userName, passWord;
  QStringList capabilities, serverResponses;
  int authState;
  QString authKey, urlPath, folderDelimiter;
  QString action;
};

class IMAP4IOJob : public KIOJobBase
{
 public:
  IMAP4IOJob( KIOConnection *_conn, IMAP4Protocol *_imap4 );
  virtual void slotError( int _errid, const char *_txt );
  virtual void slotData(void *_p, int _len);
  virtual void slotDataEnd();
  
 protected:
  IMAP4Protocol* m_pIMAP4;
};

#endif
