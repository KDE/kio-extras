// $Id$
/**********************************************************************
 *
 *   imap4.cc  - IMAP4rev1 KIOSlave
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

/*
  References:
    RFC 2060 - Internet Message Access Protocol - Version 4rev1 - December 1996
    RFC 2192 - IMAP URL Scheme - September 1997
    RFC 1731 - IMAP Authentication Mechanisms - December 1994
               (Discusses KERBEROSv4, GSSAPI, and S/Key)
    RFC 2195 - IMAP/POP AUTHorize Extension for Simple Challenge/Response
             - September 1997 (CRAM-MD5 authentication method)
    RFC 2104 - HMAC: Keyed-Hashing for Message Authentication - February 1997

  Supported URLs:
    imap://server/ - Prompt for user/pass, list all folders in home directory
    imap://user:pass@server/ - Uses LOGIN to log in
    imap://user;AUTH=method:pass@server/ - Uses AUTHENTICATE to log in

    imap://server/folder/ - List messages in folder
 */

#include "imap4.h"

#include "rfcdecoder.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <fcntl.h>

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <qbuffer.h>
#include <kprotocolmanager.h>
#include <ksock.h>
#include <kdebug.h>
#include <kinstance.h>
#include <kio/connection.h>
#include <kio/slaveinterface.h>
#include <kio/passdlg.h>
#include <klocale.h>

using namespace KIO;

FILE *myDebug;

extern "C"
{
  void sigalrm_handler (int);
  int kdemain (int argc, char **argv);
};

void
debugOut (FILE * myFile, const char *msg)
{
  fprintf (myFile, "%s\n", msg);
  fflush (myFile);
}

void
myHandler (QtMsgType type, const char *msg)
{
  if (&type);
  debugOut (myDebug, msg);
}

int
kdemain (int argc, char **argv)
{
  qDebug ("IMAP4::kdemain");

  KInstance instance ("kio_imap4");
  if (argc != 4)
  {
    kdDebug () << " Usage: kio_imap4 protocol domain-socket1 domain-socket2"
      << endl;
    ::exit (-1);
  }

  //set debug handler
#ifdef EBUGGING
  myDebug = fopen ("/tmp/imap_slave", "a");
  fprintf (myDebug, "Debugging\n");
  fflush (myDebug);
  qInstallMsgHandler (myHandler);
#endif

  IMAP4Protocol *slave;
  if (strcasecmp (argv[1], "imaps") == 0)
    slave = new IMAP4Protocol (argv[2], argv[3], true);
  else if (strcasecmp (argv[1], "imap") == 0)
    slave = new IMAP4Protocol (argv[2], argv[3], false);
  else
    abort ();
  slave->dispatchLoop ();
  delete slave;

#ifdef EBUGGING
  fclose (myDebug);
#endif

  return 0;
}

void
sigchld_handler (int signo)
{
  int pid, status;

  while (true && signo == SIGCHLD)
  {
    pid = waitpid (-1, &status, WNOHANG);
    if (pid <= 0)
    {
      // Reinstall signal handler, since Linux resets to default after
      // the signal occured ( BSD handles it different, but it should do
      // no harm ).
      signal (SIGCHLD, sigchld_handler);
      return;
    }
  }
}

const QString hidePass(KURL aUrl)
{
  aUrl.setPass(QString::null);
  return KURL::decode_string(aUrl.url());
}

IMAP4Protocol::IMAP4Protocol (const QCString & pool, const QCString & app, bool isSSL):TCPSlaveBase ((isSSL ? 993 : 143), (isSSL ? "imap4-ssl" : "imap4"), pool,
              app), imapParser (),
mimeIO ()
{
  readBuffer[0] = 0x00;
  readSize = 0;
  relayEnabled = false;
}

IMAP4Protocol::~IMAP4Protocol ()
{
  CloseDescriptor();
  qDebug ("IMAP4: Finishing");
}

