/***************************************************************************
                          mimeheader.cc  -  description
                             -------------------
    begin                : Fri Oct 20 2000
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

#include "mimeheader.h"
#include "mimehdrline.h"
#include "mailheader.h"
#include "rfcdecoder.h"

#include <qregexp.h>

#include <ostream.h>

mimeHeader::mimeHeader() :
	typeList(17,false),
	dispositionList(17,false)
{
	originalHdrLines.setAutoDelete( true );
	additionalHdrLines.setAutoDelete( false ); // is also in original lines
	nestedParts.setAutoDelete( true );
	typeList.setAutoDelete( true );
	dispositionList.setAutoDelete( true );
	nestedMessage = NULL;
	contentLength = 0;
}

mimeHeader::~mimeHeader(){}

/*
QList<mimeHeader> mimeHeader::getAllParts()
{
	QList<mimeHeader> retVal;

	// caller is responsible for clearing
	retVal.setAutoDelete( false );
	nestedParts.setAutoDelete( false );
	
	// shallow copy
	retVal = nestedParts;
	
	// can't have duplicate pointers
	nestedParts.clear();
	
	// restore initial state
	nestedParts.setAutoDelete( true );
	
	return retVal;
} */

void mimeHeader::addHdrLine(mimeHdrLine *aHdrLine)
{
	mimeHdrLine *addLine = new mimeHdrLine(aHdrLine);
	if(addLine)
	{
  	originalHdrLines.append(addLine);
  	if(qstrnicmp(addLine->getLabel(),"Content-",8))
  	{
	  	additionalHdrLines.append(addLine);
  	} else {
  		int skip;
  		char *aCStr = addLine->getValue().data();
			QDict<QString> *aList;
			
			aList = NULL;  		
  		skip = mimeHdrLine::parseSeparator(';',aCStr);
  		if(skip > 0)
  		{
	  		QCString mimeValue = QCString(aCStr,skip);
  	  	
  	  	if(!qstricmp(addLine->getLabel(),"Content-Disposition"))
  	  	{
  	  		aList = &dispositionList;
  	  		contentDisposition = mimeValue;
  	  	} else if(!qstricmp(addLine->getLabel(),"Content-Type")) {
  	  		aList = &typeList;
  	  		contentType = mimeValue;
  	  	} else if(!qstricmp(addLine->getLabel(),"Content-Transfer-Encoding")) {
  	  		contentEncoding = mimeValue;
  	  	} else if(!qstricmp(addLine->getLabel(),"Content-ID")) {
  	  		contentID = mimeValue;
  	  	} else if(!qstricmp(addLine->getLabel(),"Content-Length")) {
  	  		contentLength = mimeValue.toULong();
  	  	} else {
			  	additionalHdrLines.append(addLine);
  	  	}
//	  		cout << addLine->getLabel().data() << ": '" << mimeValue.data() << "'" << endl;
    		
    		aCStr += skip;
    		while((skip = mimeHdrLine::parseSeparator(';',aCStr)))
    		{
      		if(skip > 0)
      		{
      			QCString aParm;
      			
      			aParm = QCString(aCStr,skip);
      			aParm = aParm.simplifyWhiteSpace().stripWhiteSpace();
      			addParameter(aParm,aList);
//      			cout << "-- '" << aParm.data() << "'" << endl;
      			mimeValue = QCString(addLine->getValue().data(),skip);
      			aCStr += skip;
      		} else break;
    		}
  		}
  	}
  }
}

void mimeHeader::addParameter(QCString aParameter,QDict<QString> *aList)
{
	QString *aValue;
	QCString aLabel;
	int pos = aParameter.find('=');
//	cout << aParameter.left(pos).data();
	aValue = new QString();
	aValue->setLatin1(aParameter.right(aParameter.length() - pos - 1));
	aLabel = aParameter.left(pos);
	if((*aValue)[0] == '"')
		*aValue = aValue->mid(1,aValue->length()-2);
      			
	aList->insert(aLabel,aValue);
//	cout << "=" << aValue->data() << endl;
}

QString mimeHeader::getDispositionParm(QCString aStr)
{
	return getParameter(aStr,&dispositionList);
}

QString mimeHeader::getTypeParm(QCString aStr)
{
	return getParameter(aStr,&typeList);
}

