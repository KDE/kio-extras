/**********************************************************************
 *
 *   imapparser.cc  - IMAP4rev1 Parser
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
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *   Send comments and bug fixes to s.carstens@gmx.de
 *
 *********************************************************************/

#include "rfcdecoder.h"

#include "imapparser.h"

#include "imapinfo.h"

#include "mailheader.h"
#include "mimeheader.h"
#include "mailaddress.h"

#include <sys/types.h>

#include <stdlib.h>
#include <unistd.h>

#include <qregexp.h>
#include <qbuffer.h>
#include <qstring.h>
#include <qstringlist.h>

#include <kdebug.h>
#include <kmdcodec.h>
#include <kurl.h>
#include <kio/kdesasl.h>

imapParser::imapParser ()
{
  sentQueue.setAutoDelete (false);
  completeQueue.setAutoDelete (true);
  currentState = ISTATE_NO;
  commandCounter = 0;
  lastHandled = NULL;
}

imapParser::~imapParser ()
{
  delete lastHandled;
  lastHandled = 0L;
}

imapCommand *
imapParser::doCommand (imapCommand * aCmd)
{
  int pl = 0;
  sendCommand (aCmd);
  while (pl != -1 && !aCmd->isComplete ()) {
    while ((pl = parseLoop ()) == 0)
     ;
  }

  return aCmd;
}

imapCommand *
imapParser::sendCommand (imapCommand * aCmd)
{
  aCmd->setId (QString::number(commandCounter++));
  sentQueue.append (aCmd);

  continuation.resize(0);
  const QString& command = aCmd->command();

  if (command == "SELECT" || command == "EXAMINE")
  {
     // we need to know which box we are selecting
    parseString p;
    p.fromString(aCmd->parameter());
    currentBox = parseOneWordC(p);
    kdDebug(7116) << "imapParser::sendCommand - setting current box to " << currentBox << endl;
  }
  else if (command == "CLOSE")
  {
     // we no longer have a box open
    currentBox = QString::null;
  }
  else if (command.find ("SEARCH") != -1
           || command == "ACL"
           || command == "LISTRIGHTS"
           || command == "MYRIGHTS")
  {
    lastResults.clear ();
  }
  else if (command == "LIST"
           || command == "LSUB")
  {
    listResponses.clear ();
  }
  parseWriteLine (aCmd->getStr ());
  return aCmd;
}

bool
imapParser::clientLogin (const QString & aUser, const QString & aPass,
  QString & resultInfo)
{
  imapCommand *cmd;
  bool retVal = false;

  cmd =
    doCommand (new
               imapCommand ("LOGIN", "\"" + rfcDecoder::quoteIMAP(aUser)
               + "\" \"" + rfcDecoder::quoteIMAP(aPass) + "\""));

  if (cmd->result () == "OK")
  {
    currentState = ISTATE_LOGIN;
    retVal = true;
  }
  resultInfo = cmd->resultInfo();
  completeQueue.removeRef (cmd);

  return retVal;
}


bool
imapParser::clientAuthenticate (const QString & aUser, const QString & aPass,
  const QString & aAuth, bool isSSL, QString & resultInfo)
{
  imapCommand *cmd;
  bool retVal = false;

  // see if server supports this authenticator
  if (!hasCapability ("AUTH=" + aAuth))
    return false;

  // then lets try it
  cmd = sendCommand (new imapCommand ("AUTHENTICATE", aAuth));
  KDESasl sasl(aUser, aPass, isSSL ? "imaps" : "imap");
  sasl.setMethod(aAuth.latin1());
  while (!cmd->isComplete ())
  {
    //read the next line
    while (parseLoop() == 0);

    if (!continuation.isEmpty ())
    {
      QByteArray challenge;
      challenge.duplicate(continuation.data() + 2, continuation.size() - 2);
      challenge.resize(challenge.size() - 2); // trim CRLF

      if (aAuth.upper () == "ANONYMOUS")
      {
        // we should present the challenge to the user and ask
        // him for a mail-address or what ever
        challenge = KCodecs::base64Encode(aUser.utf8());
      } else {
        challenge = sasl.getResponse(challenge);
      }

      parseWriteLine (challenge);
      continuation.resize(0);
    }
  }

  if (cmd->result () == "OK")
  {
    currentState = ISTATE_LOGIN;
    retVal = true;
  }
  resultInfo = cmd->resultInfo();
  completeQueue.removeRef (cmd);

  return retVal;
}