void
IMAP4Protocol::get (const KURL & _url)
{
  qDebug ("IMAP4::get -  %s", _url.url ().latin1 ());
  QString aBox, aSequence, aType, aSection, aValidity;
  enum IMAP_TYPE aEnum =
    parseURL (_url, aBox, aSection, aType, aSequence, aValidity);

  if (aSequence.isEmpty ())
  {
    aSequence = "1:*";
  }

  imapCommand *cmd = NULL;
  if (!assureBox (aBox, true))
  {
    error (ERR_COULD_NOT_READ, hidePass(_url));
    return;
  }

#ifdef USE_VALIDITY
  if (selectInfo.uidValidityAvailable () && !aValidity.isEmpty ()
      && selectInfo.uidValidity () != aValidity.toULong ())
  {
    // this url is stale
    error (ERR_COULD_NOT_READ, hidePass(_url));
  }
  else
#endif
  {
    if (aSection.find ("STRUCTURE", 0, false) != -1)
    {
      aSection = "BODYSTRUCTURE";
    }
    else if (aSection.find ("ENVELOPE", 0, false) != -1)
    {
      aSection = "ENVELOPE";
    }
    else
    {
      aSection = "BODY.PEEK[" + aSection + "]";
    }
    if (aEnum == ITYPE_BOX)
    {
      aSection += " RFC822.SIZE INTERNALDATE FLAGS";

      // write the digest header
      outputLine
        ("Content-Type: multipart/digest; boundary=\"IMAPDIGEST\"\r\n");
      if (selectInfo.recentAvailable ())
        outputLineStr ("X-Recent: " +
                       QString ().setNum (selectInfo.recent ()) + "\r\n");
      if (selectInfo.countAvailable ())
        outputLineStr ("X-Count: " + QString ().setNum (selectInfo.count ()) +
                       "\r\n");
      if (selectInfo.unseenAvailable ())
        outputLineStr ("X-Unseen: " +
                       QString ().setNum (selectInfo.unseen ()) + "\r\n");
      if (selectInfo.uidValidityAvailable ())
        outputLineStr ("X-uidValidity: " +
                       QString ().setNum (selectInfo.uidValidity ()) +
                       "\r\n");
      if (selectInfo.flagsAvailable ())
        outputLineStr ("X-Flags: " + QString ().setNum (selectInfo.flags ()) +
                       "\r\n");
      if (selectInfo.permanentFlagsAvailable ())
        outputLineStr ("X-PermanentFlags: " +
                       QString ().setNum (selectInfo.permanentFlags ()) +
                       "\r\n");
      if (selectInfo.readWriteAvailable ())
        outputLineStr (QString ("X-Access: ") +
                       (selectInfo.readWrite ()? "Read/Write" : "Read only") +
                       "\r\n");
      outputLine ("\r\n");
    }

    if (aEnum == ITYPE_MSG)
      relayEnabled = true;

    cmd = sendCommand (imapCommand::clientFetch (aSequence, aSection));
    do
    {
      while (!parseLoop ());

      mailHeader *lastone = NULL;
      imapCache *cache;
      cache = getLastHandled ();
      if (cache)
        lastone = cache->getHeader ();

      if (!cmd->isComplete ())
      {
        qDebug ("IMAP4::get - got %p from client", lastone);
        if (lastone && ((aSection.find ("BODYSTRUCTURE", 0, false) != -1)
                        || (aSection.find ("ENVELOPE", 0, false) != -1)
                        || (aSection.find ("BODY.PEEK[0]", 0, false) != -1
                            && aEnum == ITYPE_BOX)))
        {
          if (aEnum == ITYPE_BOX)
          {
            // write the mime header (default is here message/rfc822)
            outputLine ("--IMAPDIGEST\r\n");
            if (cache->getUid () != 0)
              outputLineStr ("X-UID: " +
                             QString ().setNum (cache->getUid ()) + "\r\n");
            if (cache->getSize () != 0)
              outputLineStr ("X-Length: " +
                             QString ().setNum (cache->getSize ()) + "\r\n");
            if (cache->getDate ()->tm_year != 0)
              outputLineStr ("X-Date: " + cache->getDateStr () + "\r\n");
            if (cache->getFlags () != 0)
              outputLineStr ("X-Flags: " +
                             QString ().setNum (cache->getFlags ()) + "\r\n");
            outputLine ("\r\n");
          }
          lastone->outputPart (*this);
        }
      }
    }
    while (!cmd->isComplete ());
    if (aEnum == ITYPE_BOX)
    {
      // write the end boundary
      outputLine ("--IMAPDIGEST--\r\n");
    }

    completeQueue.removeRef (cmd);
  }

  // just to keep everybody happy when no data arrived
  data (QByteArray ());

  finished ();
  relayEnabled = false;
  qDebug ("IMAP4::get -  finished");
}

