/* This file is part of KDE
   Copyright (C) 2000 by Wolfram Diestel <wolfram@steloj.de>

   This is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include <qdir.h>

#include <ksock.h>
#include <kapp.h>
#include <kdebug.h>

#include "nntp.h"

#define NNTP_PORT 119
// set to 60 sec later, only for testing such a short time out
#define DEFAULT_TIME_OUT 10
#define SOCKET_CHUNK_SIZE 1024*2
#define MAX_BUFFER_SIZE 1024*20

#define UDS_ENTRY_CHUNK 50

#define DBG_AREA 7114
#define DBG kdDebug(DBG_AREA)


using namespace KIO;

extern "C" { int kdemain(int argc, char **argv); }

int kdemain(int argc, char **argv) {

  KInstance instance ("kio_nntp");
  if (argc != 4) {
    fprintf(stderr, "Usage: kio_nntp protocol domain-socket1 domain-socket2\n");
    exit(-1);
  }

  NNTPProtocol slave(argv[2],argv[3]);
  slave.dispatchLoop();

  return 0;
}

/****************** NNTPProtocol ************************/

NNTPProtocol::NNTPProtocol (const QCString &pool, const QCString &app)
  : QObject(), SlaveBase("nntp", pool, app)
{
  DBG << "=============> NNTPProtocol::NNTPProtocol" << endl;
//  socket = new TCPWrapper();
  if (!QObject::connect(&socket, SIGNAL(error(KIO::Error,const QString&)),
        this, SLOT(socketError(KIO::Error,const QString&)))) {
        DBG << "ERROR connecting socket.error() with socketError()" << endl;
  }
}

NNTPProtocol::~NNTPProtocol() {
  DBG << "<============= NNTPProtocol::~NNTPProtocol" << endl;

  // close connection
  nntp_close();
  //delete socket;
}

void NNTPProtocol::get(const KURL& url) {
  DBG << "get " << url.prettyURL() << endl;
  QString path = QDir::cleanDirPath(url.path());
  QRegExp regMsgId = QRegExp("^\\/?[a-z0-9\\.\\-_]+\\/<[a-zA-Z0-9\\.\\@\\-_]+>$",false);
  int pos;
  QString group;
  QString msg_id;

  // path should be like: /group/<msg_id>
  if (regMsgId.match(path) != 0) {
    error(ERR_DOES_NOT_EXIST,path);
    return;
  }

  pos = path.find('<');
  group = path.left(pos);
  msg_id = path.right(path.length()-pos);
  if (group.left(1) == "/") group.remove(0,1);
  if ((pos = group.find('/')) > 0) group = group.left(pos);
  DBG << "get group: " << group << " msg: " << msg_id << endl;

  nntp_open(); // opens only if necessary

  // select group
  int res_code = send_cmd("GROUP "+group);
  if (res_code == 411){
    error(ERR_DOES_NOT_EXIST,path);
    return;
  } else if (res_code != 211) {
    error(ERR_INTERNAL,"Unexpected server response on GROUP: "+
        resp_line);
    nntp_close();
    return;
  }

  // get article
  res_code = send_cmd("ARTICLE "+msg_id);
  if (res_code == 430) {
    error(ERR_DOES_NOT_EXIST,path);
    return;
  } else if (res_code != 220) {
    error(ERR_INTERNAL,"Unexpected server response on GROUP: "+
        resp_line);
    nntp_close();
    return;
  }

  // read and send data
  QCString line;
  QByteArray buffer;
  //socket.read(buffer,MAX_BUFFER_SIZE);
  while (socket.readLine(line) && line != ".\r\n") {
    DBG << "data: [" << line << "]" << endl;
    // cannot use QCString, because it would send the 0-terminator too
    buffer.setRawData(line.data(),line.length());
    data(buffer);
    buffer.resetRawData(line.data(),line.length());
  }
  // end of data
  buffer.resize(0);
  data(buffer);

  // finish
  finished();
}

void NNTPProtocol::special(const QByteArray& data) {
  // 1 = post article
  int cmd;
  QDataStream stream(data, IO_ReadOnly);

  stream >> cmd;
  if (cmd == 1) {
    if (post_article()) finished();
  } else {
    error(ERR_UNSUPPORTED_ACTION,"Invalid command.");
  }
}

