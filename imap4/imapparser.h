#ifndef _IMAPPARSER_H
#define _IMAPPARSER_H "$Id: imapparser.h,v 1.0 2000/12/04"
/**********************************************************************
 *
 *   imapparser.h  - IMAP4rev1 Parser
 *   Copyright (C) 2001-2002 Michael Haeckel <haeckel@kde.org>
 *   Copyright (C) 2000 s.carstens@gmx.de
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
 *   Send comments and bug fixes to s.carstens@gmx.de
 *
 *********************************************************************/

#include <qstringlist.h>
#include <qvaluelist.h>
#include <qptrlist.h>
#include <qdict.h>

#include "imaplist.h"
#include "imapcommand.h"
#include "imapinfo.h"

#include "mailheader.h"

class KURL;
class QString;
class mailAddress;
class mimeHeader;

class parseString
{
public:
  parseString() { pos = 0; }
  char operator[](uint i) { return data[i + pos]; }
  bool isEmpty() { return pos >= data.size(); }
  QCString cstr()
  {
    if (pos >= data.size()) return QCString();
    return QCString(data.data() + pos, data.size() - pos + 1);
  }
  int find(char c, int index = 0)
  {
    int res = data.find(c, index + pos);
    return (res == -1) ? res : (res - pos);
  }
  QCString left(uint len)
  {
    return QCString(data.data() + pos, len + 1);
  }
  void clear()
  {
    data.resize(0);
    pos = 0;
  }
  uint length()
  {
    return data.size() - pos;
  }
  void fromString(const QString &s)
  {
    clear();
    data.duplicate(s.latin1(), s.length());
  }
  QByteArray data;
  uint pos;
};

class imapCache
{
public:
  imapCache ()
  {
    myHeader = NULL;
    mySize = 0;
    myFlags = 0;
    myUid = 0;
  }

  ~imapCache ()
  {
    if (myHeader) delete myHeader;
  }

  mailHeader *getHeader ()
  {
    return myHeader;
  }
  void setHeader (mailHeader * inHeader)
  {
    myHeader = inHeader;
  }

  ulong getSize ()
  {
    return mySize;
  }
  void setSize (ulong inSize)
  {
    mySize = inSize;
  }

  ulong getUid ()
  {
    return myUid;
  }
  void setUid (ulong inUid)
  {
    myUid = inUid;
  }

  ulong getFlags ()
  {
    return myFlags;
  }
  void setFlags (ulong inFlags)
  {
    myFlags = inFlags;
  }

  QCString getDate ()
  {
    return myDate;
  }
  void setDate (const QCString & _str)
  {
    myDate = _str;
  }
  void clear()
  {
    if (myHeader) delete myHeader;
    myHeader = NULL;
    mySize = 0;
    myFlags = 0;
    myDate = QCString();
    myUid = 0;
  }

protected:
  mailHeader * myHeader;
  ulong mySize;
  ulong myFlags;
  ulong myUid;
  QCString myDate;
};


class imapParser
{

public:

  // the different states the client can be in
  enum IMAP_STATE
  {
    ISTATE_NO,
    ISTATE_CONNECT,
    ISTATE_LOGIN,
    ISTATE_SELECT
  };

public:
    imapParser ();
    virtual ~ imapParser ();

  virtual enum IMAP_STATE getState () { return currentState; }
  virtual void setState(enum IMAP_STATE state) { currentState = state; }

  const QString getCurrentBox ()
  {
    return rfcDecoder::fromIMAP(currentBox);
  };

  imapCommand *sendCommand (imapCommand *);
  imapCommand *doCommand (imapCommand *);


  bool clientLogin (const QString &, const QString &, QString &);
  bool clientAuthenticate (const QString &, const QString &, const QString &,
    bool, QString &);

  // main loop for the parser
  // reads one line and dispatches it to the appropriate sub parser
  int parseLoop ();

  // parses all untagged responses and passes them on to the following parsers
  void parseUntagged (parseString & result);

