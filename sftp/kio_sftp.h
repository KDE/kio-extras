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

#include "kprocessblockingrw.h"
#include "sftpfileattr.h"

#define KIO_SFTP_DB 7116

class QCString;

class kio_sftpProtocol : public QObject, public KIO::SlaveBase
{
    Q_OBJECT

public:
  kio_sftpProtocol(const QCString &pool_socket, const QCString &app_socket);
  virtual ~kio_sftpProtocol();
  virtual void setHost(const QString& h, int port, const QString& user, const QString& pass);
  virtual void get(const KURL& url);
  /** No descriptions */
  virtual void listDir(const KURL& url) ;
  /** No descriptions */
  virtual void mimetype(const KURL& url);
  /** No descriptions */
  virtual void stat(const KURL& url);
  /** No descriptions */
  virtual void put(const KURL& url, int permissions, bool overwrite, bool resume);
  /** No descriptions */
  virtual void closeConnection();
  /** No descriptions */
  virtual void reparseConfiguration();
  /** No descriptions */
  virtual void slave_status();
  /** No descriptions */
  virtual void del(const KURL &url, bool isfile);
  /** No descriptions */
  virtual void copy(const KURL &src, const KURL &dest, int permissions, bool overwrite);
  /** No descriptions */
  virtual void chmod(const KURL& url, int permissions);
  /** No descriptions */
  virtual void symlink(const QString& target, const KURL& dest, bool overwrite);
  /** No descriptions */
  virtual void rename(const KURL& src, const KURL& dest, bool overwrite);
  /** No descriptions */
  virtual void mkdir(const KURL&url, int permissions);
  /** No descriptions */
  virtual void openConnection();
  /** Gets the DirAttrs flag. */
  bool getDirAttrsFlag() const;
  /** Sets the DirAttrs flag.  This flag affects how the >> operator works on data streams. */
  void setDirAttrsFlag(bool flag);

private: // Private variables
  /** True if ioslave is connected to sftp server. */
  bool mConnected;

  /** Host we are connected to. */
  QString mHost;

  /** Port we are connected to. */
  int mPort;

  /** Ssh process to which we send the sftp packets. */
  KProcessBlockingRW ssh;

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

private: // private methods
  /** Starts the ssh process and authenticates the user by waiting for the
      ssh password prompt, sending the users password, and checking
      for success.  Will keep trying until user cancels or ssh dies. */
  bool startSsh();

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
private slots: // Private slots
  void slotSshExited(KProcess *);

};

void mymemcpy(const char* b, QByteArray& a, unsigned int offset, unsigned int len);

kdbgstream& operator<< (kdbgstream& s, QByteArray& a);

#endif