bool NNTPProtocol::post_article() {
  DBG << "post article " << endl;

  // send post command
  int res_code = send_cmd("POST");
  if (res_code == 440) { // posting not allowed
    error(ERR_WRITE_ACCESS_DENIED,QString::null);
    return false;
  } else if (res_code != 340) { // ok, send article
    error(ERR_INTERNAL,"Unexpected response from NNTP server"+resp_line);
    nntp_close();
    return false;
  }

  // send article now
  int result;
  do {
    QByteArray buffer;
    QCString data;
    dataReq();
    result = readData(buffer);
    if (result>0) {
      data = QCString(buffer,buffer.size());
      socket.write(data);
    }
  } while (result>0);

  // error occured?
  if (result<0) {
    DBG << "error while getting article data for posting" << endl;
    nntp_close();
    return false;
  }

  // send end mark
  socket.write(QCString("\r\n.\r\n"));

  // get answer
  res_code = eval_resp();
  if (res_code == 441) { // posting failed
    error(ERR_COULD_NOT_WRITE,resp_line);
    return false;
  } else if (res_code != 240) {
    error(ERR_INTERNAL,"Unexpected response from server after sending article "+
      resp_line);
    nntp_close();
    return false;
  }

  return true;
}


void NNTPProtocol::stat( const KURL& url ) {
  DBG << "stat " << url.prettyURL() << endl;
  UDSEntry entry;
  QString path = QDir::cleanDirPath(url.path());
  QRegExp regGroup = QRegExp("^\\/?[a-z\\.\\-_]+\\/?$",false);
  QRegExp regMsgId = QRegExp("^\\/?[a-z0-9\\.\\-_]+\\/<[a-zA-Z0-9\\.\\@\\-_]+>$",false);
  int pos;
  QString group;
  QString msg_id;

  // / = group list
  if (path.isEmpty() || path == "/") {
    DBG << "stat root" << endl;
    entry = makeUDSEntry(QString::null, -1, postingAllowed, false);

  // /group = message list
  } else if (regGroup.match(path) == 0) {
    if (path.left(1) == "/") path.remove(0,1);
    if ((pos = path.find('/')) > 0) group = path.left(pos);
    else group = path;
    DBG << "stat group: " << group << endl;
    // postingAllowed should be ored here with "group not moderated" flag
    // as size the num of messages (GROUP cmd) could be given
    entry = makeUDSEntry(group, -1, postingAllowed, false);

  // /group/<msg_id> = message
  } else if (regMsgId.match(path) == 0) {
    pos = path.find('<');
    group = path.left(pos);
    msg_id = path.right(path.length()-pos);
    if (group.left(1) == "/") group.remove(0,1);
    if ((pos = group.find('/')) > 0) group = group.left(pos);
    DBG << "stat group: " << group << " msg: " << msg_id << endl;
    entry = makeUDSEntry(msg_id, -1, false, true);

  // invalid url
  } else {
    error(ERR_DOES_NOT_EXIST,path);
    return;
  }

  statEntry(entry);
  finished();
}

void NNTPProtocol::listDir( const KURL& url ) {
  DBG << "listDir " << url.prettyURL() << endl;
  nntp_open(); // opens only if necessary

  QString path = QDir::cleanDirPath(url.path());

  if (path.isEmpty() || path == "/") {
    fetchGroups();
    finished();
  } else {
    // if path = /group
    int pos;
    QString group;
    if (path.left(1) == "/") path.remove(0,1);
    if ((pos = path.find('/')) > 0) group = path.left(pos);
    else group = path;
    if (fetchGroup(group)) finished();
  }
}

void NNTPProtocol::fetchGroups() {

  // send LIST command
  int res_code = send_cmd("LIST");
  if (res_code != 215) {
    error(ERR_INTERNAL, "Unexpected response from server after LIST: "+
        resp_line);
    nntp_close();
    return;
  }

  // read newsgroups line by line
  QCString line, group;
  int pos, pos2, msg_cnt;
  bool moderated;
  UDSEntry entry;
  UDSEntryList entryList;

  while (socket.readLine(line) && line != ".\r\n") {
    // group name
    if ((pos = line.find(' ')) > 0) {

      group = line.left(pos);
      DBG << "group: " << group << endl;

      // number of messages
      line.remove(0,pos+1);
      if (((pos = line.find(' ')) > 0 || (pos = line.find('\t')) > 0) &&
          ((pos2 = line.find(' ',pos+1)) > 0 || (pos2 = line.find('\t',pos+1)) > 0)) {
        int last = line.left(pos).toInt();
        int first = line.mid(pos+1,pos2-pos-1).toInt();
        DBG << "line: " << line  << endl;
        DBG << "pos: " << pos << " pos2: " << pos2 << endl;
        DBG << "first: " << first << " last: " << last << endl;
        DBG << "number: " << abs(last-first+1) << endl;
        msg_cnt = abs(last-first+1);
        // moderated group?
        moderated = (line[pos2+1] == 'n');
      } else {
        msg_cnt = 0;
        moderated = false;
      }

      entry = makeUDSEntry(group, msg_cnt, postingAllowed && !moderated, false);
      entryList.append(entry);

      if (entryList.count() >= UDS_ENTRY_CHUNK) {
        listEntries(entryList);
        entryList.clear();
      }
    }
  }

  // send rest of entryList
  if (entryList.count() > 0) listEntries(entryList);
}

