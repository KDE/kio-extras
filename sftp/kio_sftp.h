/***************************************************************************
                          kio_sftpProtocol.h  -  description
                             -------------------
    begin                : Sat Jun 30 20:08:47 CDT 2001
    copyright            : (C) 2001 by Lucas Fisher
    email                : ljfisher@iastate.edu
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef __kio_sftp_h__
#define __kio_sftp_h__

#include <qstring.h>
#include <qcstring.h>
#include <qobject.h>

#include <kurl.h>
#include <kio/global.h>
#include <kio/slavebase.h>
#include <kdebug.h>

#include "process.h"
#include "sftpfileattr.h"
#include "ksshprocess.h"

#define KIO_SFTP_DB 7116

class QCString;

class kio_sftpProtocol : public QObject, public KIO::SlaveBase
{

public:
  kio_sftpProtocol(const QCString &pool_socket, const QCString &app_socket);
  virtual ~kio_sftpProtocol();
  virtual void setHost(const QString& h, int port, const QString& user, const QString& pass);
  virtual void get(const KURL& url);
  virtual void listDir(const KURL& url) ;
  virtual void mimetype(const KURL& url);
  virtual void stat(const KURL& url);
  virtual void put(const KURL& url, int permissions, bool overwrite, bool resume);
  virtual void closeConnection();
  virtual void slave_status();
  virtual void del(const KURL &url, bool isfile);
  virtual void chmod(const KURL& url, int permissions);
  virtual void symlink(const QString& target, const KURL& dest, bool overwrite);
  virtual void rename(const KURL& src, const KURL& dest, bool overwrite);
  virtual void mkdir(const KURL&url, int permissions);
  virtual void openConnection();

private: // Private variables
  /** True if ioslave is connected to sftp server. */
  bool mConnected;

  /** Host we are connected to. */
  QString mHost;

  /** Port we are connected to. */
  int mPort;

  /** Ssh process to which we send the sftp packets. */
  KSshProcess ssh;

  /** Username to use when connecting */
  QString mUsername;

  /** Message id of the last sftp packet we sent. */
  unsigned int mMsgId;

  /** Type of packet we are expecting to recieve next. */
  unsigned char mExpected;

  /** Current url */
  KURL mUrl;

  /** User's password */
  QString mPassword;

  /* default port for ssh */
  int defaultPort;

private: // private methods
  bool getPacket(QByteArray& msg);

  /** Used to have the server canonicalize any given path name to an absolute path.
      This is useful for converting path names containing ".." components or relative
      pathnames without a leading slash into absolute paths.
      Returns the canonicalized url. */
  int sftpRealPath(const KURL& url, KURL& newUrl);

  /** Send an sftp packet to stdin of the ssh process. */
  bool putPacket(QByteArray& p);
  /** Process SSH_FXP_STATUS packets. */
  void processStatus(Q_UINT8, QString message = QString::null);
  /** Opens a directory handle for url.path. Returns true if succeeds. */
  int sftpOpenDirectory(const KURL& url, QByteArray& handle);
  /** Closes a directory or file handle. */
  int sftpClose(const QByteArray& handle);
  /** Send a sftp command to rename a file or directoy. */
  int sftpRename(const KURL& src, const KURL& dest);
  /** Set a files attributes. */
  int sftpSetStat(const KURL& url, const sftpFileAttr& attr);
  /** Sends a sftp command to remove a file or directory. */
  int sftpRemove(const KURL& url, bool isfile);
  /** Creates a symlink named dest to target. */
  int sftpSymLink(const QString& target, const KURL& dest);
  /** Get directory listings. */
  int sftpReadDir(const QByteArray& handle, const KURL& url);
  /** Retrieves the destination of a link. */
  int sftpReadLink(const KURL& url, QString& target);
  /** Stats a file. */
  int sftpStat(const KURL& url, sftpFileAttr& attr);
  /** No descriptions */
  int sftpOpen(const KURL& url, const Q_UINT32 pflags, const sftpFileAttr& attr, QByteArray& handle);
  /** No descriptions */
  int sftpRead(const QByteArray& handle, Q_UINT32 offset, Q_UINT32 len, QByteArray& data);
  /** No descriptions */
  int sftpWrite(const QByteArray& handle, Q_UINT32 offset, const QByteArray& data);
};

void mymemcpy(const char* b, QByteArray& a, unsigned int offset, unsigned int len);

kdbgstream& operator<< (kdbgstream& s, QByteArray& a);

#endif