void
imapParser::parseUntagged (parseString & result)
{
  //kdDebug(7116) << "imapParser::parseUntagged - '" << result.cstr() << "'" << endl;

  parseOneWordC(result);        // *
  QByteArray what = parseLiteral (result); // see whats coming next

  switch (what[0])
  {
    //the status responses
  case 'B':                    // BAD or BYE
    if (qstrncmp(what, "BAD", what.size()) == 0)
    {
      parseResult (what, result);
    }
    else if (qstrncmp(what, "BYE", what.size()) == 0)
    {
      parseResult (what, result);
      currentState = ISTATE_NO;
    }
    break;

  case 'N':                    // NO
    if (what[1] == 'O' && what.size() == 2)
    {
      parseResult (what, result);
    }
    break;

  case 'O':                    // OK
    if (what[1] == 'K' && what.size() == 2)
    {
      parseResult (what, result);
    }
    break;

  case 'P':                    // PREAUTH
    if (qstrncmp(what, "PREAUTH", what.size()) == 0)
    {
      parseResult (what, result);
      currentState = ISTATE_LOGIN;
    }
    break;

    // parse the other responses
  case 'C':                    // CAPABILITY
    if (qstrncmp(what, "CAPABILITY", what.size()) == 0)
    {
      parseCapability (result);
    }
    break;

  case 'F':                    // FLAGS
    if (qstrncmp(what, "FLAGS", what.size()) == 0)
    {
      parseFlags (result);
    }
    break;

  case 'L':                    // LIST or LSUB or LISTRIGHTS
    if (qstrncmp(what, "LIST", what.size()) == 0)
    {
      parseList (result);
    }
    else if (qstrncmp(what, "LSUB", what.size()) == 0)
    {
      parseLsub (result);
    }
    else if (qstrncmp(what, "LISTRIGHTS", what.size()) == 0)
    {
      parseListRights (result);
    }
    break;

  case 'M': // MYRIGHTS
    if (qstrncmp(what, "MYRIGHTS", what.size()) == 0)
    {
      parseMyRights (result);
    }
    break;
  case 'S':                    // SEARCH or STATUS
    if (qstrncmp(what, "SEARCH", what.size()) == 0)
    {
      parseSearch (result);
    }
    else if (qstrncmp(what, "STATUS", what.size()) == 0)
    {
      parseStatus (result);
    }
    break;

  case 'A': // ACL
    if (qstrncmp(what, "ACL", what.size()) == 0)
    {
      parseAcl (result);
    }
    break;
  default:
    //better be a number
    {
      ulong number;
      bool valid;

      number = QCString(what, what.size() + 1).toUInt(&valid);
      if (valid)
      {
        what = parseLiteral (result);
        switch (what[0])
        {
        case 'E':
          if (qstrncmp(what, "EXISTS", what.size()) == 0)
          {
            parseExists (number, result);
          }
          else if (qstrncmp(what, "EXPUNGE", what.size()) == 0)
          {
            parseExpunge (number, result);
          }
          break;

        case 'F':
          if (qstrncmp(what, "FETCH", what.size()) == 0)
          {
            seenUid = QString::null;
            if (lastHandled) lastHandled->clear();
            else lastHandled = new imapCache();
            parseFetch (number, result);
          }
          break;

        case 'S':
          if (qstrncmp(what, "STORE", what.size()) == 0)  // deprecated store
          {
            seenUid = QString::null;
            parseFetch (number, result);
          }
          break;

        case 'R':
          if (qstrncmp(what, "RECENT", what.size()) == 0)
          {
            parseRecent (number, result);
          }
          break;
        default:
          break;
        }
      }
    }
    break;
  }                             //switch
}                               //func


void
imapParser::parseResult (QByteArray & result, parseString & rest,
  const QString & command)
{
  kdDebug() << k_funcinfo << "rest=" << rest.cstr() << " command=" << command << endl;
  if (command == "SELECT") selectInfo.setReadWrite(true);

  if (rest[0] == '[')
  {
    rest.pos++;
    QCString option = parseOneWordC(rest, TRUE);

    switch (option[0])
    {
    case 'A':                  // ALERT
      if (option == "ALERT")
      {
      }
      break;

    case 'N':                  // NEWNAME
      if (option == "NEWNAME")
      {
      }
      break;

    case 'P':                  //PARSE or PERMANENTFLAGS
      if (option == "PARSE")
      {
      }
      else if (option == "PERMANENTFLAGS")
      {
        uint end = rest.data.find(']', rest.pos);
        QCString flags(rest.data.data() + rest.pos, end - rest.pos);
        selectInfo.setPermanentFlags (flags);
        rest.pos = end;
      }
      break;

    case 'R':                  //READ-ONLY or READ-WRITE
      if (option == "READ-ONLY")
      {
        selectInfo.setReadWrite (false);
      }
      else if (option == "READ-WRITE")
      {
        selectInfo.setReadWrite (true);
      }
      break;

    case 'T':                  //TRYCREATE
      if (option == "TRYCREATE")
      {
      }
      break;

    case 'U':                  //UIDVALIDITY or UNSEEN
      if (option == "UIDVALIDITY")
      {
        ulong value;
        if (parseOneNumber (rest, value))
          selectInfo.setUidValidity (value);
      }
      else if (option == "UNSEEN")
      {
        ulong value;
        if (parseOneNumber (rest, value))
          selectInfo.setUnseen (value);
      }
      else if (option == "UIDNEXT")
      {
        ulong value;
        if (parseOneNumber (rest, value))
          selectInfo.setUidNext (value);
      }
      else
      break;

    }
    if (rest[0] == ']')
      rest.pos++; //tie off ]
    skipWS (rest);
  }

  if (command.isEmpty())
  {
    // This happens when parsing an intermediate result line (those that start with '*').
    // No state change involved, so we can stop here.
    return;
  }

  switch (command[0].latin1 ())
  {
  case 'A':
    if (command == "AUTHENTICATE")
      if (qstrncmp(result, "OK", result.size()) == 0)
        currentState = ISTATE_LOGIN;
    break;

  case 'L':
    if (command == "LOGIN")
      if (qstrncmp(result, "OK", result.size()) == 0)
        currentState = ISTATE_LOGIN;
    break;

  case 'E':
    if (command == "EXAMINE")
    {
      if (qstrncmp(result, "OK", result.size()) == 0)
        currentState = ISTATE_SELECT;
      else
      {
        if (currentState == ISTATE_SELECT)
          currentState = ISTATE_LOGIN;
        currentBox = QString::null;
      }
      kdDebug(7116) << "imapParser::parseResult - current box is now " << currentBox << endl;
    }
    break;

  case 'S':
    if (command == "SELECT")
    {
      if (qstrncmp(result, "OK", result.size()) == 0)
        currentState = ISTATE_SELECT;
      else
      {
        if (currentState == ISTATE_SELECT)
          currentState = ISTATE_LOGIN;
        currentBox = QString::null;
      }
      kdDebug(7116) << "imapParser::parseResult - current box is now " << currentBox << endl;
    }
    break;

  default:
    break;
  }

}

