/***************************************************************************
                          mailheader.cc  -  description
                             -------------------
    begin                : Tue Oct 24 2000
    copyright            : (C) 2000 by Sven Carstens
    email                : s.carstens@gmx.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "mailheader.h"
#include "rfcdecoder.h"

mailHeader::mailHeader ()
{
  toAdr.setAutoDelete (true);
  ccAdr.setAutoDelete (true);
  bccAdr.setAutoDelete (true);
  setType ("text/plain");
  gmt_offset = 0;
}

mailHeader::~mailHeader ()
{
}

void
mailHeader::addHdrLine (mimeHdrLine * inLine)
{
  mimeHdrLine *addLine = new mimeHdrLine (inLine);
  if (!addLine)
	  return;

  const QCString label(addLine->getLabel());
  const QCString value(addLine->getValue());
  
  if (!qstricmp (label, "Return-Path")) {
	returnpathAdr.parseAddress (value.data ());
	goto out;
  }
  if (!qstricmp (label, "Sender")) {
	senderAdr.parseAddress (value.data ());
	goto out;
  }
  if (!qstricmp (label, "From")) {
	fromAdr.parseAddress (value.data ());
	goto out;
  }
  if (!qstricmp (label, "Reply-To")) {
	replytoAdr.parseAddress (value.data ());
	goto out;
  }
  if (!qstricmp (label, "To")) {
	mailHeader::parseAddressList (value, &toAdr);
	goto out;
  }
  if (!qstricmp (label, "CC")) {
	mailHeader::parseAddressList (value, &ccAdr);
	goto out;
  }
  if (!qstricmp (label, "BCC")) {
	mailHeader::parseAddressList (value, &bccAdr);
	goto out;
  }
  if (!qstricmp (label, "Subject")) {
	_subject = value.stripWhiteSpace().simplifyWhiteSpace();
	goto out;
  }
  if (!qstricmp (label.data (), "Date")) {
	mDate = value;
	goto out;
  }
  if (!qstricmp (label.data (), "Message-ID")) {
      int start = value.findRev ('<');
      int end = value.findRev ('>');
      if (start < end)
          messageID = value.mid (start, end - start + 1);
      else {
	  qWarning("bad Message-ID");
          /* messageID = value; */
      }
      goto out;
  }
  if (!qstricmp (label.data (), "In-Reply-To")) {
      int start = value.findRev ('<');
      int end = value.findRev ('>');
      if (start < end)
        inReplyTo = value.mid (start, end - start + 1);
      goto out;
  }

  // everything else is handled by mimeHeader
  mimeHeader::addHdrLine (inLine);
  delete addLine;
  return;
  
 out:
//  cout << label.data() << ": '" << value.data() << "'" << endl;

  //need only to add this line if not handled by mimeHeader       
  originalHdrLines.append (addLine);
}

void
mailHeader::outputHeader (mimeIO & useIO)
{
  if (!returnpathAdr.isEmpty ())
    useIO.outputMimeLine (QCString ("Return-Path: ") +
                          returnpathAdr.getStr ());
  if (!fromAdr.isEmpty ())
    useIO.outputMimeLine (QCString ("From: ") + fromAdr.getStr ());
  if (!senderAdr.isEmpty ())
    useIO.outputMimeLine (QCString ("Sender: ") + senderAdr.getStr ());
  if (!replytoAdr.isEmpty ())
    useIO.outputMimeLine (QCString ("Reply-To: ") + replytoAdr.getStr ());

  if (toAdr.count ())
    useIO.
      outputMimeLine (mimeHdrLine::
                      truncateLine (QCString ("To: ") +
                                    mailHeader::getAddressStr (&toAdr)));
  if (ccAdr.count ())
    useIO.
      outputMimeLine (mimeHdrLine::
                      truncateLine (QCString ("CC: ") +
                                    mailHeader::getAddressStr (&ccAdr)));
  if (bccAdr.count ())
    useIO.
      outputMimeLine (mimeHdrLine::
                      truncateLine (QCString ("BCC: ") +
                                    mailHeader::getAddressStr (&bccAdr)));
  if (!_subject.isEmpty ())
    useIO.
      outputMimeLine (mimeHdrLine::
                      truncateLine (QCString ("Subject: ") + _subject));
  if (!messageID.isEmpty ())
    useIO.
      outputMimeLine (mimeHdrLine::
                      truncateLine (QCString ("Message-ID: ") + messageID));
  if (!inReplyTo.isEmpty ())
    useIO.
      outputMimeLine (mimeHdrLine::
                      truncateLine (QCString ("In-Reply-To: ") + inReplyTo));
  if (!mDate.isEmpty())
    useIO.outputMimeLine (QCString ("Date: ") + mDate);
  mimeHeader::outputHeader (useIO);
}

int
mailHeader::parseAddressList (const char *inCStr,
                              QPtrList < mailAddress > *aList)
{
  int advance = 0;
  int skip = 1;
  char *aCStr = (char *) inCStr;

  if (!aCStr || !aList)
    return 0;
  while (skip > 0)
  {
    mailAddress *aAddress = new mailAddress;
    skip = aAddress->parseAddress (aCStr);
    if (skip)
    {
      aCStr += skip;
      if (skip < 0)
        advance -= skip;
      else
        advance += skip;
      aList->append (aAddress);
    }
    else
    {
      delete aAddress;
      break;
    }
  }
  return advance;
}

QCString
mailHeader::getAddressStr (QPtrList < mailAddress > *aList)
{
  QCString retVal;

  QPtrListIterator < mailAddress > it = QPtrListIterator < mailAddress > (*aList);
  while (it.current ())
  {
    retVal += it.current ()->getStr ();
    ++it;
    if (it.current ())
      retVal += ", ";
  }
  return retVal;
}
