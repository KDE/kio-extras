/***************************************************************************
                          fish.h  -  a FISH kioslave
                             -------------------
    begin                : Thu Oct  4 17:09:14 CEST 2001
    copyright            : (C) 2001 by Jörg Walter
    email                : trouble@garni.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation, version 2 of the License                *
 *                                                                         *
 ***************************************************************************/
#ifndef __fish_h__
#define __fish_h__

#include <QString>

#include <kurl.h>
#include <kio/global.h>
#include <kio/slavebase.h>
#include <k3process.h>
#include <kio/authinfo.h>
#include <time.h>

#define FISH_EXEC_CMD 'X'

class fishProtocol : public KIO::SlaveBase
{
public:
  fishProtocol(const QByteArray &pool_socket, const QByteArray &app_socket);
  virtual ~fishProtocol();

  /**
Connects to a server and logs us in via SSH. Then starts FISH protocol.
@ref isConnected is set to true if logging on was successful.
It is set to false if the connection becomes closed.

 */
  void openConnection();

  /**
   Clean up connection
  */
  void shutdownConnection(bool forced=false);
  /** sets connection information for subsequent commands */
  void setHost(const QString & host, int port, const QString & user, const QString & pass);
  /** Forced close of the connection */
  void closeConnection();
  /** get a file */
  void get(const KUrl& url);
  /** put a file */
  void put(const KUrl& url, int permissions, bool overwrite, bool resume);
  /** aborts command sequence and calls error() */
  void error(int type, const QString &detail);
  /** executes next command in sequence or calls finished() if all is done */
  void finished();
  /** stat a file */
  void stat(const KUrl& url);
  /** find mimetype for a file */
  void mimetype(const KUrl& url);
  /** list a directory */
  void listDir(const KUrl& url);
  /** create a directory */
  void mkdir(const KUrl&url, int permissions);
  /** rename a file */
  void rename(const KUrl& src, const KUrl& dest, bool overwrite);
  /** create a symlink */
  void symlink(const QString& target, const KUrl& dest, bool overwrite);
  /** change file permissions */
  void chmod(const KUrl& url, int permissions);
  /** copies a file */
  void copy(const KUrl &src, const KUrl &dest, int permissions, bool overwrite);
  /** report status */
  void slave_status();
  /** removes a file or directory */
  void del(const KUrl &u, bool isfile);
  /** special like background execute */
  void special( const QByteArray &data );

private: // Private attributes
  /** the SSH process used to communicate with the remote end */
  pid_t childPid;
  /** fd for reading and writing to the process */
  int childFd;
  /** buffer for data to be written */
  const char *outBuf;
  /** current write position in buffer */
  KIO::fileoffset_t outBufPos;
  /** length of buffer */
  KIO::fileoffset_t outBufLen;
  /** use su if true else use ssh */
  bool local;
  /**  // FIXME: just a workaround for konq deficiencies */
  bool isStat;
  /**  // FIXME: just a workaround for konq deficiencies */
  QString redirectUser, redirectPass;

protected: // Protected attributes
  /** for LIST/STAT */
  KIO::UDSEntry udsEntry;
  /** for LIST/STAT */
  KIO::UDSEntry udsStatEntry;
  /** for LIST/STAT */
  long long udsType;
  /** for LIST/STAT */
  QString udsMime;
  /** for LIST/STAT */
  QString thisFn;
  /** for STAT */
  QString wantedFn;
  QString statPath;
  /** url of current request */
  KUrl url;
  /** true if connection is logged in successfully */
  bool isLoggedIn;
  /** host name of current connection */
  QString connectionHost;
  /** user name of current connection */
  QString connectionUser;
  /** port of current connection */
  int connectionPort;
  /** password of current connection */
  QString connectionPassword;
  /** AuthInfo object used for logging in */
  KIO::AuthInfo connectionAuth;
  /** number of lines received, == 0 -> everything went ok */
  int errorCount;
  /** queue for lines to be sent */
  QStringList qlist;
  /** queue for commands to be sent */
  QStringList commandList;
  /** queue for commands to be sent */
  QList<int> commandCodes;
  /** bytes still to be read in raw mode */
  KIO::fileoffset_t rawRead;
  /** bytes still to be written in raw mode */
  KIO::fileoffset_t rawWrite;
  /** data bytes to read in next read command */
  KIO::fileoffset_t recvLen;
  /** data bytes to write in next write command */
  KIO::fileoffset_t sendLen;
  /** true if the last write operation was finished */
  bool writeReady;
  /** true if a command stack is currently executing */
  bool isRunning;
  /** reason of LIST command */
  enum { CHECK, LIST } listReason;
  /** true if FISH server understands APPEND command */
  bool hasAppend;
  /** permission of created file */
  int putPerm;
  /** true if file may be overwritten */
  bool checkOverwrite;
  /** current position of write */
  KIO::fileoffset_t putPos;
  /** true if file already existed */
  bool checkExist;
  /** true if this is the first login attempt (== use cached password) */
  bool firstLogin;
  /** write buffer */
  QByteArray rawData;
  /** buffer for storing bytes used for MimeMagic */
  QByteArray mimeBuffer;
  /** whther the mimetype has been sent already */
  bool mimeTypeSent;
  /** number of bytes read so far */
  KIO::fileoffset_t dataRead;
  /** details about each fishCommand */
  static const struct fish_info {
      const char *command;
      int params;
      const char *alt;
      int lines;
  } fishInfo[];
  /** last FISH command sent to server */
  enum fish_command_type { FISH_FISH, FISH_VER, FISH_PWD, FISH_LIST, FISH_STAT,
    FISH_RETR, FISH_STOR,
    FISH_CWD, FISH_CHMOD, FISH_DELE, FISH_MKD, FISH_RMD,
    FISH_RENAME, FISH_LINK, FISH_SYMLINK, FISH_CHOWN,
    FISH_CHGRP, FISH_READ, FISH_WRITE, FISH_COPY, FISH_APPEND, FISH_EXEC } fishCommand;
  int fishCodeLen;
protected: // Protected methods
  /** manages initial communication setup including password queries */
  int establishConnection(char *buffer, KIO::fileoffset_t buflen);
  int received(const char *buffer, KIO::fileoffset_t buflen);
  void sent();
  /** builds each FISH request and sets the error counter */
  bool sendCommand(fish_command_type cmd, ...);
  /** checks response string for result code, converting 000 and 001 appropriately */
  int handleResponse(const QString &str);
  /** parses a ls -l time spec */
  int makeTimeFromLs(const QString &dayStr, const QString &monthStr, const QString &timeyearStr);
  /** executes a chain of commands */
  void run();
  /** creates the subprocess */
  bool connectionStart();
  /** writes one chunk of data to stdin of child process */
  void writeChild(const char *buf, KIO::fileoffset_t len);
  /** parses response from server and acts accordingly */
  void manageConnection(const QString &line);
  /** writes to process */
  void writeStdin(const QString &line);
};


#endif