void imapParser::parseCapability (parseString & result)
{
  imapCapabilities = QStringList::split (' ', result.cstr().lower());
}

void imapParser::parseFlags (parseString & result)
{
  selectInfo.setFlags(result.cstr());
}

void imapParser::parseList (parseString & result)
{
  imapList this_one;

  if (result[0] != '(')
    return;                     //not proper format for us

  result.pos++; // tie off (

  //process the attributes
  QCString attribute;

  while (!result.isEmpty () && result[0] != ')')
  {
    attribute = parseOneWordC(result).lower();
    if (-1 != attribute.find ("\\noinferiors"))
      this_one.setNoInferiors (true);
    else if (-1 != attribute.find ("\\noselect"))
      this_one.setNoSelect (true);
    else if (-1 != attribute.find ("\\marked"))
      this_one.setMarked (true);
    else if (-1 != attribute.find ("\\unmarked"))
      this_one.setUnmarked (true);
    else if (-1 != attribute.find ("\\hasnochildren")) {
      // not important
    } else
      kdDebug(7116) << "imapParser::parseList - unknown attribute " << attribute << endl;
  }

  result.pos++; // tie off )
  skipWS (result);

  this_one.setHierarchyDelimiter(parseLiteralC(result));
  this_one.setName (rfcDecoder::fromIMAP(parseLiteralC(result)));  // decode modified UTF7

  listResponses.append (this_one);
}

void imapParser::parseLsub (parseString & result)
{
  imapList this_one (result.cstr());
  listResponses.append (this_one);
}

void imapParser::parseListRights (parseString & result)
{
  parseOneWordC (result); // skip mailbox name
  parseOneWordC (result); // skip user id
  int outlen = 1;
  while ( outlen ) {
    QCString word = parseOneWordC (result, false, &outlen);
    lastResults.append (word);
  }
}

void imapParser::parseAcl (parseString & result)
{
  parseOneWordC (result); // skip mailbox name
  int outlen = 1;
  // The result is user1 perm1 user2 perm2 etc. The caller will sort it out.
  while ( outlen ) {
    QCString word = parseOneWordC (result, false, &outlen);
    lastResults.append (word);
  }
}

void imapParser::parseMyRights (parseString & result)
{
  parseOneWordC (result); // skip mailbox name
  Q_ASSERT( lastResults.isEmpty() ); // we can only be called once
  lastResults.append (parseOneWordC (result) );
}

void imapParser::parseSearch (parseString & result)
{
  ulong value;

  while (parseOneNumber (result, value))
  {
    lastResults.append (QString::number(value));
  }
}

void imapParser::parseStatus (parseString & inWords)
{
  lastStatus = imapInfo ();

  parseLiteralC(inWords);       // swallow the box
  if (inWords[0] != '(')
    return;

  inWords.pos++;
  skipWS (inWords);

  while (!inWords.isEmpty() && inWords[0] != ')')
  {
    ulong value;

    QCString label = parseOneWordC(inWords);
    if (parseOneNumber (inWords, value))
    {
      if (label == "MESSAGES")
        lastStatus.setCount (value);
      else if (label == "RECENT")
        lastStatus.setRecent (value);
      else if (label == "UIDVALIDITY")
        lastStatus.setUidValidity (value);
      else if (label == "UNSEEN")
        lastStatus.setUnseen (value);
      else if (label == "UIDNEXT")
        lastStatus.setUidNext (value);
    }
  }

  if (inWords[0] == ')')
    inWords.pos++;
  skipWS (inWords);
}

void imapParser::parseExists (ulong value, parseString & result)
{
  selectInfo.setCount (value);
  result.pos = result.data.size();
}

void imapParser::parseExpunge (ulong value, parseString & result)
{
  Q_UNUSED(value);
  Q_UNUSED(result);
}

void imapParser::parseAddressList (parseString & inWords, QPtrList<mailAddress>& list)
{
  if (inWords[0] != '(')
  {
    parseOneWord (inWords);     // parse NIL
  }
  else
  {
    inWords.pos++;
    skipWS (inWords);

    while (!inWords.isEmpty () && inWords[0] != ')')
    {
      if (inWords[0] == '(') {
        mailAddress *addr = new mailAddress;
        parseAddress(inWords, *addr);
        list.append(addr);
      } else {
        break;
      }
    }

    if (inWords[0] == ')')
      inWords.pos++;
    skipWS (inWords);
  }
}

