/****************************************************************************
** $Id$
**
** Copyright (C) 1992-2000 Troll Tech AS.  All rights reserved.
**
** This file is part of an example program for Qt.  This example
** program may be used, distributed and modified without limitation.
**
*****************************************************************************/

#ifndef LISTVIEWS_H
#define LISTVIEWS_H

#include <qsplitter.h>
#include <qstring.h>
#include <qobject.h>
#include <qdatetime.h>
#include <qlist.h>
#include "kdelistview.h"

class QLabel;
class QPainter;
class QColorGroup;
class QObjectList;
class QPopupMenu;
class QPushButton;

// -----------------------------------------------------------------

class MessageHeader
{
public:
    MessageHeader( const QString &_sender, const QString &_subject, const QDateTime &_datetime )
	: msender( _sender ), msubject( _subject ), mdatetime( _datetime )
    {}

    MessageHeader( const MessageHeader &mh );
    MessageHeader &operator=( const MessageHeader &mh );

    QString sender() { return msender; }
    QString subject() { return msubject; }
    QDateTime datetime() { return mdatetime; }

protected:
    QString msender, msubject;
    QDateTime mdatetime;

};

// -----------------------------------------------------------------

class Message
{
public:
    enum State { Read = 0,
		 Unread};

    Message( const MessageHeader &mh, const QString &_body )
	: mheader( mh ), mbody( _body ), mstate( Unread )
    {}

    Message( const Message &m )
	: mheader( m.mheader ), mbody( m.mbody ), mstate( m.mstate )
    {}

    MessageHeader header() { return mheader; }
    QString body() { return mbody; }

    void setState( const State &s ) { mstate = s; }
    State state() { return mstate; }

protected:
    MessageHeader mheader;
    QString mbody;
    State mstate;

};

// -----------------------------------------------------------------

class Folder : public QObject
{
    Q_OBJECT

public:
    Folder( Folder *parent, const QString &name );
    ~Folder()
    {}

    void addMessage( Message *m )
    { lstMessages.append( m ); }

    QString folderName() { return fName; }

    Message *firstMessage() { return lstMessages.first(); }
    Message *nextMessage() { return lstMessages.next(); }

protected:
    QString fName;
    QList<Message> lstMessages;

};

// -----------------------------------------------------------------

class FolderListItem : public CListViewItem
{
public:
    FolderListItem( CKDEListView *parent, Folder *f );
    FolderListItem( FolderListItem *parent, Folder *f );

    void insertSubFolders( const QObjectList *lst );

    Folder *folder() { return myFolder; }

protected:
    Folder *myFolder;

};

// -----------------------------------------------------------------

class MessageListItem : public CListViewItem
{
public:
    MessageListItem( CKDEListView *parent, Message *m );

    virtual void paintCell( QPainter *p, const QColorGroup &cg,
			    int column, int width, int alignment );

    Message *message() { return myMessage; }

protected:
    Message *myMessage;

};

// -----------------------------------------------------------------

class MessageList : public CKDEListView
{
    Q_OBJECT

public:
    MessageList( QWidget *parent = 0, const char *name = 0 );

public slots:
    void moveCols( void );
    void toggleStationaryHeader( void );
private:
    bool m_bStationaryHeader;
};

// -----------------------------------------------------------------

class ListViews : public QSplitter
{
    Q_OBJECT

public:
    ListViews( QWidget *parent = 0, const char *name = 0 );
    ~ListViews()
    {}

protected:
    void initFolders();
    void initFolder( Folder *folder, unsigned int &count );
    void setupFolders();

    CKDEListView *messages, *folders;
    QLabel *message;
    QPopupMenu* menu;

    QList<Folder> lstFolders;

protected slots:
    void slotFolderChanged( CListViewItem* );
    void slotMessageChanged();
    void slotRMB( CListViewItem*, const QPoint &, int );
    void slotHeaderClicked( int );

private:
    int m_sortCol;
    bool m_sortAscending;
};

// -----------------------------------------------------------------

#endif