  void parseRecent (ulong value, parseString & result);
  void parseResult (QByteArray & result, parseString & rest,
    const QString & command = QString::null);
  void parseCapability (parseString & result);
  void parseFlags (parseString & result);
  void parseList (parseString & result);
  void parseLsub (parseString & result);
  void parseSearch (parseString & result);
  void parseStatus (parseString & result);
  void parseExists (ulong value, parseString & result);
  void parseExpunge (ulong value, parseString & result);

  // parses the results of a fetch command
  // processes it with the following sub parsers
  void parseFetch (ulong value, parseString & inWords);

  // read a envelope from imap and parse the adresses
  mailHeader *parseEnvelope (parseString & inWords);
  QValueList < mailAddress > parseAdressList (parseString & inWords);
  mailAddress parseAdress (parseString & inWords);

  // parse the result of the body command
  void parseBody (parseString & inWords);

  // parse the body structure recursively
  mimeHeader *parseBodyStructure (parseString & inWords,
    const QString & section, mimeHeader * inHeader = NULL);

  // parse only one not nested part
  mimeHeader *parseSimplePart (parseString & inWords, const QString & section);

  // parse a parameter list (name value pairs)
  QDict < QString > parseParameters (parseString & inWords);

  // parse the disposition list (disposition (name value pairs))
  // the disposition has the key 'content-disposition'
  QDict < QString > parseDisposition (parseString & inWords);

  // reimplement these

  // relay hook to send the fetched data directly to an upper level
  virtual void parseRelay (const QByteArray & buffer);

  // relay hook to announce the fetched data directly to an upper level
  virtual void parseRelay (ulong);

  // read at least len bytes
  virtual bool parseRead (QByteArray & buffer, ulong len, ulong relay = 0);

  // read at least a line (up to CRLF)
  virtual bool parseReadLine (QByteArray & buffer, ulong relay = 0);

  // write argument to server
  virtual void parseWriteLine (const QString &);

  // generic parser routines

  // parse a parenthesized list
  void parseSentence (parseString & inWords);

  // parse a literal or word, may require more data
  QByteArray parseLiteral (parseString & inWords, bool relay = false);

  // static parser routines, can be used elsewhere

  static QCString b2c(const QByteArray &ba)
  { return QCString(ba.data(), ba.size() + 1); }

  // skip over whitespace
  static void skipWS (parseString & inWords);

  // parse one word (maybe quoted) upto next space " ) ] }
  static QByteArray parseOneWord (parseString & inWords,
    bool stopAtBracket = FALSE);

  // parse one number using parseOneWord
  static bool parseOneNumber (parseString & inWords, ulong & num);

  // extract the box,section,list type, uid, uidvalidity from an url
  static void parseURL (const KURL & _url, QString & _box, QString & _section,
                        QString & _type, QString & _uid, QString & _validity);


  imapCache *getLastHandled ()
  {
    return lastHandled;
  };

  const QStringList & getResults ()
  {
    return lastResults;
  };

  const imapInfo & getStatus ()
  {
    return lastStatus;
  };
  const imapInfo & getSelected ()
  {
    return selectInfo;
  };

  const QByteArray & getContinuation ()
  {
    return continuation;
  };

  bool hasCapability (const QString &);

protected:

  // the current state we're in
  enum IMAP_STATE currentState;

  // the box selected
  QString currentBox;

  // here we store the result from select/examine and unsolicited updates
  imapInfo selectInfo;

  // the results from the last status command
  imapInfo lastStatus;

  // the results from the capabilities, split at ' '
  QStringList imapCapabilities;

  // the results from list/lsub commands
  QValueList < imapList > listResponses;

  // queues handling the running commands
  QPtrList < imapCommand > sentQueue;  // no autodelete
  QPtrList < imapCommand > completeQueue;  // autodelete !!

  // everything we didn't handle, everything but the greeting is bogus
  QStringList unhandled;

  // the last continuation request (there MUST not be more than one pending)
  QByteArray continuation;

  // the last uid seen while a fetch
  QString seenUid;
  imapCache *lastHandled;

  ulong commandCounter;

  QStringList lastResults;

private:
  imapParser & operator = (const imapParser &); // hide the copy ctor

};
#endif
