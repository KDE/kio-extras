#ifndef _IMAP4_H
#define _IMAP4_H "$Id$"
/**********************************************************************
 *
 *   imap4.h  - IMAP4rev1 KIOSlave
 *   Copyright (C) 2001-2002  Michael Haeckel <haeckel@kde.org>
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
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *   Send comments and bug fixes to jcorey@fruity.ath.cx
 *
 *********************************************************************/

#include "imapparser.h"
#include "mimeio.h"

#include <kio/tcpslavebase.h>

#define IMAP_BUFFER 8192

/** @brief type of object the url refers too */
enum IMAP_TYPE
{
  ITYPE_UNKNOWN, /*< unknown type */
  ITYPE_DIR,     /*< Object is a directory. i.e. does not contain message, just mailboxes */
  ITYPE_BOX,     /*< Object is a mailbox. i.e. contains mails */
  ITYPE_DIR_AND_BOX, /*< Object contains both mails and mailboxes */
  ITYPE_MSG   /*< Object is a mail */
};

/** @brief IOSlave derived class */
class IMAP4Protocol:public
  KIO::TCPSlaveBase,
  public
  imapParser,
  public
  mimeIO
{

public:

  // reimplement the TCPSlave
  IMAP4Protocol (const QCString & pool, const QCString & app, bool isSSL);
  virtual ~IMAP4Protocol ();

  virtual void openConnection();
  virtual void closeConnection();

  virtual void setHost (const QString & _host, int _port, const QString & _user,
    const QString & _pass);
  virtual void get (const KURL & _url);
  virtual void stat (const KURL & _url);
  virtual void slave_status ();
  virtual void mimetype (const KURL & _url);
  virtual void del (const KURL & _url, bool isFile);
  /** Change the status. data = 'S' + URL + '\0' + Flags + '\0'
   *  Copy a mail: data = 'C' + srcURL + '\0' + destURL + '\0' */
  virtual void special (const QByteArray & data);
  virtual void listDir (const KURL & _url);
  virtual void setSubURL (const KURL & _url);
  virtual void dispatch (int command, const QByteArray & data);
  virtual void mkdir (const KURL & url, int permissions);
  virtual void put (const KURL & url, int permissions, bool overwrite,
    bool resume);
  virtual void rename (const KURL & src, const KURL & dest, bool overwrite);
  virtual void copy (const KURL & src, const KURL & dest, int permissions,
    bool overwrite);

  /** @brief reimplement the parser
   * relay hook to send the fetched data directly to an upper level 
   */
  virtual void parseRelay (const QByteArray & buffer);

  /** @brief reimplement the parser
   * relay hook to announce the fetched data directly to an upper level
   */
  virtual void parseRelay (ulong);

  /** @brief reimplement the parser
   * read at least len bytes */
  virtual bool parseRead (QByteArray &buffer,ulong len,ulong relay=0);

  /** @brief reimplement the parser
   * @brief read at least a line (up to CRLF) */
  virtual bool parseReadLine (QByteArray & buffer, ulong relay = 0);

  /** @brief reimplement the parser
   * @brief write argument to the server */
  virtual void parseWriteLine (const QString &);

  /** @brief reimplement the mimeIO */
  virtual int outputLine (const QCString & _str);

  /** @brief send out cached data to the application */
  virtual void flushOutput();

protected:

  // select or examine the box if needed
  bool assureBox (const QString & aBox, bool readonly);

  ssize_t myRead(void *data, ssize_t len);

  enum IMAP_TYPE
  parseURL (const KURL & _url, QString & _box, QString & _section,
            QString & _type, QString & _uid, QString & _validity,
            QString & _hierarchyDelimiter);
  QString getMimeType (enum IMAP_TYPE);

  bool makeLogin ();

  void outputLineStr (const QString & _str)
  {
    outputLine (_str.latin1 ());
  }
  void doListEntry (const KURL & _url, int stretch, imapCache * cache = NULL,
    bool withFlags = FALSE, bool withSubject = FALSE);
  void doListEntry (const KURL & _url, const QString & myBox,
                    const imapList & item);

private:

  QString myHost, myUser, myPass, myAuth, myTLS;
  int myPort;
  bool mySSL;

  bool relayEnabled, cacheOutput;
  QByteArray outputCache;
  KIO::filesize_t mProcessedSize;

  char readBuffer[IMAP_BUFFER];
  ssize_t readBufferLen;
  int readSize;
};

#endif
