/**********************************************************************
 *
 *   imapinfo.cc  - IMAP4rev1 EXAMINE / SELECT handler
 *   Copyright (C) 2000 Sven Carstens
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
 *   Send comments and bug fixes to
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

#include "rfcdecoder.h"
#include "imaplist.h"
#include "imapparser.h"

#include <kdebug.h>

imapList::imapList ():noInferiors_ (false),
noSelect_ (false), marked_ (false), unmarked_ (false)
{
}

imapList::imapList (const imapList & lr):hierarchyDelimiter_ (lr.hierarchyDelimiter_),
name_ (lr.name_),
noInferiors_ (lr.noInferiors_),
noSelect_ (lr.noSelect_), marked_ (lr.marked_), unmarked_ (lr.unmarked_)
{
}

imapList & imapList::operator = (const imapList & lr)
{
  // Avoid a = a.
  if (this == &lr)
    return *this;

  hierarchyDelimiter_ = lr.hierarchyDelimiter_;
  name_ = lr.name_;
  noInferiors_ = lr.noInferiors_;
  noSelect_ = lr.noSelect_;
  marked_ = lr.marked_;
  unmarked_ = lr.unmarked_;

  return *this;
}

imapList::imapList (const QString & inStr):noInferiors_ (false),
noSelect_ (false),
marked_ (false), unmarked_ (false)
{
  parseString s;
  s.data.duplicate(inStr.latin1(), inStr.length());

  if (s[0] != '(')
    return;                     //not proper format for us

  s.pos++;  // tie off (

  //process the attributes
  QCString attribute;

  while (!s.isEmpty () && s[0] != ')')
  {
    attribute = imapParser::parseOneWordC(s).lower();
    if (-1 != attribute.find ("\\noinferiors"))
      noInferiors_ = true;
    else if (-1 != attribute.find ("\\noselect"))
      noSelect_ = true;
    else if (-1 != attribute.find ("\\marked"))
      marked_ = true;
    else if (-1 != attribute.find ("\\unmarked"))
      unmarked_ = true;
    else
      kdDebug(7116) << "imapList::imapList: bogus attribute " << attribute << endl;
  }

  s.pos++;  // tie off )
  imapParser::skipWS (s);

  hierarchyDelimiter_ = imapParser::parseOneWordC(s);
  if (hierarchyDelimiter_ == "NIL")
    hierarchyDelimiter_ = QString::null;
  name_ = rfcDecoder::fromIMAP (imapParser::parseOneWord (s));  // decode modified UTF7
}
