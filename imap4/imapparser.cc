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
  while (pl != -1 && !aCmd->isComplete ())
    while ((pl = parseLoop ()) == 0);

  return aCmd;
}

imapCommand *
imapParser::sendCommand (imapCommand * aCmd)
{
  aCmd->setId (QString ().setNum (commandCounter++));
  sentQueue.append (aCmd);

  continuation.resize(0);

  if (aCmd->command () == "SELECT" || aCmd->command () == "EXAMINE")
  {
     // we need to know which box we are selecting
    parseString p;
    p.fromString(aCmd->parameter());
    currentBox = b2c(parseOneWord(p));
    kdDebug(7116) << "imapParser::sendCommand - setting current box to " << currentBox << endl;
  }
  else if (aCmd->command () == "CLOSE")
  {
     // we no longer have a box open
    currentBox = QString::null;
  }
  else if (aCmd->command ().find ("SEARCH") != -1)
  {
    lastResults.clear ();
  }
  else if (aCmd->command ().find ("LIST") != -1)
  {
    listResponses.clear ();
  }
  else if (aCmd->command ().find ("LSUB") != -1)
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
//  kdDebug(7116) << "imapParser::parseUntagged - '" << result << "'" << endl;

  parseOneWord (result);        // *
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

  case 'L':                    // LIST or LSUB
    if (qstrncmp(what, "LIST", what.size()) == 0)
    {
      parseList (result);
    }
    else if (qstrncmp(what, "LSUB", what.size()) == 0)
    {
      parseLsub (result);
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
  if (command == "SELECT") selectInfo.setReadWrite(true);

  if (rest[0] == '[')
  {
    rest.pos++;
    QByteArray option = parseOneWord (rest, TRUE);

    switch (option[0])
    {
    case 'A':                  // ALERT
      if (qstrncmp(option, "ALERT", option.size()) == 0)
      {
      }
      break;

    case 'N':                  // NEWNAME
      if (qstrncmp(option, "NEWNAME", option.size()) == 0)
      {
      }
      break;

    case 'P':                  //PARSE or PERMANENTFLAGS
      if (qstrncmp(option, "PARSE", option.size()) == 0)
      {
      }
      else if (qstrncmp(option, "PERMANENTFLAGS", option.size()) == 0)
      {
        uint end = rest.data.find(']', rest.pos);
        QCString flags(rest.data.data() + rest.pos, end - rest.pos);
        selectInfo.setPermanentFlags (flags);
      }
      break;

    case 'R':                  //READ-ONLY or READ-WRITE
      if (qstrncmp(option, "READ-ONLY", option.size()) == 0)
      {
        selectInfo.setReadWrite (false);
      }
      else if (qstrncmp(option, "READ-WRITE", option.size()) == 0)
      {
        selectInfo.setReadWrite (true);
      }
      break;

    case 'T':                  //TRYCREATE
      if (qstrncmp(option, "TRYCREATE", option.size()) == 0)
      {
      }
      break;

    case 'U':                  //UIDVALIDITY or UNSEEN
      if (qstrncmp(option, "UIDVALIDITY", option.size()) == 0)
      {
        ulong value;
        if (parseOneNumber (rest, value))
          selectInfo.setUidValidity (value);
      }
      else if (qstrncmp(option, "UNSEEN", option.size()) == 0)
      {
        ulong value;
        if (parseOneNumber (rest, value))
          selectInfo.setUnseen (value);
      }
      else if (qstrncmp(option, "UIDNEXT", option.size()) == 0)
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
  QString action = command;
  if (command.isEmpty())
  {
    QByteArray action = parseOneWord (rest);
    if (qstrncmp(action, "UID", action.size()) == 0)
      action = parseOneWord (rest);
  }

  switch (action[0].latin1 ())
  {
  case 'A':
    if (action == "AUTHENTICATE")
      if (qstrncmp(result, "OK", result.size()) == 0)
        currentState = ISTATE_LOGIN;
    break;

  case 'L':
    if (action == "LOGIN")
      if (qstrncmp(result, "OK", result.size()) == 0)
        currentState = ISTATE_LOGIN;
    break;

  case 'E':
    if (action == "EXAMINE")
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
    if (action == "SELECT")
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
  imapCapabilities = QStringList::split (" ", result.cstr());
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
    QByteArray ba = imapParser::parseOneWord(result);
    attribute = QCString(ba.data(), ba.size() + 1);
    if (-1 != attribute.find ("\\Noinferiors", 0, false))
      this_one.setNoInferiors (true);
    else if (-1 != attribute.find ("\\Noselect", 0, false))
      this_one.setNoSelect (true);
    else if (-1 != attribute.find ("\\Marked", 0, false))
      this_one.setMarked (true);
    else if (-1 != attribute.find ("\\Unmarked", 0, false))
      this_one.setUnmarked (true);
    else if (-1 != attribute.find ("\\HasNoChildren", 0, false)) {
      // not important
    } else
      kdDebug(7116) << "imapParser::parseList - unknown attribute " << attribute << endl;
  }

  result.pos++; // tie off )
  imapParser::skipWS (result);

  this_one.setHierarchyDelimiter(imapParser::parseLiteral(result));
  this_one.setName (rfcDecoder::fromIMAP (imapParser::parseLiteral (result)));  // decode modified UTF7

  listResponses.append (this_one);
}

void imapParser::parseLsub (parseString & result)
{
  imapList this_one (result.cstr());
  listResponses.append (this_one);
}

void imapParser::parseSearch (parseString & result)
{
  QString entry;
  ulong value;

  while (parseOneNumber (result, value))
  {
    lastResults.append (QString ().setNum (value));
  }
}

void imapParser::parseStatus (parseString & inWords)
{
  lastStatus = imapInfo ();

  parseLiteral(inWords);       // swallow the box
  if (inWords[0] != '(')
    return;

  inWords.pos++;
  skipWS (inWords);

  while (!inWords.isEmpty() && inWords[0] != ')')
  {
    QByteArray label;
    ulong value;

    label = parseOneWord (inWords);
    if (parseOneNumber (inWords, value))
    {
      if (qstrncmp(label, "MESSAGES", label.size()) == 0)
        lastStatus.setCount (value);
      else if (qstrncmp(label, "RECENT", label.size()) == 0)
        lastStatus.setRecent (value);
      else if (qstrncmp(label, "UIDVALIDITY", label.size()) == 0)
        lastStatus.setUidValidity (value);
      else if (qstrncmp(label, "UNSEEN", label.size()) == 0)
        lastStatus.setUnseen (value);
      else if (qstrncmp(label, "UIDNEXT", label.size()) == 0)
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

QValueList < mailAddress > imapParser::parseAddressList (parseString & inWords)
{
  QValueList < mailAddress > retVal;

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
      if (inWords[0] == '(')
        retVal.append (parseAddress (inWords));
      else
        break;
    }

    if (inWords[0] == ')')
      inWords.pos++;
    skipWS (inWords);
  }

  return retVal;
}

mailAddress imapParser::parseAddress (parseString & inWords)
{
  QByteArray user, host, full, comment;
  mailAddress retVal;

  if (inWords[0] != '(')
    return retVal;
  inWords.pos++;
  skipWS (inWords);

  full = parseLiteral (inWords);
  comment = parseLiteral (inWords);
  user = parseLiteral (inWords);
  host = parseLiteral (inWords);

  retVal.setFullName (rfcDecoder::quoteIMAP(QCString(full, full.size() + 1)));
  retVal.setCommentRaw (QCString(comment, comment.size() + 1));
  retVal.setUser (QCString(user, user.size() + 1));
  retVal.setHost (QCString(host, host.size() + 1));

  if (inWords[0] == ')') inWords.pos++;
  skipWS (inWords);

  return retVal;
}

mailHeader * imapParser::parseEnvelope (parseString & inWords)
{
  mailHeader *envelope = NULL;
  QValueList < mailAddress > list;

  if (inWords[0] != '(')
    return envelope;
  inWords.pos++;
  skipWS (inWords);

  envelope = new mailHeader;

  //date
  QString date = parseLiteral (inWords);
  envelope->setDate (date.ascii ());

  //subject
  QString subject = parseLiteral (inWords);
  envelope->setSubjectEncoded (subject.ascii ());

  //from
  list = parseAddressList (inWords);
  for (QValueListIterator < mailAddress > it = list.begin ();
       it != list.end (); ++it)
  {
    envelope->setFrom ((*it));
  }

  //sender
  list = parseAddressList (inWords);
  for (QValueListIterator < mailAddress > it = list.begin ();
       it != list.end (); ++it)
  {
    envelope->setSender ((*it));
  }

  //reply-to
  list = parseAddressList (inWords);
  for (QValueListIterator < mailAddress > it = list.begin ();
       it != list.end (); ++it)
  {
    envelope->setReplyTo ((*it));
  }

  //to
  list = parseAddressList (inWords);
  for (QValueListIterator < mailAddress > it = list.begin ();
       it != list.end (); ++it)
  {
    envelope->addTo ((*it));
  }

  //cc
  list = parseAddressList (inWords);
  for (QValueListIterator < mailAddress > it = list.begin ();
       it != list.end (); ++it)
  {
    envelope->addCC ((*it));
  }

  //bcc
  list = parseAddressList (inWords);
  for (QValueListIterator < mailAddress > it = list.begin ();
       it != list.end (); ++it)
  {
    envelope->addBCC ((*it));
  }

  //in-reply-to
  QString reply = parseLiteral (inWords);
  envelope->setInReplyTo (reply.ascii ());

  //message-id
  QString message = parseLiteral (inWords);
  envelope->setMessageId (message.ascii ());

  // see if we have more to come
  while (!inWords.isEmpty () && inWords[0] != ')')
  {
    //eat the extensions to this part
    if (inWords[0] == '(')
      parseSentence (inWords);
    else
      parseLiteral (inWords);
  }

  if (inWords[0] == ')')
    inWords.pos++;
  skipWS (inWords);

  return envelope;
}

// parse parameter pairs into a dictionary
// caller must clean up the dictionary items
QDict < QString > imapParser::parseDisposition (parseString & inWords)
{
  QByteArray disposition;
  QDict < QString > retVal (17, false);

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
    QString str = b2c(disposition);
    retVal.insert ("content-disposition", new QString(str));
  }

  return retVal;
}

// parse parameter pairs into a dictionary
// caller must clean up the dictionary items
QDict < QString > imapParser::parseParameters (parseString & inWords)
{
  QDict < QString > retVal (17, false);

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
      QByteArray label, value;
      label = parseLiteral (inWords);
      value = parseLiteral (inWords);
      QString str = b2c(value);
      retVal.insert (b2c(label), new QString (str));
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
  QByteArray type, subtype, id, description, encoding;
  QDict < QString > parameters (17, false);
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
  type = parseLiteral (inWords);

  //body subtype
  subtype = parseLiteral (inWords);

  localPart->setType (b2c(type) + "/" + b2c(subtype));

  //body parameter parenthesized list
  parameters = parseParameters (inWords);
  {
    QDictIterator < QString > it (parameters);

    while (it.current ())
    {
      localPart->setTypeParm (it.currentKey ().ascii (), *(it.current ()));
      ++it;
    }
    parameters.clear ();
  }

  //body id
  id = parseLiteral (inWords);
  localPart->setID (b2c(id));

  //body description
  description = parseLiteral (inWords);
  localPart->setDescription (b2c(description));

  //body encoding
  encoding = parseLiteral (inWords);
  localPart->setEncoding (b2c(encoding));

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
    if (type.data() ==  "TEXT")
    {
      //text lines
      ulong lines;
      parseOneNumber (inWords, lines);
    }

    // md5
    parseLiteral (inWords);

    // body disposition
    parameters = parseDisposition (inWords);
    {
      QDictIterator < QString > it (parameters);
      QString *disposition = parameters[QString ("content-disposition")];

      if (disposition)
        localPart->setDisposition (disposition->ascii ());
      parameters.remove (QString ("content-disposition"));
      while (it.current ())
      {
        localPart->setDispositionParm (it.currentKey ().ascii (),
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
      parseLiteral (inWords);
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
    QDict < QString > parameters (17, false);
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
      outSection = "";
      section++;
      outSection.setNum (section);
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
      QDictIterator < QString > it (parameters);

      while (it.current ())
      {
        localPart->setTypeParm (it.currentKey ().ascii (), *(it.current ()));
        ++it;
      }
      parameters.clear ();
    }

    // body disposition
    parameters = parseDisposition (inWords);
    {
      QDictIterator < QString > it (parameters);
      QString *disposition = parameters[QString ("content-disposition")];

      if (disposition)
        localPart->setDisposition (disposition->ascii ());
      parameters.remove (QString ("content-disposition"));
      while (it.current ())
      {
        localPart->setDispositionParm (it.currentKey ().ascii (),
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
      parseLiteral (inWords);
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
        parseLiteral (inWords, true);
      }
      else
      {
        kdDebug(7116) << "imapParser::parseBody - reading " << envelope << " " << seenUid.ascii () << endl;
        // fill it up with data
        QString theHeader = parseLiteral (inWords, true);
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
         parseLiteral (inWords, true);
       }
       else
       {
         QByteArray res = parseLiteral (inWords, true);
         QCString references = QCString(res.data(), res.size()+1);
         int start = references.find ('<');
         int end = references.findRev ('>');
         if (start < end)
                 references = references.mid (start, end - start + 1);
         references = references.simplifyWhiteSpace();
         envelope->setReferences(references);
       }
      }
      else
      { // not a header we care about throw it away
        parseLiteral (inWords, true);
      }
    }
    else
    {
      // throw it away
      parseLiteral (inWords, true);
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
      QByteArray array = parseLiteral(inWords, false, true);
      QCString word(array, array.size() + 1);

      switch (array[0])
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
          parseLiteral (inWords, true);
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
          seenUid = b2c(parseOneWord(inWords));
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
          parseLiteral (inWords, true);
        }
        break;

      case 'I':
        if (word == "INTERNALDATE")
        {
          QCString date;
          date = b2c(parseOneWord (inWords));
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
        parseLiteral (inWords);
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
      parseLiteral (inWords);
  }

  if (inWords[0] != ')')
    return;
  inWords.pos++;
  skipWS (inWords);
}


// default parser
void imapParser::parseSentence (parseString & inWords)
{
  QString stack;
  bool first = true;

  //find the first nesting parentheses

  while (!inWords.isEmpty () && (!stack.isEmpty () || first))
  {
    first = false;
    skipWS (inWords);

    unsigned char ch = inWords[0];
    switch (ch)
    {
    case '(':
      inWords.pos++;
      stack += ')';
      break;
    case ')':
      inWords.pos++;
      stack.truncate(stack.length() - 1);
      break;
    case '[':
      inWords.pos++;
      stack += ']';
      break;
    case ']':
      inWords.pos++;
      stack.truncate(stack.length() - 1);
      break;
    default:
      parseLiteral (inWords);
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

  if (result.data.isNull ())
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
        QByteArray tag, resultCode;

        tag = parseLiteral (result);
        if (b2c(tag) == current->id().latin1())
        {
          result.data.resize(result.data.size() - 2);  // tie off CRLF
          resultCode = parseLiteral (result); //the result
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
          kdDebug(7116) << "imapParser::parseLoop - unknown tag '" << b2c(tag) << "'" << endl;
          QCString cstr = b2c(tag) + " " + result.cstr();
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
  parameters = QStringList::split (";", _box);  //split parameters
  if (parameters.count () > 0)  //assertion failure otherwise
    parameters.remove (parameters.begin ());  //strip path
  _box = _box.left (_box.find (';')); // strip parameters
  for (QStringList::ConstIterator it (parameters.begin ());
       it != parameters.end (); ++it)
  {
    QString temp = (*it);

    // if we have a '/' separator we'll just nuke it
    if (temp.find ("/") > 0)
      temp = temp.left (temp.find ("/"));
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

void imapParser::skipWS (parseString & inWords)
{
  while (!inWords.isEmpty() &&
    (inWords[0] == ' ' || inWords[0] == '\t'
    || inWords[0] == '\r' || inWords[0] == '\n'))
  {
    inWords.pos++;
  }
}

QByteArray imapParser::parseLiteral (parseString & inWords, bool relay, bool stopAtBracket)
{
  QByteArray retVal;

  if (inWords[0] == '{')
  {
    ulong runLen;
    QString strLen;

    runLen = inWords.find ('}', 1);
    if (runLen > 0)
    {
      bool proper;
      strLen = inWords.mid(1, runLen - 1);
      inWords.pos += runLen + 1;
      runLen = strLen.toULong (&proper);
      if (proper)
      {
        //now get the literal from the server
        QByteArray fill;

        if (relay)
          parseRelay (runLen);
        parseRead (fill, runLen, relay ? runLen : 0);
        retVal = fill;
        retVal.resize(QMAX(runLen, retVal.size()));
        inWords.clear();
        parseReadLine (inWords.data); // must get more

        // no duplicate data transfers
        relay = false;
      }
      else
      {
        kdDebug(7116) << "imapParser::parseLiteral - error parsing {} - " << strLen << endl;
      }
    }
    else
    {
      inWords.clear();
      kdDebug(7116) << "imapParser::parseLiteral - error parsing unmatched {" << endl;
    }
  }
  else
  {
    retVal = parseOneWord(inWords, stopAtBracket);
  }
  skipWS (inWords);
  return retVal;
}

// does not know about literals ( {7} literal )

QByteArray imapParser::parseOneWord (parseString & inWords, bool stopAtBracket)
{
  QCString retVal;

  if (inWords.length() && inWords[0] == '"')
  {
    unsigned int i = 1;
    bool quote = FALSE;
    while (i < inWords.length() && (inWords[i] != '"' || quote))
    {
      if (inWords[i] == '\\') quote = !quote;
      else quote = FALSE;
      i++;
    }
    if (i < inWords.length())
    {
      inWords.pos++;
      retVal = inWords.left(i - 1);
      for (unsigned int j = 0; j < retVal.length(); j++)
        if (retVal[j] == '\\') retVal.remove(j, 1);
      inWords.pos += i;
    }
    else
    {
      kdDebug(7116) << "imapParser::parseOneWord - error parsing unmatched \"" << endl;
      retVal = inWords.cstr();
      inWords.clear();
    }
  }
  else
  {
      unsigned int i;
      for (i = 0; i < inWords.length(); ++i) {
          char ch = inWords[i];
          if (ch <= ' ' || ch == '(' || ch == ')' ||
              (stopAtBracket && (ch == '[' || ch == ']')))
              break;
      }

    if (i < inWords.length())
    {
      retVal = inWords.left (i);
      inWords.pos += i;
    }
    else
    {
      retVal = inWords.cstr();
      inWords.clear();
    }
    if (retVal == "NIL")
      retVal = "";
  }
  skipWS (inWords);
  QByteArray ba;
  ba.duplicate(retVal.data(), retVal.length());
  return ba;
}

bool imapParser::parseOneNumber (parseString & inWords, ulong & num)
{
  bool valid;
  num = b2c(parseOneWord(inWords, TRUE)).toULong(&valid);
  return valid;
}

bool imapParser::hasCapability (const QString & cap)
{
//  kdDebug(7116) << "imapParser::hasCapability - Looking for '" << cap << "'" << endl;
  for (QStringList::Iterator it = imapCapabilities.begin ();
       it != imapCapabilities.end (); ++it)
  {
//    kdDebug(7116) << "imapParser::hasCapability - Examining '" << (*it) << "'" << endl;
    if (cap.lower () == (*it).lower ())
    {
      return true;
    }
  }
  return false;
}