bool NNTPProtocol::fetchGroup(QString& group) {
  int res_code;

  // select group
  res_code = send_cmd("GROUP "+group);
  if (res_code == 411){
    error(ERR_DOES_NOT_EXIST,group);
    return false;
  } else if (res_code != 211) {
    error(ERR_INTERNAL,"Unexpected server response on GROUP: "+
        resp_line);
    nntp_close();
    return false;
  }

  //GROUP res_line: 211 cnt first last group ...
  int pos, pos2;
  QString first;
  if (((pos = resp_line.find(' ',4)) > 0 || (pos = resp_line.find('\t',4)) > 0) &&
      ((pos2 = resp_line.find(' ',pos+1)) > 0 || (pos = resp_line.find('\t',pos+1)) > 0))
  {
    first = resp_line.mid(pos+1,pos2-pos-1);
  } else {
    error(ERR_INTERNAL,"Could not extract first message number from server response: "+
      resp_line);
    return false;
  }

  UDSEntry entry;
  UDSEntryList entryList;

  // set art pointer to first article and get msg-id of it
  res_code = send_cmd("STAT "+first);
  if (res_code != 223) {
    error(ERR_INTERNAL,"Unexpected response from server: "+
      resp_line);
    return false;
  }

  //STAT res_line: 223 nnn <msg_id> ...
  QString msg_id;
  if ((pos = resp_line.find('<')) > 0 && (pos2 = resp_line.find('>',pos+1))) {
    msg_id = resp_line.mid(pos,pos2-pos+1);
    entry = makeUDSEntry(msg_id, 0, false, true);
    entryList.append(entry);
  } else {
    error(ERR_INTERNAL,"Could not extract first message id from server response: "+
      resp_line);
    return false;
  }

  // go through all articles
  while (true) {
    res_code = send_cmd("NEXT");
    if (res_code == 421) {
      // last article reached
      listEntries(entryList);
      return true;
    } else if (res_code != 223) {
      error(ERR_INTERNAL,"Unexpected response from server: "+resp_line);
      nntp_close();
      return false;
    }

    //res_line: 223 nnn <msg_id> ...
    if ((pos = resp_line.find('<')) > 0 && (pos2 = resp_line.find('>',pos+1))) {
      msg_id = resp_line.mid(pos,pos2-pos+1);
      entry = makeUDSEntry(msg_id, 0, false, true);
      entryList.append(entry);
      if (entryList.count() >= UDS_ENTRY_CHUNK) {
        listEntries(entryList);
        entryList.clear();
      }
    } else {
      error(ERR_INTERNAL,"Could not extract message id from server response: "+
        resp_line);
      return false;
    }
  }
  return true; // to keep the compiler happy
}

UDSEntry& NNTPProtocol::makeUDSEntry(const QString& name, int size,
  bool posting_allowed, bool is_article) {

  long posting=0;

  UDSAtom atom;
  UDSEntry *entry = new UDSEntry();

  // entry name
  atom.m_uds = UDS_NAME;
  atom.m_str = name;
  atom.m_long = 0;
  entry->append(atom);

  // entry size
  atom.m_uds = UDS_SIZE;
  atom.m_str = QString::null;
  atom.m_long = size;
  entry->append(atom);

  // file type
  atom.m_uds = UDS_FILE_TYPE;
  atom.m_long = is_article? S_IFREG : S_IFDIR;
  atom.m_str = QString::null;
  entry->append(atom);

  // access permissions
  atom.m_uds = UDS_ACCESS;
  posting = posting_allowed? (S_IWUSR | S_IWGRP | S_IWOTH) : 0;
  atom.m_long = (is_article)? (S_IRUSR | S_IRGRP | S_IROTH) :
    (S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH | posting);
  atom.m_str = QString::null;
  entry->append(atom);

  atom.m_uds = UDS_USER;
  atom.m_str = user.isEmpty() ? user : "root";
  atom.m_long= 0;
  entry->append(atom);

  /*
  atom.m_uds = UDS_GROUP;
  atom.m_str = "root";
  atom.m_long=0;
  entry->append(atom);
  */

  // MIME type
  if (is_article) {
    atom.m_uds = UDS_MIME_TYPE;
    atom.m_long= 0;
    atom.m_str = "text/plain";
    entry->append(atom);
  }

  return *entry;
}