void
IMAP4Protocol::listDir (const KURL & _url)
{
  qDebug ("IMAP4::listDir - %s", _url.url ().latin1 ());

  QString myBox, mySequence, myLType, mySection, myValidity;
  enum IMAP_TYPE myType =
    parseURL (_url, myBox, mySection, myLType, mySequence, myValidity);

  if (makeLogin () && myType == ITYPE_DIR)
  {
    QString listStr = myBox;
    imapCommand *cmd;

    if (!listStr.isEmpty ())
      listStr += "/";
    listStr += "%";
//    listResponses.clear();
    cmd =
      doCommand (imapCommand::clientList ("", listStr, myLType == "LSUB"));
    if (cmd->result () == "OK")
    {
      QString mailboxName;
      UDSEntry entry;
      UDSAtom atom;
      KURL aURL = _url;

      qDebug ("IMAP4Protocol::listDir - got %d", listResponses.count ());

      for (QValueListIterator < imapList > it = listResponses.begin ();
           it != listResponses.end (); ++it)
        doListEntry (_url, myBox, (*it));
      entry.clear ();
      listEntry (entry, true);
    }
    else
    {
      error (ERR_CANNOT_ENTER_DIRECTORY, hidePass(_url));
    }
    completeQueue.removeRef (cmd);

  }
  else if (makeLogin () && myType == ITYPE_BOX)
  {
    if (!_url.query ().isEmpty ())
    {
      QString query = KURL::decode_string (_url.query ());
      query = query.right (query.length () - 1);
      if (!query.isEmpty ())
      {
        imapCommand *cmd = NULL;

        if (assureBox (myBox, true))
        {
          cmd = doCommand (imapCommand::clientSearch (query));
          completeQueue.removeRef (cmd);

          QStringList list = getResults ();
          int stretch = 0;

          if (selectInfo.uidNextAvailable ())
            stretch = QString ().setNum (selectInfo.uidNext ()).length ();
          UDSEntry entry;
          mailHeader *lastone = NULL;
          imapCache *cache;
          mailHeader fake;

//          qDebug("SEARCH returned - %d",list.count());
          for (QStringList::Iterator it = list.begin (); it != list.end ();
               ++it)
          {
//            qDebug("SEARCH processing - %s",(*it).latin1());

            // get the cached entry
            cache = getUid ((*it));
            if (cache)
              lastone = cache->getHeader ();
            else
              lastone = NULL;

            // if the uid is not in the cache we fake an entry
            if (!lastone)
            {
//              qDebug("SEARCH faking - %s",(*it).latin1());
              fake.setPartSpecifier ((*it));
              lastone = &fake;
            }
            doListEntry (_url, lastone, stretch);
          }
          entry.clear ();
          listEntry (entry, true);
        }
      }
    }
    else
    {

//      imapCommand *cmd = NULL;
      if (assureBox (myBox, true))
      {
        qDebug ("IMAP4: select returned:");
        if (selectInfo.recentAvailable ())
          qDebug ("Recent: %ld", selectInfo.recent ());
        if (selectInfo.countAvailable ())
          qDebug ("Count: %ld", selectInfo.count ());
        if (selectInfo.unseenAvailable ())
          qDebug ("Unseen: %ld", selectInfo.unseen ());
        if (selectInfo.uidValidityAvailable ())
          qDebug ("uidValidity: %ld", selectInfo.uidValidity ());
        if (selectInfo.flagsAvailable ())
          qDebug ("Flags: %ld", selectInfo.flags ());
        if (selectInfo.permanentFlagsAvailable ())
          qDebug ("PermanentFlags: %ld", selectInfo.permanentFlags ());
        if (selectInfo.readWriteAvailable ())
          qDebug ("Access: %s",
                  selectInfo.readWrite ()? "Read/Write" : "Read only");

#ifdef USE_VALIDITY
        if (selectInfo.uidValidityAvailable ()
            && selectInfo.uidValidity () != myValidity.toULong ())
        {
          //redirect
          KURL newUrl = _url;

          newUrl.setPath ("/" + myBox + ";UIDVALIDITY=" +
                          QString ().setNum (selectInfo.uidValidity ()));
          qDebug ("IMAP4::listDir - redirecting to %s",
                  newUrl.url ().latin1 ());
          redirection (newUrl);


        }
        else
#endif
        if (selectInfo.count () > 0)
        {
          int stretch = 0;

          if (selectInfo.uidNextAvailable ())
            stretch = QString ().setNum (selectInfo.uidNext ()).length ();
          //        qDebug("%ld used to stretch %d",selectInfo.uidNext(),stretch);
          UDSEntry entry;

          if (mySequence.isEmpty ())
            mySequence = "1:*";
          imapCommand *fetch =
            sendCommand (imapCommand::
                         clientFetch (mySequence, "UID RFC822.SIZE"));
          do
          {
            while (!parseLoop ());

            mailHeader *lastone;
            imapCache *cache;
            cache = getLastHandled ();
            if (cache)
              lastone = cache->getHeader ();
            else
              lastone = NULL;

            if (!fetch->isComplete ())
            {
              doListEntry (_url, lastone, stretch);
            }
          }
          while (!fetch->isComplete ());
          entry.clear ();
          listEntry (entry, true);
        }
      }
      else
      {
        error (ERR_CANNOT_ENTER_DIRECTORY, hidePass(_url));
      }
//      completeQueue.removeRef(cmd);
    }
  }
  else
  {
    error (ERR_CANNOT_ENTER_DIRECTORY, hidePass(_url));
  }

  qDebug ("IMAP4Protcol::listDir - Finishing listDir");
  finished ();
}

void
IMAP4Protocol::setHost (const QString & _host, int _port,
                        const QString & _user, const QString & _pass)
{
  qDebug ("IMAP4::setHost - host= %s, port= %d, user= %s, pass=xx",
          _host.latin1 (), _port, _user.latin1 ());

  if (myHost != _host || myPort != _port || myUser != _user)
  {
    if (!myHost.isEmpty ())
      closeConnection ();

    if (ConnectToHost (_host.ascii (), _port))
    {

      fcntl (m_iSock, F_SETFL, (fcntl (m_iSock, F_GETFL) | O_NDELAY));

//WORK      myState = ISTATE_CONNECT;
      myHost = _host;
      myUser = _user.left (_user.find (";AUTH=", 0, false));
      myPass = _pass;

      if (_user.find (";AUTH=", 0, false) != -1)
        myAuth =
          _user.right (_user.length () - _user.find (";AUTH=", 0, false) - 6);
      qDebug
        ("IMAP4::setHost - host= %s, port= %d, user= %s, auth= %s, pass=xx",
         _host.latin1 (), _port, _user.latin1 (), myAuth.latin1 ());

      imapCommand *cmd;

      while (!parseLoop ());    //get greeting
      unhandled.clear ();       //get rid of it
      cmd = doCommand (new imapCommand ("CAPABILITY", ""));

      qDebug ("IMAP4: setHost: capability");
      for (QStringList::Iterator it = imapCapabilities.begin ();
           it != imapCapabilities.end (); ++it)
      {
        qDebug ("'%s'", (*it).latin1 ());
      }

      completeQueue.removeRef (cmd);
    }
    else
    {
      qDebug ("IMAP4: setHost: ConnectToHost Failed!");
      myHost = QString::null;
      myUser = QString::null;
      myPass = QString::null;
      myAuth = QString::null;
    }
  }
  else
  {
    qDebug ("IMAP4: setHost: reusing connection");
  }
//  parseLoop();
}

void
IMAP4Protocol::parseRelay (const QByteArray & buffer)
{
  if (relayEnabled)
    data (buffer);
}

void
IMAP4Protocol::parseRelay (ulong len)
{
  if (relayEnabled)
    totalSize (len);
}