const mailAddress& imapParser::parseAddress (parseString & inWords, mailAddress& retVal)
{
  // This is already done in parseAddressList
  //if (inWords[0] != '(')
  //  return retVal;
  inWords.pos++;
  skipWS (inWords);

  retVal.setFullName(rfcDecoder::quoteIMAP(parseLiteralC(inWords)));
  retVal.setCommentRaw(parseLiteralC(inWords));
  retVal.setUser(parseLiteralC(inWords));
  retVal.setHost(parseLiteralC(inWords));

  if (inWords[0] == ')')
    inWords.pos++;
  skipWS (inWords);

  return retVal;
}

mailHeader * imapParser::parseEnvelope (parseString & inWords)
{
  mailHeader *envelope = NULL;

  if (inWords[0] != '(')
    return envelope;
  inWords.pos++;
  skipWS (inWords);

  envelope = new mailHeader;

  //date
  envelope->setDate(parseLiteralC(inWords));

  //subject
  envelope->setSubjectEncoded(parseLiteralC(inWords));

  QPtrList<mailAddress> list;
  list.setAutoDelete(true);

  //from
  parseAddressList(inWords, list);
  if (!list.isEmpty()) {
	  envelope->setFrom(*list.last());
	  list.clear();
  }

  //sender
  parseAddressList(inWords, list);
  if (!list.isEmpty()) {
	  envelope->setSender(*list.last());
	  list.clear();
  }

  //reply-to
  parseAddressList(inWords, list);
  if (!list.isEmpty()) {
	  envelope->setReplyTo(*list.last());
	  list.clear();
  }

  //to
  parseAddressList (inWords, envelope->to());

  //cc
  parseAddressList (inWords, envelope->cc());

  //bcc
  parseAddressList (inWords, envelope->bcc());

  //in-reply-to
  envelope->setInReplyTo(parseLiteralC(inWords));

  //message-id
  envelope->setMessageId(parseLiteralC(inWords));

  // see if we have more to come
  while (!inWords.isEmpty () && inWords[0] != ')')
  {
    //eat the extensions to this part
    if (inWords[0] == '(')
      parseSentence (inWords);
    else
      parseLiteralC (inWords);
  }

  if (inWords[0] == ')')
    inWords.pos++;
  skipWS (inWords);

  return envelope;
}

// parse parameter pairs into a dictionary
// caller must clean up the dictionary items
QAsciiDict < QString > imapParser::parseDisposition (parseString & inWords)
{
  QByteArray disposition;
  QAsciiDict < QString > retVal (17, false);

  // return value is a shallow copy
  retVal.setAutoDelete (false);

  if (inWords[0] != '(')
  {
    //disposition only
    disposition = parseOneWord (inWords);
  }
  else
  {
    inWords.pos++;
    skipWS (inWords);

    //disposition
    disposition = parseOneWord (inWords);

    retVal = parseParameters (inWords);
    if (inWords[0] != ')')
      return retVal;
    inWords.pos++;
    skipWS (inWords);
  }

  if (!disposition.isEmpty ())
  {
    retVal.insert ("content-disposition", new QString(b2c(disposition)));
  }

  return retVal;
}

// parse parameter pairs into a dictionary
// caller must clean up the dictionary items
QAsciiDict < QString > imapParser::parseParameters (parseString & inWords)
{
  QAsciiDict < QString > retVal (17, false);

  // return value is a shallow copy
  retVal.setAutoDelete (false);

  if (inWords[0] != '(')
  {
    //better be NIL
    parseOneWord (inWords);
  }
  else
  {
    inWords.pos++;
    skipWS (inWords);

    while (!inWords.isEmpty () && inWords[0] != ')')
    {
      retVal.insert (parseLiteralC(inWords), new QString(parseLiteralC(inWords)));
    }

    if (inWords[0] != ')')
      return retVal;
    inWords.pos++;
    skipWS (inWords);
  }

  return retVal;
}

mimeHeader * imapParser::parseSimplePart (parseString & inWords,
  QString & inSection, mimeHeader * localPart)
{
  QCString subtype;
  QCString typeStr;
  QAsciiDict < QString > parameters (17, false);
  ulong size;

  parameters.setAutoDelete (true);

  if (inWords[0] != '(')
    return NULL;

  if (!localPart)
    localPart = new mimeHeader;

  localPart->setPartSpecifier (inSection);

  inWords.pos++;
  skipWS (inWords);

  //body type
  typeStr = parseLiteralC(inWords);

  //body subtype
  subtype = parseLiteralC(inWords);

  localPart->setType (typeStr + "/" + subtype);

  //body parameter parenthesized list
  parameters = parseParameters (inWords);
  {
    QAsciiDictIterator < QString > it (parameters);

    while (it.current ())
    {
      localPart->setTypeParm (it.currentKey (), *(it.current ()));
      ++it;
    }
    parameters.clear ();
  }

  //body id
  localPart->setID (parseLiteralC(inWords));

  //body description
  localPart->setDescription (parseLiteralC(inWords));

  //body encoding
  localPart->setEncoding (parseLiteralC(inWords));

  //body size
  if (parseOneNumber (inWords, size))
    localPart->setLength (size);

  // type specific extensions
  if (localPart->getType().upper() == "MESSAGE/RFC822")
  {
    //envelope structure
    mailHeader *envelope = parseEnvelope (inWords);

    //body structure
    parseBodyStructure (inWords, inSection, envelope);

    localPart->setNestedMessage (envelope);

    //text lines
    ulong lines;
    parseOneNumber (inWords, lines);
  }
  else
  {
    if (typeStr ==  "TEXT")
    {
      //text lines
      ulong lines;
      parseOneNumber (inWords, lines);
    }

    // md5
    parseLiteralC(inWords);

    // body disposition
    parameters = parseDisposition (inWords);
    {
      QString *disposition = parameters["content-disposition"];

      if (disposition)
        localPart->setDisposition (disposition->ascii ());
      parameters.remove ("content-disposition");
      QAsciiDictIterator < QString > it (parameters);
      while (it.current ())
      {
        localPart->setDispositionParm (it.currentKey (),
                                       *(it.current ()));
        ++it;
      }
      parameters.clear ();
    }

    // body language
    parseSentence (inWords);
  }

  // see if we have more to come
  while (!inWords.isEmpty () && inWords[0] != ')')
  {
    //eat the extensions to this part
    if (inWords[0] == '(')
      parseSentence (inWords);
    else
      parseLiteralC(inWords);
  }

  if (inWords[0] == ')')
    inWords.pos++;
  skipWS (inWords);

  return localPart;
}