void NNTPProtocol::nntp_close () {
  if (socket.connected()) {
    send_cmd("QUIT");
    socket.disconnect();
  }
}

void NNTPProtocol::nntp_open() {

  if (!port) port = NNTP_PORT;

  // if still connected reuse connection
  if (socket.connected())
    {
      DBG << "reusing old connection" << endl;
      return;
    }
  else
    {
      DBG << "connecting to " << host << ":" << port << endl;
      if (socket.connect(host,port)) {
        DBG << "socket connection succeeded" << endl;
        // read greeting
        int res_code = eval_resp();

        /* expect one of
             200 server ready - posting allowed
             201 server ready - no posting allowed
        */
        if ( !(res_code == 200 || res_code == 201) ) {
          error(ERR_UNKNOWN, "Unexpected response from NNTP server: "+
                resp_line+". Expected 200 or 201");
          nntp_close();
          return;
        }

        postingAllowed = (res_code == 200);
      }

      connected();
      return;
    }
};

int NNTPProtocol::eval_resp() {
  QCString line;
  socket.readLine(line);
  int res_code = line.left(3).toInt();
  resp_line = QString::fromUtf8(line);

  DBG << "eval_resp:" << resp_line << endl;

  // check for errors
  if (res_code == 0) {
    error(ERR_INTERNAL, "Unexpected response from server or out of sync:"+
        resp_line);
    nntp_close();
  } else if (res_code == 400) {
    error(ERR_SERVICE_NOT_AVAILABLE,"400 - service discontinued");
    nntp_close();
  } else if (res_code == 500) {
    error(ERR_INTERNAL,"500 - command not recognized");
    nntp_close();
  } else if (res_code == 501) {
    error(ERR_INTERNAL,"501 - command syntax error");
    nntp_close();
  } else if (res_code == 502) {
    error(ERR_ACCESS_DENIED, "502 - access restriction or permission denied");
    nntp_close();
  } else if (res_code == 503) {
    error(ERR_INTERNAL_SERVER, "503 program fault - command not performed");
    nntp_close();
  };

  return res_code;
}

int NNTPProtocol::send_cmd(const QString &cmd) {
  int res_code;
  QCString _cmd = cmd.utf8();

  if (!socket.connected()) {
    DBG << "NOT CONNECTED, cannot send cmd " << cmd << endl;
    return 0;
  }

  DBG << "sending cmd " << cmd << endl;

  socket.writeLine(_cmd);
  res_code = eval_resp();

  // if authorization needed send user info
  if (res_code == 480) {
    DBG << "auth needed, sending user info" << endl;
    _cmd = "AUTHINFO USER "; _cmd += user.utf8();
    socket.writeLine(_cmd);
    res_code = eval_resp();

    if (res_code != 381) {
      error(ERR_UNKNOWN,"unexpected response from NNTP server after sending user info");
      nntp_close();
      return res_code;
    }

    // send password
    DBG << "sending password" << endl;
    _cmd = "AUTHINFO PASS "; _cmd += pass.utf8();
    socket.writeLine(_cmd);
    res_code = eval_resp();

    if (res_code != 281) {
      error(ERR_UNKNOWN,"unexpected response from NNTP server after sending password");
      nntp_close();
      return res_code;
    }

    // ok now, resend command
    DBG << "resending cmd " << cmd << endl;
    _cmd = cmd.utf8();
    socket.writeLine(_cmd);
    res_code = eval_resp();
  }

  return res_code;
}

void NNTPProtocol::socketError(Error errnum, const QString& errinfo) {
  DBG << "ERROR (socket): " << errnum << " " << errinfo << endl;
  error(errnum, errinfo);
}