void
IMAP4Protocol::parseReadLine (QByteArray & buffer, ulong relay)
{
  char buf[1024];
//  struct timeval m_tTimeout = {1, 0};
  fd_set FDs;
  ulong readLen;

  FD_ZERO (&FDs);
  FD_SET (m_iSock, &FDs);

  errno = 0;
  while (1)
  {
    memset (&buf, sizeof (buf), 0);
    if ((readLen = ReadLine (buf, sizeof (buf) - 1)) == 0)
    {
//        if (ferror(fp)) {
      //TODO: act on loss of connection
      if (buffer.isEmpty () || buffer[buffer.size () - 1] == '\n')
        break;
      /*        qDebug("IMAP4: Error while freading something[%d]: %s",errno,strerror(errno));
         return;
         } else {
         debug("IMAP4: Attempting select");
         while (::select(m_iSock+1, &FDs, 0, 0, &m_tTimeout) ==0) {
         debug("IMAP4: Sleeping");
         sleep(10);
         }
         debug("IMAP4: Finished select"); */
//        }  
    }
    else
    {
      if (relay > 0)
      {
        QByteArray relayData;

        if (readLen < relay)
          relay = readLen;
        relayData.setRawData (buf, relay);
        parseRelay (relayData);
        relayData.resetRawData (buf, relay);
        qDebug ("relayed : %ld", relay);
      }
      // append to buffer
      {
        QBuffer stream (buffer);

        stream.open (IO_WriteOnly);
        stream.at (buffer.size ());
        stream.writeBlock (buf, readLen);
        stream.close ();
//        qDebug("appended %ld got now %d",readLen,buffer.size());
      }
      if (buffer[buffer.size () - 1] != '\n')
        qDebug ("************************** Partial filled buffer");
      else
        break;
    }
    sleep (1);
  }
}


void
IMAP4Protocol::mimetype (const KURL & _url)
{
  qDebug ("IMAP4::mimetype - %s", _url.url ().latin1 ());
  QString aBox, aSequence, aType, aSection, aValidity;

  mimeType (getMimeType
            (parseURL (_url, aBox, aSection, aType, aSequence, aValidity)));
  finished ();
}

void
IMAP4Protocol::setSubURL (const KURL & _url)
{
  qDebug ("IMAP4::setSubURL - %s", _url.url ().latin1 ());
  KIO::TCPSlaveBase::setSubURL (_url);
}

void
IMAP4Protocol::put (const KURL & _url, int, bool, bool)
{
  qDebug ("IMAP4::put - %s", _url.url ().latin1 ());
//  KIO::TCPSlaveBase::put(_url,permissions,overwrite,resume);
  QString aBox, aSequence, aLType, aSection, aValidity;
  enum IMAP_TYPE aType =
    parseURL (_url, aBox, aSection, aLType, aSequence, aValidity);

  // see if it is a box
  if (aType != ITYPE_BOX)
  {
    if (aBox[aBox.length () - 1] == '/')
      aBox = aBox.right (aBox.length () - 1);
    imapCommand *cmd = doCommand (imapCommand::clientCreate (aBox));

    if (cmd->result () != "OK")
      error (ERR_COULD_NOT_WRITE, hidePass(_url));
    completeQueue.removeRef (cmd);
  }
  else
  {
    QList < QByteArray > bufferList;
    int length = 0;

    int result;
    // Loop until we got 'dataEnd'
    do
    {
      QByteArray *buffer = new QByteArray ();
      dataReq ();               // Request for data
      result = readData (*buffer);
      if (result > 0)
      {
        bufferList.append (buffer);
        length += result;
      }
    }
    while (result > 0);

    if (result != 0)
    {
      error (ERR_ABORTED, hidePass(_url));
      finished ();
      return;
    }

    imapCommand *cmd =
      sendCommand (imapCommand::clientAppend (aBox, "", length));
    while (!parseLoop ());

    // see if server is waiting
    if (!cmd->isComplete () && !getContinuation ().isEmpty ())
    {
      bool sendOk = true;
      ulong wrote = 0;

      QByteArray *buffer;
      while (!bufferList.isEmpty () && sendOk)
      {
        buffer = bufferList.take (0);

        sendOk =
          (Write (buffer->data (), buffer->size ()) ==
           (ssize_t) buffer->size ());
        wrote += buffer->size ();
        delete buffer;
        if (!sendOk)
        {
          error (ERR_CONNECTION_BROKEN, myHost);
        }
      }
      parseWriteLine ("");
      while (!cmd->isComplete ())
        parseLoop ();
      if (cmd->result () != "OK")
        error (ERR_COULD_NOT_WRITE, myHost);
      else
      {
        // MUST reselect to get the new message
        if (aBox == getCurrentBox ())
        {
          cmd =
            doCommand (imapCommand::
                       clientSelect (aBox, !selectInfo.readWrite ()));
          completeQueue.removeRef (cmd);
        }
      }
    }
    else
    {
      error (ERR_COULD_NOT_WRITE, myHost);
    }

    completeQueue.removeRef (cmd);
  }

  finished ();
}

void
IMAP4Protocol::mkdir (const KURL & _url, int)
{
  qDebug ("IMAP4::mkdir - %s", _url.url ().latin1 ());
//  KIO::TCPSlaveBase::mkdir(_url,permissions);
  QString aBox, aSequence, aLType, aSection, aValidity;
  parseURL (_url, aBox, aSection, aLType, aSequence, aValidity);
  if (aBox[aBox.length () - 1] != '/')
    aBox += "/";
  imapCommand *cmd = doCommand (imapCommand::clientCreate (aBox));

  if (cmd->result () != "OK")
    error (ERR_COULD_NOT_MKDIR, hidePass(_url));

  completeQueue.removeRef (cmd);
  finished ();
}

