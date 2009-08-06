/***************************************************************************
                          sftpProtocol.h  -  description
                             -------------------
    begin                : Sat Jun 30 20:08:47 CDT 2001
    copyright            : (C) 2001 by Lucas Fisher
    email                : ljfisher@purdue.edu
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



#include <kurl.h>
#include <kio/global.h>
#include <kio/slavebase.h>
#include <kdebug.h>

#include "process.h"
#include "sftpfileattr.h"
#include "ksshprocess.h"

#define KIO_SFTP_DB 7120


class sftpProtocol : public KIO::SlaveBase
{

public:
  sftpProtocol(const QByteArray &pool_socket, const QByteArray &app_socket);
  virtual ~sftpProtocol();
  virtual void setHost(const QString& h, quint16 port, const QString& user, const QString& pass);
  virtual void get(const KUrl& url);
  virtual void listDir(const KUrl& url) ;
  virtual void mimetype(const KUrl& url);
  virtual void stat(const KUrl& url);
  virtual void copy(const KUrl &src, const KUrl &dest, int permissions, KIO::JobFlags flags);
  virtual void put(const KUrl& url, int permissions, KIO::JobFlags flags);
  virtual void closeConnection();
  virtual void slave_status();
  virtual void del(const KUrl &url, bool isfile);
  virtual void chmod(const KUrl& url, int permissions);
  virtual void symlink(const QString& target, const KUrl& dest, KIO::JobFlags flags);
  virtual void rename(const KUrl& src, const KUrl& dest, KIO::JobFlags flags);
  virtual void mkdir(const KUrl&url, int permissions);
  virtual void openConnection();

  // KIO::FileJob interface
  virtual void open(const KUrl &url, QIODevice::OpenMode mode);
  virtual void read(KIO::filesize_t size);
  virtual void write(const QByteArray &data);
  virtual void seek(KIO::filesize_t offset);
  virtual void close();

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

  /** User's password */
  QString mPassword;

  /** Message id of the last sftp packet we sent. */
  unsigned int mMsgId;

  /** Type of packet we are expecting to receive next. */
  unsigned char mExpected;

  /** Version of the sftp protocol we are using. */
  int sftpVersion;

  struct Status
  {
    int code;
    KIO::filesize_t size;
    QString text;
  };

  // KIO::FileJob interface
  /** The opened handle */
  QByteArray openHandle;
  KUrl openUrl;
  KIO::filesize_t openOffset;

private: // private methods
  bool getPacket(QByteArray& msg);

   /* Type is a sftp packet type found in .sftp.h'.
   * Example: SSH2_FXP_READLINK, SSH2_FXP_RENAME, etc.
   *
   * Returns true if the type is supported by the sftp protocol
   * version negotiated by the client and server (sftpVersion).
   */
  bool isSupportedOperation(int type);
  /** Used to have the server canonicalize any given path name to an absolute path.
      This is useful for converting path names containing ".." components or relative
      pathnames without a leading slash into absolute paths.
      Returns the canonicalized url. */
  int sftpRealPath(const KUrl& url, KUrl& newUrl);

  /** Send an sftp packet to stdin of the ssh process. */
  bool putPacket(QByteArray& p);
  /** Process SSH_FXP_STATUS packets. */
  void processStatus(quint8, const QString& message = QString());
  /** Process SSH_FXP_STATUS packes and return the result. */
  Status doProcessStatus(quint8, const QString& message = QString());
  /** Opens a directory handle for url.path. Returns true if succeeds. */
  int sftpOpenDirectory(const KUrl& url, QByteArray& handle);
  /** Closes a directory or file handle. */
  int sftpClose(const QByteArray& handle);
  /** Send a sftp command to rename a file or directory. */
  int sftpRename(const KUrl& src, const KUrl& dest);
  /** Set a files attributes. */
  int sftpSetStat(const KUrl& url, const sftpFileAttr& attr);
  /** Sends a sftp command to remove a file or directory. */
  int sftpRemove(const KUrl& url, bool isfile);
  /** Creates a symlink named dest to target. */
  int sftpSymLink(const QString& target, const KUrl& dest);
  /** Get directory listings. */
  int sftpReadDir(const QByteArray& handle, const KUrl& url);
  /** Retrieves the destination of a link. */
  int sftpReadLink(const KUrl& url, QString& target);
  /** Stats a file. */
  int sftpStat(const KUrl& url, sftpFileAttr& attr);
  /** No descriptions */
  int sftpOpen(const KUrl& url, const quint32 pflags, const sftpFileAttr& attr, QByteArray& handle);
  /** No descriptions */
  int sftpRead(const QByteArray& handle, KIO::filesize_t offset, quint32 len, QByteArray& data);
  /** No descriptions */
  int sftpWrite(const QByteArray& handle, KIO::filesize_t offset, const QByteArray& data);

  /** Performs faster upload when the source is a local file... */
  void sftpCopyPut(const KUrl& src, const KUrl& dest, int mode, KIO::JobFlags flags);
  /** Performs faster download when the destination is a local file... */
  void sftpCopyGet(const KUrl& dest, const KUrl& src, int mode, KIO::JobFlags flags);

  /** Read a file. This is used by get(), copy(), and mimetype(). */
  Status sftpGet( const KUrl& src, KIO::filesize_t offset = 0, int fd = -1, bool abortAfterMimeType = false);
  /** Write a file */
  void sftpPut( const KUrl& dest, int permissions, KIO::JobFlags flags, int fd = -1);
};
#endif
