/***************************************************************************
                          mimeheader.h  -  description
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

#ifndef MIMEHEADER_H
#define MIMEHEADER_H

#include <qlist.h>
#include <qdict.h>

#include "mimehdrline.h"
#include "mimeio.h"

/**
  *@author Sven Carstens
  */

class mimeHeader {
public: 
	mimeHeader();
	virtual ~mimeHeader();

	virtual QString internalType() { return QString("mimeHeader"); };

	virtual void addHdrLine(mimeHdrLine *);
	virtual void outputHeader(mimeIO &);
	virtual void outputPart(mimeIO &);


	QCString outputParameter(QDict<QString> *);
	
	int parsePart(mimeIO &,QString);
	int parseBody(mimeIO &,QCString &,QString);
	void parseHeader(mimeIO &);
	
	QString getDispositionParm(QCString);
	void setDispositionParm(QCString,QString);
	QDictIterator<QString> getDispositionIterator();

	QString getTypeParm(QCString);
	void setTypeParm(QCString,QString);
	QDictIterator<QString> getTypeIterator();

	QCString getType() { return contentType; };
	void setType(const QCString &_str) { contentType = _str; }

	QCString getDisposition() { return contentDisposition; };
	void setDisposition(const QCString &_str) { contentDisposition = _str; }
	
	QCString getEncoding() { return contentEncoding; };
	void setEncoding(const QCString &_str) { contentEncoding = _str; };

	QCString getID() { return contentID; };
	void setID(const QCString &_str) { contentID = _str; };
	
	unsigned long getLength() { return contentLength; };
	void setLength(unsigned long _len) { contentLength = _len; };

	const QString &getPartSpecifier() { return partSpecifier;};
	void setPartSpecifier(const QString &_str) { partSpecifier = _str; };
	
	QListIterator<mimeHdrLine> getOriginalIterator();
	QListIterator<mimeHdrLine> getAdditionalIterator();
	void setContent(QCString aContent) { mimeContent = aContent; };
	QCString getContent() { return mimeContent; };

	QCString getBody() { return preMultipartBody + postMultipartBody; };
	QCString getPreBody() { return preMultipartBody; };
	void setPreBody(QCString &inBody) { preMultipartBody = inBody; };

	QCString getPostBody() { return postMultipartBody; };
	void setPostBody(QCString &inBody) { postMultipartBody = inBody; };

	mimeHeader *getNestedMessage() { return nestedMessage; };
	void setNestedMessage(mimeHeader *inPart,bool destroy=true) { if(nestedMessage && destroy) delete nestedMessage; nestedMessage = inPart; };

//	mimeHeader *getNestedPart() { return nestedPart; };
	void addNestedPart(mimeHeader *inPart) { nestedParts.append(inPart); };
	QListIterator<mimeHeader> getNestedIterator() { return QListIterator<mimeHeader>(nestedParts); }; 	

	// clears all parts and deletes them from memory
	void clearNestedParts() { nestedParts.clear(); };

	// clear all parameters to content-type
	void clearTypeParameters() { typeList.clear(); };
	
	// clear all parameters to content-disposition
	void clearDispositionParameters() { dispositionList.clear(); };
	
protected:
	static void addParameter(QCString,QDict<QString> *);
	static QString getParameter(QCString,QDict<QString> *);
	static void setParameter(QCString,QString,QDict<QString> *);

	QList<mimeHdrLine> originalHdrLines;

private:
	QList<mimeHdrLine> additionalHdrLines;
	QDict<QString>	typeList;
	QDict<QString>	dispositionList;
	QCString contentType;
	QCString contentDisposition;
	QCString contentEncoding;
	QCString contentID;
	unsigned long contentLength;
	QCString mimeContent;
	QCString preMultipartBody;
	QCString postMultipartBody;
	mimeHeader *nestedMessage;
	QList<mimeHeader> nestedParts;
	QString partSpecifier;
	
};

#endif