void
IMAP4Protocol::copy (const KURL & src, const KURL & dest, int, bool overwrite)
{
  qDebug ("IMAP4::copy - [%s] %s -> %s",
          overwrite ? "Overwrite" : "NoOverwrite", src.url ().latin1 (),
          dest.url ().latin1 ());
  QString sBox, sSequence, sLType, sSection, sValidity;
  QString dBox, dSequence, dLType, dSection, dValidity;
  enum IMAP_TYPE sType =
    parseURL (src, sBox, sSection, sLType, sSequence, sValidity);
  enum IMAP_TYPE dType =
    parseURL (dest, dBox, dSection, dLType, dSequence, dValidity);

  // see if we have to create anything
  if (dType != ITYPE_BOX)
  {
    // this might be konqueror
    int sub = dBox.find (sBox);

    // might be moving to upper folder
    if (sub > 0)
    {
      KURL testDir = dest;

      QString subDir = dBox.right (dBox.length () - dBox.findRev ("/"));
      QString topDir = dBox.left (sub);
      testDir.setPath ("/" + topDir);
      dType =
        parseURL (testDir, topDir, dSection, dLType, dSequence, dValidity);

      qDebug ("IMAP4::copy - checking this destination %s", topDir.latin1 ());
      // see if this is what the user wants
      if (dType == ITYPE_BOX)
      {
        qDebug ("IMAP4::copy - assuming this destination %s",
                topDir.latin1 ());
        dBox = topDir;
      }
      else
      {

        // maybe if we create a new mailbox
        topDir = "/" + topDir + subDir;
        testDir.setPath (topDir);
        qDebug ("IMAP4::copy - checking this destination %s",
                topDir.latin1 ());
        dType =
          parseURL (testDir, topDir, dSection, dLType, dSequence, dValidity);
        if (dType != ITYPE_BOX)
        {
          // ok then we'll create a mailbox
          imapCommand *cmd = doCommand (imapCommand::clientCreate (topDir));

          // on success we'll use it, else we'll just try to create the given dir
          if (cmd->result () == "OK")
          {
            qDebug ("IMAP4::copy - assuming this destination %s",
                    topDir.latin1 ());
            dType = ITYPE_BOX;
            dBox = topDir;
          }
          else
          {
            completeQueue.removeRef (cmd);
            cmd = doCommand (imapCommand::clientCreate (dBox));
            if (cmd->result () == "OK")
              dType = ITYPE_BOX;
            else
              error (ERR_COULD_NOT_WRITE, hidePass(dest));
          }
          completeQueue.removeRef (cmd);
        }
      }

    }
  }
  if (sType == ITYPE_MSG)
  {
    //select the source box
    if (assureBox (sBox, true))
    {
      qDebug ("IMAP4::copy - %s -> %s", sBox.latin1 (), dBox.latin1 ());

      //issue copy command
      imapCommand *cmd =
        doCommand (imapCommand::clientCopy (dBox, sSequence));
      if (cmd->result () != "OK")
        error (ERR_COULD_NOT_WRITE, hidePass(dest));
      completeQueue.removeRef (cmd);

    }
    else
    {
      error (ERR_ACCESS_DENIED, hidePass(src));
    }
  }
  else
  {
    error (ERR_ACCESS_DENIED, hidePass(src));
  }
  finished ();
}

void
IMAP4Protocol::del (const KURL & _url, bool isFile)
{
  qDebug ("IMAP4::del - [%s] %s", isFile ? "File" : "NoFile",
          _url.url ().latin1 ());
  QString aBox, aSequence, aLType, aSection, aValidity;
  enum IMAP_TYPE aType =
    parseURL (_url, aBox, aSection, aLType, aSequence, aValidity);

  switch (aType)
  {
  case ITYPE_BOX:
    if (!aSequence.isEmpty ())
    {
      if (aSequence == "*")
      {
        if (assureBox (aBox, false))
        {
          imapCommand *cmd = doCommand (imapCommand::clientExpunge ());
          if (cmd->result () != "OK")
            error (ERR_CANNOT_DELETE, hidePass(_url));
          completeQueue.removeRef (cmd);
        }
        else error (ERR_CANNOT_DELETE, hidePass(_url));
      }
      else
      {
        // if open for read/write 
        if (assureBox (aBox, false))
        {
          imapCommand *cmd =
            doCommand (imapCommand::
                       clientStore (aSequence, "+FLAGS", "\\DELETED"));
          if (cmd->result () != "OK")
            error (ERR_CANNOT_DELETE, hidePass(_url));
          completeQueue.removeRef (cmd);
        }
        else
          error (ERR_CANNOT_DELETE, hidePass(_url));
      }
    }
    else
    {
      //TODO delete the mailbox
    }
    break;

  case ITYPE_DIR:
    {
      imapCommand *cmd = doCommand (imapCommand::clientDelete (aBox));
      if (cmd->result () != "OK")
        error (ERR_COULD_NOT_RMDIR, hidePass(_url));
      completeQueue.removeRef (cmd);
    }
    break;

  case ITYPE_MSG:
    {
      // if open for read/write 
      if (assureBox (aBox, false))
      {
        imapCommand *cmd =
          doCommand (imapCommand::
                     clientStore (aSequence, "+FLAGS", "\\DELETED"));
        if (cmd->result () != "OK")
          error (ERR_CANNOT_DELETE, hidePass(_url));
        completeQueue.removeRef (cmd);
      }
      else
        error (ERR_CANNOT_DELETE, hidePass(_url));
    }
    break;

  case ITYPE_UNKNOWN:
    error (ERR_CANNOT_DELETE, hidePass(_url));
    break;
  }
  finished ();
}