mimeHeader * imapParser::parseBodyStructure (parseString & inWords,
  QString & inSection, mimeHeader * localPart)
{
  bool init = false;
  if (inSection.isEmpty())
  {
    // first run
    init = true;
    // assume one part
    inSection = "1";
  }
  int section = 0;

  if (inWords[0] != '(')
  {
    // skip ""
    parseOneWord (inWords);
    return 0;
  }
  inWords.pos++;
  skipWS (inWords);

  if (inWords[0] == '(')
  {
    QByteArray subtype;
    QAsciiDict < QString > parameters (17, false);
    QString outSection;
    parameters.setAutoDelete (true);
    if (!localPart)
      localPart = new mimeHeader;
    else
    {
      // might be filled from an earlier run
      localPart->clearNestedParts ();
      localPart->clearTypeParameters ();
      localPart->clearDispositionParameters ();
      // an envelope was passed in so this is the multipart header
      outSection = inSection + ".HEADER";
    }
    if (inWords[0] == '(' && init)
      inSection = "0";

    // set the section
    if ( !outSection.isEmpty() ) {
      localPart->setPartSpecifier(outSection);
    } else {
      localPart->setPartSpecifier(inSection);
    }

    // is multipart (otherwise its a simplepart and handled later)
    while (inWords[0] == '(')
    {
      outSection = QString::number(++section);
      if (!init)
        outSection = inSection + "." + outSection;
      mimeHeader *subpart = parseBodyStructure (inWords, outSection, 0);
      localPart->addNestedPart (subpart);
    }

    // fetch subtype
    subtype = parseOneWord (inWords);

    localPart->setType ("MULTIPART/" + b2c(subtype));

    // fetch parameters
    parameters = parseParameters (inWords);
    {
      QAsciiDictIterator < QString > it (parameters);

      while (it.current ())
      {
        localPart->setTypeParm (it.currentKey (), *(it.current ()));
        ++it;
      }
      parameters.clear ();
    }

    // body disposition
    parameters = parseDisposition (inWords);
    {
      QString *disposition = parameters["content-disposition"];

      if (disposition)
        localPart->setDisposition (disposition->ascii ());
      parameters.remove ("content-disposition");
      QAsciiDictIterator < QString > it (parameters);
      while (it.current ())
      {
        localPart->setDispositionParm (it.currentKey (),
                                       *(it.current ()));
        ++it;
      }
      parameters.clear ();
    }

    // body language
    parseSentence (inWords);

  }
  else
  {
    // is simple part
    inWords.pos--;
    inWords.data[inWords.pos] = '('; //fake a sentence
    if ( localPart )
      inSection = inSection + ".1";
    localPart = parseSimplePart (inWords, inSection, localPart);
    inWords.pos--;
    inWords.data[inWords.pos] = ')'; //remove fake
  }

  // see if we have more to come
  while (!inWords.isEmpty () && inWords[0] != ')')
  {
    //eat the extensions to this part
    if (inWords[0] == '(')
      parseSentence (inWords);
    else
      parseLiteralC(inWords);
  }

  if (inWords[0] == ')')
    inWords.pos++;
  skipWS (inWords);

  return localPart;
}

