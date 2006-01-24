/***************************************************************************
                          mac.cpp
                             -------------------
    copyright            : (C) 2002 Jonathan Riddell
    email                : jr@jriddell.org
    version              : 1.0
    release date         : 10 Feburary 2002
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <kio/slavebase.h>
#include <kio/global.h>
#include <kurl.h>
#include <kprocess.h>

#include <qstring.h>
#include <qfile.h>
#include <qtextstream.h>

class MacProtocol : public QObject, public KIO::SlaveBase
{
    Q_OBJECT
public:
    MacProtocol(const QByteArray &pool, const QByteArray &app);
    ~MacProtocol();
    virtual void get(const KUrl& url );
    virtual void listDir(const KUrl& url);
    virtual void stat(const KUrl& url);
protected Q_SLOTS:
    void slotGetStdOutput(KProcess*, char*, int);
    void slotSetDataStdOutput(KProcess*, char *s, int len);
protected:
    QString prepareHP(const KUrl& _url);
    KIO::UDSEntry makeUDS(const QString& _line);
    int makeTime(QString mday, QString mon, QString third);
    QString getMimetype(QString type, QString app);
    KIO::UDSEntry doStat(const KUrl& url);

    KIO::filesize_t processedBytes;
    QString standardOutputStream;
    KProcess* myKProcess;

    //for debugging
    //QFile* logFile;
    //QTextStream* logStream;
};
