// $Id$
/**********************************************************************
 *
 *   imapinfo.cc  - IMAP4rev1 SELECT / EXAMINE handler
 *   Copyright (C) 2000
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

#include "imapinfo.h"

imapInfo::imapInfo()
  : count_(0),
    recent_(0),
    unseen_(0),
    uidValidity_(0),
	uidNext_(0),
    flags_(0),
    permanentFlags_(0),
    readWrite_(false),
    countAvailable_(false),
    recentAvailable_(false),
    unseenAvailable_(false),
    uidValidityAvailable_(false),
	uidNextAvailable_(false),
    flagsAvailable_(false),
    permanentFlagsAvailable_(false),
    readWriteAvailable_(false)
{
}

imapInfo::imapInfo(const imapInfo & mi)
  : count_(mi.count_),
    recent_(mi.recent_),
    unseen_(mi.unseen_),
    uidValidity_(mi.uidValidity_),
    uidNext_(mi.uidNext_),
    flags_(mi.flags_),
    permanentFlags_(mi.permanentFlags_),
    readWrite_(mi.readWrite_),
    countAvailable_(mi.countAvailable_),
    recentAvailable_(mi.recentAvailable_),
    unseenAvailable_(mi.unseenAvailable_),
    uidValidityAvailable_(mi.uidValidityAvailable_),
    uidNextAvailable_(mi.uidNextAvailable_),
    flagsAvailable_(mi.flagsAvailable_),
    permanentFlagsAvailable_(mi.permanentFlagsAvailable_),
    readWriteAvailable_(mi.readWriteAvailable_)
{
}

  imapInfo &
imapInfo::operator = (const imapInfo & mi)
{
  // Avoid a = a.
  if (this == &mi)
    return *this;

  count_ = mi.count_;
  recent_ = mi.recent_;
  unseen_ = mi.unseen_;
  uidValidity_ = mi.uidValidity_;
  uidNext_ = mi.uidNext_;
  flags_ = mi.flags_;
  permanentFlags_ = mi.permanentFlags_;
  readWrite_ = mi.readWrite_;
  countAvailable_ = mi.countAvailable_;
  recentAvailable_ = mi.recentAvailable_;
  unseenAvailable_ = mi.unseenAvailable_;
  uidValidityAvailable_ = mi.uidValidityAvailable_;
  uidNextAvailable_ = mi.uidNextAvailable_;
  flagsAvailable_ = mi.flagsAvailable_;
  permanentFlagsAvailable_ = mi.permanentFlagsAvailable_;
  readWriteAvailable_ = mi.readWriteAvailable_;

  return *this;
}

imapInfo::imapInfo(const QStringList & list)
  : count_(0),
    recent_(0),
    unseen_(0),
    uidValidity_(0),
    uidNext_(0),
    flags_(0),
    permanentFlags_(0),
    readWrite_(false),
    countAvailable_(false),
    recentAvailable_(false),
    unseenAvailable_(false),
    uidValidityAvailable_(false),
    uidNextAvailable_(false),
    flagsAvailable_(false),
    permanentFlagsAvailable_(false),
    readWriteAvailable_(false)
{
  for (QStringList::ConstIterator it(list.begin()); it != list.end(); ++it)
  {
    QString line(*it);

	line = line.left(line.length()-2);
    QStringList tokens(QStringList::split(' ', line));

	qDebug("Processing: %s",line.latin1());
    if (tokens[0] != "*")
      continue;

    if (tokens[1] == "OK")
	{
    	if (tokens[2] == "[UNSEEN")
    	  setUnseen(tokens[3].left(tokens[3].length() - 1).toULong());

    	else if (tokens[2] == "[UIDVALIDITY")
    	  setUidValidity(tokens[3].left(tokens[3].length() - 1).toULong());

    	else if (tokens[2] == "[UIDNEXT")
    	  setUidNext(tokens[3].left(tokens[3].length() - 1).toULong());

    	else if (tokens[2] == "[PERMANENTFLAGS") {
    	  int flagsStart  = line.find('(');
    	  int flagsEnd    = line.find(')');

			qDebug("Checking permFlags from %d to %d",flagsStart,flagsEnd);
    	  if ((-1 != flagsStart) && (-1 != flagsEnd) && flagsStart < flagsEnd)
        	setPermanentFlags(_flags(line.mid(flagsStart, flagsEnd)));

    	} else if (tokens[2] == "[READ-WRITE") {
		  setReadWrite(true);
    	} else if (tokens[2] == "[READ-ONLY") {
		  setReadWrite(false);
    	} else {
			qDebug("unknown token2: %s",tokens[2].latin1());
		}
    }else if (tokens[1] == "FLAGS") {
      int flagsStart  = line.find('(');
      int flagsEnd    = line.find(')');

      if ((-1 != flagsStart) && (-1 != flagsEnd) && flagsStart < flagsEnd)
        setFlags(_flags(line.mid(flagsStart, flagsEnd)));
    } else {
    	if (tokens[2] == "EXISTS")
    	  setCount(tokens[1].toULong());

    	else if (tokens[2] == "RECENT")
    	  setRecent(tokens[1].toULong());
		  
		else
		  qDebug("unknown token1/2: %s %s",tokens[1].latin1(),tokens[2].latin1());
	}
  }

}

  ulong
imapInfo::_flags(const QString & flagsString) const
{
  ulong flags = 0;

  if (0 != flagsString.contains("\\Seen"))
    flags ^= Seen;
  if (0 != flagsString.contains("\\Answered"))
    flags ^= Answered;
  if (0 != flagsString.contains("\\Flagged"))
    flags ^= Flagged;
  if (0 != flagsString.contains("\\Deleted"))
    flags ^= Deleted;
  if (0 != flagsString.contains("\\Draft"))
    flags ^= Draft;
  if (0 != flagsString.contains("\\Recent"))
    flags ^= Recent;
  if (0 != flagsString.contains("\\*"))
    flags ^= User;

  return flags;
}