void imapParser::parseBody (parseString & inWords)
{
  // see if we got a part specifier
  if (inWords[0] == '[')
  {
    QByteArray specifier;
    QByteArray label;
    inWords.pos++;

    specifier = parseOneWord (inWords, TRUE);

    if (inWords[0] == '(')
    {
      inWords.pos++;

      while (!inWords.isEmpty () && inWords[0] != ')')
      {
        label = parseOneWord (inWords);
      }

      if (inWords[0] == ')')
        inWords.pos++;
    }
    if (inWords[0] == ']')
      inWords.pos++;
    skipWS (inWords);

    // parse the header
    if (qstrncmp(specifier, "0", specifier.size()) == 0)
    {
      mailHeader *envelope = NULL;
      if (lastHandled)
        envelope = lastHandled->getHeader ();

      if (!envelope || seenUid.isEmpty ())
      {
        kdDebug(7116) << "imapParser::parseBody - discarding " << envelope << " " << seenUid.ascii () << endl;
        // don't know where to put it, throw it away
        parseLiteralC(inWords, true);
      }
      else
      {
        kdDebug(7116) << "imapParser::parseBody - reading " << envelope << " " << seenUid.ascii () << endl;
        // fill it up with data
        QString theHeader = parseLiteralC(inWords, true);
        mimeIOQString myIO;

        myIO.setString (theHeader);
        envelope->parseHeader (myIO);

      }
    }
    else if (qstrncmp(specifier, "HEADER.FIELDS", specifier.size()) == 0)
    {
      // BODY[HEADER.FIELDS (References)] {n}
      //kdDebug(7116) << "imapParser::parseBody - HEADER.FIELDS: "
      // << QCString(label.data(), label.size()+1) << endl;
      if (qstrncmp(label, "REFERENCES", label.size()) == 0)
      {
       mailHeader *envelope = NULL;
       if (lastHandled)
         envelope = lastHandled->getHeader ();

       if (!envelope || seenUid.isEmpty ())
       {
         kdDebug(7116) << "imapParser::parseBody - discarding " << envelope << " " << seenUid.ascii () << endl;
         // don't know where to put it, throw it away
         parseLiteralC (inWords, true);
       }
       else
       {
         QCString references = parseLiteralC(inWords, true);
         int start = references.find ('<');
         int end = references.findRev ('>');
         if (start < end)
                 references = references.mid (start, end - start + 1);
         envelope->setReferences(references.simplifyWhiteSpace());
       }
      }
      else
      { // not a header we care about throw it away
        parseLiteralC(inWords, true);
      }
    }
    else
    {
      // throw it away
      parseLiteralC(inWords, true);
    }

  }
  else
  {
    mailHeader *envelope = NULL;
    if (lastHandled)
      envelope = lastHandled->getHeader ();

    if (!envelope || seenUid.isEmpty ())
    {
      kdDebug(7116) << "imapParser::parseBody - discarding " << envelope << " " << seenUid.ascii () << endl;
      // don't know where to put it, throw it away
      parseSentence (inWords);
    }
    else
    {
      kdDebug(7116) << "imapParser::parseBody - reading " << envelope << " " << seenUid.ascii () << endl;
      // fill it up with data
      QString section;
      mimeHeader *body = parseBodyStructure (inWords, section, envelope);
      if (body != envelope)
        delete body;
    }
  }
}

void imapParser::parseFetch (ulong /* value */, parseString & inWords)
{
  if (inWords[0] != '(')
    return;
  inWords.pos++;
  skipWS (inWords);

  delete lastHandled;
  lastHandled = NULL;

  while (!inWords.isEmpty () && inWords[0] != ')')
  {
    if (inWords[0] == '(')
      parseSentence (inWords);
    else
    {
      QCString word = parseLiteralC(inWords, false, true);

      switch (word[0])
      {
      case 'E':
        if (word == "ENVELOPE")
        {
          mailHeader *envelope = NULL;

          if (lastHandled)
            envelope = lastHandled->getHeader ();
          else lastHandled = new imapCache();

          if (envelope && !envelope->getMessageId ().isEmpty ())
          {
            // we have seen this one already
            // or don't know where to put it
            parseSentence (inWords);
          }
          else
          {
            envelope = parseEnvelope (inWords);
            if (envelope)
            {
              envelope->setPartSpecifier (seenUid + ".0");
              lastHandled->setHeader (envelope);
              lastHandled->setUid (seenUid.toULong ());
            }
          }
        }
        break;

      case 'B':
        if (word == "BODY")
        {
          parseBody (inWords);
        }
        else if (word == "BODY[]" )
        {
	  // Do the same as with "RFC822"
          parseLiteralC(inWords, true);
        }
        else if (word == "BODYSTRUCTURE")
        {
          mailHeader *envelope = NULL;

          if (lastHandled)
            envelope = lastHandled->getHeader ();

          // fill it up with data
          QString section;
          mimeHeader *body =
            parseBodyStructure (inWords, section, envelope);
          QByteArray data;
          QDataStream stream( data, IO_WriteOnly );
          body->serialize(stream);
          parseRelay(data);

          delete body;
        }
        break;

      case 'U':
        if (word == "UID")
        {
          seenUid = parseOneWordC(inWords);
          mailHeader *envelope = NULL;
          if (lastHandled)
            envelope = lastHandled->getHeader ();
          else lastHandled = new imapCache();

          if (envelope || seenUid.isEmpty ())
          {
            // we have seen this one already
            // or don't know where to put it
          }
          else
          {
            lastHandled->setUid (seenUid.toULong ());
          }
          if (envelope)
            envelope->setPartSpecifier (seenUid);
        }
        break;

      case 'R':
        if (word == "RFC822.SIZE")
        {
          ulong size;
          parseOneNumber (inWords, size);

          if (!lastHandled) lastHandled = new imapCache();
          lastHandled->setSize (size);
        }
        else if (word.find ("RFC822") == 0)
        {
          // might be RFC822 RFC822.TEXT RFC822.HEADER
          parseLiteralC(inWords, true);
        }
        break;

      case 'I':
        if (word == "INTERNALDATE")
        {
          QCString date = parseOneWordC(inWords);
          if (!lastHandled) lastHandled = new imapCache();
          lastHandled->setDate(date);
        }
        break;

      case 'F':
        if (word == "FLAGS")
        {
	  //kdDebug(7116) << "GOT FLAGS " << inWords.cstr() << endl;
          if (!lastHandled) lastHandled = new imapCache();
          lastHandled->setFlags (imapInfo::_flags (inWords.cstr()));
        }
        break;

      default:
        parseLiteralC(inWords);
        break;
      }
    }
  }

  // see if we have more to come
  while (!inWords.isEmpty () && inWords[0] != ')')
  {
    //eat the extensions to this part
    if (inWords[0] == '(')
      parseSentence (inWords);
    else
      parseLiteralC(inWords);
  }

  if (inWords[0] != ')')
    return;
  inWords.pos++;
  skipWS (inWords);
}