void
IMAP4Protocol::rename (const KURL & src, const KURL & dest, bool overwrite)
{
  qDebug ("IMAP4::rename - [%s] %s -> %s",
          overwrite ? "Overwrite" : "NoOverwrite", src.url ().latin1 (),
          dest.url ().latin1 ());
  QString sBox, sSequence, sLType, sSection, sValidity;
  QString dBox, dSequence, dLType, dSection, dValidity;
  enum IMAP_TYPE sType =
    parseURL (src, sBox, sSection, sLType, sSequence, sValidity);
  enum IMAP_TYPE dType =
    parseURL (dest, dBox, dSection, dLType, dSequence, dValidity);

  if (dType == ITYPE_UNKNOWN)
  {
    switch (sType)
    {
    case ITYPE_BOX:
    case ITYPE_DIR:
      {
        imapCommand *cmd = doCommand (imapCommand::clientRename (sBox, dBox));
        if (cmd->result () != "OK")
          error (ERR_CANNOT_RENAME, cmd->result ());
        completeQueue.removeRef (cmd);
      }
      break;

    case ITYPE_MSG:
    case ITYPE_UNKNOWN:
      error (ERR_CANNOT_RENAME, hidePass(src));
      break;
    }
  }
  else
  {
    error (ERR_CANNOT_RENAME, hidePass(src));
  }
  finished ();
}

void
IMAP4Protocol::slave_status ()
{
  qDebug ("IMAP4::slave_status");
//  KIO::TCPSlaveBase::slave_status();
  slaveStatus (myHost, !(getState () == ISTATE_NO));
//  slaveStatus(QString::null,false);
}

void
IMAP4Protocol::dispatch (int command, const QByteArray & data)
{
  qDebug ("IMAP4::dispatch - command=%d", command);
  KIO::TCPSlaveBase::dispatch (command, data);
}

void
IMAP4Protocol::stat (const KURL & _url)
{
  qDebug ("IMAP4::stat - %s", _url.url ().latin1 ());
  QString aBox, aSequence, aLType, aSection, aValidity;
  enum IMAP_TYPE aType =
    parseURL (_url, aBox, aSection, aLType, aSequence, aValidity);

  if (aType == ITYPE_BOX || aType == ITYPE_MSG)
  {
    ulong validity = 0;
    // see if the box is already in select/examine state
    if (aBox == getCurrentBox ())
      validity = selectInfo.uidValidity ();
    else
    {
      // do a status lookup on the box
      // only do this if the box is not selected
      // the server might change the validity for new select/examine
      imapCommand *cmd =
        doCommand (imapCommand::clientStatus (aBox, "UIDVALIDITY"));
      completeQueue.removeRef (cmd);
      validity = getStatus ().uidValidity ();
    }
    validity = 0;               // temporary

    if (aType == ITYPE_BOX)
    {
      // has no or an invalid uidvalidity
      if (validity > 0 && validity != aValidity.toULong ())
      {
        //redirect
        KURL newUrl = _url;

        newUrl.setPath ("/" + aBox + ";UIDVALIDITY=" +
                        QString ().setNum (validity));
        qDebug ("IMAP4::stat - redirecting to %s", newUrl.url ().latin1 ());
        redirection (newUrl);
      }
    }
    else if (aType == ITYPE_MSG)
    {
      //must determine if this message exists
      //cause konqueror will check this on paste operations

      // has an invalid uidvalidity
      // or no messages in box
      if (validity > 0 && validity != aValidity.toULong ())
      {
        aType = ITYPE_UNKNOWN;
        qDebug ("IMAP4::stat - url has invalid validity [%ld] %s", validity,
                _url.url ().latin1 ());
      }
    }
  }


  UDSEntry entry;
  UDSAtom atom;

  atom.m_uds = UDS_NAME;
  atom.m_str = aBox;
  entry.append (atom);

  atom.m_uds = UDS_MIME_TYPE;
  atom.m_str = getMimeType (aType);
  entry.append (atom);

  qDebug ("IMAP4: stat: %s", atom.m_str.latin1 ());
  switch (aType)
  {
  case ITYPE_DIR:
    atom.m_uds = UDS_FILE_TYPE;
    atom.m_str = "";
    atom.m_long = S_IFDIR;
    entry.append (atom);
    break;

  case ITYPE_BOX:
    atom.m_uds = UDS_FILE_TYPE;
    atom.m_str = "";
    atom.m_long = S_IFDIR;
    entry.append (atom);
    break;

  case ITYPE_MSG:
    atom.m_uds = UDS_FILE_TYPE;
    atom.m_str = "";
    atom.m_long = S_IFREG;
    entry.append (atom);
    break;

  case ITYPE_UNKNOWN:
    error (ERR_DOES_NOT_EXIST, hidePass(_url));
    break;
  }



  statEntry (entry);
  qDebug ("IMAP4::stat - Finishing stat");
  finished ();
}

void IMAP4Protocol::openConnection()
{
  if (makeLogin()) connected();
}

