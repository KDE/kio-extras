/**********************************************************************
 *
 *   imapparser.cc  - IMAP4rev1 Parser
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

#include "rfcdecoder.h"

#include "imapparser.h"

#include "imapinfo.h"

#include "mailheader.h"
#include "mimeheader.h"
#include "mailaddress.h"

#include <stdlib.h>

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#include <qregexp.h>
#include <qbuffer.h>
#include <qstring.h>
#include <qstringlist.h>

#include <kurl.h>

imapParser::imapParser() :
	uidCache(17,false)
{
	uidCache.setAutoDelete( true );
	sentQueue.setAutoDelete(false);
	completeQueue.setAutoDelete(true);
	currentState = ISTATE_NO;
	commandCounter = 0;
	lastHandled = NULL;
}

imapParser::~imapParser()
{
}

imapCommand *imapParser::doCommand(imapCommand *aCmd)
{
	sendCommand( aCmd );
	while(!aCmd->isComplete())
		while(!parseLoop());

	return aCmd;
}

imapCommand *imapParser::sendCommand(imapCommand *aCmd)
{
	aCmd->setId(QString().setNum(commandCounter++));
	sentQueue.append(aCmd);

	continuation = QString::null;
	
	if(aCmd->command() == "SELECT" || aCmd->command() == "EXAMINE")
	{
		currentBox = aCmd->parameter();
		currentBox = parseOneWord(currentBox);
		qDebug("imapParser::sendCommand - setting current box to %s",currentBox.latin1());
	} else if(aCmd->command().find("SEARCH") != -1) {
		lastResults.clear();
	} else if(aCmd->command().find("LIST") != -1) {
		listResponses.clear();
	} else if(aCmd->command().find("LSUB") != -1) {
		listResponses.clear();
	}
	parseWriteLine(aCmd->getStr());
	return aCmd;
}

bool imapParser::clientLogin(const QString &aUser,const QString &aPass)
{
	imapCommand *cmd;
	bool retVal = false;
	
	cmd = doCommand( new imapCommand("LOGIN","\"" +aUser+ "\" \"" +aPass+ "\""));
	
	if(cmd->result() == "OK") retVal = true;
	completeQueue.removeRef(cmd);

	return retVal;
}


bool imapParser::clientAuthenticate(const QString &aUser,const QString &aPass,const QString &aAuth)
{
	imapCommand *cmd;
	bool retVal = false;
	
	// see if server supports this authenticator
	if(!hasCapability("AUTH="+aAuth)) return false;

	// then lets try it
	cmd = sendCommand( new imapCommand("AUTHENTICATE",aAuth));
	while(!cmd->isComplete())
	{
		//read the next line
		while(!parseLoop());

		if(!continuation.isEmpty())
		{
			QString challenge = continuation;
			
			parseOneWord(challenge); 								// +
			challenge = challenge.left(challenge.length()-2); 		// trim CRLF
			challenge = rfcDecoder::decodeBase64(challenge.utf8()); // challenge is BASE64 encoded

			qDebug("IMAP4: authenticate key=%s",challenge.latin1());

			if(aAuth.upper() == "LOGIN")
			{
				if(challenge.find("User",0,false) != -1)
				{
					challenge = rfcDecoder::encodeBase64(aUser.utf8());
				} else if(challenge.find("Pass",0,false) != -1)
				{
					challenge = rfcDecoder::encodeBase64(aPass.utf8());
				}
			} else if(aAuth.upper() == "CRAM-MD5") {
				QCString password = aPass.latin1();
				QCString cchallenge = challenge.latin1();
				
				challenge = rfcDecoder::encodeRFC2104(cchallenge,password);
				challenge = aUser + " " + challenge;
//				qDebug("IMAP4: authenticate response=%s",challenge.latin1());
				challenge = rfcDecoder::encodeBase64(challenge.utf8());
			} else if(aAuth.upper() == "ANONYMOUS") {
				// we should present the challenge to the user and ask
				// him for a mail-adress or what ever
				challenge = rfcDecoder::encodeBase64(aUser.utf8());
			}

			// we will ALWAYS write back a line to satisfiy the continuation
			parseWriteLine(challenge);
			
//			qDebug("Wrote: '%s'",rfcDecoder::decodeBase64(challenge.utf8()).data());
			continuation = QString::null;
		}
	}
	
	if(cmd->result() == "OK") retVal = true;
	completeQueue.removeRef(cmd);
	
	return retVal;
}

void imapParser::parseUntagged(QString &result)
{
//	qDebug("imapParser::parseUntagged - '%s'",result.latin1());

	parseOneWord(result); // *
	QString what = parseOneWord(result); // see whats coming next
	
	switch( what[0].latin1() )
	{
		//the status responses
		case 'B' :  // BAD or BYE
			if(what == "BAD")
			{
				parseResult(what,result);
			} else if(what == "BYE") {
				parseResult(what,result);
				currentState = ISTATE_NO;
			} else {
				qDebug("imapParser::parseUntagged - unknown response %s %s",what.latin1(),result.latin1());
			}
			break;
			
		case 'N' : // NO
			if(what[1] == 'O' && what.length() == 2)
			{
				parseResult(what,result);
			} else {
				qDebug("imapParser::parseUntagged - unknown response %s %s",what.latin1(),result.latin1());
			}
			break;

		case 'O' : // OK
			if(what[1] == 'K' && what.length() == 2)
			{
				parseResult(what,result);
			} else {
				qDebug("imapParser::parseUntagged - unknown response %s %s",what.latin1(),result.latin1());
			}
			break;

		case 'P' : // PREAUTH
			if(what == "PREAUTH")
			{
				parseResult(what,result);
				currentState = ISTATE_LOGIN;
			} else {			
				qDebug("imapParser::parseUntagged - unknown response %s %s",what.latin1(),result.latin1());
			}
			break;
			
		// parse the other responses
		case 'C' : // CAPABILITY
			if(what == "CAPABILITY")
			{
				parseCapability(result);
			} else {			
				qDebug("imapParser::parseUntagged - unknown response %s %s",what.latin1(),result.latin1());
			}
			break;
			
		case 'F' : // FLAGS
			if(what == "FLAGS")
			{
				parseFlags(result);
			} else {			
				qDebug("imapParser::parseUntagged - unknown response %s %s",what.latin1(),result.latin1());
			}
			break;
			
		case 'L' :  // LIST or LSUB
			if(what == "LIST")
			{
				parseList(result);
			} else if(what == "LSUB") {
				parseLsub(result);
			} else {
				qDebug("imapParser::parseUntagged - unknown response %s %s",what.latin1(),result.latin1());
			}
			break;
			
		case 'S' :  // SEARCH or STATUS
			if(what == "SEARCH")
			{
				parseSearch(result);
			} else if(what == "STATUS") {
				parseStatus(result);
			} else {
				qDebug("imapParser::parseUntagged - unknown response %s %s",what.latin1(),result.latin1());
			}
			break;
			
		default:
			//better be a number
			{
				ulong number;
				bool valid;
				
				number = what.toUInt(&valid);
				if(valid)
				{
					what = parseLiteral(result);
					switch( what[0].latin1())
					{
						case 'E' :
							if(what == "EXISTS")
							{
								parseExists(number,result);
							} else if(what == "EXPUNGE") {
								parseExpunge(number,result);
							} else {
								qDebug("imapParser::parseUntagged - unknown response %ld %s %s",number,what.latin1(),result.latin1());
							}
							break;

						case 'F' :
							if(what == "FETCH")
							{
								seenUid = QString::null;
								parseFetch(number,result);
							} else {
								qDebug("imapParser::parseUntagged - unknown response %ld %s %s",number,what.latin1(),result.latin1());
							}
							break;

						case 'S' :
							if(what == "STORE") // deprecated store
							{
								seenUid = QString::null;
								parseFetch(number,result);
							} else {
								qDebug("imapParser::parseUntagged - unknown response %ld %s %s",number,what.latin1(),result.latin1());
							}
							break;

						case 'R' :
							if(what == "RECENT")
							{
								parseRecent(number,result);
							} else {
								qDebug("imapParser::parseUntagged - unknown response %ld %s %s",number,what.latin1(),result.latin1());
							}
							break;
						default:
							qDebug("imapParser::parseUntagged - unknown response %ld %s %s",number,what.latin1(),result.latin1());
							break;
					}
				} else {
					qDebug("imapParser::parseUntagged - unknown response %s %s",what.latin1(),result.latin1());
				}
			}
			break;
	}  //switch
}  //func


void imapParser::parseResult(QString &result,QString &rest)
{
	if(rest[0] == '[')
	{
		rest = rest.right(rest.length()-1); //tie off [
		QString option = parseOneWord(rest);
		
		switch( option[0].latin1() )
		{
			case 'A' : // ALERT
				if( option == "ALERT" )
				{
					qDebug("imapParser::parseResult - %s %s %s",result.latin1(),option.latin1(),rest.latin1());
				} else {
					qDebug("imapParser::parseResult - unknown response %s %s %s",result.latin1(),option.latin1(),rest.latin1());
				}
				break;
				
			case 'N' : // NEWNAME
				if( option == "NEWNAME" )
				{
					qDebug("imapParser::parseResult - %s %s %s",result.latin1(),option.latin1(),rest.latin1());
				} else {
					qDebug("imapParser::parseResult - unknown response %s %s %s",result.latin1(),option.latin1(),rest.latin1());
				}
				break;
				
			case 'P' : //PARSE or PERMANENTFLAGS
				if( option == "PARSE" )
				{
					qDebug("imapParser::parseResult - %s %s %s",result.latin1(),option.latin1(),rest.latin1());
				} else if( option == "PERMANENTFLAGS" )
				{
//					qDebug("imapParser::parseResult - %s %s %s",result.latin1(),option.latin1(),rest.latin1());
					QString flags = rest.left(rest.find(']'));
					selectInfo.setPermanentFlags(flags);
				} else {
					qDebug("imapParser::parseResult - unknown response %s %s %s",result.latin1(),option.latin1(),rest.latin1());
				}
				break;
				
			case 'R' : //READ-ONLY or READ-WRITE
				if( option == "READ-ONLY" )
				{
//					qDebug("imapParser::parseResult - %s %s %s",result.latin1(),option.latin1(),rest.latin1());
					selectInfo.setReadWrite(false);
				} else if( option == "READ-WRITE" )
				{
//					qDebug("imapParser::parseResult - %s %s %s",result.latin1(),option.latin1(),rest.latin1());
					selectInfo.setReadWrite(true);
				} else {
					qDebug("imapParser::parseResult - unknown response %s %s %s",result.latin1(),option.latin1(),rest.latin1());
				}
				break;
				
			case 'T' : //TRYCREATE
				if( option == "TRYCREATE" )
				{
					qDebug("imapParser::parseResult - %s %s %s",result.latin1(),option.latin1(),rest.latin1());
				} else {
					qDebug("imapParser::parseResult - unknown response %s %s %s",result.latin1(),option.latin1(),rest.latin1());
				}
				break;
				
			case 'U' : //UIDVALIDITY or UNSEEN
				if( option == "UIDVALIDITY" )
				{
//					qDebug("imapParser::parseResult - %s %s %s",result.latin1(),option.latin1(),rest.latin1());
					ulong value;
					if(parseOneNumber(rest,value)) selectInfo.setUidValidity(value);
				} else if( option == "UNSEEN" )
				{
//					qDebug("imapParser::parseResult - %s %s %s",result.latin1(),option.latin1(),rest.latin1());
					ulong value;
					if(parseOneNumber(rest,value)) selectInfo.setUnseen(value);
				} else if( option == "UIDNEXT" )
				{
//					qDebug("imapParser::parseResult - %s %s %s",result.latin1(),option.latin1(),rest.latin1());
					ulong value;
					if(parseOneNumber(rest,value)) selectInfo.setUidNext(value);
				} else {
					qDebug("imapParser::parseResult - unknown response %s %s %s",result.latin1(),option.latin1(),rest.latin1());
				}
				break;
				
		}
		if(rest[0] == ']')
			rest = rest.right(rest.length()-1); //tie off ]
		skipWS(rest);
	}
	QString action = parseOneWord(rest);
	if(action == "UID") action = parseOneWord(rest);

	switch( action[0].latin1())
	{
		case 'A' :
			if(action == "AUTHENTICATE")
				if(result == "OK") currentState = ISTATE_LOGIN;
			break;

		case 'L' :
			if(action == "LOGIN")
				if(result == "OK") currentState = ISTATE_LOGIN;
			break;

		case 'E' :
			if(action == "EXAMINE")
			{
				uidCache.clear();
				if(result == "OK") currentState = ISTATE_SELECT;
				else
				{
					if(currentState == ISTATE_SELECT) currentState = ISTATE_LOGIN;
					currentBox = QString::null;
				}
				qDebug("imapParser::parseResult - current box is now %s",currentBox.latin1());
			}
			break;
			
		case 'S' :
			if(action == "SELECT")
			{
				uidCache.clear();
				if(result == "OK") currentState = ISTATE_SELECT;
				else
				{
					if(currentState == ISTATE_SELECT) currentState = ISTATE_LOGIN;
					currentBox = QString::null;
				}
				qDebug("imapParser::parseResult - current box is now %s",currentBox.latin1());
			}
			break;
			
		default :
			break;
	}
	
}

void imapParser::parseCapability(QString &result)
{
	imapCapabilities = QStringList::split(" ",result);
}

void imapParser::parseFlags(QString &result)
{
//	qDebug("imapParser::parseFlags - %s",result.latin1());
	selectInfo.setFlags(result);
}

void imapParser::parseList(QString &result)
{
//	qDebug("imapParser::parseList - %s",result.latin1());
	imapList this_one;
	
	if(result[0] != '(') return; //not proper format for us
  
	result = result.right(result.length()-1); // tie off (

	//process the attributes
	QString attribute;
	
	while(!result.isEmpty() && result[0] != ')')
	{
		attribute = imapParser::parseOneWord(result);
		if(-1 != attribute.find("\\Noinferiors",0,false)) this_one.setNoInferiors(true);
		else if(-1 != attribute.find("\\Noselect",0,false)) this_one.setNoSelect(true);
		else if(-1 != attribute.find("\\Marked",0,false)) this_one.setMarked(true);
		else if(-1 != attribute.find("\\Unmarked",0,false)) this_one.setUnmarked(true);
		else qDebug("imapParser::parseList - unknown attribute %s",attribute.latin1());
	}

	result = result.right(result.length()-1); // tie off )
	imapParser::skipWS(result);

	this_one.setHierarchyDelimiter(imapParser::parseLiteral(result));
  	this_one.setName(rfcDecoder::fromIMAP(imapParser::parseLiteral(result))); // decode modified UTF7

	listResponses.append(this_one);
}

void imapParser::parseLsub(QString &result)
{
	qDebug("imapParser::parseLsub - %s",result.latin1());
	imapList this_one(result);
	listResponses.append(this_one);
}

void imapParser::parseSearch(QString &result)
{
//	qDebug("imapParser::parseSearch - %s",result.latin1());
	QString entry;
	ulong value;
		
	while(parseOneNumber(result,value))
	{
		lastResults.append( QString().setNum(value));
	}
//	qDebug("imapParser::parseSearch - %s",result.latin1());
}

void imapParser::parseStatus(QString &inWords)
{
	qDebug("imapParser::parseStatus - %s",inWords.latin1());
	lastStatus = imapInfo();
	
	parseOneWord(inWords); // swallow the box
	if(inWords[0] != '(') return;

	inWords = inWords.right(inWords.length() - 1);	
	skipWS(inWords);

	while(!inWords.isEmpty() && inWords[0] != ')')
	{
		QString label;
		ulong value;
		
		label = parseOneWord(inWords);
		if(parseOneNumber(inWords,value))
		{
			if( label == "MESSAGES") lastStatus.setCount(value);
			else if( label == "RECENT") lastStatus.setRecent(value);
			else if( label == "UIDVALIDITY") lastStatus.setUidValidity(value);
			else if( label == "UNSEEN") lastStatus.setUnseen(value);
			else if( label == "UIDNEXT") lastStatus.setUidNext(value);
		}
	}

	if(inWords[0] == ')') inWords = inWords.right(inWords.length() - 1);	
	skipWS(inWords);
}

void imapParser::parseExists(ulong value,QString &result)
{
//	qDebug("imapParser::parseExists - [%ld] %s",value,result.latin1());
	selectInfo.setCount(value);
	result = QString::null;
}

void imapParser::parseExpunge(ulong value,QString &result)
{
	qDebug("imapParser::parseExpunge - [%ld] %s",value,result.latin1());
}

QValueList<mailAddress> imapParser::parseAdressList(QString &inWords)
{
	QValueList<mailAddress> retVal;
	
//	qDebug("imapParser::parseAdressList - %s",inWords.latin1());
	if(inWords[0] != '(')
	{
		parseOneWord(inWords); // parse NIL
	} else {
		inWords = inWords.right(inWords.length() - 1);	
		skipWS(inWords);

		while(!inWords.isEmpty() && inWords[0] != ')')
		{
			if(inWords[0] == '(') retVal.append(parseAdress(inWords));
			else break;
		}

		if(inWords[0] == ')') inWords = inWords.right(inWords.length() - 1);	
		skipWS(inWords);
	}

	return retVal;
}

mailAddress imapParser::parseAdress(QString &inWords)
{
	QString user,host,full,comment;
	mailAddress retVal;
		
//	qDebug("imapParser::parseAdress - %s",inWords.latin1());
	if(inWords[0] != '(') return retVal;
	inWords = inWords.right(inWords.length() - 1);	
	skipWS(inWords);
	
	full = parseLiteral(inWords);
	comment = parseLiteral(inWords);
	user = parseLiteral(inWords);
	host = parseLiteral(inWords);

	retVal.setFullNameRaw(full.ascii());
	retVal.setCommentRaw(comment.ascii());
	retVal.setUser(user.ascii());
	retVal.setHost(host.ascii());
	
//	qDebug("imapParser::parseAdress - '%s' '%s' '%s' '%s'",full.latin1(),comment.latin1(),user.latin1(),host.latin1());	
	if(inWords[0] == ')') inWords = inWords.right(inWords.length() - 1);	
	skipWS(inWords);
//	qDebug("imapParser::parseAdress - %s",inWords.latin1());

	return retVal;
}

mailHeader *imapParser::parseEnvelope(QString &inWords)
{
	mailHeader *envelope=NULL;
	QValueList<mailAddress> list;
	
//	qDebug("imapParser::parseEnvelope - %s",inWords.latin1());

	if(inWords[0] != '(') return envelope;
	inWords = inWords.right(inWords.length() - 1);	
	skipWS(inWords);


	envelope = new mailHeader;
	qDebug("imapParser::parseEnvelope - creating %p",envelope);

	//date
	QString date = parseLiteral(inWords);
	envelope->setDate( date.ascii());

	//subject
	QString subject = parseLiteral(inWords);
	envelope->setSubjectEncoded(subject.ascii());

	//from
	list = parseAdressList(inWords);
	for (	QValueListIterator<mailAddress> it = list.begin(); it != list.end(); ++it )
	{
		envelope->setFrom((*it));
	}

	//sender
	list = parseAdressList(inWords);
	for (	QValueListIterator<mailAddress> it = list.begin(); it != list.end(); ++it )
	{
		envelope->setSender((*it));
	}

	//reply-to
	list = parseAdressList(inWords);
	for (	QValueListIterator<mailAddress> it = list.begin(); it != list.end(); ++it )
	{
		envelope->setReplyTo((*it));
	}

	//to
	list = parseAdressList(inWords);
	for (	QValueListIterator<mailAddress> it = list.begin(); it != list.end(); ++it )
	{
		envelope->addTo((*it));
	}

	//cc
	list = parseAdressList(inWords);
	for (	QValueListIterator<mailAddress> it = list.begin(); it != list.end(); ++it )
	{
		envelope->addCC((*it));
	}

	//bcc
	list = parseAdressList(inWords);
	for (	QValueListIterator<mailAddress> it = list.begin(); it != list.end(); ++it )
	{
		envelope->addBCC((*it));
	}

	//in-reply-to
	QString reply = parseLiteral(inWords);
	envelope->setInReplyTo(reply.ascii());

	//message-id	
	QString message = parseLiteral(inWords);
	envelope->setMessageId(message.ascii());

	// see if we have more to come
	while(!inWords.isEmpty() &&inWords[0] != ')')
	{
		//eat the extensions to this part
		if(inWords[0] == '(') parseSentence(inWords);
		else parseLiteral(inWords);
	}

	if(inWords[0] == ')') inWords = inWords.right(inWords.length() - 1);	
	skipWS(inWords);

	return envelope;
//	qDebug("imapParser::parseEnvelope - %s",inWords.latin1());
}

// parse parameter pairs into a dictionary
// caller must clean up the dictionary items
QDict<QString> imapParser::parseDisposition(QString &inWords)
{
	QString disposition;
	QDict<QString> retVal(17,false);
	
	// return value is a shallow copy
	retVal.setAutoDelete(false);
	
	if(inWords[0] != '(')
	{
		//disposition only
		disposition = parseOneWord(inWords);
	} else {
		inWords = inWords.right(inWords.length() - 1);	
		skipWS(inWords);

		//disposition
		disposition = parseOneWord(inWords);

		retVal = parseParameters(inWords);
		if(inWords[0] != ')') return retVal;
		inWords = inWords.right(inWords.length() - 1);	
		skipWS(inWords);
	}
	
	if(!disposition.isEmpty()) retVal.insert("content-disposition",new QString(disposition));

	return retVal;
}

// parse parameter pairs into a dictionary
// caller must clean up the dictionary items
QDict<QString> imapParser::parseParameters(QString &inWords)
{
	QDict<QString> retVal(17,false);
	
	// return value is a shallow copy
	retVal.setAutoDelete(false);
	
	if(inWords[0] != '(')
	{
		//better be NIL
		parseOneWord(inWords);
	} else {
		inWords = inWords.right(inWords.length() - 1);	
		skipWS(inWords);

		while(!inWords.isEmpty() && inWords[0] != ')')
		{
			QString label,value;
			
			label = parseLiteral(inWords);
			value = parseLiteral(inWords);
			retVal.insert(label,new QString(value));
//			qDebug("imapParser::parseParameters - %s = '%s'",label.latin1(),value.latin1());
		}

		if(inWords[0] != ')') return retVal;
		inWords = inWords.right(inWords.length() - 1);	
		skipWS(inWords);
	}

	return retVal;
}

mimeHeader *imapParser::parseSimplePart(QString &inWords,const QString &inSection)
{
	QString type,subtype,id,description,encoding;
	QDict<QString> parameters(17,false);
	mimeHeader *localPart = NULL;
	ulong size;
	
	parameters.setAutoDelete( true );
	
	if(inWords[0] != '(') return NULL;

	localPart = new mimeHeader;

	qDebug("imapParser::parseSimplePart - section %s",inSection.ascii());

	localPart->setPartSpecifier(inSection);
	
	inWords = inWords.right(inWords.length() - 1);	
	skipWS(inWords);

	//body type
	type = parseLiteral(inWords);

	//body subtype
	subtype = parseLiteral(inWords);

	localPart->setType(QCString(type.ascii())+"/"+subtype.ascii());
	
	//body parameter parenthesized list
	parameters = parseParameters(inWords);
	{
		QDictIterator<QString> it(parameters);
		
		while( it.current() )
		{
			localPart->setTypeParm(it.currentKey().ascii(),*(it.current()));
			++it;
		}
		parameters.clear();
	}
	
	//body id
	id = parseLiteral(inWords);
	localPart->setID(id.latin1());
	
	//body description
	description = parseLiteral(inWords);

	//body encoding
	encoding = parseLiteral(inWords);
	localPart->setEncoding(encoding.ascii());
	
	//body size
	if(parseOneNumber(inWords,size));
		localPart->setLength(size);

	// type specific extensions
	if(type.upper() == "MESSAGE" && subtype.upper() == "RFC822") {
//		qDebug("imapParse::parseSimplePart - parse up message %s",inWords.latin1());

		//envelope structure
		mailHeader * envelope = parseEnvelope(inWords);
		if(envelope) envelope->setPartSpecifier( inSection+".0" );

		//body structure
		parseBodyStructure(inWords,inSection,envelope);

		localPart->setNestedMessage(envelope);
		
		//text lines
		ulong lines;
		parseOneNumber(inWords,lines);
	} else {
		if(type.upper() == "TEXT")
		{
			//text lines
			ulong lines;
			parseOneNumber(inWords,lines);
		}
		
		// md5
		parseLiteral(inWords);
		
		// body disposition
		parameters = parseDisposition(inWords);
		{
			QDictIterator<QString> it(parameters);
			QString *disposition = parameters[QString("content-disposition")];

			if(disposition) localPart->setDisposition(disposition->ascii());
			parameters.remove(QString("content-disposition"));
			while( it.current() )
			{
				localPart->setDispositionParm(it.currentKey().ascii(),*(it.current()));
				++it;
			}
			parameters.clear();
		}
		
		// body language
		parseSentence(inWords);
	}
	
	// see if we have more to come
	while(!inWords.isEmpty() &&inWords[0] != ')')
	{
		//eat the extensions to this part
		if(inWords[0] == '(') parseSentence(inWords);
		else parseLiteral(inWords);
	}

//	qDebug("imapParser::parseSimplePart - %s/%s - %s",type.latin1(),subtype.latin1(),encoding.latin1());

	if(inWords[0] == ')')
		inWords = inWords.right(inWords.length() - 1);	
	skipWS(inWords);
	
	return localPart;
}

mimeHeader *imapParser::parseBodyStructure(QString &inWords,const QString &inSection,mimeHeader *localPart)
{
	int section=0;
	QString outSection;
	
	if(inWords[0] != '(')
	{
		// skip ""
		parseOneWord(inWords);
		return NULL;
	}
	inWords = inWords.right(inWords.length() - 1);	
	skipWS(inWords);

	if(inWords[0].latin1() == '(')
	{
		QString subtype;
		QDict<QString> parameters(17,false);
		parameters.setAutoDelete(true);
		if(!localPart) localPart = new mimeHeader;
		else {
			// might be filled from an earlier run
			localPart->clearNestedParts();
			localPart->clearTypeParameters();
			localPart->clearDispositionParameters();
		}
		
		// is multipart
		while(inWords[0].latin1() == '(')
		{
			section++;
			outSection.setNum(section);
			outSection = inSection + "." + outSection;
			mimeHeader *subpart = parseBodyStructure(inWords,outSection);
			localPart->addNestedPart(subpart);
		}
		
		// fetch subtype
		subtype = parseOneWord(inWords);
//		qDebug("imapParser::parseBodyStructure - multipart/%s",subtype.latin1());
		
		localPart->setType(("MULTIPART/"+subtype).ascii());

		// fetch parameters
		parameters = parseParameters(inWords);
		{
			QDictIterator<QString> it(parameters);

			while( it.current() )
			{
				localPart->setTypeParm(it.currentKey().ascii(),*(it.current()));
				++it;
			}
			parameters.clear();
		}

		// body disposition
		parameters = parseDisposition(inWords);
		{
			QDictIterator<QString> it(parameters);
			QString *disposition = parameters[QString("content-disposition")];

			if(disposition) localPart->setDisposition(disposition->ascii());
			parameters.remove(QString("content-disposition"));
			while( it.current() )
			{
				localPart->setDispositionParm(it.currentKey().ascii(),*(it.current()));
				++it;
			}
			parameters.clear();
		}

		// body language
		parseSentence(inWords);		

	} else {
		// is simple part
		inWords = "(" + inWords; //fake a sentence
		localPart = parseSimplePart(inWords,inSection);
		inWords = ")" + inWords; //remove fake
	}

	// see if we have more to come
	while(!inWords.isEmpty() &&inWords[0] != ')')
	{
		//eat the extensions to this part
		if(inWords[0] == '(') parseSentence(inWords);
		else parseLiteral(inWords);
	}

	if(inWords[0] == ')')
		inWords = inWords.right(inWords.length() - 1);	
	skipWS(inWords);
	
	return localPart;
}

void imapParser::parseBody(QString &inWords)
{
	// see if we got a part specifier
	if(inWords[0] == '[')
	{
		QString specifier;
		inWords = inWords.right(inWords.length() - 1);	// eat it

		specifier = parseOneWord(inWords);
		qDebug("imapParser::parseBody : specifier [%s]",specifier.latin1());

		if(inWords[0] == '(')
		{
			QString label;
			inWords = inWords.right(inWords.length() - 1);	// eat it

			while(!inWords.isEmpty() && inWords[0] != ')')
			{
				label = parseOneWord(inWords);
				qDebug("imapParser::parseBody - mimeLabel : %s",label.latin1());
			}

			if(inWords[0] == ')')
			inWords = inWords.right(inWords.length() - 1);	// eat it
		}
		if(inWords[0] == ']')
		inWords = inWords.right(inWords.length() - 1);	// eat it
		skipWS(inWords);

		// parse the header
		if(specifier == "0") {
			mailHeader *envelope=NULL;
			imapCache *cache = uidCache[seenUid];
			if(cache) envelope = cache->getHeader();

			if(!envelope || seenUid.isEmpty())
			{
				qDebug("imapParser::parseBody - discarding %p %s",envelope,seenUid.ascii());
				// don't know where to put it, throw it away
				parseLiteral(inWords,true);
			} else {
				qDebug("imapParser::parseBody - reading %p %s",envelope,seenUid.ascii());
				// fill it up with data
				QString theHeader = parseLiteral(inWords,true);
				mimeIOQString myIO;
				
				myIO.setString(theHeader);
				envelope->parseHeader(myIO);
				
			}
			lastHandled = cache;
		} else {
			// throw it away
			parseLiteral(inWords,true);
		}

	} else {
		mailHeader *envelope=NULL;
		imapCache *cache = uidCache[seenUid];
		if(cache) envelope = cache->getHeader();

		if(!envelope || seenUid.isEmpty())
		{
			qDebug("imapParser::parseBody - discarding %p %s",envelope,seenUid.ascii());
			// don't know where to put it, throw it away
			parseSentence( inWords );
		} else {
			qDebug("imapParser::parseBody - reading %p %s",envelope,seenUid.ascii());
			// fill it up with data
			mimeHeader *body = parseBodyStructure(inWords,seenUid,envelope);
			if(body != envelope) delete body;
		}
		lastHandled = cache;
	}
}

void imapParser::parseFetch(ulong value,QString &inWords)
{
//	qDebug("imapParser::parseFetch - [%ld] %s",value,inWords.latin1());

	// just the id
	if(value);

	if(inWords[0] != '(') return;
	inWords = inWords.right(inWords.length() - 1);	
	skipWS(inWords);
	
	lastHandled = NULL;

	while(!inWords.isEmpty() && inWords[0] != ')')
	{
		if(inWords[0] == '(') parseSentence(inWords);
		else
		{
			QString word = parseLiteral(inWords);
			switch(word[0].latin1())
			{
				case 'E' :
					if(word == "ENVELOPE")
					{
						mailHeader *envelope = NULL;
						imapCache *cache = uidCache[seenUid];
						
						if(cache) envelope = cache->getHeader();

						qDebug("imapParser::parseFetch - got %p from Cache for %s",envelope,seenUid.latin1());
						if((envelope  && !envelope->getMessageId().isEmpty())|| seenUid.isEmpty())
						{
							// we have seen this one already
							// or don't know where to put it
							qDebug("imapParser::parseFetch - discarding %p %s",envelope,seenUid.ascii());
							parseSentence( inWords );
						} else {
							qDebug("imapParser::parseFetch - reading %p %s",envelope,seenUid.ascii());
							envelope = parseEnvelope(inWords);
							if(envelope)
							{
								cache = new imapCache;
								envelope->setPartSpecifier(seenUid+".0");
								cache->setHeader(envelope);
								cache->setUid(seenUid.toULong());
								uidCache.replace(seenUid,cache);
								qDebug("imapParser::parseFetch - giving %p to Cache for %s",envelope,seenUid.latin1());
							}
						}
						lastHandled = cache;
					}
					break;

				case 'B' :
					if(word == "BODY") 
					{
						parseBody(inWords);
						
					} else if(word == "BODYSTRUCTURE")
					{
						mailHeader *envelope = NULL;
						imapCache *cache = uidCache[seenUid];
						if(cache) envelope = cache->getHeader();
						
						if(!envelope || seenUid.isEmpty())
						{
							qDebug("imapParser::parseFetch - discarding %p %s",envelope,seenUid.ascii());
							// don't know where to put it, throw it away
							parseSentence( inWords );
						} else {
							qDebug("imapParser::parseFetch - reading %p %s",envelope,seenUid.ascii());
							// fill it up with data
							mimeHeader *body = parseBodyStructure(inWords,seenUid,envelope);
							if(body != envelope) delete body;
						}
						lastHandled = cache;
					}
					break;

				case 'U' :
					if(word == "UID")
					{
						seenUid = parseOneWord(inWords);
//						qDebug("imapParser::parseFetch - processing uid %s",seenUid.ascii());
						mailHeader *envelope = NULL;
						imapCache *cache = uidCache[seenUid];
						if(cache) envelope = cache->getHeader();

						if(envelope || seenUid.isEmpty())
						{
							// we have seen this one already
							// or don't know where to put it
						} else {
							// fill up the cache
							envelope = new mailHeader();
							if(envelope)
							{
								cache = new imapCache();
								cache->setHeader(envelope);
								cache->setUid(seenUid.toULong());
								uidCache.replace(seenUid,cache);
								qDebug("imapParser::parseFetch - creating new cache entry %p for %s",envelope,seenUid.latin1());
							}
						}
						if(envelope) envelope->setPartSpecifier(seenUid);
						lastHandled = cache;
					}
					break;

				case 'R' :
					if(word == "RFC822.SIZE")
					{
						ulong size;
						imapCache *cache = uidCache[seenUid];
						parseOneNumber(inWords,size);

						if(cache && !seenUid.isEmpty())
						{
							cache->setSize(size);
						}
						lastHandled = cache;
					} else if(word.find("RFC822") == 0) {
						// might be RFC822 RFC822.TEXT RFC822.HEADER
						parseLiteral(inWords);						
					}
					break;
					
				case 'I' :
					if(word == "INTERNALDATE")
					{
						QString date;
						date = parseOneWord(inWords);
						imapCache *cache = uidCache[seenUid];
						if(cache && !seenUid.isEmpty())
						{
							cache->setDateStr(date);
						}
						lastHandled = cache;
					}
					break;
					
				case 'F' :
					if(word == "FLAGS")
					{
						imapCache *cache = uidCache[seenUid];
						if(cache && !seenUid.isEmpty())
						{
							cache->setFlags(imapInfo::_flags(inWords));
						} else
							parseSentence(inWords);
						lastHandled = cache;
					}
					break;
					
				default:
					qDebug("imapParser::parseFetch - ignoring %s",inWords.latin1());
					parseLiteral(inWords);
					break;
			}
		}
	}

	// see if we have more to come
	while(!inWords.isEmpty() &&inWords[0] != ')')
	{
		//eat the extensions to this part
		if(inWords[0] == '(') parseSentence(inWords);
		else parseLiteral(inWords);
	}

	if(inWords[0] != ')') return;
	inWords = inWords.right(inWords.length() - 1);	
	skipWS(inWords);
}


// default parser
void imapParser::parseSentence(QString &inWords)
{
	QString stack;
	bool first = true;
	
//	qDebug("imapParser::parseSentence - %s",inWords.latin1());
	//find the first nesting parentheses
	
	while(!inWords.isEmpty() && (!stack.isEmpty() || first))
	{
		first = false;
		skipWS(inWords);

		unsigned char ch  = inWords[0].latin1();
		switch(ch)
		{
			case '(' :
				inWords = inWords.right(inWords.length()-1);
				stack += ')';
				break;
			case ')' :
				inWords = inWords.right(inWords.length()-1);
				stack = stack.left(stack.length()-1);
				break;
			case '[' :
				inWords = inWords.right(inWords.length()-1);
				stack += ']';
				break;
			case ']' :
				inWords = inWords.right(inWords.length()-1);
				stack = stack.left(stack.length()-1);
				break;
			default:
				parseLiteral(inWords);
				skipWS(inWords);
				break;
		}
	}
	skipWS(inWords);
}

void imapParser::parseRecent(ulong value,QString &result)
{
//	qDebug("imapParser::parseRecent - [%ld] %s",value,result.latin1());
	selectInfo.setRecent(value);
	result = QString::null;
}

bool imapParser::parseLoop()
{
	QString result;
	QByteArray readBuffer;
	
	parseReadLine(readBuffer);
	result = QString::fromLatin1(readBuffer.data(),readBuffer.size());
	
	if(result.isNull()) return false;
	if(!sentQueue.count())
	{
		// maybe greeting or BYE everything else SHOULD not happen, use NOOP or IDLE
		qDebug("imapParser::parseLoop - unhandledResponse: \n%s",result.latin1());
		unhandled << result;
	} else {
		imapCommand *current = sentQueue.at(0);
		
		switch(result[0].latin1()) {
			case '*' :
				result = result.left(result.length()-2); // tie off CRLF
				parseUntagged(result);
				break;
			case '+' :
				qDebug("imapParser::parseLoop - continue: \n%s",result.latin1());
				continuation = result;
				break;
			default:
				{
					QString tag,resultCode;
					
					tag = parseLiteral(result);
					if(tag ==  current->id())
					{
						result = result.left(result.length()-2); // tie off CRLF
						resultCode = parseLiteral(result); //the result
						current->setResult(resultCode);
						current->setComplete();

						sentQueue.removeRef(current);
						completeQueue.append(current);
						qDebug("imapParser::parseLoop -  completed %s: %s",resultCode.latin1(),result.latin1());
						parseResult(resultCode,result);
					} else {
						qDebug("imapParser::parseLoop - unknown tag '%s'",tag.latin1());
						result = tag + " " + result;
					}
				}
				break;
		}
	}
	
	return true;
}

void imapParser::parseRelay(const QByteArray &buffer)
{
	qWarning("imapParser::parseRelay - virtual function not reimplemented - data lost");
	if(&buffer);
}

void imapParser::parseRelay(ulong len)
{
	qWarning("imapParser::parseRelay - virtual function not reimplemented - announcement lost");
	if(len);
}

bool imapParser::parseRead (QByteArray &buffer,ulong len,ulong relay)
{
	ulong localRelay = relay;
	while(buffer.size() < len)
	{
		// beware of wrap around
		if(buffer.size() < relay) localRelay = relay - buffer.size();
		else localRelay = 0;
		
//		qDebug("imapParser::parseRead - remaining %ld",relay-buffer.length());
		qDebug("got now : %d needing still : %ld",buffer.size(),localRelay);
		parseReadLine(buffer,localRelay);
	}
	return (len <= buffer.size());
}

void imapParser::parseReadLine (QByteArray &buffer,ulong relay)
{
	qWarning("imapParser::parseReadLine - virtual function not reimplemented - no data read");
	if(&buffer && relay);
}

void imapParser::parseWriteLine (const QString &str)
{
	qWarning("imapParser::parseWriteLine - virtual function not reimplemented - no data written");
	if(&str);
}

void imapParser::parseURL(const KURL &_url,QString &_box,QString &_section,QString &_type,QString &_uid,QString &_validity)
{
   qDebug( "imapParser::parseURL - %s", _url.url().latin1());
	QStringList parameters;	
	
	_box = _url.path();
	parameters = QStringList::split(";",_box);	//split parameters
	if(parameters.count() > 0)					//assertion failure otherwise
		parameters.remove(parameters.begin());	//strip path
	_box = _box.left(_box.find(';')); 			// strip parameters
	for (QStringList::ConstIterator it(parameters.begin()); it != parameters.end(); ++it)
	{
		QString temp = (*it);
		
		// if we have a '/' separator we'll just nuke it
		if(temp.find("/") > 0) temp = temp.left(temp.find("/"));
//		if(temp[temp.length()-1] == '/')
//			temp = temp.left(temp.length()-1);
		if( temp.find("section=",0,false) == 0) _section = temp.right(temp.length()-8);
		else if( temp.find("type=",0,false) == 0) _type = temp.right(temp.length()-5);
		else if( temp.find("uid=",0,false) == 0) _uid = temp.right(temp.length()-4);
		else if( temp.find("uidvalidity=",0,false) == 0) _validity = temp.right(temp.length()-12);
	}
//	qDebug("URL: section= %s, type= %s, uid= %s",_section.latin1(),_type.latin1(),_uid.latin1());
//	qDebug("URL: url() %s",_url.url().latin1());
//	qDebug("URL: user() %s",_url.user().latin1());
//	qDebug("URL: path() %s",_url.path().latin1());
//	qDebug("URL: encodedPathAndQuery() %s",_url.encodedPathAndQuery().latin1());
//	qDebug("URL: decoded(url()) %s",KURL::decode_string(_url.url()).latin1());
	
	if(!_box.isEmpty())
	{
		if(_box[0] == '/') _box = _box.right(_box.length() -1);
		if(!_box.isEmpty() && _box[_box.length()-1] == '/') _box = _box.left(_box.length() -1);
	}
	qDebug("URL: box= %s, section= %s, type= %s, uid= %s, validity= %s",_box.latin1(),_section.latin1(),_type.latin1(),_uid.latin1(),_validity.latin1());
}

void imapParser::skipWS(QString &inWords)
{
	int i = 0;
	
	while(inWords[i] == ' ' || inWords[i] == '\t' || inWords[i] == '\r' || inWords[i] == '\n')
	{
		i++;
	}
	inWords = inWords.right(inWords.length()-i);
}

QString imapParser::parseLiteral(QString &inWords,bool relay)
{
	QString retVal;
	
	if(inWords[0] == '{') {
		ulong runLen;
		QString strLen;
		
		runLen = inWords.find('}',1);
		if(runLen > 0)
		{
			bool proper;
			strLen = inWords.left(runLen);
			strLen = strLen.right(strLen.length()-1);
			inWords = inWords.right(inWords.length()-runLen-1);
			runLen = strLen.toULong(&proper);
			if(proper)
			{
				//now get the literal from the server
				QByteArray fill;
				
				if(relay) parseRelay(runLen);
				parseRead(fill,runLen,relay ? runLen : 0);
//				qDebug("requested %ld and got %d",runLen,fill.size());
//				qDebug("last bytes %x %x %x %x",fill[runLen-4],fill[runLen-3],fill[runLen-2],fill[runLen-1]);
				retVal = QString::fromLatin1( fill.data(),runLen);  // our data
				inWords = QString::fromLatin1( fill.data()+runLen); // what remains
				if(inWords.isEmpty())
				{
					QByteArray prefetch;
					parseReadLine(prefetch);  // must get more
					inWords = QString::fromLatin1(prefetch.data(),prefetch.size());
//					qDebug("prefetched [%d] - '%s'",inWords.length(),inWords.latin1());
				}
//				qDebug("requested %ld and got %d",runLen,fill.length());
//				inWords = inWords.left(inWords.length()-2); // tie off CRLF
//				qDebug("|\n|\nV");
//				qDebug("%s^",retVal.latin1());
//				qDebug("|\n|\n'%s'",inWords.latin1());

				// no duplicate data transfers
				relay = false;
			} else {
				qDebug("imapParser::parseLiteral - error parsing {} - %s",strLen.latin1());
			}
		} else {
			inWords = "";
			qDebug("imapParser::parseLiteral - error parsing unmatched {");
		}
	} else {
		retVal = parseOneWord(inWords);
	}
//	debug((temp+QString("> '")+retVal+"'").latin1());
	skipWS(inWords);
	return retVal;
}

// does not know about literals ( {7} literal )

QString imapParser::parseOneWord(QString &inWords)
{
	QString retVal;
	
	if(inWords[0] == '"')
	{
		int i = inWords.find('"',1);
		if(i != -1)
		{
			retVal = inWords.left(i);
			retVal = retVal.right(retVal.length()-1);
			inWords = inWords.right(inWords.length()- i -1);
		} else {
			qDebug("imapParser::parseOneWord - error parsing unmatched \"");
			retVal = inWords;
			inWords = "";
		}
	} else {
		int i,j;
		i = inWords.find(' ');
		if(i == -1) i = inWords.length();
		j = inWords.find('(');
		if(j<i && j != -1) i = j;
		j = inWords.find(')');
		if(j<i && j != -1) i = j;
		j = inWords.find('[');
		if(j<i && j != -1) i = j;
		j = inWords.find(']');
		if(j<i && j != -1) i = j;
		j = inWords.find('\r');
		if(j<i && j != -1) i = j;
		j = inWords.find('\n');
		if(j<i && j != -1) i = j;
		j = inWords.find('\t');
		if(j<i && j != -1) i = j;
		if(i != -1)
		{
			retVal = inWords.left(i);
			inWords = inWords.right(inWords.length()- i);
		} else {
			retVal = inWords;
			inWords = "";
		}
		if(retVal == "NIL") retVal = QString::null;
	}
//	qDebug((temp+QString("> '")+retVal+"'").latin1());
	skipWS(inWords);
	return retVal;
}

bool imapParser::parseOneNumber(QString &inWords,ulong &num)
{
	bool valid;
	num = parseOneWord(inWords).toULong(&valid);
	return valid;
}

bool imapParser::hasCapability(const QString &cap)
{
//	qDebug("imapParser::hasCapability - Looking for '%s'",cap.latin1());
	for ( QStringList::Iterator it = imapCapabilities.begin(); it != imapCapabilities.end(); ++it )
	{
//		qDebug("imapParser::hasCapability - Examining '%s'",(*it).latin1());
		if(cap.lower() == (*it).lower()) {return true;}
	}
	return false;
}