void NNTPProtocol::slave_status() {
  DBG << "slave_status " << host << (socket.connected()? " conn": " no conn") << endl;
  slaveStatus(host,socket.connected());
}

void NNTPProtocol::setHost(const QString& _host, int _port,
        const QString& _user, const QString& _pass) {
  DBG << "setHost: " << (_user.isEmpty()? (_user+"@") : " ")
      << _host << ":" << _port << endl;

  unsigned short int prt =  _port? _port : NNTP_PORT;

  if (socket.connected() && (host != _host || port != prt ||
    user != _user || pass != _pass)) nntp_close();

  host = _host;
  port = prt;
  user = _user;
  pass = _pass;
}


/***************** class TCPWrapper ******************/

TCPWrapper::TCPWrapper()
{
  timeOut = DEFAULT_TIME_OUT;
  tcpSocket = -1;
  thisLine = 0;
}


TCPWrapper::~TCPWrapper() {
}

bool TCPWrapper::connect(const QString &host, short unsigned int port) {

  DBG << "socket connecting to " << host << ":" << port << endl;

  ksockaddr_in server_name;

  // init socket
  tcpSocket = socket(PF_INET, SOCK_STREAM, 0);
  if (tcpSocket == -1) {
    emit error(ERR_COULD_NOT_CREATE_SOCKET, NULL);
    return false;
  }

  // find server
  memset(&server_name, 0, sizeof(server_name));
  if (!KSocket::initSockaddr(&server_name, host.latin1(), port)) {
    emit error(ERR_UNKNOWN_HOST, host);
    return false;
  }

  // connect to server
  if (::connect(tcpSocket, (struct sockaddr*)(&server_name),
      sizeof(server_name)) != 0)
  {
    error(ERR_COULD_NOT_CONNECT, host);
    return false;
  }

  return true;

}

bool TCPWrapper::disconnect() {
  if (tcpSocket != -1) {
    close(tcpSocket);
    tcpSocket = -1;
  }

  return true;
}

int TCPWrapper::read(QCString &data, int chars_wanted) {
  // FIXME: if there are less than chars_wanted available
  //   on socket, a time out error is raised

  int chars = QMIN(chars_wanted,buffer.length()-thisLine);

  // is there something in the buffer still not read?
  data = buffer.mid(thisLine,chars);
  buffer.remove(0,thisLine+chars);
  thisLine = 0;

  // read more from the socket if needed
  if (chars != chars_wanted) {
    QCString newdata(SOCKET_CHUNK_SIZE);
    do {
      if (!readData(newdata)) {
          return chars;
      }
      data += newdata;
      chars += newdata.length();
    } while (chars < chars_wanted);

    // if read too much, put the rest into the buffer
    if (chars > chars_wanted) {
      buffer += data.right(chars-chars_wanted);
      data.truncate(chars_wanted);
      chars = chars_wanted;
    }
  }

  return chars;
}

bool TCPWrapper::readData(QCString &data) {
  ssize_t bytes = 0;

  if (!data.size()) {
    DBG << "readData: buffer data has size=0" << endl;
    return false;
  }

  if (data.size() > SSIZE_MAX) {
    DBG << "readData: buffer size exceeded " << SSIZE_MAX << " bytes" << endl;
    data.resize(SSIZE_MAX);
  }

  if (readyForReading()) {
    // DBG << "reading up to " << data.size()-1 << " bytes from socket" << endl;

    do {
      bytes = ::read(tcpSocket, data.data(), data.size()-1);
    } while (bytes<0 && errno==EINTR); // ignore signals

    if (bytes <= 0) { // there was an error
      DBG << "error reading from socket" << endl;
      emit error(ERR_COULD_NOT_READ,strerror(errno));
      disconnect();
      return false;
    }

    DBG << bytes << " bytes read" << endl;

    data.at(bytes) = '\0';
    // data->resize(bytes+1);
    return true;

  // was not ready for reading
  } else {
    return false;
  }
}