// default parser
void imapParser::parseSentence (parseString & inWords)
{
  bool first = true;
  int stack = 0;

  //find the first nesting parentheses

  while (!inWords.isEmpty () && (stack != 0 || first))
  {
    first = false;
    skipWS (inWords);

    unsigned char ch = inWords[0];
    switch (ch)
    {
    case '(':
      inWords.pos++;
      ++stack;
      break;
    case ')':
      inWords.pos++;
      --stack;
      break;
    case '[':
      inWords.pos++;
      ++stack;
      break;
    case ']':
      inWords.pos++;
      --stack;
      break;
    default:
      parseLiteralC(inWords);
      skipWS (inWords);
      break;
    }
  }
  skipWS (inWords);
}

void imapParser::parseRecent (ulong value, parseString & result)
{
  selectInfo.setRecent (value);
  result.pos = result.data.size();
}

int imapParser::parseLoop ()
{
  parseString result;

  if (!parseReadLine(result.data)) return -1;

  if (result.data.isEmpty())
    return 0;
  if (!sentQueue.count ())
  {
    // maybe greeting or BYE everything else SHOULD not happen, use NOOP or IDLE
    kdDebug(7116) << "imapParser::parseLoop - unhandledResponse: \n" << result.cstr() << endl;
    unhandled << result.cstr();
  }
  else
  {
    imapCommand *current = sentQueue.at (0);
    switch (result[0])
    {
    case '*':
      result.data.resize(result.data.size() - 2);  // tie off CRLF
      parseUntagged (result);
      break;
    case '+':
      continuation.duplicate(result.data);
      break;
    default:
      {
        QCString tag = parseLiteralC(result);
        if (current->id() == tag.data())
        {
          result.data.resize(result.data.size() - 2);  // tie off CRLF
          QByteArray resultCode = parseLiteral (result); //the result
          current->setResult (resultCode);
          current->setResultInfo(result.cstr());
          current->setComplete ();

          sentQueue.removeRef (current);
          completeQueue.append (current);
          if (result.length())
		parseResult (resultCode, result, current->command());
        }
        else
        {
          kdDebug(7116) << "imapParser::parseLoop - unknown tag '" << tag << "'" << endl;
          QCString cstr = tag + " " + result.cstr();
          result.data = cstr;
          result.pos = 0;
          result.data.resize(cstr.length());
        }
      }
      break;
    }
  }

  return 1;
}

void
imapParser::parseRelay (const QByteArray & buffer)
{
  Q_UNUSED(buffer);
  qWarning
    ("imapParser::parseRelay - virtual function not reimplemented - data lost");
}

void
imapParser::parseRelay (ulong len)
{
  Q_UNUSED(len);
  qWarning
    ("imapParser::parseRelay - virtual function not reimplemented - announcement lost");
}

bool imapParser::parseRead (QByteArray & buffer, ulong len, ulong relay)
{
  Q_UNUSED(buffer);
  Q_UNUSED(len);
  Q_UNUSED(relay);
  qWarning
    ("imapParser::parseRead - virtual function not reimplemented - no data read");
  return FALSE;
}

bool imapParser::parseReadLine (QByteArray & buffer, ulong relay)
{
  Q_UNUSED(buffer);
  Q_UNUSED(relay);
  qWarning
    ("imapParser::parseReadLine - virtual function not reimplemented - no data read");
  return FALSE;
}

void
imapParser::parseWriteLine (const QString & str)
{
  Q_UNUSED(str);
  qWarning
    ("imapParser::parseWriteLine - virtual function not reimplemented - no data written");
}

void
imapParser::parseURL (const KURL & _url, QString & _box, QString & _section,
                      QString & _type, QString & _uid, QString & _validity)
{
//  kdDebug(7116) << "imapParser::parseURL - " << endl;
  QStringList parameters;

  _box = _url.path ();
  parameters = QStringList::split (';', _box);  //split parameters
  if (parameters.count () > 0)  //assertion failure otherwise
    parameters.remove (parameters.begin ());  //strip path
  _box.truncate(_box.find (';')); // strip parameters
  for (QStringList::ConstIterator it (parameters.begin ());
       it != parameters.end (); ++it)
  {
    QString temp = (*it);

    // if we have a '/' separator we'll just nuke it
    int pt = temp.find ('/');
    if (pt > 0)
      temp.truncate(pt);
//    if(temp[temp.length()-1] == '/')
//      temp = temp.left(temp.length()-1);
    if (temp.find ("section=", 0, false) == 0)
      _section = temp.right (temp.length () - 8);
    else if (temp.find ("type=", 0, false) == 0)
      _type = temp.right (temp.length () - 5);
    else if (temp.find ("uid=", 0, false) == 0)
      _uid = temp.right (temp.length () - 4);
    else if (temp.find ("uidvalidity=", 0, false) == 0)
      _validity = temp.right (temp.length () - 12);
  }
//  kdDebug(7116) << "URL: section= " << _section << ", type= " << _type << ", uid= " << _uid << endl;
//  kdDebug(7116) << "URL: user() " << _url.user() << endl;
//  kdDebug(7116) << "URL: path() " << _url.path() << endl;
//  kdDebug(7116) << "URL: encodedPathAndQuery() " << _url.encodedPathAndQuery() << endl;

  if (!_box.isEmpty ())
  {
    if (_box[0] == '/')
      _box = _box.right (_box.length () - 1);
    if (!_box.isEmpty () && _box[_box.length () - 1] == '/')
      _box.truncate(_box.length() - 1);
  }
  kdDebug(7116) << "URL: box= " << _box << ", section= " << _section << ", type= " << _type << ", uid= " << _uid << ", validity= " << _validity << endl;
}