bool IMAP4Protocol::makeLogin ()
{
  bool skipFirst = true;

  qDebug ("IMAP4::makeLogin - checking login");
  if (getState () == ISTATE_LOGIN || getState () == ISTATE_SELECT)
    return true;

  if (!myAuth.isEmpty () && myAuth != "*"
      && !hasCapability (QString ("AUTH=") + myAuth))
  {
    error (ERR_UNSUPPORTED_PROTOCOL, myAuth);
    return false;
  }

  qDebug ("IMAP4::makeLogin - attempting login");

  if (myUser.isEmpty () || myPass.isEmpty ())
    skipFirst = false;

  while (skipFirst
         ||
         openPassDlg (i18n ("Username and password for your IMAP account:"),
                      myUser, myPass))
  {

    qDebug ("IMAP4::makeLogin - open_PassDlg: user=%s pass=xx",
            myUser.latin1 ());
    skipFirst = false;

    if (myAuth.isEmpty () || myAuth == "*")
    {
      if (clientLogin (myUser, myPass))
      {
        qDebug ("IMAP4::makeLogin - login succeded");
      }
      else
        qDebug ("IMAP4::makeLogin - login failed");
    }
    else
    {
      if (clientAuthenticate (myUser, myPass, myAuth))
      {
        qDebug ("IMAP4::makeLogin: %s succeded", myAuth.latin1 ());
      }
      else
        qDebug ("IMAP4::makeLogin: %s failed", myAuth.latin1 ());
    }
    if (getState () == ISTATE_LOGIN)
      break;
  }



  if (getState () == ISTATE_LOGIN)
    return true;

  return false;
}

void
IMAP4Protocol::parseWriteLine (const QString & aStr)
{
  QCString writer = aStr.utf8 ();

  // append CRLF if necessary
  if (writer[writer.length () - 1] != '\n')
    writer += "\r\n";

  // write it
  Write (writer.data (), writer.length ());
}

QString
IMAP4Protocol::getMimeType (enum IMAP_TYPE aType)
{
  QString retVal = "unknown/unknown";

  switch (aType)
  {
  case ITYPE_UNKNOWN:
    retVal = "unknown/unknown";
    break;

  case ITYPE_DIR:
    retVal = "inode/directory";
    break;

  case ITYPE_BOX:
    retVal = "message/digest";
    break;

  case ITYPE_MSG:
    retVal = "message/rfc822-imap";
    break;
  }

  return retVal;
}



void
IMAP4Protocol::doListEntry (const KURL & _url, mailHeader * what, int stretch)
{
  if (what)
  {
    UDSEntry entry;
    UDSAtom atom;
    KURL aURL = _url;
    aURL.setQuery (QString::null);

    entry.clear ();

    atom.m_uds = UDS_NAME;
    atom.m_str = what->getPartSpecifier ();
    atom.m_long = 0;
    if (stretch > 0)
    {
      atom.m_str = "0000000000000000" + atom.m_str;
      atom.m_str = atom.m_str.right (stretch);
    }
//    atom.m_str.prepend(";UID=");
    entry.append (atom);

    atom.m_uds = UDS_URL;
    atom.m_str = aURL.url ();
    if (atom.m_str[atom.m_str.length () - 1] != '/')
      atom.m_str += "/";
    atom.m_str += ";UID=" + what->getPartSpecifier ();
    atom.m_long = 0;
    entry.append (atom);

    atom.m_uds = UDS_FILE_TYPE;
    atom.m_str = "";
    atom.m_long = S_IFREG;
    entry.append (atom);

    atom.m_uds = UDS_SIZE;
    atom.m_long = what->getLength ();
    entry.append (atom);
    atom.m_uds = UDS_MIME_TYPE;
    atom.m_str = "message/rfc822-imap";
    atom.m_long = 0;
    entry.append (atom);

    atom.m_uds = UDS_USER;
    atom.m_str = myUser;
    entry.append (atom);

    atom.m_uds = KIO::UDS_ACCESS;
    atom.m_long = S_IRUSR | S_IXUSR | S_IWUSR;
    entry.append (atom);

    listEntry (entry, false);
  }
}

void
IMAP4Protocol::doListEntry (const KURL & _url, const QString & myBox,
                            const imapList & item)
{
  KURL aURL = _url;
  aURL.setQuery (QString::null);
  UDSEntry entry;
  UDSAtom atom;

  {
    QString mailboxName = item.name ();

    // some beautification
    if (mailboxName.find (myBox) == 0)
    {
      mailboxName =
        mailboxName.right (mailboxName.length () - myBox.length ());
    }
    if (mailboxName[0] == '/')
      mailboxName = mailboxName.right (mailboxName.length () - 1);

    atom.m_uds = UDS_NAME;
    atom.m_str = mailboxName;

    // konqueror will die with an assertion failure otherwise
    if (atom.m_str.isEmpty ())
      atom.m_str = "..";

    if (!atom.m_str.isEmpty ())
    {
      atom.m_long = 0;
      entry.append (atom);


      if (!item.noSelect ())
      {
        atom.m_uds = UDS_MIME_TYPE;
        atom.m_str = "message/digest";
        atom.m_long = 0;
        entry.append (atom);
        mailboxName += "/";
      }
      else if (!item.noInferiors ())
      {
        atom.m_uds = UDS_MIME_TYPE;
        atom.m_str = "inode/directory";
        atom.m_long = 0;
        entry.append (atom);
        mailboxName += "/";
      }
      else
      {
        atom.m_uds = UDS_MIME_TYPE;
        atom.m_str = "unknown/unknown";
        atom.m_long = 0;
        entry.append (atom);
      }

      atom.m_uds = UDS_URL;
      atom.m_str = aURL.url ();
      if (atom.m_str[atom.m_str.length () - 1] != '/')
        atom.m_str += "/";
      atom.m_str += mailboxName;
      atom.m_long = 0;
      entry.append (atom);

      atom.m_uds = UDS_USER;
      atom.m_str = myUser;
      entry.append (atom);

      atom.m_uds = KIO::UDS_ACCESS;
      atom.m_long = S_IRUSR | S_IXUSR | S_IWUSR;
      entry.append (atom);

      listEntry (entry, false);
    }
  }
}

