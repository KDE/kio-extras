/* This file is part of KDE
   Copyright (C) 2000 by Wolfram Diestel <wolfram@steloj.de>

   This is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.
*/

#ifndef _NNTP_H
#define _NNTP_H "$Id$"

#include <qobject.h>
#include <qstring.h>
#include <kio/global.h>
#include <kio/slavebase.h>

/* TODO:

  - test special post command
  - chars_wanted -> max_chars in TCPWrapper.read()
  - QCString -> QByteArray in TCPWrapper readData/writeData,
    this should be more efficient
  - use read instead of readline in get
  - progress information in get, and maybe post
  - .. <-> . at line startings
  - better NNTP error handling, method returning all the strings for
    error numbers and decides, if disconnect or ignore the error
  - second arg in error() somtimes is wrong, where it's documented,
    for which errors this is add info, filename, a.s.o.?
  - i18n, error handling should be ok before this to evite translating
    the strings more than once
  - remove lot of debug stuff
*/

class TCPWrapper: public QObject {

  Q_OBJECT

  public:
    TCPWrapper();
    virtual ~TCPWrapper();

    bool connect(const QString &host, short unsigned int port); // connect to host
    bool connected() { return tcpSocket >= 0; }; // socket exist
    bool disconnect();                           // close socket

    int  read(QCString &data, int chars_wanted); // read from buffer
    bool readLine(QCString &line);               // read next line
    bool write(const QCString &data) { return writeData(data); };  // write to socket
    bool writeLine(const QCString &line) { return writeData(line + "\r\n"); }; // write to socket

    void setTimeOut(int tm_out);                 // sets a new timeout value,

  protected:
    bool readData(QCString &data);               // read data from socket
    bool writeData(const QCString &data);        // write data to socket

  signals:
    void error(KIO::Error errnum, const QString &errinfo);

  private:
    int timeOut;        // socket timeout in sec
    int tcpSocket;      // socket handle
    int thisLine, nextLine; // line positions in the buffer
    QCString buffer;    // buffer for accessing by readLine

    bool readyForReading(); // waits until socket is ready for reading or error
    bool readyForWriting(); // waits until socket is ready for writing or error
};

class NNTPProtocol : public QObject, public KIO::SlaveBase
{

 Q_OBJECT

 public:
  NNTPProtocol (const QCString &pool, const QCString &app );
  virtual ~NNTPProtocol();

  virtual void get(const KURL& url );
  virtual void stat(const KURL& url );
  virtual void listDir(const KURL& url );
  virtual void slave_status();
  virtual void setHost(const QString& host, int port,
        const QString& user, const QString& pass);

  /**
    *  Special command: 1 = post article
    *  it takes no other args, the article data are
    *  requested by dataReq() and should be valid
    *  as in RFC850. It's not checked for correctness here.
    */
  virtual void special(const QByteArray& data);

 protected:

  /**
    *  Send a command to the server. Returns the response code and
    *  the response line
    */
  int send_cmd (const QString &cmd);

  /**
    *  Attempt to properly shut down the NNTP connection by sending
    *  "QUIT\r\n" before closing the socket.
    */
  void nntp_close ();

  /**
    * Attempt to initiate a NNTP connection via a TCP socket.  If no port
    * is passed, port 119 is assumed, if no user || password is
    * specified, the user is prompted for them.
    */
  void nntp_open();

  /**
    * Post article. Invoked by special
    *
    */
  bool post_article();

 protected slots:
   void socketError(KIO::Error errnum, const QString &errinfo);

 private:
   // connection info for reusing it at next request
   QString host, pass, user;
   short unsigned int port;
   QString resp_line;
   bool postingAllowed;

   TCPWrapper socket;  // handles the socket stuff
   int eval_resp();    // get server response and check it for general errors

   void fetchGroups();
   bool fetchGroup(QString& group);
   KIO::UDSEntry& makeUDSEntry(const QString& name, int size, bool posting_allowed,
      bool is_article);

};


#endif