QCString imapParser::parseLiteralC(parseString & inWords, bool relay, bool stopAtBracket, int *outlen) {

  if (inWords[0] == '{')
  {
    QCString retVal;
    ulong runLen = inWords.find ('}', 1);
    if (runLen > 0)
    {
      bool proper;
      ulong runLenSave = runLen + 1;
      QCString tmpstr;
      inWords.takeMid(tmpstr, 1, runLen - 1);
      runLen = tmpstr.toULong (&proper);
      inWords.pos += runLenSave;
      if (proper)
      {
        //now get the literal from the server
        if (relay)
          parseRelay (runLen);
        QByteArray rv;
        parseRead (rv, runLen, relay ? runLen : 0);
        rv.resize(QMAX(runLen, rv.size())); // what's the point?
        retVal = b2c(rv);
        inWords.clear();
        parseReadLine (inWords.data); // must get more

        // no duplicate data transfers
        relay = false;
      }
      else
      {
        kdDebug(7116) << "imapParser::parseLiteral - error parsing {} - " /*<< strLen*/ << endl;
      }
    }
    else
    {
      inWords.clear();
      kdDebug(7116) << "imapParser::parseLiteral - error parsing unmatched {" << endl;
    }
    if (outlen) {
      *outlen = retVal.length(); // optimize me
    }
    skipWS (inWords);
    return retVal;
  }

  return parseOneWordC(inWords, stopAtBracket, outlen);
}

// does not know about literals ( {7} literal )
QCString imapParser::parseOneWordC (parseString & inWords, bool stopAtBracket, int *outLen)
{
  QCString retVal;
  uint retValSize = 0;
  uint len = inWords.length();

  if (len > 0 && inWords[0] == '"')
  {
    unsigned int i = 1;
    bool quote = FALSE;
    while (i < len && (inWords[i] != '"' || quote))
    {
      if (inWords[i] == '\\') quote = !quote;
      else quote = FALSE;
      i++;
    }
    if (i < len)
    {
      inWords.pos++;
      inWords.takeLeft(retVal, i - 1);
      len = i - 1;
#if 0
      static char *buf = 0L;
      static int buflen = 0;
      if (buflen < i) {
        buflen = i;
        buf = (char *)realloc(buf, buflen);
      }

      // If you're keen, you could do this in-place without the buffer
      int k = 0;
      for (unsigned int j = 0; j < len; j++) {
        if (retVal[j] != '\\') {
          buf[k++] = retVal[j];
        } else {
          j++; // skip the next character too
        }
      }
      buf[k] = 0;

      retVal = buf; // deep copy
      retValSize = k;
#else
      int offset = 0;
      for (unsigned int j = 0; j <= len; j++) {
        if (retVal[j] == '\\') {
          offset++;
          j++;
        }
        retVal[j - offset] = retVal[j];
      }
      retVal[len - offset] = 0;
      retValSize = len - offset;
#endif
      inWords.pos += i;
    }
    else
    {
      kdDebug(7116) << "imapParser::parseOneWord - error parsing unmatched \"" << endl;
      retVal = inWords.cstr();
      retValSize = len;
      inWords.clear();
    }
  }
  else
  {
    unsigned int i;
    for (i = 0; i < len; ++i) {
        char ch = inWords[i];
        if (ch <= ' ' || ch == '(' || ch == ')' ||
            (stopAtBracket && (ch == '[' || ch == ']')))
            break;
    }

    if (i < len)
    {
      inWords.takeLeft(retVal, i);
      retValSize = i;
      inWords.pos += i;
    }
    else
    {
      retVal = inWords.cstr();
      retValSize = len;
      inWords.clear();
    }
    if (retVal == "NIL") {
      retVal.truncate(0);
      retValSize = 0;
    }
  }
  skipWS (inWords);
  if (outLen) {
    *outLen = retValSize;
  }
  return retVal;
}

bool imapParser::parseOneNumber (parseString & inWords, ulong & num)
{
  bool valid;
  num = parseOneWordC(inWords, TRUE).toULong(&valid);
  return valid;
}

bool imapParser::hasCapability (const QString & cap)
{
  QString c = cap.lower();
//  kdDebug(7116) << "imapParser::hasCapability - Looking for '" << cap << "'" << endl;
  for (QStringList::ConstIterator it = imapCapabilities.begin ();
       it != imapCapabilities.end (); ++it)
  {
//    kdDebug(7116) << "imapParser::hasCapability - Examining '" << (*it) << "'" << endl;
    if (c == *it)
    {
      return true;
    }
  }
  return false;
}

