#ifndef _IMAP4_H
#define _IMAP4_H "$Id$"

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

class IMAP4Protocol : public IOProtocol
{
public:
  IMAP4Protocol( Connection *_conn );
  
  virtual void slotGet( const char *_url );
  virtual void slotPut( const char *_url, int _mode, bool _overwrite,
			bool _resume, unsigned int );

  void jobError( int _errid, const char *_txt );
  void startLoop();
  
  Connection* connection() { return ConnectionSignals::m_pConnection; }
  
 protected:
  QList<CMD_Struct> pending;
  unsigned int command (enum IMAP_COMMAND cmd, const char *args);
  void sendNextCommand();
  void imap4_close ();
  bool imap4_open (KURL &_url);

  int m_iSock;
  unsigned int m_uLastCmd;
  struct timeval m_tTimeout;
  FILE *fp;
  IOJob* m_pJob;
  QString m_sCurrentMBX;
};

class IMAP4IOJob : public IOJob
{
 public:
  IMAP4IOJob( Connection *_conn, IMAP4Protocol *_imap4 );
  virtual void slotError( int _errid, const char *_txt );
  
 protected:
  IMAP4Protocol* m_pIMAP4;
};

#endif