enum IMAP_TYPE
IMAP4Protocol::parseURL (const KURL & _url, QString & _box,
                         QString & _section, QString & _type, QString & _uid,
                         QString & _validity)
{
//  qDebug("IMAP4::parseURL - %s",_url.url().latin1());
  enum IMAP_TYPE retVal;
  retVal = ITYPE_UNKNOWN;

  imapParser::parseURL (_url, _box, _section, _type, _uid, _validity);
//  qDebug("URL: query - '%s'",KURL::decode_string(_url.query()).latin1());

  if (!_box.isEmpty ())
  {
//     qDebug("IMAP4::parseURL: box %s",_box.latin1());

    if (makeLogin ())
    {
      if (getCurrentBox () != _box)
      {
        imapCommand *cmd;

        cmd = doCommand (imapCommand::clientList ("", _box));
        if (cmd->result () == "OK")
        {
          for (QValueListIterator < imapList > it = listResponses.begin ();
               it != listResponses.end (); ++it)
          {
//            qDebug("IMAP4::parseURL - checking %s to %s",_box.latin1(),(*it).name().latin1());
            if (_box == (*it).name ())
            {
              if ((*it).noSelect ())
              {
                retVal = ITYPE_DIR;
              }
              else
              {
                retVal = ITYPE_BOX;
              }
            }
          }
        }
        completeQueue.removeRef (cmd);
      }
      else
      {
        retVal = ITYPE_BOX;
      }
    }
    else
      qDebug ("IMAP4::parseURL: no login!");

  }
  else
  {
    qDebug ("IMAP4: parseURL: box [root]");
    retVal = ITYPE_DIR;
  }

  //see if it is a real sequence or a simple uid
  if (retVal == ITYPE_BOX)
  {
    if (!_uid.isEmpty ())
    {
      if (_uid.find (":") == -1 && _uid.find (",") == -1
          && _uid.find ("*") == -1)
        retVal = ITYPE_MSG;
    }
  }
  switch (retVal)
  {
  case ITYPE_DIR:
    qDebug ("IMAP4::parseURL: dir");
    break;

  case ITYPE_BOX:
    qDebug ("IMAP4::parseURL: box");
    break;

  case ITYPE_MSG:
    qDebug ("IMAP4::parseURL: msg");
    break;

  case ITYPE_UNKNOWN:
    qDebug ("IMAP4::parseURL: unknown");
    break;
  }
  qDebug ("URL: box= %s, section= %s, type= %s, uid= %s, validity= %s",
          _box.latin1 (), _section.latin1 (), _type.latin1 (), _uid.latin1 (),
          _validity.latin1 ());

  return retVal;
}

int
IMAP4Protocol::outputLine (const QCString & _str)
{
  QByteArray temp;
  bool relay = relayEnabled;

  relayEnabled = true;
  temp.setRawData (_str.data (), _str.length ());
  parseRelay (temp);
  temp.resetRawData (_str.data (), _str.length ());

  relayEnabled = relay;
  return 0;
}

/* memccpy appeared first in BSD4.4 */
void *
mymemccpy (void *dest, const void *src, int c, size_t n)
{
  char *d = (char *) dest;
  const char *s = (const char *) src;

  while (n-- > 0)
    if ((*d++ = *s++) == c)
      return d;

  return NULL;
}

ssize_t
IMAP4Protocol::ReadLine (char *buf, ssize_t len)
{
  ssize_t result;
  char *copied;
//  qDebug("Request for %d",len);
  // see what is still in the buffer
  if (len > readSize)
  {
//    qDebug("Reading");
    // append to our internal buffer
    result = Read (readBuffer + readSize, IMAP_BUFFER - readSize);
    if (result > 0)
      readSize += result;
//    qDebug("Result is %d",result);
//    qDebug("Now got %d",readSize);
  }

  // give what is there to the caller
  if (readSize < len)
    len = readSize;

  if (len > 0)
  {
    // copy it to the destination
//    qDebug("Giving to caller at most %d",len);
    copied = (char *) mymemccpy (buf, readBuffer, '\n', len);
    if (copied)
      len = copied - buf;
//    qDebug("Copied %d",len);
    buf[len] = 0x00;
//    qDebug("Giving to caller %d",len);
//    qDebug("That is '%s'",buf);

    // now we need to readjust our buffer
    memcpy (readBuffer, readBuffer + len, readSize - len);
    readSize -= len;
    readBuffer[readSize] = 0x00;
//    qDebug("Keeping %d [%s]",readSize,readBuffer);
  }

  if (len <= 0)
    len = 0;
  return len;
}

bool
IMAP4Protocol::assureBox (const QString & aBox, bool readonly)
{
  imapCommand *cmd = NULL;

  if (aBox != getCurrentBox ())
  {
    // open the box with the appropriate mode
    qDebug ("IMAP4Protocol::assureBox - opening box");
    cmd = doCommand (imapCommand::clientSelect (aBox, readonly));
    completeQueue.removeRef (cmd);
  }
  else
  {
    // check if it is the mode we want
    if (getSelected ().readWrite () || readonly)
    {
      // give the server a chance to deliver updates
      qDebug ("IMAP4Protocol::assureBox - reusing box");
      cmd = doCommand (imapCommand::clientNoop ());
      completeQueue.removeRef (cmd);
    }
    else
    {
      // reopen the box with the appropriate mode
      qDebug ("IMAP4Protocol::assureBox - reopening box");
      cmd = doCommand (imapCommand::clientSelect (aBox, readonly));
      completeQueue.removeRef (cmd);
    }
  }

  // if it isn't opened
  if (aBox != getCurrentBox ())
    return false;

  // if it is the mode we want
  if (getSelected ().readWrite () || readonly)
    return true;

  // we goofed somewhere
  return false;
}