void mimeHeader::setDispositionParm(QCString aLabel,QString aValue)
{
	return setParameter(aLabel,aValue,&dispositionList);
}

void mimeHeader::setTypeParm(QCString aLabel,QString aValue)
{
	return setParameter(aLabel,aValue,&typeList);
}

QDictIterator<QString> mimeHeader::getDispositionIterator()
{
	return QDictIterator<QString>(dispositionList);
}

QDictIterator<QString> mimeHeader::getTypeIterator()
{
	return QDictIterator<QString>(typeList);
}

QListIterator<mimeHdrLine> mimeHeader::getOriginalIterator()
{
	return QListIterator<mimeHdrLine>(originalHdrLines);
}

QListIterator<mimeHdrLine> mimeHeader::getAdditionalIterator()
{
	return QListIterator<mimeHdrLine>(additionalHdrLines);
}

void mimeHeader::outputHeader(mimeIO &useIO)
{	
	if(!getDisposition().isEmpty())
	{
  	useIO.outputMimeLine(QCString("Content-Disposition: ")
  		+ getDisposition()
  		+ outputParameter(&dispositionList));
	}
	
	if(!getType().isEmpty())
	{
  	useIO.outputMimeLine(QCString("Content-Type: ")
  		+ getType()
  		+ outputParameter(&typeList));
	}
	if(!getID().isEmpty())
  	useIO.outputMimeLine(QCString("Content-ID: ") + getID());
	if(!getEncoding().isEmpty())
  	useIO.outputMimeLine(QCString("Content-Transfer-Encoding: ") + getEncoding());
	
	QListIterator<mimeHdrLine> ait = getAdditionalIterator();
	while(ait.current())
	{
		useIO.outputMimeLine(ait.current()->getLabel() + ": " + ait.current()->getValue());
		++ait;
	}
	useIO.outputMimeLine(QCString(""));
}

QString mimeHeader::getParameter(QCString aStr,QDict<QString> *aDict)
{
	QString retVal,*found;
	if(aDict)
	{
		//see if it is a normal parameter
		found = aDict->find(aStr);
		if(!found)
		{
			//might be a continuated or encoded parameter
			found = aDict->find(aStr+"*");
			if(!found)
			{
				//continuated parameter
				QString decoded,encoded;
				int part=0;
				
				do {
    			QCString search;
    			search.setNum(part);
    			search = aStr + "*" + search;
    			found = aDict->find(search);
    			if(!found) {
	    			found = aDict->find(search+"*");
	    			if(found) encoded += rfcDecoder::encodeRFC2231String(*found);
	    		} else {
	    			encoded += *found;
	    		}
	    		part++;
				} while (found);
				if(encoded.find("'") >= 0)
				{
					retVal = rfcDecoder::decodeRFC2231String(encoded.local8Bit());
				} else {
					retVal = rfcDecoder::decodeRFC2231String(QCString("''") + encoded.local8Bit());
				}
			} else {
				//simple encoded parameter
				retVal = rfcDecoder::decodeRFC2231String(found->local8Bit());
			}
		} else {
			retVal = *found;
		}
	}
	return retVal;
}

void mimeHeader::setParameter(QCString aLabel,QString aValue,QDict<QString> *aDict)
{
	bool encoded=true;

  if(aDict)
  {

	//see if it needs to get encoded
  	if(encoded && aLabel.find('*') == -1)
  	{
  		aValue = rfcDecoder::encodeRFC2231String(aValue);
  	}
  	//see if it needs to be truncated
  	if(aValue.length() + aLabel.length() + 4 > 80)
  	{
  		unsigned int limit = 80 - 8 - aLabel.length();
  		int i=0;
			QString shortValue;
			QCString shortLabel;
			  		
  		while(!aValue.isEmpty())
  		{
  			//don't truncate the encoded chars
  			int offset=0;
  			if(limit > aValue.length()) limit = aValue.length();
  			offset = aValue.findRev('%',limit);
  			if(offset == limit-1 || offset == limit-2)
  			{
//  				cout << "offset " << offset << "-" << limit << "=" << limit-offset << endl;
  				offset = limit-offset;
  			} else offset = 0;
  			shortValue = aValue.left(limit-offset);
  			shortLabel.setNum(i);
  			shortLabel = aLabel + "*" + shortLabel;
  			aValue = aValue.right(aValue.length()-limit+offset);
  			if(encoded)
  			{
	  			if(i == 0)
  				{
  					shortValue = "''" + shortValue;
  				}
  				shortLabel += "*";
  			}
//  			cout << shortLabel << "-" << shortValue << endl;
	  		aDict->insert(shortLabel,new QString(shortValue));
  			i++;
  		}
  	} else {
  		aDict->insert(aLabel,new QString(aValue));
  	}
  }
}