bool TCPWrapper::readyForReading(){
  fd_set fdsR, fdsE;
  timeval tv;

  DBG << "waiting until socket is ready for reading" << endl;

  // wait until we can read or exception or timeout
  int ret;
  do {
    FD_ZERO(&fdsR);
    FD_SET(tcpSocket, &fdsR);
    FD_ZERO(&fdsE);
    FD_SET(tcpSocket, &fdsE);
    tv.tv_sec = timeOut;
    tv.tv_usec = 0;
    ret = select(FD_SETSIZE, &fdsR, NULL, &fdsE, &tv);
    DBG << "select (r): " << ret << " errno: " << errno << endl;
  } while ((ret<0) && (errno == EINTR)); // ignore signals

  if (ret == -1) { // select failed
    emit error(ERR_CONNECTION_BROKEN, strerror(errno));
    disconnect();
    return false;
  }

  if (ret == 0) { // timeout
    emit error(ERR_SERVER_TIMEOUT, QString::null);
    disconnect();
    return false;
  }

  if (ret > 0) {
    if (FD_ISSET(tcpSocket,&fdsE)) { // exception
      emit error(ERR_CONNECTION_BROKEN, "");
      disconnect();
      return false;
    }
    if (FD_ISSET(tcpSocket,&fdsR)) { // we can read
      return true;
    }
  }

  // shouldn't get here
  emit error(ERR_INTERNAL, "");
  disconnect();
  return false;
}

bool TCPWrapper::writeData(const QCString &data) {
  ssize_t bytes;
  ssize_t byteCount = 0;

  if (readyForWriting()) {

     DBG << "writing " << data.length() << "bytes [" << data.data() << "]" << endl;

     while (byteCount < data.length()) {

       bytes = ::write(tcpSocket, &data.data()[byteCount], data.length()-byteCount);
       if (bytes <= 0) {
         DBG << "error writing to socket" << endl;
         emit error(ERR_COULD_NOT_WRITE,strerror(errno));
         disconnect();
         return false;
       } else {
          byteCount += bytes;
       }
     }

     DBG << bytes << " bytes written" << endl;

     return true;
  }

  return false;
}

bool TCPWrapper::readyForWriting() {
  fd_set fdsW, fdsE;
  timeval tv;

  // wait until we can read or exception or timeout
  int ret;
  do {
    FD_ZERO(&fdsW);
    FD_SET(tcpSocket, &fdsW);
    FD_ZERO(&fdsE);
    FD_SET(tcpSocket, &fdsE);
    tv.tv_sec = timeOut;
    tv.tv_usec = 0;
    ret = select(FD_SETSIZE, NULL, &fdsW, &fdsE, &tv);
    DBG << "select (w): " << ret << " errno: " << errno << endl;
  } while ((ret<0) && (errno == EINTR)); // ignore signals

  if (ret == -1) { // select failed
    emit error(ERR_CONNECTION_BROKEN, strerror(errno));
    disconnect();
    return false;
  }

  if (ret == 0) { // timeout
    emit error(ERR_SERVER_TIMEOUT, "");
    disconnect();
    return false;
  }

  if (ret > 0) {
    if (FD_ISSET(tcpSocket,&fdsE)) { // exception
      emit error(ERR_CONNECTION_BROKEN, "");
      disconnect();
      return false;
    }
    if (FD_ISSET(tcpSocket,&fdsW)) { // we can write
      return true;
    }
  }

  // shouldn't get here
  emit error(ERR_INTERNAL, "");
  disconnect();
  return false;
}

void TCPWrapper::setTimeOut(int tm_out) {
  timeOut = tm_out;
}

bool TCPWrapper::readLine(QCString &line) {
  // if there is a complete line in buffer, return it
  if ((nextLine=buffer.find("\r\n", thisLine)) >= 0) {
    DBG << "there is a line in buffer at " << thisLine << "..." << nextLine << endl;
    line = buffer.mid(thisLine,nextLine-thisLine+2);
    thisLine=nextLine+2;
    return true;
  }

  DBG << "need to read more data from buffer" << endl;

  // we need to read more data from socket...
  buffer.remove(0,thisLine); // delete obsoleted lines from buffer
  thisLine=0;

  QCString newdata(SOCKET_CHUNK_SIZE);
  do {
    if (!readData(newdata)) {
        return false;
    }
    buffer += newdata;

    // buffer shouldn't get too big
    if (buffer.length() > MAX_BUFFER_SIZE - SOCKET_CHUNK_SIZE) {
      error(ERR_OUT_OF_MEMORY,"line read from socket exceeds max buffer size");
      disconnect();
      return false;
    }
  } while ((nextLine=buffer.find("\r\n", thisLine)) < 0);

  // now there is a complete line in the buffer, return it
  DBG << "read line from socket, in buffer now at " << thisLine << "..." << nextLine << endl;
  line = buffer.mid(thisLine,nextLine-thisLine+2);
  thisLine=nextLine+2;
  return true;
}




