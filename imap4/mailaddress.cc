// $Id$
/**********************************************************************
 *
 *   mailaddress.cc  - mail address parser
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


#include "mailaddress.h"
#include "rfcdecoder.h"

mailAddress::mailAddress() :
	user((const char *)NULL),
	host((const char *)NULL),
	fullName((const char *)NULL),
	comment((const char *)NULL)
{
}

mailAddress::mailAddress(const mailAddress &lr) :
	user(lr.user),
	host(lr.host),
	fullName(lr.fullName),
	comment(lr.comment)
{
}

mailAddress &mailAddress::operator = (const mailAddress & lr)
{
  // Avoid a = a.
  if (this == &lr)
    return *this;

	user = lr.user;
	host = lr.host;
	fullName = lr.fullName;
	comment = lr.comment;
	
  return *this;
}




mailAddress::~mailAddress(){
}

mailAddress::mailAddress(char * aCStr) :
	user((const char *)NULL),
	host((const char *)NULL),
	fullName((const char *)NULL),
	comment((const char *)NULL)
{
	parseAddress(aCStr);
}

int mailAddress::parseAddress(char *aCStr){
	int retVal=0;
/*	int skip;
			
	if(aCStr) {
		//skip leading white space
		skip = rfcDecoder::skipWS((const char *)aCStr);
		if(skip > 0) { aCStr += skip;retVal+=skip;}
		while(*aCStr) {
			int advance;
			
  		switch( *aCStr )
  		{
  			case '"' :
  				advance = rfcDecoder::parseQuoted('"','"',aCStr);
  				fullName += QCString(aCStr,advance+1);
  				break;
  			case '(' :
  				advance = rfcDecoder::parseQuoted('(',')',aCStr);
  				comment += QCString(aCStr,advance+1);
  				break;
  			case '<' :
  				advance = rfcDecoder::parseQuoted('<','>',aCStr);
  				mailName = QCString(aCStr,advance+1);
  				break;
  			default:
  				advance = rfcDecoder::parseWord((const char *)aCStr);
  				//if we've seen a FQ mailname the rest must be quoted or is just junk
  				if(mailName.isEmpty())
  				{
    				if(*aCStr != ',')
    				{
      				fullName += QCString(aCStr,advance+1);
          		if(rfcDecoder::skipWS((const char *)&aCStr[advance]) > 0)
          		{
  	    				fullName += ' ';
          		}
        		}
  				}
  				break;
  		}
  		if(advance)
  		{
  			retVal += advance;
  			aCStr += advance;
  		} else break;
  		advance = rfcDecoder::skipWS((const char *)aCStr);
  		if(advance > 0)
  		{
  			retVal += advance;
  			aCStr += advance;
  		}
  		//reached end of current address
  		if(*aCStr == ',')
  		{
  			advance++;
  			break;
  		}
		}
		//let's see what we've got
		if(fullName.isEmpty())
		{
			if(mailName.isEmpty()) retVal = 0;
			else {
  			if(mailName.find('@') < 0)
  			{
  				fullName = mailName;
  				mailName = "";
  			}
			}
		} else if(mailName.isEmpty()) {
			if(fullName.find('@') >= 0)
			{
				mailName = fullName;
				fullName = "";
			}
		}
		//get rid of <> and ""
		if(!fullName.isEmpty())
		{
			if(fullName[0] == '"')
				fullName = fullName.mid(1,fullName.length()-2);
			fullName = fullName.simplifyWhiteSpace().stripWhiteSpace();
			fullName = rfcDecoder::decodeRFC1522String(fullName.ascii());
		}
		if(!mailName.isEmpty() && mailName[0] == '<')
			mailName = mailName.mid(1,mailName.length()-2);
		if(!comment.isEmpty())
		{
			if(comment[0] == '(')
				comment = comment.mid(1,comment.length()-2);
			comment = comment.simplifyWhiteSpace().stripWhiteSpace();
			comment = rfcDecoder::decodeRFC1522String(comment.ascii());
		}
	} else {
		//debug();
	} */
	return retVal;
}

const QCString mailAddress::getStr()
{
	QCString retVal;
	
//	qDebug("mailAddress::getStr - \"%s\" <%s@%s> (%s)",fullName.ascii(),user.data(),host.data(),comment.ascii());
//	qDebug("mailAddress::getStr - fullname '%s'",rfcDecoder::encodeRFC2047String(fullName).ascii());
//	qDebug("mailAddress::getStr - comment '%s'",rfcDecoder::encodeRFC2047String(comment).ascii());
	if(!fullName.isEmpty())
	{
		retVal = QCString("\"") + rfcDecoder::encodeRFC2047String(fullName).ascii() + "\" ";
	}
	if(!user.isEmpty())
	{
		retVal += "<" + user;
		if(!host.isEmpty()) retVal += "@" + host;
		retVal += ">";
	}	
	if(!comment.isEmpty())
	{
		retVal = '(' + rfcDecoder::encodeRFC2047String(comment).ascii() + ')';
	}
//	qDebug("mailAddress::getStr - '%s'",retVal.data());
	return retVal;
}

bool mailAddress::isEmpty()
{
	return (user.isEmpty());
}

void mailAddress::setFullNameRaw(const QCString &_str)
{
	fullName = rfcDecoder::decodeRFC2047String(_str);
};

void mailAddress::setCommentRaw(const QCString &_str)
{
	comment = rfcDecoder::decodeRFC2047String(_str);
};