QCString mimeHeader::outputParameter(QDict<QString> *aDict)
{
	QCString retVal;
	if(aDict)
	{
  	QDictIterator<QString> it(*aDict);
  	while ( it.current() ) {
  		retVal += (";\n\t" + it.currentKey() + "=").latin1();
  		if(it.current()->find(' ') > 0 || it.current()->find(';') > 0)
  		{
  			retVal += '"' + it.current()->utf8() + '"';
  		} else {
  			retVal += it.current()->utf8();
  		}
  		// << it.current()->utf8() << "'";
  		++it;
  	}
  	retVal += "\n";
  }
	return retVal;
}

void mimeHeader::outputPart(mimeIO &useIO)
{
	QListIterator<mimeHeader> nestedParts = getNestedIterator();
	QCString boundary;
	if(!getTypeParm("boundary").isEmpty()) boundary = getTypeParm("boundary").latin1();
	
	outputHeader(useIO);
	if(!getPreBody().isEmpty()) useIO.outputMimeLine(getPreBody());
	if(getNestedMessage()) getNestedMessage()->outputPart(useIO);
	while( nestedParts.current() ) {
		if(!boundary.isEmpty())
			useIO.outputMimeLine("--" + boundary);
		nestedParts.current()->outputPart(useIO);
		++nestedParts;
	}
	if(!boundary.isEmpty())
		useIO.outputMimeLine("--" + boundary + "--");
	if(!getPostBody().isEmpty()) useIO.outputMimeLine(getPostBody());
}

int mimeHeader::parsePart(mimeIO &useIO,QString boundary)
{  	
		int retVal = 0;		
		QCString preNested,postNested;
		parseHeader(useIO);
		
		if(!qstrnicmp(getType(),"Multipart",9))
		{
			retVal = parseBody(useIO,preNested,getTypeParm("boundary")); //this is a message in mime format stuff
			setPreBody(preNested);
			int localRetVal;
			do {
				mimeHeader *aHeader = new mimeHeader;
				localRetVal = aHeader->parsePart(useIO,getTypeParm("boundary"));
				addNestedPart(aHeader);
			} while(localRetVal);   //get nested stuff
		}
		if(retVal && !qstrnicmp(getType(),"Message/RFC822",14))
		{
			mailHeader *msgHeader = new mailHeader;
			retVal = msgHeader->parsePart(useIO,boundary);
			setNestedMessage(msgHeader);			
		} else
		retVal = parseBody(useIO,postNested,boundary); //just a simple part remaining
		setPostBody(postNested);
	return retVal;
}

int mimeHeader::parseBody(mimeIO &useIO,QCString &messageBody,QString boundary)
{		
		QCString inputStr;
		QString partBoundary;
		QString partEnd;
  	int retVal = 0; //default is last part
		
		if(!boundary.isEmpty())
		{
			partBoundary = QString("--") + boundary;
			partEnd = QString("--") + boundary + "--";
		}
  	
  	while(useIO.inputLine(inputStr))
  	{
			//check for the end of all parts
			if(!partEnd.isEmpty() && !qstrnicmp(inputStr,partEnd.latin1(),partEnd.length()-1))
			{
				retVal = 0; //end of these parts
				break;
			} else if(!partBoundary.isEmpty() && !qstrnicmp(inputStr,partBoundary.latin1(),partBoundary.length()-1))
			{
				retVal = 1; //continue with next part
				break;
			}
			messageBody += inputStr;
		}

	return retVal;
}

void mimeHeader::parseHeader(mimeIO &useIO)
{  	
	mimeHdrLine my_line;
	QCString inputStr;

  	while(useIO.inputLine(inputStr))
  	{
  			int appended = my_line.appendStr(inputStr);
  			if(!appended)
  			{
					addHdrLine(&my_line);
  				appended = my_line.setStr(inputStr);
  			}
  			if(appended <= 0) break;
  			inputStr = (const char *)NULL;
  	}
}
