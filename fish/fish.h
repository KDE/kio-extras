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

#include <qstring.h>
#include <qcstring.h>


#include <kurl.h>
#include <kio/global.h>
#include <kio/slavebase.h>
#include <kprocess.h>
#include <kio/authinfo.h>
#include <time.h>

#define FISH_EXEC_CMD 'X'

class fishProtocol : public KIO::SlaveBase
{
public:
  fishProtocol(const QCString &pool_socket, const QCString &app_socket);
  virtual ~fishProtocol();

  /**
Connects to a server and logs us in via SSH. Then starts FISH protocol.
@ref isConnected is set to true if logging on was successfull.
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
  void get(const KURL& url);
  /** put a file */
  void put(const KURL& url, int permissions, bool overwrite, bool resume);
  /** aborts command sequence and calls error() */
  void error(int type, const QString &detail);
  /** executes next command in sequence or calls finished() if all is done */
  void finished();
  /** stat a file */
  void stat(const KURL& url);
  /** find mimetype for a file */
  void mimetype(const KURL& url);
  /** list a directory */
  void listDir(const KURL& url);
  /** create a directory */
  void mkdir(const KURL&url, int permissions);
  /** rename a file */
  void rename(const KURL& src, const KURL& dest, bool overwrite);
  /** create a symlink */
  void symlink(const QString& target, const KURL& dest, bool overwrite);
  /** change file permissions */
  void chmod(const KURL& url, int permissions);
  /** copies a file */
  void copy(const KURL &src, const KURL &dest, int permissions, bool overwrite);
  /** report status */
  void slave_status();
  /** removes a file or directory */
  void del(const KURL &u, bool isfile);
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
  int outBufPos;
  /** length of buffer */
  int outBufLen;
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
  KIO::UDSAtom typeAtom;
  /** for LIST/STAT */
  bool isFirst;
  /** for LIST/STAT */
  QString thisFn;
  /** for STAT */
  QString wantedFn;
  QString statPath;
  /** url of current request */
  KURL url;
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
  QValueList<int> commandCodes;
  /** bytes still to be read in raw mode */
  int rawRead;
  /** bytes still to be written in raw mode */
  int rawWrite;
  /** data bytes to read in next read command */
  int recvLen;
  /** data bytes to write in next write command */
  int sendLen;
  /** true if the last write operation was finished */
  bool writeReady;
  /** true if a command stack is currently executing */
  bool isRunning;
  /** reason of LIST command */
  enum { CHECK, LIST, STAT, SIZE, STATCHECK } listReason;
  /** true if FISH server understands COPY command */
  bool hasCopy;
  /** true if FISH server understands RSYNC command */
  bool hasRsync;
  /** true if FISH server understands APPEND command */
  bool hasAppend;
  /** true if FISH server understands EXEC command */
  bool hasExec;
  /** permission of created file */
  int putPerm;
  /** true if file may be overwritten */
  bool checkOverwrite;
  /** current position of write */
  int putPos;
  /** true if file already existed */
  bool checkExist;
  /** write buffer */
  QByteArray rawData;
  /** buffer for storing bytes used for MimeMagic */
  QByteArray mimeBuffer;
  /** number of bytes read so far */
  int dataRead;
  time_t t_last, t_start;
  /** unix timestamp "0" as QDateTime */
  QDateTime epoch;
  /** details about each fishCommand */
  static const struct fish_info {
      const char *command;
      int params;
      const char *alt;
      int lines;
  } fishInfo[];
  /** last FISH command sent to server */
  enum fish_command_type { FISH_FISH, FISH_VER, FISH_PWD, FISH_LIST, FISH_RETR, FISH_STOR,
    FISH_CWD, FISH_CHMOD, FISH_DELE, FISH_MKD, FISH_RMD,
    FISH_RENAME, FISH_LINK, FISH_SYMLINK, FISH_CHOWN,
    FISH_CHGRP, FISH_READ, FISH_WRITE, FISH_COPY, FISH_APPEND, FISH_EXEC } fishCommand;
  int fishCodeLen;
protected: // Protected methods
  /** manages initial communication setup including password queries */
  int establishConnection(char *buffer, int buflen);
  void debugConnection(char *buffer, int buflen);
  int received(const char *buffer, int buflen);
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
  void writeChild(const char *buf, int len);
  /** parses response from server and acts accordingly */
  void manageConnection(const QString &line);
  /** writes to process */
  void writeStdin(const QString &line);
};


#endif
